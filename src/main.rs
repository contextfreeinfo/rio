use std::{fs::File, io::Read, sync::Arc};

use anyhow::Result;
use clap::{Args, Parser, Subcommand};
use lasso::ThreadedRodeo;
use lex::{Interner, Lexer};
use log::info;

mod lex;

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
    // dbg!(tokens);
    let _ = tokens;
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
