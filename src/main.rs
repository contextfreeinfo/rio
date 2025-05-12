use std::{
    collections::HashMap,
    fs::{File, create_dir_all},
    io::{BufWriter, Read, Write},
    num::NonZeroU32,
    path::{Path, PathBuf},
    sync::Arc,
};

use anyhow::{Error, Result};
use clap::{Args, Parser, Subcommand, ValueEnum};
use lasso::ThreadedRodeo;
use lex::{Intern, Interner, Lexer, TokenKind};
use norm::write_tree;
use parse::{ParseNode, write_parse_tree};
use refine::resolve::UidInfo;
use tree::{TreeBuilder, TreeWriter};

mod lex;
mod norm;
mod parse;
mod refine;
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

/// Contains the reusable resources for building a module.
pub struct Cart {
    pub args: BuildArgs,
    pub core_interns: CoreInterns,
    /// From uid num to def idx.
    pub defs: Vec<usize>,
    pub interner: Interner,
    pub outdir: Option<PathBuf>,
    pub scope: Vec<UidInfo>,
    pub text: String,
    // TODO Just use tree for tokens?
    pub tokens: Vec<u8>,
    pub tops: HashMap<Intern, usize>,
    pub tree: Vec<u8>,
    pub tree_builder: TreeBuilder,
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
    let mut cart = Cart::new(args.clone());
    if let Err(err) = cart.build() {
        println!("Failed building: {}", &args.app);
        return Err(err);
    }
    Ok(())
}

impl Cart {
    fn new(args: BuildArgs) -> Self {
        // Resources
        let interner = Arc::new(ThreadedRodeo::default());
        // Reserve first slot for empty. TODO Reserve others?
        let empty_intern = interner.get_or_intern("");
        assert_eq!(Intern::default(), empty_intern);
        assert_eq!(NonZeroU32::new(1).unwrap(), empty_intern.into_inner());
        Self {
            args: args.clone(),
            core_interns: CoreInterns::new(&interner),
            defs: Default::default(),
            interner: interner.clone(),
            outdir: Default::default(),
            scope: Default::default(),
            text: Default::default(),
            tokens: Default::default(),
            tops: Default::default(),
            tree: Default::default(),
            tree_builder: Default::default(),
        }
    }

    fn build(&mut self) -> Result<()> {
        self.outdir = self.make_outdir()?;
        self.lex()?;
        self.parse()?;
        self.norm()?;
        self.refine()?;
        Ok(())
    }

    fn maybe_dump_normed(&self, stage: &'static str) -> Result<()> {
        if self.args.dump.contains(&DumpOption::Trees) {
            if let Some(outdir) = &self.outdir {
                let mut writer = make_dump_writer(stage, outdir)?;
                let mut writer = TreeWriter::new(&self.tree, &mut writer, self.interner.as_ref());
                write_tree(&mut writer)?;
                writeln!(writer.file)?;
                writeln!(writer.file, "Bytes: {}", self.tree.len())?;
            }
        }
        Ok(())
    }

    fn lex(&mut self) -> Result<()> {
        let mut lexer = Lexer::new(self);
        lex(&mut lexer)?;
        // dbg!(self.tokens.len());
        // if let Some(outdir) = &self.outdir {
        //     let mut writer = make_dump_writer("lex", outdir)?;
        //     for token in &self.tokens {
        //         let text = &self.interner[token.intern];
        //         writeln!(writer, "{:?}: {text:?}", token.kind)?;
        //     }
        // }
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

    fn norm(&mut self) -> Result<()> {
        norm::Normer::new(self).norm();
        self.maybe_dump_normed("norm")?;
        Ok(())
    }

    fn parse(&mut self) -> Result<()> {
        parse::Parser::new(self).parse();
        // dbg!(self.tree_bytes.len());
        if self.args.dump.contains(&DumpOption::Trees) {
            if let Some(outdir) = &self.outdir {
                let mut writer = make_dump_writer("parse", outdir)?;
                let mut writer = TreeWriter::new(&self.tree, &mut writer, self.interner.as_ref());
                write_parse_tree(&mut writer)?;
                writeln!(writer.file)?;
                writeln!(writer.file, "Bytes: {}", self.tree.len())?;
            }
        }
        Ok(())
    }

    fn refine(&mut self) -> Result<()> {
        refine::Refiner::new(self).refine();
        self.maybe_dump_normed("refine")?;
        Ok(())
    }
}

fn lex(lexer: &mut Lexer) -> Result<()> {
    let name = lexer.cart.args.app.as_str();
    match name {
        "core" => todo!(), // lexer.lex(include_str!("core.rio"))
        _ => {
            let mut file = File::open(name)?;
            file.read_to_string(&mut lexer.cart.text)?;
            lexer.lex();
            lexer.cart.text.clear();
        }
    };
    Ok(())
}

fn make_dump_writer(stage: &str, outdir: &Path) -> Result<BufWriter<File>> {
    let name = outdir.file_name().unwrap().to_string_lossy();
    let path = outdir.join(format!("{name}.{stage}.txt"));
    let file = File::create(path)?;
    Ok(BufWriter::new(file))
}

/// Provide easy access for comparing resolutions to core native definitions.
#[derive(Clone, Copy, Debug, Default)]
pub struct CoreInterns {
    add: Intern,
    dot: Intern,
    eq: Intern,
    ge: Intern,
    gt: Intern,
    le: Intern,
    lt: Intern,
    ne: Intern,
    pair: Intern,
    sub: Intern,
}

impl CoreInterns {
    pub fn new(interner: &Interner) -> Self {
        Self {
            add: interner.get_or_intern("add"), // TODO Straight to Uid?
            dot: interner.get_or_intern("dot"),
            eq: interner.get_or_intern("eq"),
            ge: interner.get_or_intern("ge"),
            gt: interner.get_or_intern("gt"),
            le: interner.get_or_intern("le"),
            lt: interner.get_or_intern("lt"),
            ne: interner.get_or_intern("ne"),
            pair: interner.get_or_intern("pair"), // TODO Straight to Uid?
            sub: interner.get_or_intern("sub"),
        }
    }

    pub fn token_to_intern(&self, node: ParseNode) -> Option<Intern> {
        let intern = match node {
            ParseNode::Leaf(token) => match token.kind {
                TokenKind::AngleClose => self.gt,
                TokenKind::AngleOpen => self.lt,
                TokenKind::Dot => self.dot,
                TokenKind::Eq => self.eq,
                TokenKind::GreaterEq => self.ge,
                TokenKind::LessEq => self.le,
                TokenKind::Minus => self.sub,
                TokenKind::NotEq => self.ne,
                TokenKind::Plus => self.add,
                TokenKind::To => self.pair,
                _ => return None,
            },
            _ => return None,
        };
        Some(intern)
    }
}
