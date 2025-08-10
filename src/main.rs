use std::{
    cell::RefCell,
    fs::{File, create_dir_all},
    io::{BufWriter, Read, Write},
    path::{Path, PathBuf},
};

use anyhow::{Error, Result};
use argh::{FromArgValue, FromArgs};

use crate::{
    intern::Interner,
    lex::Lexer,
    parse::{Parser, write_parse_tree},
    tree::{TreeBuilder, TreeWriter},
};

mod intern;
mod lex;
mod parse;
mod tree;

#[derive(Clone, FromArgs, PartialEq, Debug)]
/// Top-level command.
struct Args {
    #[argh(subcommand)]
    command: Command,
}

#[derive(Clone, FromArgs, PartialEq, Debug)]
#[argh(subcommand)]
enum Command {
    Build(BuildArgs),
}

#[derive(Clone, FromArgs, PartialEq, Debug)]
/// Build from Rio source.
#[argh(subcommand, name = "build")]
struct BuildArgs {
    #[argh(positional)]
    /// path to Rio source root
    path: String,

    #[argh(option)]
    /// debug info for dumping
    dump: Vec<DumpOption>,

    #[argh(option)]
    /// output directory
    outdir: Option<String>,

    #[argh(switch)]
    /// whether to report build timing
    time: bool,
}

#[derive(Clone, Copy, Debug, Eq, FromArgValue, Ord, PartialEq, PartialOrd)]
enum DumpOption {
    Trees,
}

fn main() -> Result<()> {
    // Here for now just to see what it adds to bin size.
    // let token = lex::Token {
    //     intern: 0,
    //     kind: lex::TokenKind::Define,
    // };
    // dbg!(postcard::to_allocvec(&token)?);
    // dbg!(wasm_encoder::Module::new().as_slice());
    let args: Args = argh::from_env();
    match args.command {
        Command::Build(args) => build(args),
    }
}

fn build(args: BuildArgs) -> Result<()> {
    let mut cart = Cart::new(args);
    cart.build()
}

struct Cart {
    pub args: BuildArgs,
    pub bytes: Vec<u8>,
    pub name: String,
    pub interner: RefCell<Interner>,
    pub outdir: Option<PathBuf>,
    pub text: String,
    pub tree_builder: TreeBuilder,
}

impl Cart {
    fn new(args: BuildArgs) -> Self {
        let s = Self {
            args: args.clone(),
            bytes: Default::default(),
            interner: Default::default(),
            name: Default::default(),
            outdir: Default::default(),
            text: Default::default(),
            tree_builder: Default::default(),
        };
        // Always keep an empty string at zero.
        s.interner.borrow_mut().intern("");
        s
    }

    fn build(&mut self) -> Result<()> {
        let name = Path::new(&self.args.path)
            .file_stem()
            .ok_or(Error::msg("no name"))?
            .to_str()
            .ok_or(Error::msg("bad name"))?;
        self.name.clear();
        self.name.push_str(name);
        self.outdir = self.make_outdir()?;
        // Always keep an empty string at zero.
        self.interner.borrow_mut().intern("");
        File::open(&self.args.path)?.read_to_string(&mut self.text)?;
        // dbg!(cart.text.len());
        Lexer::new(self).lex();
        // dbg!(cart.bytes.len());
        Parser::new(self).parse();
        if self.args.dump.contains(&DumpOption::Trees) && !self.name.is_empty() {
            if let Some(outdir) = &self.outdir {
                let interner = &*self.interner.borrow();
                let mut writer = make_dump_writer("parse", outdir)?;
                let mut writer = TreeWriter::new(&self.bytes, &mut writer, interner);
                write_parse_tree(&mut writer)?;
                writeln!(writer.file)?;
                writeln!(writer.file, "Bytes: {}", self.bytes.len())?;
            }
        }
        // dbg!(cart.bytes.len());
        Ok(())
    }

    fn make_outdir(&self) -> Result<Option<PathBuf>> {
        if self.args.dump.is_empty() {
            return Ok(None);
        }
        let Some(outdir) = self.args.outdir.as_ref() else {
            return Ok(None);
        };
        let subdir = Path::new(outdir).join(&self.name);
        create_dir_all(subdir.clone())?;
        Ok(Some(subdir))
    }
}

fn make_dump_writer(stage: &str, outdir: &Path) -> Result<BufWriter<File>> {
    let name = outdir.file_name().unwrap().to_string_lossy();
    let path = outdir.join(format!("{name}.{stage}.txt"));
    let file = File::create(path)?;
    Ok(BufWriter::new(file))
}
