use std::{
    fs::{create_dir_all, read_to_string, File},
    io::Write,
    path::Path,
    sync::Arc,
};

use anyhow::{Error, Result};
use clap::{Args, Parser, Subcommand};
use lasso::ThreadedRodeo;
use norm::Normer;
use parse::TreeBuilder;

use crate::{
    lex::Lexer,
    parse::{write_tree, Node},
};

mod lex;
mod norm;
mod parse;

#[derive(clap::Parser)]
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
    #[arg(long)]
    dump: Option<String>,
}

fn main() -> Result<()> {
    env_logger::init();
    let cli = Cli::parse();
    match &cli.command {
        Commands::Run(args) => run_app(args),
    }
}

fn run_app(args: &RunArgs) -> Result<()> {
    let interner = Arc::new(ThreadedRodeo::default());
    let mut lexer = Lexer::new(interner.clone());
    let source = read_to_string(args.app.as_str())?;
    let tokens = lexer.lex(source.as_str());
    let builder = TreeBuilder::default();
    let mut parser = parse::Parser::new(builder);
    let tree = parser.parse(&tokens);
    dump_tree(args, &tree, interner)?;
    let mut normer = Normer::new(parser.builder);
    normer.norm(&tree);
    Ok(())
}

fn dump_tree(args: &RunArgs, tree: &[Node], interner: Arc<ThreadedRodeo>) -> Result<()> {
    if let Some(dump) = &args.dump {
        create_dir_all(dump)?;
        let name = Path::new(&args.app)
            .file_stem()
            .ok_or(Error::msg("no name"))?
            .to_str()
            .ok_or(Error::msg("bad name"))?;
        let path = Path::new(dump).join(format!("{name}.parse.txt"));
        let mut file = File::create(path)?;
        write_tree(&mut file, &tree, interner.as_ref())?;
        writeln!(&mut file, "")?;
        writeln!(&mut file, "Node size: {}", std::mem::size_of::<Node>())?;
        writeln!(&mut file, "Tree len: {}", tree.len())?;
    }
    Ok(())
}
