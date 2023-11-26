use std::{
    fs::{create_dir_all, File},
    io::Write,
    path::Path,
};

use anyhow::{Error, Ok, Result};
use wasm_encoder::{
    CodeSection, DataSection, EntityType, ExportSection, FunctionSection, GlobalSection,
    ImportSection, MemorySection, MemoryType, TypeSection, ValType,
};

use crate::{BuildArgs, Cart};

pub fn write_wasm(args: &BuildArgs, cart: &Cart) -> Result<()> {
    let _ = cart;
    let mut writer = WasmWriter::new();
    writer.build()?;
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

    fn build(&mut self) -> Result<()> {
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

    fn build_types(&mut self) {
        let mut types = TypeSection::new();
        add_fd_write_type(&mut types);
        self.fd_write_type = Some(EntityType::Function(0));
        self.module.section(&types);
    }
}

fn add_fd_write_type(types: &mut TypeSection) {
    let params = vec![ValType::I32, ValType::I32, ValType::I32, ValType::I32];
    let results = vec![ValType::I32];
    types.function(params, results);
}
