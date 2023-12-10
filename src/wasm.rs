use std::{
    cell::RefCell,
    collections::{hash_map::Entry, HashMap},
    fs::{create_dir_all, File},
    hash::Hash,
    io::Write,
    ops::Range,
    path::Path,
    rc::Rc,
    time::{Duration, Instant},
    vec,
};

use anyhow::{Error, Ok, Result};
use wasm_encoder::{
    CodeSection, ConstExpr, DataSection, EntityType, ExportKind, ExportSection, Function,
    FunctionSection, GlobalSection, GlobalType, ImportSection, Instruction, MemArg, MemorySection,
    MemoryType, TypeSection, ValType,
};

use crate::{
    lex::{Token, TokenKind, Intern},
    tree::{BranchKind, Nod, Node},
    BuildArgs, Cart,
};

pub fn write_wasm(args: &BuildArgs, cart: &Cart, start: Instant) -> Result<Duration> {
    let _ = cart;
    let mut writer = WasmWriter::new(&cart);
    writer.build()?;
    let wasm = writer.module.finish();
    let duration = start.elapsed();
    write_out(args, wasm)?;
    Ok(duration)
}

fn write_out(args: &BuildArgs, wasm: Vec<u8>) -> Result<()> {
    // TODO Default to current dir?
    let Some(outdir) = &args.outdir else {
        return Ok(());
    };
    create_dir_all(outdir)?;
    let name = Path::new(args.app.as_str())
        .file_stem()
        .ok_or(Error::msg("no name"))?
        .to_str()
        .ok_or(Error::msg("bad name"))?;
    let path = Path::new(outdir).join(format!("{name}.wasm"));
    let mut file = File::create(path)?;
    file.write_all(&wasm)?;
    Ok(())
}

struct WasmWriter<'a> {
    cart: &'a Cart,
    data_offset: u32,
    imports_len: u32,
    lookup_table: Vec<Lookup>,
    main_id: Option<Intern>,
    module: wasm_encoder::Module,
    predefs: Predefs,
    stack_global: u32,
    stack_start: u32,
    type_offset: usize,
    type_table: Vec<usize>,
}

#[derive(Debug, Default)]
struct Predefs {
    fd_write_fun: u32,
    fd_write_type: Option<EntityType>,
    pop_fun: u32,
    print_fun: u32,
    print_type: u32,
    push_fun: u32,
    push_type: u32,
}

impl<'a> WasmWriter<'a> {
    fn new(cart: &Cart) -> WasmWriter {
        // Let stack go down and data go up.
        let stack_start = 4096;
        cart.interner.get("main");
        WasmWriter {
            cart,
            data_offset: stack_start,
            imports_len: 0,
            lookup_table: vec![Lookup::Boring; cart.modules[1].tree.len()],
            main_id: cart.interner.get("main"),
            module: wasm_encoder::Module::new(),
            predefs: Predefs::default(),
            stack_global: 0,
            stack_start,
            type_offset: 0,
            type_table: vec![],
        }
    }

    fn build(&mut self) -> Result<()> {
        self.scrape_data();
        self.build_types();
        self.build_imports();
        self.build_functions();
        self.build_memory();
        self.build_globals();
        self.build_exports();
        self.build_codes();
        self.build_data();
        Ok(())
    }

    fn add_fd_write_import(&mut self, imports: &mut ImportSection) {
        self.predefs.fd_write_fun = imports.len();
        imports.import(
            "wasi_snapshot_preview1",
            "fd_write",
            self.predefs.fd_write_type.unwrap(),
        );
    }

    fn build_codes(&mut self) {
        let mut codes = CodeSection::new();
        self.code_print(&mut codes);
        self.code_pop(&mut codes);
        self.code_push(&mut codes);
        fn dig(node: Node, wasm: &mut WasmWriter, codes: &mut CodeSection) {
            if let Nod::Branch { kind, range } = node.nod {
                if kind == BranchKind::Fun && node.typ.0 != 0 {
                    wasm.translate_fun(codes, node);
                }
                let range: Range<usize> = range.into();
                for kid_index in range {
                    dig(wasm.cart.modules[1].tree[kid_index], wasm, codes);
                }
            }
        }
        dig(*self.tree().last().unwrap(), self, &mut codes);
        self.module.section(&codes);
    }

    fn build_data(&mut self) {
        // TODO Option to just concatenate everything into one?
        // TODO Probably makes smaller file, but easier to dig around this way.
        let mut data = DataSection::new();
        let mut buffer = vec![];
        for (index, lookup) in self.lookup_table.iter().enumerate() {
            if let Lookup::Datum { address } = *lookup {
                let node = self.tree()[index];
                match node.nod {
                    Nod::Leaf {
                        token:
                            Token {
                                kind: TokenKind::String,
                                intern,
                            },
                    } => {
                        let text = &self.cart.interner[intern];
                        let len_bytes = (text.len() as u32).to_le_bytes();
                        buffer.clear();
                        buffer.extend_from_slice(&len_bytes);
                        buffer.extend_from_slice(text.as_bytes());
                        // Null-terminate for safety, but not included in length.
                        buffer.push(0);
                        data.active(
                            0,
                            &ConstExpr::i32_const(address as i32),
                            buffer.iter().copied(),
                        );
                    }
                    _ => {}
                }
            }
        }
        self.module.section(&data);
    }

    fn build_exports(&mut self) {
        let mut exports = ExportSection::new();
        exports.export("memory", ExportKind::Memory, 0);
        // TODO Dig for functions again?
        self.module.section(&exports);
    }

    fn build_functions(&mut self) {
        let mut functions = FunctionSection::new();
        // Predef funs
        self.predefs.print_fun = add_fun(&mut functions, self.predefs.print_type);
        self.predefs.pop_fun = add_fun(&mut functions, self.predefs.print_type); // reusing print_type
        self.predefs.push_fun = add_fun(&mut &mut functions, self.predefs.push_type);
        // User funs
        fn dig(tree: &[Node], wasm: &mut WasmWriter, functions: &mut FunctionSection) {
            let node = tree.last().unwrap();
            if let Nod::Branch { kind, range } = node.nod {
                if kind == BranchKind::Fun && node.typ.0 != 0 {
                    let typ = node.typ.0 as usize - 1 - wasm.type_offset;
                    let typ = wasm.type_table[typ];
                    if typ != 0 {
                        functions.function(typ as u32 - 1);
                    }
                }
                let range: Range<usize> = range.into();
                for kid_index in range {
                    dig(&tree[0..=kid_index], wasm, functions);
                }
            }
        }
        dig(&self.cart.modules[1].tree, self, &mut functions);
        self.module.section(&functions);
    }

    fn build_globals(&mut self) {
        let mut globals = GlobalSection::new();
        globals.global(
            GlobalType {
                val_type: ValType::I32,
                mutable: true,
            },
            &ConstExpr::i32_const(self.stack_start as i32),
        );
        self.module.section(&globals);
    }

    fn build_imports(&mut self) {
        let mut imports = ImportSection::new();
        self.add_fd_write_import(&mut imports);
        self.module.section(&imports);
        self.imports_len = imports.len();
    }

    fn build_memory(&mut self) {
        let mut memory = MemorySection::new();
        memory.memory(MemoryType {
            minimum: 1,
            maximum: None,
            memory64: false,
            shared: false,
        });
        self.module.section(&memory);
    }

    fn build_types(&mut self) {
        let mut types = TypeSection::new();
        let val_types = Rc::new(RefCell::new(Vec::<ValType>::new()));
        // Duplicate some of the tree push, find duplicate, and pop logic, but
        // specifically for wasm fun types.
        // TODO Reuse type tree logic from elsewhere?
        #[derive(Clone)]
        struct WasmFunType {
            val_types: Rc<RefCell<Vec<ValType>>>,
            params: Range<usize>,
            results: Range<usize>,
        }
        impl Eq for WasmFunType {}
        impl Hash for WasmFunType {
            fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
                let val_types = self.val_types.borrow();
                for param in self.params.clone() {
                    val_types[param].hash(state);
                }
                for result in self.results.clone() {
                    val_types[result].hash(state);
                }
            }
        }
        impl PartialEq for WasmFunType {
            fn eq(&self, other: &Self) -> bool {
                if self.params.len() != other.params.len() {
                    return false;
                }
                if self.results.len() != other.results.len() {
                    return false;
                }
                let val_types = self.val_types.borrow();
                for (a, b) in self.params.clone().zip(other.params.clone()) {
                    if val_types[a] != val_types[b] {
                        return false;
                    }
                }
                for (a, b) in self.results.clone().zip(other.results.clone()) {
                    if val_types[a] != val_types[b] {
                        return false;
                    }
                }
                true
            }
        }
        let mut type_indices = HashMap::<WasmFunType, usize>::new();
        // TODO Use wasm type logic for custom type also, so we don't duplicate those either.
        // TODO Do that by including it in the common module first in some fashion?
        self.predefs.fd_write_type = Some(add_fd_write_type(&mut types));
        self.predefs.print_type = add_print_type(&mut types);
        self.predefs.push_type = add_push_type(&mut types);
        // Add function types.
        let prebaked_types_offset = types.len() as usize;
        let cart = &self.cart;
        let core = cart.core_exports;
        let void = core.void_type.num;
        // TODO We probably do want to link everything into one module before getting here so we can keep this simple.
        let tree = &cart.modules[1].tree;
        if let Nod::Branch { range, .. } = tree.last().unwrap().nod {
            let range: Range<usize> = range.into();
            // Reverse because we expect types near the end.
            for kid_index in range.rev() {
                let kid = tree[kid_index];
                if let Nod::Branch {
                    kind: BranchKind::Types,
                    range: types_range,
                    ..
                } = kid.nod
                {
                    let types_range: Range<usize> = types_range.into();
                    self.type_offset = types_range.start;
                    for type_index in types_range {
                        let type_kid = tree[type_index];
                        if let Nod::Branch {
                            kind: BranchKind::FunType,
                            range: params_range,
                            ..
                        } = type_kid.nod
                        {
                            let params_start = val_types.borrow().len();
                            let params_range: Range<usize> = params_range.into();
                            for _ in params_range.clone() {
                                // TODO Check for things other than int, like float.
                                val_types.borrow_mut().push(ValType::I32);
                            }
                            let results_start = val_types.borrow().len();
                            match type_kid.typ.0 {
                                0 => {}
                                typ @ _ => match tree[typ as usize - 1].nod {
                                    Nod::Uid { module: 1, num, .. } => match num {
                                        _ if num == void => {}
                                        _ => val_types.borrow_mut().push(ValType::I32),
                                    },
                                    _ => {
                                        println!("What to do? {typ}");
                                    }
                                },
                            };
                            let fun_type = WasmFunType {
                                val_types: val_types.clone(),
                                params: params_start..results_start,
                                results: results_start..val_types.borrow().len(),
                            };
                            let next = type_indices.len() + prebaked_types_offset;
                            let entry = type_indices.entry(fun_type.clone());
                            let wasm_index = match entry {
                                Entry::Occupied(old) => {
                                    // We don't need the latest additions.
                                    val_types.borrow_mut().truncate(params_start);
                                    *old.get()
                                }
                                Entry::Vacant(vacancy) => {
                                    // TODO Keep map from module type to wasm type.
                                    let val_types = val_types.borrow();
                                    let params = &val_types[fun_type.params];
                                    let results = &val_types[fun_type.results];
                                    types.function(params.iter().cloned(), results.iter().cloned());
                                    vacancy.insert(next);
                                    next
                                }
                            };
                            self.type_table.push(wasm_index + 1);
                        } else {
                            self.type_table.push(0);
                        }
                    }
                    break;
                }
            }
        }
        self.module.section(&types);
    }

    fn code_pop(&self, codes: &mut CodeSection) {
        add_codes(
            codes,
            &[],
            &[
                Instruction::GlobalGet(self.stack_global),
                Instruction::LocalGet(0),
                Instruction::I32Add,
                Instruction::GlobalSet(0),
            ],
        );
    }

    fn code_print(&self, codes: &mut CodeSection) {
        let (text, iovec, nwritten) = (0, 1, 2);
        add_codes(
            codes,
            &[(2, ValType::I32)],
            &[
                // Push iovec.
                Instruction::I32Const(8),
                Instruction::Call(self.predefs.push_fun),
                Instruction::LocalSet(iovec),
                // Push nwritten.
                Instruction::I32Const(4),
                Instruction::Call(self.predefs.push_fun),
                Instruction::LocalSet(nwritten),
                // Put text pointer in iovec.
                Instruction::LocalGet(iovec),
                Instruction::LocalGet(text),
                Instruction::I32Const(4),
                Instruction::I32Add,
                Instruction::I32Store(MemArg {
                    offset: 0,
                    align: 0,
                    memory_index: 0,
                }),
                // Put text size in iovec.
                Instruction::LocalGet(iovec),
                Instruction::I32Const(4),
                Instruction::I32Add,
                Instruction::LocalGet(text),
                Instruction::I32Load(MemArg {
                    offset: 0,
                    align: 2,
                    memory_index: 0,
                }),
                Instruction::I32Store(MemArg {
                    offset: 0,
                    align: 2,
                    memory_index: 0,
                }),
                // Call fd_write.
                Instruction::I32Const(1),
                Instruction::LocalGet(iovec),
                Instruction::I32Const(1),
                Instruction::LocalGet(nwritten),
                Instruction::Call(self.predefs.fd_write_fun),
                Instruction::Drop,
                // Finish.
                Instruction::I32Const(12),
                Instruction::Call(self.predefs.pop_fun),
            ],
        );
    }

    fn code_push(&self, codes: &mut CodeSection) {
        add_codes(
            codes,
            &[(1, ValType::I32)],
            &[
                Instruction::GlobalGet(self.stack_global),
                Instruction::LocalGet(0),
                Instruction::I32Sub,
                Instruction::LocalTee(1),
                Instruction::GlobalSet(0),
                Instruction::LocalGet(1),
            ],
        );
    }

    fn scrape_data(&mut self) {
        fn dig(tree: &[Node], wasm: &mut WasmWriter) {
            let node = tree.last().unwrap();
            match node.nod {
                Nod::Branch { range, .. } => {
                    let range: Range<usize> = range.into();
                    for kid_index in range {
                        dig(&tree[0..=kid_index], wasm);
                    }
                }
                Nod::Leaf {
                    token:
                        Token {
                            kind: TokenKind::String,
                            intern,
                        },
                } => {
                    // TODO Tokenize string literals more for better content.
                    // TODO If we already have a reference to this intern, reuse that. Need a map?
                    let text = &wasm.cart.interner[intern];
                    let index = align(4, wasm.data_offset);
                    wasm.lookup_table[tree.len() - 1] = Lookup::Datum { address: index };
                    // Space for both size and data.
                    wasm.data_offset = index + 4 + text.len() as u32;
                }
                _ => {}
            }
        }
        dig(&self.cart.modules[1].tree, self);
    }

    fn translate_any(&self, fun: &mut Function, index: usize) {
        let node = self.tree()[index];
        match node.nod {
            Nod::Branch { range, .. } => {
                let range: Range<usize> = range.into();
                for kid_index in range {
                    self.translate_any(fun, kid_index);
                }
            }
            Nod::Leaf {
                token:
                    Token {
                        kind: TokenKind::String,
                        ..
                    },
            } => {
                let Lookup::Datum { address } = self.lookup_table[index] else {
                    panic!()
                };
                fun.instruction(&Instruction::I32Const(address as i32));
            }
            _ => {}
        }
    }

    fn translate_fun(&self, codes: &mut CodeSection, node: Node) {
        let locals = vec![];
        let mut func = Function::new(locals);
        let Nod::Branch {
            kind: BranchKind::Fun,
            range,
        } = node.nod
        else {
            panic!()
        };
        if range.len() == 3 {
            let tree = self.tree();
            let Nod::Branch {
                kind: BranchKind::Block,
                range: body_range,
            } = tree[range.start as usize + 2].nod
            else {
                panic!()
            };
            _ = body_range;
            self.translate_any(&mut func, range.start as usize + 2);
            // println!("body {}", body_range.end - body_range.start);
            // // Params
            // let Nod::Branch {
            //     kind: BranchKind::Params,
            //     range: params_range,
            // } = tree[range.start].nod
            // else {
            //     panic!()
            // };
            // let params_range: Range<usize> = params_range.into();
            // for param_index in params_range {
            //     _ = params_range;
            //     // fun.instruction(&Instruction)
            // }
            // End
        }
        func.instruction(&Instruction::End);
        codes.function(&func);
    }

    fn tree(&self) -> &[Node] {
        &self.cart.modules[1].tree
    }
}

fn align(size: u32, index: u32) -> u32 {
    index
        + match index % size {
            0 => 0,
            offset @ _ => size - offset,
        }
}

fn add_codes(codes: &mut CodeSection, locals: &[(u32, ValType)], instructions: &[Instruction]) {
    let mut func = Function::new(locals.iter().copied());
    for instruction in instructions {
        func.instruction(instruction);
    }
    codes.function(&func);
}

fn add_fd_write_type(types: &mut TypeSection) -> EntityType {
    let params = vec![ValType::I32, ValType::I32, ValType::I32, ValType::I32];
    let results = vec![ValType::I32];
    types.function(params, results);
    EntityType::Function(types.len() - 1)
}

fn add_fun(functions: &mut FunctionSection, type_index: u32) -> u32 {
    functions.function(type_index);
    functions.len() - 1
}

fn add_print_type(types: &mut TypeSection) -> u32 {
    let params = vec![ValType::I32];
    let results = vec![];
    types.function(params, results);
    types.len() as u32 - 1
}

fn add_push_type(types: &mut TypeSection) -> u32 {
    let params = vec![ValType::I32];
    let results = vec![ValType::I32];
    types.function(params, results);
    types.len() as u32 - 1
}

#[derive(Clone, Copy, Debug)]
enum Lookup {
    Boring,
    Datum { address: u32 },
    Fun { index: u32 },
}
