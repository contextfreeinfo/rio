use std::{
    fs::{File, create_dir_all},
    io::{BufWriter, Read, Write},
    path::{Path, PathBuf},
    sync::Arc,
};

use anyhow::{Error, Result};
use clap::{Args, Parser, Subcommand, ValueEnum};
use lasso::ThreadedRodeo;
use lex::{Interner, Lexer, Token, TokenKind};
use log::debug;
use parse::{ParseBranch, ParseBranchKind, ParseNode};

mod lex;
mod parse;
mod tree;

#[derive(clap::Parser)]
#[command(about, version, long_about = None)]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand)]
enum Commands {
    Build(BuildArgs),
}

#[derive(Args, Clone, Debug)]
pub struct BuildArgs {
    app: String,
    #[arg(long)]
    dump: Vec<DumpOption>,
    #[arg(long)]
    outdir: Option<String>,
    #[arg(long)]
    time: bool,
}

#[derive(Clone, Copy, Debug, Eq, Ord, PartialEq, PartialOrd, ValueEnum)]
enum DumpOption {
    Trees,
}

pub struct Cart {
    pub args: BuildArgs,
    pub buffer: String,
    pub interner: Interner,
}

fn main() -> Result<()> {
    env_logger::init();
    // dbg!(size_of::<ParseNode>());
    // dbg!(align_of::<ParseNode>());
    // dbg!(size_of::<ParseBranch>());
    // dbg!(align_of::<ParseBranch>());
    // dbg!(size_of::<Token>());
    // dbg!(align_of::<Token>());
    // dbg!(align_of::<f64>());
    // dbg!(TokenKind::AngleClose as u32);
    // dbg!(ParseBranchKind::Infix as u32);
    let cli = Cli::parse();
    match cli.command {
        Commands::Build(args) => build(args),
    }
}

fn build(args: BuildArgs) -> Result<()> {
    // Resources
    let interner = Arc::new(ThreadedRodeo::default());
    // Reserve first slot for empty. TODO Reserve others?
    interner.get_or_intern("");
    let mut cart = Cart {
        args: args.clone(),
        buffer: String::new(),
        interner: interner.clone(),
    };
    if let Err(err) = cart.build() {
        println!("Failed building: {}", &args.app);
        return Err(err);
    }
    Ok(())
}

impl Cart {
    fn build(&mut self) -> Result<()> {
        let outdir = self.make_outdir()?;
        self.lex(outdir.as_ref())?;
        Ok(())
    }

    fn lex(&mut self, outdir: Option<&PathBuf>) -> Result<()> {
        let mut lexer = Lexer::new(self);
        lex(&mut lexer)?;
        let tokens = lexer.tokens;
        if let Some(outdir) = outdir {
            let name = outdir.file_name().unwrap().to_string_lossy();
            let stage = "lex";
            let path = outdir.join(format!("{name}.{stage}.txt"));
            dbg!(&path);
            let file = File::create(path)?;
            let mut writer = BufWriter::new(file);
            for token in &tokens {
                let text = &self.interner[token.intern];
                writeln!(writer, "{:?}: {text:?}", token.kind)?;
            }
        }
        Ok(())
    }

    fn make_outdir(&self) -> Result<Option<PathBuf>> {
        if self.args.dump.is_empty() {
            return Ok(None);
        }
        let Some(outdir) = self.args.outdir.as_ref() else {
            return Ok(None);
        };
        let name = Path::new(&self.args.app)
            .file_stem()
            .ok_or(Error::msg("no name"))?
            .to_str()
            .ok_or(Error::msg("bad name"))?;
        let subdir = Path::new(outdir).join(name);
        create_dir_all(subdir.clone())?;
        Ok(Some(subdir))
    }
}

fn lex(lexer: &mut Lexer) -> Result<()> {
    let name = lexer.cart.args.app.as_str();
    match name {
        "core" => todo!(), // lexer.lex(include_str!("core.rio"))
        _ => {
            let mut file = File::open(name)?;
            let mut source = String::new();
            file.read_to_string(&mut source)?;
            lexer.lex(&source);
        }
    };
    Ok(())
}
