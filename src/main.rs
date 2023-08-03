use std::{
    fs::{create_dir_all, read_to_string, File},
    io::Write,
    ops::Index,
    path::Path,
    sync::Arc,
};

use anyhow::{Error, Result};
use clap::{Args, Parser, Subcommand};
use lasso::ThreadedRodeo;
use lex::Intern;
use norm::Normer;
use parse::TreeBuilder;
use run::Runner;

use crate::{
    lex::Lexer,
    parse::{write_tree, Node},
};

mod lex;
mod norm;
mod parse;
mod run;

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
    let none_key = interner.get_or_intern("");
    let mut lexer = Lexer::new(interner.clone());
    let source = read_to_string(args.app.as_str())?;
    let tokens = lexer.lex(source.as_str());
    let builder = TreeBuilder::default();
    let mut parser = parse::Parser::new(builder);
    let parsed_tree = parser.parse(&tokens);
    dump_tree("parse", args, &parsed_tree, interner.as_ref())?;
    let mut normer = Normer::new(parser.builder, none_key);
    let normed_tree = normer.norm(&parsed_tree);
    dump_tree("norm", args, &normed_tree, interner.as_ref())?;
    let mut runner = Runner::new(normer.builder);
    runner.run(&normed_tree);
    Ok(())
}

fn dump_tree<Map>(stage: &str, args: &RunArgs, tree: &[Node], map: &Map) -> Result<()>
where
    Map: Index<Intern, Output = str>,
{
    if let Some(dump) = &args.dump {
        create_dir_all(dump)?;
        let name = Path::new(&args.app)
            .file_stem()
            .ok_or(Error::msg("no name"))?
            .to_str()
            .ok_or(Error::msg("bad name"))?;
        let path = Path::new(dump).join(format!("{name}.{stage}.txt"));
        let mut file = File::create(path)?;
        write_tree(&mut file, &tree, map)?;
        writeln!(&mut file, "")?;
        writeln!(&mut file, "Node size: {}", std::mem::size_of::<Node>())?;
        writeln!(&mut file, "Tree len: {}", tree.len())?;
    }
    Ok(())
}
