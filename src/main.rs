use std::{
    collections::HashMap,
    fs::{create_dir_all, File},
    io::{Read, Write},
    ops::Index,
    path::Path,
    sync::Arc,
};

use anyhow::{Error, Result};
use clap::{Args, Parser, Subcommand};
use lasso::ThreadedRodeo;
use lex::{Intern, Interner};
use norm::Normer;
use run::{CoreExports, Module, Runner};
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
    pub core: Option<Module>,
    pub core_exports: CoreExports,
    pub interner: Interner,
    pub modules: HashMap<Intern, Module>,
    pub tree_builder: TreeBuilder,
}

fn run_app(args: &RunArgs) -> Result<()> {
    // Resources
    let interner = Arc::new(ThreadedRodeo::default());
    // Reserve first slot for empty. TODO Reserve others?
    interner.get_or_intern("");
    let tree_builder = TreeBuilder::default();
    let cart = Cart {
        core: None,
        core_exports: Default::default(),
        interner: interner.clone(),
        modules: HashMap::new(),
        tree_builder,
    };
    // Process
    let cart = build(args, "core", cart)?;
    build(args, args.app.as_str(), cart)?;
    Ok(())
}

fn build(args: &RunArgs, name: &str, cart: Cart) -> Result<Cart> {
    let interner = cart.interner.clone();
    // Lex
    // TODO Reserver lexer somewhere to reuse its resources?
    let mut lexer = Lexer::new(interner.clone());
    lex(name, &mut lexer)?;
    // Parse
    let (parsed_tree, cart) = parse(cart, &lexer);
    dump_tree("parse", args, name, &parsed_tree, interner.as_ref())?;
    // Norm
    let (mut tree, mut cart) = norm(cart, parsed_tree);
    dump_tree("norm", args, name, &tree, interner.as_ref())?;
    // Run
    // TODO Keep runner to reuse resources, or move them into module?
    let runner = Runner::new(&mut cart);
    let module = runner.run(interner.get_or_intern(name), &mut tree);
    if name == "core" {
        cart.core = Some(module);
        cart.core_exports = CoreExports::extract(cart.core.as_ref().unwrap(), &cart.interner);
        // println!("{:?}", cart.core_exports);
    } else {
        cart.modules.insert(module.name, module);
    }
    dump_tree("run", args, name, &tree, interner.as_ref())?;
    // Done
    Ok(cart)
}

fn lex(name: &str, lexer: &mut Lexer) -> Result<(), Error> {
    match name {
        "core" => lexer.lex(include_str!("core.rio")),
        _ => {
            let mut file = File::open(name)?;
            let mut source = String::new();
            file.read_to_string(&mut source)?;
            lexer.lex(&source);
        }
    };
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

fn dump_tree<Map, Node>(
    stage: &str,
    args: &RunArgs,
    name: &str,
    tree: &[Node],
    map: &Map,
) -> Result<()>
where
    Map: Index<Intern, Output = str>,
    Node: Nody,
{
    if let Some(dump) = &args.dump {
        create_dir_all(dump)?;
        let name = Path::new(name)
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
