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
    BlockType, CodeSection, ConstExpr, DataSection, EntityType, ExportKind, ExportSection,
    Function, FunctionSection, GlobalSection, GlobalType, ImportSection, Instruction, MemArg,
    MemorySection, MemoryType, TypeSection, ValType,
};

use crate::{
    lex::{Intern, Token, TokenKind},
    run::{CoreExports, ScopeEntry},
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
    newline_address: u32,
    pop_type: u32,
    pop_fun: u32,
    print_fun: u32,
    print_type: u32,
    print_inline_fun: u32,
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

    fn add_fun(&self, functions: &mut FunctionSection, type_index: u32) -> u32 {
        functions.function(type_index);
        functions.len() - 1 + self.imports_len
    }

    fn build_codes(&mut self) {
        let mut codes = CodeSection::new();
        self.code_print(&mut codes);
        self.code_print_inline(&mut codes);
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
        data.active(
            0,
            &ConstExpr::i32_const(self.predefs.newline_address as i32),
            NEWLINE.as_bytes().iter().copied(),
        );
        let mut buffer = vec![]; // TODO Reuse cart buffer? String vs Vec<u8>?
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
                        buffer.clear();
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
        for ScopeEntry {
            intern,
            module,
            num,
        } in self.cart.modules[1].tops.values.iter().copied()
        {
            // Check exported and if it's a function.
            if module != 0 {
                if let Lookup::Fun { index } = self.lookup_table[num as usize - 1] {
                    let name = if Some(intern) == self.main_id {
                        // TODO Main separate from start? Just some top level non-named main indicator?
                        "_start"
                    } else {
                        &self.cart.interner[intern]
                    };
                    exports.export(name, ExportKind::Func, index);
                }
            }
        }
        self.module.section(&exports);
    }

    fn build_functions(&mut self) {
        let mut functions = FunctionSection::new();
        // Predef funs
        self.predefs.print_fun = self.add_fun(&mut functions, self.predefs.print_type);
        self.predefs.print_inline_fun = self.add_fun(&mut functions, self.predefs.print_type);
        self.predefs.pop_fun = self.add_fun(&mut functions, self.predefs.pop_type);
        self.predefs.push_fun = self.add_fun(&mut &mut functions, self.predefs.push_type);
        // User funs
        fn dig(tree: &[Node], wasm: &mut WasmWriter, functions: &mut FunctionSection) {
            let node = *tree.last().unwrap();
            let check = match node.nod {
                Nod::Branch {
                    kind: BranchKind::Def,
                    range,
                } => tree[range.start as usize + 2],
                _ => node,
            };
            if let Nod::Branch { kind, range } = check.nod {
                if kind == BranchKind::Fun && check.typ.0 != 0 {
                    let typ = node.typ.0 as usize - 1 - wasm.type_offset;
                    let typ = wasm.type_table[typ];
                    if typ != 0 {
                        functions.function(typ as u32 - 1);
                        wasm.lookup_table[tree.len() - 1] = Lookup::Fun {
                            index: functions.len() - 1 + wasm.imports_len,
                        };
                    }
                }
                let range: Range<usize> = range.into();
                for kid_index in range.clone() {
                    dig(&tree[0..=kid_index], wasm, functions);
                }
                // After kids, possibly propagate up to def.
                if kind == BranchKind::Def && range.len() == 3 {
                    let last = range.end - 1;
                    if let Nod::Branch {
                        kind: BranchKind::Fun,
                        ..
                    } = tree[last].nod
                    {
                        wasm.lookup_table[tree.len() - 1] = wasm.lookup_table[last];
                    }
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
        self.predefs.pop_type = add_pop_type(&mut types);
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
                            for param_index in params_range.clone() {
                                let val_types_mut = &mut val_types.borrow_mut();
                                match simple_wasm_type(cart, tree, tree[param_index]) {
                                    SimpleWasmType::Span => {
                                        val_types_mut.push(ValType::I32);
                                        val_types_mut.push(ValType::I32);
                                    }
                                    _ => val_types_mut.push(ValType::I32),
                                }
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
        let (text_len, text) = (0, 1);
        add_codes(
            codes,
            &[],
            &[
                Instruction::LocalGet(text_len),
                Instruction::LocalGet(text),
                Instruction::Call(self.predefs.print_inline_fun),
                Instruction::I32Const(NEWLINE.len() as i32 - 1),
                Instruction::I32Const(self.predefs.newline_address as i32),
                Instruction::Call(self.predefs.print_inline_fun),
            ],
        );
    }

    fn code_print_inline(&self, codes: &mut CodeSection) {
        let (text_len, text, iovec, nwritten) = (0, 1, 2, 3);
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
                Instruction::I32Store(mem_arg(0)),
                // Put text size in iovec.
                Instruction::LocalGet(iovec),
                Instruction::I32Const(4),
                Instruction::I32Add,
                Instruction::LocalGet(text_len),
                Instruction::I32Store(mem_arg(2)),
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
        // Add a "\n" for newline printing convenience.
        self.predefs.newline_address = self.data_offset;
        self.data_offset += NEWLINE.len() as u32;
        // Now check in program.
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
                    let index = wasm.data_offset;
                    // let index = align(4, wasm.data_offset); // Had included 4-byte size before.
                    wasm.lookup_table[tree.len() - 1] = Lookup::Datum { address: index };
                    // Space for raw data and null char
                    wasm.data_offset = index + text.len() as u32 + 1;
                }
                _ => {}
            }
        }
        dig(&self.cart.modules[1].tree, self);
    }

    fn translate_any(&self, fun: &mut Function, index: usize) {
        let node = self.tree()[index];
        match node.nod {
            Nod::Branch {
                kind: BranchKind::Call,
                range,
            } => {
                let mut handled = false;
                let range: Range<usize> = range.into();
                let args_range = range.start + 1..range.end;
                let core = self.cart.core_exports;
                match self.tree()[range.start].nod {
                    Nod::Uid { module, num, .. } => {
                        // TODO Instead track all modules.
                        if module == 1 {
                            if num == core.branch_fun.num {
                                self.translate_branch(fun, args_range.clone());
                                handled = true;
                            }
                        }
                        if !handled {
                            for arg_index in args_range.clone() {
                                self.translate_any(fun, arg_index);
                            }
                            let arg_type = if args_range.is_empty() {
                                None
                            } else {
                                Some(simple_wasm_type(
                                    self.cart,
                                    self.tree(),
                                    self.tree()[args_range.start],
                                ))
                            };
                            if module == 1 {
                                // TODO Put these in a lookup table also?
                                handled = true;
                                if num == core.gt_fun.num {
                                    match arg_type {
                                        Some(SimpleWasmType::I32) => {
                                            fun.instruction(&Instruction::I32GtS);
                                        }
                                        _ => {}
                                    }
                                } else if num == core.lt_fun.num {
                                    match arg_type {
                                        Some(SimpleWasmType::I32) => {
                                            fun.instruction(&Instruction::I32LtS);
                                        }
                                        _ => {}
                                    }
                                } else if num == core.print_fun.num {
                                    fun.instruction(&Instruction::Call(self.predefs.print_fun));
                                } else {
                                    handled = false;
                                }
                            } else if module == 0 || module == 2 {
                                if let Lookup::Fun { index } = self.lookup_table[num as usize - 1] {
                                    fun.instruction(&Instruction::Call(index));
                                    handled = true;
                                }
                            }
                        }
                    }
                    _ => {}
                }
                if !handled {
                    println!("Call target@{} unhandled", range.start);
                }
            }
            Nod::Branch {
                kind: BranchKind::Def,
                range,
            } => {
                let range: Range<usize> = range.into();
                if range.len() == 3 {
                    self.translate_any(fun, range.end - 1);
                    let Lookup::Local { index: local_index } = self.lookup_table[index] else {
                        panic!()
                    };
                    match simple_wasm_type(self.cart, self.tree(), node) {
                        SimpleWasmType::Span => {
                            add_instructions(
                                fun,
                                &[
                                    // Reverse order because we'd have loaded the last value most recently.
                                    Instruction::LocalSet(local_index + 1),
                                    Instruction::LocalSet(local_index),
                                ],
                            );
                        }
                        _ => {
                            fun.instruction(&Instruction::LocalSet(local_index));
                        }
                    }
                }
            }
            Nod::Branch { range, .. } => {
                let range: Range<usize> = range.into();
                for kid_index in range {
                    self.translate_any(fun, kid_index);
                }
            }
            Nod::Int32 { value } => {
                add_instructions(fun, &[Instruction::I32Const(value)]);
            }
            Nod::Leaf {
                token:
                    Token {
                        kind: TokenKind::String,
                        intern,
                    },
            } => {
                let text = &self.cart.interner[intern];
                let Lookup::Datum { address } = self.lookup_table[index] else {
                    panic!()
                };
                add_instructions(
                    fun,
                    &[
                        // String size and set memory address.
                        Instruction::I32Const(text.len() as i32),
                        Instruction::I32Const(address as i32),
                    ],
                );
            }
            Nod::Uid { module, num, .. } => {
                if module == 0 || module == 2 {
                    if let Lookup::Local { index: local_index } =
                        self.lookup_table[num as usize - 1]
                    {
                        match simple_wasm_type(self.cart, self.tree(), self.tree()[index]) {
                            SimpleWasmType::Span => {
                                fun.instruction(&Instruction::LocalGet(local_index));
                                fun.instruction(&Instruction::LocalGet(local_index + 1));
                            }
                            _ => {
                                // TODO Check type.
                                fun.instruction(&Instruction::LocalGet(local_index));
                            }
                        }
                    }
                }
            }
            _ => {}
        }
    }

    fn translate_branch(&self, fun: &mut Function, args_range: Range<usize>) {
        if args_range.len() != 1 {
            return;
        }
        let Nod::Branch { range, .. } = self.tree()[args_range.start].nod else {
            return;
        };
        let range: Range<usize> = range.into();
        self.translate_branch_cases(fun, range);
    }

    fn translate_branch_cases(&self, fun: &mut Function, range: Range<usize>) {
        let CoreExports {
            else_fun,
            pair_type,
            ..
        } = self.cart.core_exports;
        if !range.is_empty() {
            let Nod::Branch {
                range: case_range, ..
            } = self.tree()[range.start].nod
            else {
                return;
            };
            let case_range: Range<usize> = case_range.into();
            if case_range.is_empty() {
                return;
            }
            // Check if Pair or else.
            let Nod::Uid { module, num, .. } = self.tree()[case_range.start].nod else {
                return;
            };
            if module != pair_type.module {
                return;
            }
            if num == pair_type.num {
                if case_range.len() != 3 {
                    return;
                }
                self.translate_any(fun, case_range.start + 1);
                fun.instruction(&Instruction::If(BlockType::Result(ValType::I32)));
                self.translate_any(fun, case_range.start + 2);
                if range.len() > 1 {
                    fun.instruction(&Instruction::Else);
                    self.translate_branch_cases(fun, range.start + 1..range.end);
                }
                fun.instruction(&Instruction::End);
            } else if num == else_fun.num {
                if case_range.len() != 2 {
                    return;
                }
                self.translate_any(fun, case_range.start + 1);
            }
        }
    }

    fn translate_fun(&mut self, codes: &mut CodeSection, node: Node) {
        let Nod::Branch {
            kind: BranchKind::Fun,
            range,
        } = node.nod
        else {
            panic!()
        };
        let range: Range<usize> = range.into();
        let mut func = if range.len() == 3 {
            // Params
            let Nod::Branch {
                kind: BranchKind::Params,
                range: params_range,
            } = self.tree()[range.start].nod
            else {
                panic!()
            };
            let params_range: Range<usize> = params_range.into();
            let mut param_local_count = 0u32;
            let mut locals = vec![];
            for param_index in params_range {
                self.lookup_table[param_index] = Lookup::Local {
                    index: param_local_count,
                };
                match simple_wasm_type(self.cart, self.tree(), self.tree()[param_index]) {
                    SimpleWasmType::Span => {
                        param_local_count += 2;
                    }
                    _ => {
                        // TODO Check type.
                        param_local_count += 1;
                    }
                }
            }
            // Dig for more locals.
            fn dig_locals(
                node_index: usize,
                wasm: &mut WasmWriter,
                locals: &mut Vec<ValType>,
                param_local_count: u32,
            ) {
                let node = wasm.tree()[node_index];
                if let Nod::Branch { kind, range } = node.nod {
                    if kind == BranchKind::Def {
                        wasm.lookup_table[node_index] = Lookup::Local {
                            index: locals.len() as u32 + param_local_count,
                        };
                        match simple_wasm_type(wasm.cart, wasm.tree(), wasm.tree()[node_index]) {
                            SimpleWasmType::Span => {
                                locals.push(ValType::I32);
                                locals.push(ValType::I32);
                            }
                            _ => {
                                locals.push(ValType::I32);
                            }
                        }
                    }
                    let range: Range<usize> = range.into();
                    for kid_index in range {
                        dig_locals(kid_index, wasm, locals, param_local_count);
                    }
                }
            }
            let body_index = range.start + 2;
            dig_locals(body_index, self, &mut locals, param_local_count);
            let mut func = Function::new_with_locals_types(locals);
            // Body
            let Nod::Branch {
                kind: BranchKind::Block,
                range: body_range,
            } = self.tree()[body_index].nod
            else {
                panic!()
            };
            _ = body_range;
            self.translate_any(&mut func, body_index);
            // End
            func
        } else {
            Function::new([])
        };
        func.instruction(&Instruction::End);
        codes.function(&func);
    }

    fn tree(&self) -> &[Node] {
        &self.cart.modules[1].tree
    }
}

#[allow(dead_code)] // We'll definitely use this later.
fn align(size: u32, index: u32) -> u32 {
    index
        + match index % size {
            0 => 0,
            offset @ _ => size - offset,
        }
}

fn add_codes(codes: &mut CodeSection, locals: &[(u32, ValType)], instructions: &[Instruction]) {
    let mut func = Function::new(locals.iter().copied());
    add_instructions(&mut func, instructions);
    func.instruction(&Instruction::End);
    codes.function(&func);
}

fn add_instructions(func: &mut Function, instructions: &[Instruction<'_>]) {
    for instruction in instructions {
        func.instruction(instruction);
    }
}

fn add_fd_write_type(types: &mut TypeSection) -> EntityType {
    let params = vec![ValType::I32, ValType::I32, ValType::I32, ValType::I32];
    let results = vec![ValType::I32];
    types.function(params, results);
    EntityType::Function(types.len() - 1)
}

fn add_pop_type(types: &mut TypeSection) -> u32 {
    let params = vec![ValType::I32];
    let results = vec![];
    types.function(params, results);
    types.len() as u32 - 1
}

fn add_print_type(types: &mut TypeSection) -> u32 {
    let params = vec![ValType::I32, ValType::I32];
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

fn mem_arg(align: u32) -> MemArg {
    MemArg {
        offset: 0,
        align,
        memory_index: 0,
    }
}

#[derive(Clone, Copy, Debug)]
enum Lookup {
    Boring,
    Datum { address: u32 },
    Fun { index: u32 },
    Local { index: u32 },
}

#[derive(Clone, Copy, Debug)]
enum SimpleWasmType {
    I32,
    Other,
    Span,
}

fn simple_wasm_type(cart: &Cart, tree: &[Node], node: Node) -> SimpleWasmType {
    if node.typ.0 > 0 {
        if let Nod::Uid { module, num, .. } = tree[node.typ.0 as usize - 1].nod {
            if module == 1 {
                if num == cart.core_exports.int32_type.num {
                    return SimpleWasmType::I32;
                } else if num == cart.core_exports.text_type.num {
                    return SimpleWasmType::Span;
                }
            }
        }
    }
    // TODO I32 vs float types vs ...
    return SimpleWasmType::Other;
}

const NEWLINE: &str = "\n\0";
