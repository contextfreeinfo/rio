use std::{
    collections::hash_map::DefaultHasher,
    fmt::Debug,
    hash::{Hash, Hasher},
    io::Write,
    ops::{Index, Range},
};

use anyhow::Result;

use crate::lex::{Intern, Token};

const SHOW_SOURCES: bool = false;

#[derive(Clone, Copy, Debug, Default, Eq, Hash, PartialEq)]
pub struct Source(pub u32);

impl Into<Source> for usize {
    fn into(self) -> Source {
        Source(self as u32)
    }
}

#[derive(Clone, Copy, Debug, Default, Eq, Hash, PartialEq)]
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

#[derive(Clone, Copy, Eq, Hash, PartialEq)]
pub struct Node {
    pub nod: Nod,
    pub source: Source,
    pub typ: Type,
}

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub enum Nod {
    Branch {
        kind: BranchKind,
        range: SimpleRange<u32>,
    },
    Float64 {
        value: [u8; 8],
    },
    Int32 {
        value: i32,
    },
    Leaf {
        token: Token,
    },
    ModuleId {
        name: Intern,
        module: u16,
    },
    // For scoped or struct id.
    Sid {
        intern: Intern,
        num: u32,
    },
    // For unique id.
    Uid {
        intern: Intern,
        module: u16,
        num: u32,
    },
}

pub trait Nody: Copy + Debug + Eq + Hash {
    fn nod(&self) -> Nod;
    fn source(&self) -> Source;
    fn typ(&self) -> Type;
}

impl Nody for Node {
    fn nod(&self) -> Nod {
        self.nod
    }

    fn source(&self) -> Source {
        self.source
    }

    fn typ(&self) -> Type {
        self.typ
    }
}

impl Nody for Nod {
    fn nod(&self) -> Nod {
        *self
    }

    fn source(&self) -> Source {
        Source(0)
    }

    fn typ(&self) -> Type {
        Type(0)
    }
}

#[allow(dead_code)]
pub fn tree_hash<N: Nody>(tree: &[N]) -> u64 {
    // I haven't worked out how to implement Iterator for trees without heap
    // allocation, so use recursion to walk through it.
    let mut hasher = DefaultHasher::new();
    if let Some(node) = tree.last() {
        tree_hash_with(&mut hasher, node, tree);
    }
    hasher.finish()
}

pub fn tree_eq<N: Nody>(a: N, b: N, tree: &[N]) -> bool {
    // println!("tree_eq: {a:?} vs {b:?}");
    if a == b {
        return true;
    }
    // Check type.
    if a.typ() != b.typ() {
        return false;
    }
    // Also check source because hash picks them up by default.
    if a.source() != b.source() {
        return false;
    }
    // If both are branches, check kinds and kids.
    if let Nod::Branch {
        kind: kind_a,
        range: range_a,
    } = a.nod()
    {
        let range_a: Range<usize> = range_a.into();
        if let Nod::Branch {
            kind: kind_b,
            range: range_b,
            ..
        } = b.nod()
        {
            // Check branch-specific things.
            if kind_a != kind_b {
                return false;
            }
            let range_b: Range<usize> = range_b.into();
            if range_a.len() != range_b.len() {
                return false;
            }
            for (index_a, index_b) in range_a.zip(range_b) {
                if !tree_eq(tree[index_a], tree[index_b], tree) {
                    return false;
                }
            }
            return true;
        }
    }
    // Default to direct equality, where we already checked typ.
    a.nod() == b.nod()
}

pub fn tree_hash_with<N: Nody>(hasher: &mut impl Hasher, node: &N, tree: &[N]) {
    if let Nod::Branch { kind, range } = node.nod() {
        'B'.hash(hasher);
        kind.hash(hasher);
        let range: Range<usize> = range.into();
        for index in range {
            tree_hash_with(hasher, &tree[index], tree);
        }
    } else {
        node.hash(hasher);
    }
}

#[derive(Clone, Copy, Debug, Default, Eq, Ord, PartialEq, PartialOrd)]
pub struct DefNum {
    pub module: u32,
    pub num: u32,
}

pub fn write_tree<Map>(file: &mut impl Write, nodes: &[impl Nody], map: &Map) -> Result<()>
where
    Map: Index<Intern, Output = str>,
{
    write_at(file, None::<Nod>, nodes, map, 0, nodes.len() - 1, 0, 0)?;
    Ok(())
}

fn write_at<Map, File>(
    file: &mut File,
    parent: Option<Nod>,
    nodes: &[impl Nody],
    map: &Map,
    tree_module: u16,
    index: usize,
    kid_index: usize,
    indent: usize,
) -> Result<usize>
where
    Map: Index<Intern, Output = str>,
    File: Write,
{
    let mut line_count = 0;
    let node = &nodes[index];
    let mut finish = |file: &mut File| -> Result<()> {
        if node.typ().0 != 0 {
            write!(file, " :@{}", node.typ().0)?;
        }
        if SHOW_SOURCES && node.source().0 != 0 {
            write!(file, " ({})", node.source().0)?;
        }
        writeln!(file)?;
        line_count += 1;
        Ok(())
    };
    // Indent.
    write!(file, "{: <1$}", "", indent)?;
    // Type index.
    let write_index = |file: &mut File| -> Result<()> {
        if SHOW_SOURCES
            || tree_module != 0
                && matches!(
                    node.nod(),
                    Nod::Branch {
                        kind: BranchKind::Def,
                        ..
                    }
                )
            || matches!(
                parent,
                Some(Nod::Branch {
                    kind: BranchKind::Types,
                    ..
                })
            )
        {
            write!(file, "@{}", index + 1)?;
        }
        Ok(())
    };
    // Node info.
    match node.nod() {
        Nod::Branch { kind, range } => {
            write!(file, "{kind:?}")?;
            write_index(file)?;
            finish(file)?;
            let range: Range<usize> = range.into();
            let mut sub_count = 0;
            let mut tree_module = tree_module;
            for (kid_index, index) in range.clone().enumerate() {
                if let Nod::ModuleId { module, .. } = nodes[index].nod() {
                    tree_module = module;
                }
                sub_count += write_at(
                    file,
                    Some(node.nod()),
                    nodes,
                    map,
                    tree_module,
                    index,
                    kid_index,
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
        Nod::Int32 { value } => {
            write!(file, "Int32")?;
            write!(file, " {}", value)?;
            finish(file)?;
        }
        Nod::Leaf { token } => {
            write!(file, "{:?}", token.kind)?;
            write_index(file)?;
            write!(file, " {:?}", &map[token.intern])?;
            finish(file)?;
        }
        Nod::ModuleId {
            name: intern,
            module,
        } => {
            write!(file, "Module")?;
            write_index(file)?;
            write!(file, " {:?} {module}", &map[intern])?;
            finish(file)?;
        }
        Nod::Sid { intern, num } => {
            write!(file, "Sid")?;
            write_index(file)?;
            write!(file, " {}", &map[intern])?;
            write!(file, "@{num}")?;
            finish(file)?;
        }
        Nod::Uid {
            intern,
            module,
            num,
        } => {
            write!(file, "Uid")?;
            write_index(file)?;
            write!(file, " {}", &map[intern])?;
            // Try to be both pretty and clear to the extent that makes sense.
            let star = if tree_module != 0 {
                // Try to be pretty, since we have some context.
                if module != 0 && module != tree_module {
                    // External symbol so give the source.
                    write!(file, "@{module}")?;
                }
                // TODO If module == tree_module or 0 then no prefix at all?
                if module != 0
                    && kid_index == 0
                    && matches!(
                        parent,
                        Some(Nod::Branch {
                            kind: BranchKind::Def,
                            ..
                        })
                    )
                {
                    // Explicitly public def.
                    "*"
                } else {
                    // Ref or private def.
                    ""
                }
            } else {
                // No tree module info, so let raw number speak for itself.
                write!(file, "@{module}")?;
                ""
            };
            write!(file, "@{num}{star}")?;
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
            Nod::ModuleId { name, module } => {
                f.write_fmt(format_args!("Module {:?} {module}", name.into_inner()))
            }
            Nod::Uid {
                intern,
                module,
                num,
            } => f.write_fmt(format_args!("{:?}@{module}@{num}", intern.into_inner())),
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

#[derive(Debug, Clone, Copy, Eq, Hash, PartialEq)]
pub enum BranchKind {
    Block,
    Call,
    Def,
    Dot,
    Fun,
    FunType,     // (ParamsType, Type)
    Generic,     // (Params, <Something>)
    GenericCall, // (<Something>, ...) distinct from other Call because infer.
    // Group,
    Infix,
    List,
    None,
    Params,
    ParamTypes,
    Pub,
    StringParts,
    Struct,
    StructDef,
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

impl SimpleRange<u32> {
    pub fn len(&self) -> usize {
        (self.end - self.start).try_into().unwrap()
    }
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
    pub working: Vec<Node>,
}

impl TreeBuilder {
    pub fn clear(&mut self) {
        self.nodes.clear();
        self.working.clear();
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
            _ => self.pop_working(),
        }
    }

    fn pop_working(&mut self) -> Option<Node> {
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

    pub fn pop_working_tree(&mut self) {
        if self.working.is_empty() {
            return;
        }
        let goal = self.pos() - 1;
        while self.pos() > goal {
            self.pop_working();
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

    pub fn working_tree_eq(&self, a: u32, b: u32) -> bool {
        // println!("eq");
        if a == b {
            return true;
        }
        if let Some(a) = self.working.get(a as usize) {
            if let Some(b) = self.working.get(b as usize) {
                return tree_eq(*a, *b, &self.nodes);
            }
        }
        false
    }

    pub fn working_tree_hash(&self, index: u32) -> u64 {
        let mut hasher = DefaultHasher::new();
        if let Some(node) = self.working.get(index as usize) {
            tree_hash_with(&mut hasher, node, &self.nodes);
        }
        hasher.finish()
    }
}
