use std::{
    fmt::Debug,
    fs::File,
    io::Write,
    ops::{Index, Range},
};

use anyhow::Result;

use crate::lex::{Intern, Token};

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct Source(u32);

impl Into<Source> for usize {
    fn into(self) -> Source {
        Source(self as u32)
    }
}

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct Type(pub u32);

impl Type {
    pub fn or(self, other: Type) -> Type {
        match self.0 {
            0 => other,
            _ => self,
        }
    }
}

impl Into<Type> for usize {
    fn into(self) -> Type {
        Type(self as u32)
    }
}

#[derive(Clone, Copy, Eq, PartialEq)]
pub struct Node {
    pub typ: Type,
    pub source: Source,
    pub nod: Nod,
}

#[derive(Clone, Copy, Eq, PartialEq)]
pub enum Nod {
    Branch {
        kind: BranchKind,
        range: SimpleRange<u32>,
    },
    Float64 {
        value: [u8; 8],
    },
    Leaf {
        token: Token,
    },
    Uid {
        intern: Intern,
        module: u16,
        num: u32,
    },
}

pub trait Nody {
    fn nod(&self) -> Nod;
    fn typ(&self) -> Type;
}

impl Nody for Node {
    fn nod(&self) -> Nod {
        self.nod
    }

    fn typ(&self) -> Type {
        self.typ
    }
}

impl Nody for Nod {
    fn nod(&self) -> Nod {
        *self
    }

    fn typ(&self) -> Type {
        Type(0)
    }
}

#[derive(Clone, Copy, Debug, Default, Eq, Ord, PartialEq, PartialOrd)]
pub struct DefNum {
    pub module: u32,
    pub num: u32,
}

pub fn write_tree<Map>(file: &mut File, nodes: &[impl Nody], map: &Map) -> Result<()>
where
    Map: Index<Intern, Output = str>,
{
    write_at(file, None::<Nod>, nodes, map, 0, nodes.len() - 1, 0)?;
    Ok(())
}

fn write_at<Map>(
    file: &mut File,
    parent: Option<Nod>,
    nodes: &[impl Nody],
    map: &Map,
    kid_index: usize,
    index: usize,
    indent: usize,
) -> Result<usize>
where
    Map: Index<Intern, Output = str>,
{
    let mut line_count = 0;
    let node = &nodes[index];
    let mut finish = |file: &mut File| -> Result<()> {
        let typ = node.typ().0;
        match typ {
            0 => writeln!(file),
            _ => writeln!(file, " ({})", node.typ().0),
        }?;
        line_count += 1;
        Ok(())
    };
    // Indent.
    write!(file, "{: <1$}", "", indent)?;
    // Type index.
    if matches!(
        parent,
        Some(Nod::Branch {
            kind: BranchKind::Types,
            ..
        }),
    ) {
        write!(file, "({}) ", kid_index + 1)?;
    }
    // Node info.
    match node.nod() {
        Nod::Branch { kind, range } => {
            write!(file, "{kind:?}")?;
            finish(file)?;
            let range: Range<usize> = range.into();
            let mut sub_count = 0;
            for (kid_index, index) in range.clone().enumerate() {
                sub_count += write_at(
                    file,
                    Some(node.nod()),
                    nodes,
                    map,
                    kid_index,
                    index,
                    indent + 2,
                )?;
            }
            line_count += sub_count;
            if sub_count > 1 {
                // More than one kid might be easier to track with an explicit close.
                write!(file, "{: <1$}", "", indent)?;
                writeln!(file, "/{kind:?}")?;
                line_count += 1;
            }
        }
        Nod::Leaf { token } => {
            write!(file, "{:?} {:?}", token.kind, &map[token.intern])?;
            finish(file)?;
        }
        Nod::Uid {
            intern,
            module,
            num,
        } => {
            write!(file, "Uid {}@{module}:{num}", &map[intern])?;
            finish(file)?;
        }
        _ => todo!(),
    }
    Ok(line_count)
}

impl Debug for Node {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self.nod {
            Nod::Branch { kind, range, .. } => f.write_fmt(format_args!("{kind:?}({range:?})")),
            Nod::Leaf { token, .. } => f.write_fmt(format_args!("{token:?}")),
            Nod::Uid {
                intern,
                module,
                num,
            } => f.write_fmt(format_args!("{:?}@{module}:{num}", intern.into_inner())),
            _ => todo!(),
        }
    }
}

impl From<Token> for Node {
    fn from(token: Token) -> Self {
        Node {
            typ: Type::default(),
            source: Source(0),
            nod: Nod::Leaf { token },
        }
    }
}

impl From<&Token> for Node {
    fn from(token: &Token) -> Self {
        (*token).into()
    }
}

#[derive(Debug, Clone, Copy, Eq, PartialEq)]
pub enum BranchKind {
    Block,
    Call,
    Def,
    Fun,
    FunType,     // (ParamsType, Type)
    Generic,     // (Params, <Something>)
    GenericCall, // (<Something>, ...) distinct from other Call because infer.
    // Group,
    None,
    Params,
    ParamTypes,
    Pub,
    Struct,
    Type, // FunType | GenericCall | Uid
    Typed,
    Types, // Type*
}

// Duplicated from std Range so it can be Copy.
// See: https://github.com/rust-lang/rust/pull/27186#issuecomment-123390413
#[derive(Clone, Copy, Default, PartialEq, Eq, Hash)]
pub struct SimpleRange<Idx> {
    /// The lower bound of the range (inclusive).
    pub start: Idx,
    /// The upper bound of the range (exclusive).
    pub end: Idx,
}

impl<Idx> From<SimpleRange<Idx>> for Range<Idx> {
    fn from(value: SimpleRange<Idx>) -> Self {
        value.start..value.end
    }
}

impl From<SimpleRange<u32>> for Range<usize> {
    fn from(value: SimpleRange<u32>) -> Self {
        value.start as usize..value.end as usize
    }
}

impl<Idx> From<Range<Idx>> for SimpleRange<Idx> {
    fn from(value: Range<Idx>) -> Self {
        SimpleRange {
            start: value.start,
            end: value.end,
        }
    }
}

impl From<Range<usize>> for SimpleRange<u32> {
    fn from(value: Range<usize>) -> Self {
        (value.start as u32..value.end as u32).into()
    }
}

impl<T> Debug for SimpleRange<T>
where
    T: Debug,
{
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        f.write_fmt(format_args!("{:?}..{:?}", self.start, self.end))
    }
}

#[derive(Default)]
pub struct TreeBuilder {
    pub nodes: Vec<Node>,
    working: Vec<Node>,
    working_sources: Vec<Source>,
}

impl TreeBuilder {
    pub fn clear(&mut self) {
        self.nodes.clear();
        self.working.clear();
        self.working_sources.clear();
    }

    pub fn drain_into(&mut self, tree: &mut Vec<Node>) {
        tree.clone_from(&self.nodes);
        self.clear();
    }

    pub fn pop(&mut self) -> Option<Node> {
        if self.nodes.is_empty() {
            // Nothing to pop.
            return None;
        }
        match self.working.len() {
            0 => {
                // Pop the top node into working.
                self.working.push(self.nodes.pop().unwrap());
                None
            }
            _ => {
                // Pop the top working, pulling in its kids.
                let top = self.working.pop().unwrap();
                match top.nod {
                    Nod::Branch { range, .. } => {
                        let range: Range<usize> = range.into();
                        self.working.extend(self.nodes.drain(range));
                    }
                    _ => {}
                }
                Some(top)
            }
        }
    }

    pub fn pos(&self) -> u32 {
        self.working.len() as u32
    }

    pub fn push(&mut self, node: Node) {
        self.working.push(node);
    }

    pub fn push_at<N, S>(&mut self, node: N, source: S)
    where
        N: Into<Node>,
        S: Into<Source>,
    {
        self.working.push(Node {
            source: source.into(),
            ..node.into()
        });
    }

    pub fn push_tree(&mut self, nodes: &[Node]) {
        if nodes.is_empty() {
            return;
        }
        let node = *nodes.last().unwrap();
        match node.nod {
            Nod::Branch { kind, range } => {
                let start = self.pos();
                let range: Range<usize> = range.into();
                for kid_index in range.clone() {
                    self.push_tree(&nodes[..=kid_index]);
                }
                self.wrap(kind, start, node.typ, node.source);
            }
            _ => self.push(node),
        }
    }

    pub fn push_none<S>(&mut self, source: S)
    where
        S: Into<Source>,
    {
        self.wrap(BranchKind::None, self.pos(), Type::default(), source);
    }

    pub fn wrap<S>(&mut self, kind: BranchKind, start: u32, typ: Type, source: S)
    where
        S: Into<Source>,
    {
        let start = start as usize;
        let nodes_start = self.nodes.len();
        self.nodes.extend(self.working.drain(start..));
        let source = source.into();
        self.push(Node {
            typ,
            source,
            nod: Nod::Branch {
                kind,
                range: (nodes_start..self.nodes.len()).into(),
            },
        });
    }
}
