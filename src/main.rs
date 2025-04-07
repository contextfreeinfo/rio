use std::{fs::File, io::Read, sync::Arc};

use anyhow::Result;
use clap::{Args, Parser, Subcommand};
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

#[derive(Args, Debug)]
pub struct BuildArgs {
    app: String,
}

pub struct Cart {
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
        buffer: String::new(),
        interner: interner.clone(),
    };
    let mut lexer = Lexer::new(&mut cart);
    lex(args.app.as_str(), &mut lexer)?;
    let tokens = lexer.tokens;
    debug!("{tokens:?}");
    Ok(())
}

fn lex(name: &str, lexer: &mut Lexer) -> Result<()> {
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
