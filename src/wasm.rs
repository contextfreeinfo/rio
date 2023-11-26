use std::{
    fs::{create_dir_all, File},
    io::Write,
    ops::Range,
    path::Path,
    vec,
};

use anyhow::{Error, Ok, Result};
use wasm_encoder::{
    CodeSection, DataSection, EntityType, ExportSection, FunctionSection, GlobalSection,
    ImportSection, MemorySection, MemoryType, TypeSection, ValType,
};

use crate::{
    tree::{BranchKind, Nod},
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
    module: wasm_encoder::Module,
}

impl WasmWriter {
    fn new() -> WasmWriter {
        WasmWriter {
            fd_write_type: None,
            module: wasm_encoder::Module::new(),
        }
    }

    fn build(&mut self, cart: &Cart) -> Result<()> {
        self.build_types(cart);
        self.build_imports();
        self.build_functions();
        self.build_memory();
        self.build_globals();
        self.build_exports();
        self.build_codes();
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

    fn build_codes(&mut self) {
        let codes = CodeSection::new();
        self.module.section(&codes);
    }

    fn build_data(&mut self) {
        let data = DataSection::new();
        self.module.section(&data);
    }

    fn build_exports(&mut self) {
        let exports = ExportSection::new();
        self.module.section(&exports);
    }

    fn build_functions(&mut self) {
        let functions = FunctionSection::new();
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
        add_fd_write_type(&mut types);
        self.fd_write_type = Some(EntityType::Function(0));
        // Add function types.
        let core = cart.core_exports;
        let void = core.void_type.num;
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
                    for type_index in types_range {
                        let type_kid = tree[type_index];
                        if let Nod::Branch {
                            kind: BranchKind::FunType,
                            range: params_range,
                            ..
                        } = type_kid.nod
                        {
                            // TODO Check for things other than int, like float.
                            let params = vec![ValType::I32; params_range.len()];
                            let results = match type_kid.typ.0 {
                                0 => vec![],
                                typ @ _ => match tree[typ as usize - 1].nod {
                                    Nod::Uid { module: 1, num, .. } => match num {
                                        _ if num == void => vec![],
                                        _ => vec![ValType::I32],
                                    },
                                    _ => {
                                        println!("What to do? {typ}");
                                        vec![]
                                    }
                                },
                            };
                            // TODO Reuse previously defined types at wasm level.
                            // TODO Keep map from module type to wasm type.
                            types.function(params, results);
                        }
                    }
                    break;
                }
            }
        }
        self.module.section(&types);
    }
}

fn add_fd_write_type(types: &mut TypeSection) {
    let params = vec![ValType::I32, ValType::I32, ValType::I32, ValType::I32];
    let results = vec![ValType::I32];
    types.function(params, results);
}
