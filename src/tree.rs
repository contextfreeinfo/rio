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

#[derive(Clone, Copy)]
pub enum Node {
    Branch {
        typ: Type,
        kind: BranchKind,
        range: SimpleRange<u32>,
    },
    Float64 {
        typ: Type,
        value: f64,
    },
    IdDef {
        typ: Type,
        intern: Intern,
        num: u32,
    },
    Leaf {
        typ: Type,
        token: Token,
    },
}

impl Node {
    pub fn typ(&self) -> Type {
        match *self {
            Node::Branch { typ, .. } => typ,
            Node::Float64 { typ, .. } => typ,
            Node::IdDef { typ, .. } => typ,
            Node::Leaf { typ, .. } => typ,
        }
    }
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
    match node {
        Node::Branch { kind, range, .. } => {
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
        Node::IdDef { intern, num, .. } => {
            writeln!(file, "IdDef {}@{num}", &map[intern])?;
            line_count += 1;
        }
        Node::Leaf { token, .. } => {
            writeln!(file, "{:?} {:?}", token.kind, &map[token.intern])?;
            line_count += 1;
        }
        _ => todo!(),
    }
    Ok(line_count)
}

impl Debug for Node {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Branch { kind, range, .. } => f.write_fmt(format_args!("{kind:?}({range:?})")),
            Self::IdDef { intern, num, .. } => {
                f.write_fmt(format_args!("{:?}@{num}", intern.into_inner()))
            }
            Self::Leaf { token, .. } => f.write_fmt(format_args!("{token:?}")),
            _ => todo!(),
        }
    }
}

impl From<Token> for Node {
    fn from(token: Token) -> Self {
        Node::Leaf {
            token,
            typ: Type::default(),
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
    // Source separate from node so we don't throw off alignment and waste ram.
    pub sources: Vec<Source>,
    source_map: Vec<Source>,
    working: Vec<Node>,
    working_sources: Vec<Source>,
}

impl TreeBuilder {
    pub fn clear(&mut self) {
        if self.sources.is_empty() {
            if self.source_map.is_empty() {
                // Fill initial.
                self.source_map
                    .extend((0..self.nodes.len()).map(|it| Source(it as u32)))
            }
        } else {
            // Pass along the original sources across the rounds.
            self.source_map.clear();
            self.source_map.extend(self.sources.drain(..));
        }
        // println!("Sources: {:?}", self.source_map);
        // Others can just be cleared.
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

    pub fn push<N, S>(&mut self, node: N, source: S)
    where
        N: Into<Node>,
        S: Into<Source>,
    {
        self.working.push(node.into());
        self.push_source(source.into());
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
        if !self.working_sources.is_empty() {
            self.sources.extend(self.working_sources.drain(start..));
        }
        self.working.push(Node::Branch {
            kind,
            range: (nodes_start..self.nodes.len()).into(),
            typ,
        });
        self.push_source(source.into());
    }

    fn push_source(&mut self, source: Source) {
        if self.source_map.len() > source.0 as usize {
            // Pass along from earlier rounds.
            let source = self.source_map[source.0 as usize];
            self.working_sources.push(source);
        }
    }
}
