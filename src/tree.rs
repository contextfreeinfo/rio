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

#[derive(Clone, Copy, Default, Eq, PartialEq)]
pub struct Type(u32);

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
    IdDef {
        intern: Intern,
        num: u32,
    },
    Leaf {
        token: Token,
    },
}

pub fn write_tree<Map>(file: &mut File, nodes: &[Node], map: &Map) -> Result<()>
where
    Map: Index<Intern, Output = str>,
{
    write_at(file, nodes, map, nodes.len() - 1, 0)?;
    Ok(())
}

fn write_at<Map>(
    file: &mut File,
    nodes: &[Node],
    map: &Map,
    index: usize,
    indent: usize,
) -> Result<usize>
where
    Map: Index<Intern, Output = str>,
{
    let mut line_count = 0;
    let node = nodes[index];
    write!(file, "{: <1$}", "", indent)?;
    match node.nod {
        Nod::Branch { kind, range } => {
            writeln!(file, "{kind:?}")?;
            line_count += 1;
            let range: Range<usize> = range.into();
            let mut sub_count = 0;
            for index in range.clone() {
                sub_count += write_at(file, nodes, map, index, indent + 2)?;
            }
            line_count += sub_count;
            if sub_count > 1 {
                // More than one kid might be easier to track with an explicit close.
                write!(file, "{: <1$}", "", indent)?;
                writeln!(file, "/{kind:?}")?;
                line_count += 1;
            }
        }
        Nod::IdDef { intern, num } => {
            writeln!(file, "IdDef {}@{num}", &map[intern])?;
            line_count += 1;
        }
        Nod::Leaf { token } => {
            writeln!(file, "{:?} {:?}", token.kind, &map[token.intern])?;
            line_count += 1;
        }
        _ => todo!(),
    }
    Ok(line_count)
}

impl Debug for Node {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self.nod {
            Nod::Branch { kind, range, .. } => f.write_fmt(format_args!("{kind:?}({range:?})")),
            Nod::IdDef { intern, num, .. } => {
                f.write_fmt(format_args!("{:?}@{num}", intern.into_inner()))
            }
            Nod::Leaf { token, .. } => f.write_fmt(format_args!("{token:?}")),
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
    // Group,
    None,
    Params,
    Typed,
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
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
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
        self.working.push(Node {
            typ,
            source,
            nod: Nod::Branch {
                kind,
                range: (nodes_start..self.nodes.len()).into(),
            },
        });
    }
}
