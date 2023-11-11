use std::{
    fs::{create_dir_all, File},
    io::Write,
    path::Path,
};

use anyhow::{Error, Ok, Result};
use wasm_encoder::{CodeSection, ExportSection, FunctionSection, TypeSection};

use crate::{BuildArgs, Cart};

pub fn write_wasm(args: &BuildArgs, cart: &Cart) -> Result<()> {
    // TODO Default to current dir?
    let Some(outdir) = &args.outdir else {
        return Ok(());
    };
    let mut module = wasm_encoder::Module::new();
    let _ = cart;
    // Types
    let types = TypeSection::new();
    module.section(&types);
    // Functions
    let functions = FunctionSection::new();
    module.section(&functions);
    // Exports
    let exports = ExportSection::new();
    module.section(&exports);
    // Codes
    let codes = CodeSection::new();
    module.section(&codes);
    // Finish
    let wasm = module.finish();
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
