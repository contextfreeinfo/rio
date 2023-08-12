use std::{
    fs::{create_dir_all, File},
    io::{Read, Write},
    ops::Index,
    path::Path,
    sync::Arc,
};

use anyhow::{Error, Result};
use clap::{Args, Parser, Subcommand};
use lasso::ThreadedRodeo;
use lex::{Intern, Token, TokenKind};
use norm::Normer;
use run::Runner;
use tree::{write_tree, Nod, Node, Nody, TreeBuilder};

use crate::lex::Lexer;

// mod core;
mod lex;
mod norm;
mod parse;
mod run;
mod tree;

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

pub struct Cart {
    pub tree_builder: TreeBuilder,
    pub none: Token,
}

fn run_app(args: &RunArgs) -> Result<()> {
    // Resources
    let interner = Arc::new(ThreadedRodeo::default());
    let none_key = interner.get_or_intern("");
    // println!("none_key: {:?}", none_key.into_inner());
    let none = Token::new(TokenKind::None, none_key);
    let mut lexer = Lexer::new(interner.clone());
    let tree_builder = TreeBuilder::default();
    let cart = Cart { tree_builder, none };
    // Lex
    lex(args, &mut lexer)?;
    // Parse
    let (parsed_tree, cart) = parse(cart, &lexer);
    dump_tree("parse", args, &parsed_tree, lexer.interner.as_ref())?;
    // Norm
    let (mut tree, cart) = norm(cart, parsed_tree);
    dump_tree("norm", args, &tree, lexer.interner.as_ref())?;
    // Run
    let mut runner = Runner::new(cart);
    runner.run(&mut tree);
    dump_tree("run", args, &tree, lexer.interner.as_ref())?;
    // let cart = runner.cart;
    // Done
    Ok(())
}

fn lex(args: &RunArgs, lexer: &mut Lexer) -> Result<(), Error> {
    let mut file = File::open(args.app.as_str())?;
    let mut source = String::new();
    file.read_to_string(&mut source)?;
    lexer.lex(source.as_str());
    Ok(())
}

fn parse(cart: Cart, lexer: &Lexer) -> (Vec<Nod>, Cart) {
    let mut parser = parse::Parser::new(cart);
    parser.parse(&lexer.tokens);
    let mut parsed_tree = Vec::<Nod>::with_capacity(parser.cart.tree_builder.nodes.len());
    parsed_tree.extend(parser.cart.tree_builder.nodes.iter().map(|n| n.nod()));
    (parsed_tree, parser.cart)
}

fn norm(cart: Cart, parsed_tree: Vec<Nod>) -> (Vec<Node>, Cart) {
    let mut tree = Vec::<Node>::new();
    let mut normer = Normer::new(cart);
    normer.norm(&parsed_tree, &mut tree);
    (tree, normer.cart)
}

fn dump_tree<Map, Node>(stage: &str, args: &RunArgs, tree: &[Node], map: &Map) -> Result<()>
where
    Map: Index<Intern, Output = str>,
    Node: Nody,
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
