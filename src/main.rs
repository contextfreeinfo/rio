use std::{
    collections::HashMap,
    fs::{File, create_dir_all},
    io::{BufWriter, Read, Write},
    num::NonZeroU32,
    path::{Path, PathBuf},
    sync::{Arc, RwLock},
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
mod value;

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

pub type DefNum = usize;
pub type TreeIdx = usize;

#[derive(Default)]
pub struct Module {
    pub defs: Vec<TreeIdx>,
    pub tops: HashMap<Intern, DefNum>,
    pub tree: Vec<u8>,
}

#[derive(Default)]
pub struct Catalog {
    // TODO cart pool?
    // TODO module map?
    pub modules: Vec<Module>,
}

/// Contains the reusable resources for building a module.
pub struct Cart {
    pub args: BuildArgs,
    pub catalog: Arc<RwLock<Catalog>>,
    pub core_defs: CoreDefs,
    pub core_interns: CoreInterns,
    /// From uid num to def idx.
    pub defs: Vec<TreeIdx>,
    pub interner: Interner,
    pub name: String,
    pub outdir: Option<PathBuf>,
    pub path: String,
    pub scope: Vec<UidInfo>,
    pub text: String,
    // TODO Just use tree for tokens?
    pub tokens: Vec<u8>,
    pub tops: HashMap<Intern, DefNum>,
    pub tree: Vec<u8>,
    pub tree_builder: TreeBuilder,
}

fn main() -> Result<()> {
    env_logger::init();
    let cli = Cli::parse();
    match cli.command {
        Commands::Build(args) => build(args),
    }
}

fn build(args: BuildArgs) -> Result<()> {
    // Start catalog and interner, avoiding zeros.
    let mut catalog = Catalog::default();
    catalog.modules.push(Default::default());
    let catalog = Arc::new(RwLock::new(catalog));
    let interner = Arc::new(ThreadedRodeo::default());
    let empty_intern = interner.get_or_intern("");
    assert_eq!(Intern::default(), empty_intern);
    assert_eq!(NonZeroU32::new(1).unwrap(), empty_intern.into_inner());
    // Use shared resources above for each cart (once we have multiple carts).
    let mut cart = Cart::new(args.clone(), catalog.clone(), interner.clone())?;
    // Put in core as module 1.
    if let Err(err) = cart.build("core", Some(CORE_TEXT)) {
        println!("Failed building: core");
        return Err(err);
    }
    cart.extract_core_defs();
    // Then build requested module.
    // TODO And imports and things.
    if let Err(err) = cart.build(&args.app, None) {
        println!("Failed building: {}", &args.app);
        return Err(err);
    }
    Ok(())
}

impl Cart {
    fn new(args: BuildArgs, catalog: Arc<RwLock<Catalog>>, interner: Interner) -> Result<Self> {
        Ok(Self {
            args: args.clone(),
            catalog,
            core_defs: Default::default(),
            core_interns: CoreInterns::new(&interner),
            defs: Default::default(),
            interner,
            name: Default::default(),
            outdir: Default::default(),
            path: Default::default(),
            scope: Default::default(),
            text: Default::default(),
            tokens: Default::default(),
            tops: Default::default(),
            tree: Default::default(),
            tree_builder: Default::default(),
        })
    }

    fn build(&mut self, path: &str, text: Option<&str>) -> Result<()> {
        // Update path and name.
        self.path.clear();
        self.path.push_str(path);
        let name = Path::new(path)
            .file_stem()
            .ok_or(Error::msg("no name"))?
            .to_str()
            .ok_or(Error::msg("bad name"))?;
        self.name.clear();
        self.name.push_str(name);
        self.outdir = self.make_outdir()?;
        // Build.
        self.lex(text)?;
        self.parse()?;
        self.norm()?;
        self.refine()?;
        let module = Module {
            // Expect clone to trim capacity, which it seems to at the moment.
            defs: self.defs.clone(),
            tops: self.tops.clone(),
            tree: self.tree.clone(),
        };
        let mut catalog = self.catalog.write().unwrap();
        catalog.modules.push(module);
        Ok(())
    }

    fn extract_core_defs(&mut self) {
        let catalog = self.catalog.read().unwrap();
        let core = &catalog.modules[1];
        // Caching frequently used things in each cart should speed processing.
        self.core_defs = CoreDefs {
            int: core.tops[&self.core_interns.int],
            log: core.tops[&self.core_interns.log],
            text: core.tops[&self.core_interns.text],
        };
    }

    fn lex(&mut self, text: Option<&str>) -> Result<()> {
        let mut lexer = Lexer::new(self);
        lex(&mut lexer, text)?;
        Ok(())
    }

    fn make_outdir(&self) -> Result<Option<PathBuf>> {
        if self.args.dump.is_empty() {
            return Ok(None);
        }
        let Some(outdir) = self.args.outdir.as_ref() else {
            return Ok(None);
        };
        let subdir = Path::new(outdir).join(&self.name);
        create_dir_all(subdir.clone())?;
        Ok(Some(subdir))
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

    fn norm(&mut self) -> Result<()> {
        norm::Normer::new(self).norm();
        self.maybe_dump_normed("norm")?;
        Ok(())
    }

    fn parse(&mut self) -> Result<()> {
        parse::Parser::new(self).parse();
        if self.args.dump.contains(&DumpOption::Trees) && !self.name.is_empty() {
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

fn lex(lexer: &mut Lexer, text: Option<&str>) -> Result<()> {
    let path = lexer.cart.path.as_str();
    lexer.cart.text.clear();
    match text {
        Some(text) => lexer.cart.text.push_str(text),
        _ => {
            File::open(path)?.read_to_string(&mut lexer.cart.text)?;
        }
    };
    lexer.lex();
    lexer.cart.text.clear();
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
    eq: Intern,
    ge: Intern,
    gt: Intern,
    int: Intern,
    le: Intern,
    lt: Intern,
    log: Intern,
    ne: Intern,
    pair: Intern,
    sub: Intern,
    text: Intern,
}

/// Provide easy access for comparing resolutions to core native definitions.
#[derive(Clone, Copy, Debug, Default)]
pub struct CoreDefs {
    pub int: DefNum,
    pub log: DefNum,
    pub text: DefNum,
}

impl CoreInterns {
    pub fn new(interner: &Interner) -> Self {
        Self {
            add: interner.get_or_intern("add"), // TODO Straight to Uid?
            eq: interner.get_or_intern("eq"),
            int: interner.get_or_intern("Int"),
            ge: interner.get_or_intern("ge"),
            gt: interner.get_or_intern("gt"),
            le: interner.get_or_intern("le"),
            log: interner.get_or_intern("log"),
            lt: interner.get_or_intern("lt"),
            ne: interner.get_or_intern("ne"),
            pair: interner.get_or_intern("pair"), // TODO Straight to Uid?
            sub: interner.get_or_intern("sub"),
            text: interner.get_or_intern("Text"),
        }
    }

    pub fn token_to_intern(&self, node: ParseNode) -> Option<Intern> {
        let intern = match node {
            ParseNode::Leaf(token) => match token.kind {
                TokenKind::AngleClose => self.gt,
                TokenKind::AngleOpen => self.lt,
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

const CORE_TEXT: &str = include_str!("core.rio");
