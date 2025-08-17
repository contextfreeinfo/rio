use std::{
    cell::RefCell,
    collections::HashMap,
    fs::{File, create_dir_all},
    io::{BufWriter, Read, Write},
    path::{Path, PathBuf},
    rc::Rc,
};

use anyhow::{Error, Result};
use argh::{FromArgValue, FromArgs};

use crate::{
    intern::{Intern, Interner},
    lex::{Lexer, TokenKind},
    norm::write_tree,
    parse::{ParseNode, Parser, write_parse_tree},
    refine::resolve::UidInfo,
    tree::{TreeBuilder, TreeWriter},
};

mod intern;
mod lex;
mod norm;
mod parse;
mod refine;
mod tree;

#[derive(Clone, FromArgs, PartialEq, Debug)]
/// Top-level command.
struct Args {
    #[argh(subcommand)]
    command: Command,
}

#[derive(Clone, FromArgs, PartialEq, Debug)]
#[argh(subcommand)]
enum Command {
    Build(BuildArgs),
}

#[derive(Clone, FromArgs, PartialEq, Debug)]
/// Build from Rio source.
#[argh(subcommand, name = "build")]
struct BuildArgs {
    #[argh(positional)]
    /// path to Rio source root
    path: String,

    #[argh(option)]
    /// debug info for dumping
    dump: Vec<DumpOption>,

    #[argh(option)]
    /// output directory
    outdir: Option<String>,

    #[argh(switch)]
    /// whether to report build timing
    time: bool,
}

#[derive(Clone, Copy, Debug, Eq, FromArgValue, Ord, PartialEq, PartialOrd)]
enum DumpOption {
    Trees,
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

fn build(args: BuildArgs) -> Result<()> {
    // Start catalog (TODO and interner?), avoiding zeros.
    let mut catalog = Catalog::default();
    catalog.modules.push(Default::default());
    let catalog = Rc::new(RefCell::new(catalog));
    let mut cart = Cart::new(args.clone(), catalog);
    if let Err(err) = cart.build("core", Some(CORE_TEXT)) {
        println!("Failed building: core");
        return Err(err);
    }
    cart.extract_core_defs();
    // TODO And imports and things.
    if let Err(err) = cart.build(&args.path, None) {
        println!("Failed building: {}", &args.path);
        return Err(err);
    }
    Ok(())
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

struct Cart {
    pub args: BuildArgs,
    pub bytes: Vec<u8>,
    pub catalog: Rc<RefCell<Catalog>>,
    pub core_defs: CoreDefs,
    pub core_interns: CoreInterns,
    pub defs: Vec<TreeIdx>,
    pub name: String,
    pub interner: RefCell<Interner>,
    pub outdir: Option<PathBuf>,
    pub scope: Vec<UidInfo>,
    pub text: String,
    pub tops: HashMap<Intern, DefNum>,
    pub tree_builder: TreeBuilder,
}

impl Cart {
    fn new(args: BuildArgs, catalog: Rc<RefCell<Catalog>>) -> Self {
        let mut interner = Interner::default();
        // Always keep an empty string at zero.
        interner.intern("");
        Self {
            args: args.clone(),
            bytes: Default::default(),
            catalog,
            core_defs: Default::default(),
            core_interns: CoreInterns::new(&mut interner),
            defs: Default::default(),
            interner: RefCell::new(interner),
            name: Default::default(),
            outdir: Default::default(),
            scope: Default::default(),
            text: Default::default(),
            tops: Default::default(),
            tree_builder: Default::default(),
        }
    }

    fn build(&mut self, path: &str, text: Option<&str>) -> Result<()> {
        let name = Path::new(path)
            .file_stem()
            .ok_or(Error::msg("no name"))?
            .to_str()
            .ok_or(Error::msg("bad name"))?;
        self.name.clear();
        self.name.push_str(name);
        self.outdir = self.make_outdir()?;
        // Always keep an empty string at zero.
        self.interner.borrow_mut().intern("");
        match text {
            Some(text) => {
                self.text.clear();
                self.text.push_str(text);
            }
            None => {
                File::open(&self.args.path)?.read_to_string(&mut self.text)?;
            }
        }
        // dbg!(cart.text.len());
        Lexer::new(self).lex();
        // dbg!(cart.bytes.len());
        Parser::new(self).parse();
        if self.args.dump.contains(&DumpOption::Trees)
            && !self.name.is_empty()
            && let Some(outdir) = &self.outdir
        {
            let interner = &*self.interner.borrow();
            let mut writer = make_dump_writer("parse", outdir)?;
            let mut writer = TreeWriter::new(&self.bytes, &mut writer, interner);
            write_parse_tree(&mut writer)?;
            writeln!(writer.file)?;
            writeln!(writer.file, "Bytes: {}", self.bytes.len())?;
        }
        // dbg!(cart.bytes.len());
        self.norm()?;
        self.refine()?;
        let module = Module {
            // Expect clone to trim capacity, which it seems to at the moment.
            defs: self.defs.clone(),
            tops: self.tops.clone(),
            tree: self.bytes.clone(),
        };
        let mut catalog = self.catalog.borrow_mut();
        catalog.modules.push(module);
        Ok(())
    }

    fn extract_core_defs(&mut self) {
        let catalog = self.catalog.borrow_mut();
        let core = &catalog.modules[1];
        // Caching frequently used things in each cart should speed processing.
        // TODO Cache instead only in catalog if not behind a mutex?
        self.core_defs = CoreDefs {
            int: core.tops[&self.core_interns.int],
            log: core.tops[&self.core_interns.log],
            text: core.tops[&self.core_interns.text],
        };
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
        if self.args.dump.contains(&DumpOption::Trees)
            && let Some(outdir) = &self.outdir
        {
            let interner = &*self.interner.borrow();
            let mut writer = make_dump_writer(stage, outdir)?;
            let mut writer = TreeWriter::new(&self.bytes, &mut writer, interner);
            write_tree(&mut writer)?;
            writeln!(writer.file)?;
            writeln!(writer.file, "Bytes: {}", self.bytes.len())?;
        }
        Ok(())
    }

    fn norm(&mut self) -> Result<()> {
        norm::Normer::new(self).norm();
        self.maybe_dump_normed("norm")?;
        Ok(())
    }

    fn refine(&mut self) -> Result<()> {
        refine::Refiner::new(self).refine();
        self.maybe_dump_normed("refine")?;
        Ok(())
    }
}

fn make_dump_writer(stage: &str, outdir: &Path) -> Result<BufWriter<File>> {
    let name = outdir.file_name().unwrap().to_string_lossy();
    let path = outdir.join(format!("{name}.{stage}.txt"));
    let file = File::create(path)?;
    Ok(BufWriter::new(file))
}

/// Provide easy access for comparing resolutions to core native definitions.
#[derive(Clone, Copy, Debug)]
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
    pub fn new(interner: &mut Interner) -> Self {
        Self {
            add: interner.intern("add"), // TODO Straight to Uid?
            eq: interner.intern("eq"),
            int: interner.intern("Int"),
            ge: interner.intern("ge"),
            gt: interner.intern("gt"),
            le: interner.intern("le"),
            log: interner.intern("log"),
            lt: interner.intern("lt"),
            ne: interner.intern("ne"),
            pair: interner.intern("pair"), // TODO Straight to Uid?
            sub: interner.intern("sub"),
            text: interner.intern("Text"),
        }
    }

    pub fn token_to_intern(&self, node: ParseNode) -> Option<Intern> {
        let intern = match node {
            ParseNode::Leaf(token) => match token.kind {
                TokenKind::Eq => self.eq,
                TokenKind::Greater => self.gt,
                TokenKind::GreaterEq => self.ge,
                TokenKind::Less => self.lt,
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
