use std::{
    cell::RefCell,
    collections::{hash_map::Entry, HashMap},
    fs::{create_dir_all, File},
    hash::Hash,
    io::Write,
    ops::Range,
    path::Path,
    rc::Rc,
    vec,
};

use anyhow::{Error, Ok, Result};
use wasm_encoder::{
    CodeSection, DataSection, EntityType, ExportKind, ExportSection, Function, FunctionSection,
    GlobalSection, ImportSection, Instruction, MemorySection, MemoryType, TypeSection, ValType,
};

use crate::{
    tree::{BranchKind, Nod, Node},
    BuildArgs, Cart,
};

pub fn write_wasm(args: &BuildArgs, cart: &Cart) -> Result<()> {
    let _ = cart;
    let mut writer = WasmWriter::new();
    writer.build(&cart)?;
    let wasm = writer.module.finish();
    write_out(args, wasm)
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

struct WasmWriter {
    fd_write_type: Option<EntityType>,
    lookup_table: Vec<Lookup>,
    module: wasm_encoder::Module,
    stack_start: usize,
    type_offset: usize,
    type_table: Vec<usize>,
}

impl WasmWriter {
    fn new() -> WasmWriter {
        WasmWriter {
            fd_write_type: None,
            lookup_table: vec![],
            module: wasm_encoder::Module::new(),
            stack_start: 4096,
            type_offset: 0,
            type_table: vec![],
        }
    }

    fn build(&mut self, cart: &Cart) -> Result<()> {
        self.build_types(cart);
        self.build_imports();
        self.build_functions(cart);
        self.build_memory();
        self.build_globals();
        self.build_exports();
        self.build_codes(cart);
        self.build_data();
        Ok(())
    }

    fn add_fd_write_import(&self, imports: &mut ImportSection) {
        imports.import(
            "wasi_snapshot_preview1",
            "fd_write",
            self.fd_write_type.unwrap(),
        );
    }

    fn build_codes(&mut self, cart: &Cart) {
        let mut codes = CodeSection::new();
        fn dig(tree: &[Node], wasm: &mut WasmWriter, codes: &mut CodeSection) {
            let node = *tree.last().unwrap();
            if let Nod::Branch { kind, range } = node.nod {
                if kind == BranchKind::Fun && node.typ.0 != 0 {
                    wasm.translate_fun(codes, tree, node);
                }
                let range: Range<usize> = range.into();
                for kid_index in range {
                    dig(&tree[0..=kid_index], wasm, codes);
                }
            }
        }
        dig(&cart.modules[1].tree, self, &mut codes);
        self.module.section(&codes);
    }

    fn build_data(&mut self) {
        let data = DataSection::new();
        self.module.section(&data);
    }

    fn build_exports(&mut self) {
        let mut exports = ExportSection::new();
        exports.export("memory", ExportKind::Memory, 0);
        self.module.section(&exports);
    }

    fn build_functions(&mut self, cart: &Cart) {
        let mut functions = FunctionSection::new();
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
        dig(&cart.modules[1].tree, self, &mut functions);
        self.module.section(&functions);
    }

    fn build_globals(&mut self) {
        let globals = GlobalSection::new();
        self.module.section(&globals);
    }

    fn build_imports(&mut self) {
        let mut imports = ImportSection::new();
        self.add_fd_write_import(&mut imports);
        self.module.section(&imports);
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

    fn build_types(&mut self, cart: &Cart) {
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
        self.fd_write_type = Some(add_fd_write_type(&mut types));
        // Add function types.
        let prebaked_types_offset = types.len() as usize;
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

    fn translate_fun(&self, codes: &mut CodeSection, tree: &[Node], node: Node) {
        let locals = vec![];
        let mut fun = Function::new(locals);
        let Nod::Branch {
            kind: BranchKind::Fun,
            range,
        } = node.nod
        else {
            panic!()
        };
        if range.len() == 3 {
            // let range: Range<usize> = range.into();
            let Nod::Branch {
                kind: BranchKind::Block,
                range: body_range,
            } = tree[range.start as usize + 2].nod
            else {
                panic!()
            };
            println!("body {}", body_range.end - body_range.start);
            fun.instruction(&Instruction::I32Const(4));
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
        fun.instruction(&Instruction::End);
        codes.function(&fun);
    }
}

fn add_fd_write_type(types: &mut TypeSection) -> EntityType {
    let params = vec![ValType::I32, ValType::I32, ValType::I32, ValType::I32];
    let results = vec![ValType::I32];
    types.function(params, results);
    EntityType::Function(types.len() - 1)
}

enum Lookup {
    Datum { index: u32 },
    Fun { index: u32 },
}
