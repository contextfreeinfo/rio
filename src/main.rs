use std::{sync::Arc, fs::read_to_string};

use anyhow::Result;
use clap::{Args, Parser, Subcommand};
use lasso::ThreadedRodeo;

use crate::lex::Lexer;

mod lex;

#[derive(Parser)]
#[command(about, version, long_about = None)]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand)]
enum Commands {
    Run(RunArgs),
}

#[derive(Args)]
struct RunArgs {
    app: String,
}

fn main() -> Result<()> {
    env_logger::init();
    let cli = Cli::parse();
    match &cli.command {
        Commands::Run(args) => {
            run_app(args)
        }
    }
}

fn run_app(args: &RunArgs) -> Result<()> {
    let rodeo = Arc::new(ThreadedRodeo::default());
    let lexer = Lexer { rodeo: rodeo.clone() };
    let source = read_to_string(args.app.as_str())?;
    lexer.lex(source.as_str());
    Ok(())
}
