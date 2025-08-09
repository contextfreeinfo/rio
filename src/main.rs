use std::{cell::RefCell, fs::File, io::Read};

use anyhow::Result;
use argh::FromArgs;

use crate::{intern::Interner, lex::Lexer, parse::Parser, tree::TreeBuilder};

mod intern;
mod lex;
mod parse;
mod tree;

#[derive(FromArgs, PartialEq, Debug)]
/// Top-level command.
struct Args {
    #[argh(subcommand)]
    command: Command,
}

#[derive(FromArgs, PartialEq, Debug)]
#[argh(subcommand)]
enum Command {
    Build(BuildArgs),
}

#[derive(FromArgs, PartialEq, Debug)]
/// Build from Rio source.
#[argh(subcommand, name = "build")]
struct BuildArgs {
    #[argh(positional)]
    /// path to Rio source root
    path: String,
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

#[derive(Default)]
struct Cart {
    pub bytes: Vec<u8>,
    pub interner: RefCell<Interner>,
    pub text: String,
    pub tree_builder: TreeBuilder,
}

fn build(args: BuildArgs) -> Result<()> {
    let mut cart = Cart::default();
    // Always keep an empty string at zero.
    cart.interner.borrow_mut().intern("");
    File::open(args.path)?.read_to_string(&mut cart.text)?;
    // dbg!(cart.text.len());
    Lexer::new(&mut cart).lex();
    // dbg!(cart.bytes.len());
    Parser::new(&mut cart).parse();
    // dbg!(cart.bytes.len());
    Ok(())
}
