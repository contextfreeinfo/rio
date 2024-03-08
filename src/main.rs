use std::{
    collections::HashMap,
    fs::{create_dir_all, File},
    io::{BufWriter, Read, Write},
    ops::Index,
    path::Path,
    sync::Arc,
    time::Instant,
};

use anyhow::{Error, Result};
use clap::{Args, Parser, Subcommand, ValueEnum};
use lasso::ThreadedRodeo;
use lex::{Intern, Interner, Token};
use link::link_modules;
use norm::{CoreInterns, Normer};
use run::{CoreExports, Module, Runner};
use tree::{write_tree, Nod, Node, Nody, TreeBuilder};
use wasm::write_wasm;

use crate::lex::Lexer;

// mod core;
mod lex;
mod link;
mod norm;
mod parse;
mod run;
mod tree;
mod typ;
mod util;
mod wasm;

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

#[derive(Args)]
pub struct BuildArgs {
    app: String,
    #[arg(long)]
    dump: Vec<DumpOption>,
    #[arg(long)]
    outdir: Option<String>,
    #[arg(long)]
    time: bool,
}

#[derive(Clone, Copy, Eq, Ord, PartialEq, PartialOrd, ValueEnum)]
enum DumpOption {
    Trees,
}

fn main() -> Result<()> {
    env_logger::init();
    let cli = Cli::parse();
    match &cli.command {
        Commands::Build(args) => run_app(args),
    }
}

pub struct Cart {
    pub buffer: String,
    pub core_exports: CoreExports,
    pub core_interns: CoreInterns,
    pub interner: Interner,
    pub modules: Vec<Module>,
    pub module_map: HashMap<Intern, u16>,
    pub tree_builder: TreeBuilder,
}

fn run_app(args: &BuildArgs) -> Result<()> {
    let start = Instant::now();
    // Resources
    let interner = Arc::new(ThreadedRodeo::default());
    // Reserve first slot for empty. TODO Reserve others?
    interner.get_or_intern("");
    let tree_builder = TreeBuilder::default();
    let cart = Cart {
        buffer: String::new(),
        core_exports: Default::default(),
        core_interns: CoreInterns::new(&interner),
        interner: interner.clone(),
        modules: vec![],
        module_map: HashMap::new(),
        tree_builder,
    };
    // Process
    let mut cart = build(args, "core", cart)?;
    cart = build(args, args.app.as_str(), cart)?;
    // Link
    link_modules(&cart);
    let prewasm = start.elapsed();
    // Write
    let prewrite = write_wasm(args, &cart, start)?;
    // println!("type tree size: {}", std::mem::size_of::<typ::TypeTree>());
    if args.time {
        println!(
            "building {}: main {:.1?}, wasm {:.1?}, write {:.1?}",
            args.app.as_str(),
            prewasm,
            prewrite,
            start.elapsed()
        );
    }
    Ok(())
}

fn build(args: &BuildArgs, name: &str, mut cart: Cart) -> Result<Cart> {
    let interner = cart.interner.clone();
    // Lex
    // TODO Reserver lexer somewhere to reuse its resources?
    let mut lexer = Lexer::new(&mut cart);
    lex(name, &mut lexer)?;
    let tokens = lexer.tokens;
    // Parse
    let (parsed_tree, cart) = parse(cart, &tokens);
    dump_tree("parse", args, name, &parsed_tree, interner.as_ref())?;
    // Norm
    let (mut tree, mut cart) = norm(cart, parsed_tree);
    dump_tree("norm", args, name, &tree, interner.as_ref())?;
    // Run
    // TODO Keep runner to reuse resources, or move them into module?
    let runner = Runner::new(&mut cart);
    let module = runner.run(interner.get_or_intern(name), &mut tree);
    if name == "core" {
        cart.core_exports = CoreExports::extract(&module, &cart.interner);
        // println!("{:?}", cart.core_exports);
    }
    cart.module_map
        .insert(module.name, cart.modules.len() as u16 + 1);
    cart.modules.push(module);
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

fn parse(cart: Cart, tokens: &[Token]) -> (Vec<Nod>, Cart) {
    let mut parser = parse::Parser::new(cart);
    parser.parse(tokens);
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
    args: &BuildArgs,
    name: &str,
    tree: &[Node],
    map: &Map,
) -> Result<()>
where
    Map: Index<Intern, Output = str>,
    Node: Nody,
{
    if args
        .dump
        .iter()
        .any(|dump| matches!(dump, DumpOption::Trees))
    {
        if let Some(outdir) = &args.outdir {
            let name = Path::new(name)
                .file_stem()
                .ok_or(Error::msg("no name"))?
                .to_str()
                .ok_or(Error::msg("bad name"))?;
            let subdir = Path::new(outdir).join(name);
            create_dir_all(subdir.clone())?;
            let path = subdir.join(format!("{name}.{stage}.txt"));
            let file = File::create(path)?;
            let mut buf = BufWriter::new(file);
            write_tree(&mut buf, &tree, map)?;
            writeln!(&mut buf, "")?;
            writeln!(&mut buf, "Node size: {}", std::mem::size_of::<Node>())?;
            writeln!(&mut buf, "Tree len: {}", tree.len())?;
        }
    }
    Ok(())
}
