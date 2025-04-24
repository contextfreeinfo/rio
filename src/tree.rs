use std::{io::Write, ops::Range};

use crate::lex::Intern;

// Duplicated from std Range so it can be Copy.
// See: https://github.com/rust-lang/rust/pull/27186#issuecomment-123390413
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Hash)]
pub struct SimpleRange<Idx> {
    /// The lower bound of the range (inclusive).
    pub start: Idx,
    /// The upper bound of the range (exclusive).
    pub end: Idx,
}

impl SimpleRange<Index> {
    pub fn len(&self) -> usize {
        (self.end - self.start).try_into().unwrap()
    }
}

impl<Idx> From<SimpleRange<Idx>> for Range<Idx> {
    fn from(value: SimpleRange<Idx>) -> Self {
        value.start..value.end
    }
}

impl From<SimpleRange<Index>> for Range<usize> {
    fn from(value: SimpleRange<Index>) -> Self {
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

impl From<Range<usize>> for SimpleRange<Index> {
    fn from(value: Range<usize>) -> Self {
        (value.start as Index..value.end as Index).into()
    }
}

#[derive(Default)]
pub struct TreeBuilder {
    pub chunks: Vec<Chunk>,
    pub working: Vec<Chunk>,
}

pub type Chunk = u32;
pub const CHUNK_SIZE: usize = size_of::<Chunk>();

pub type Index = u32;

impl TreeBuilder {
    pub fn clear(&mut self) {
        self.chunks.clear();
        self.working.clear();
    }

    pub fn apply_range(&mut self, start: Index) -> SimpleRange<Index> {
        let start = start as usize;
        let applied_start = self.pos();
        self.chunks.extend(self.working.drain(start..));
        SimpleRange {
            start: applied_start,
            end: self.pos(),
        }
    }

    pub fn drain_into(&mut self, tree: &mut Vec<Chunk>) {
        tree.clone_from(&self.chunks);
        self.clear();
    }

    pub fn pos(&self) -> Index {
        self.working.len() as Index
    }

    pub fn push<T>(&mut self, node: T) {
        let ptr = &raw const node as *const Chunk;
        assert!(size_of::<T>() % CHUNK_SIZE == 0);
        let len = size_of::<T>() / CHUNK_SIZE;
        let slice = unsafe { std::slice::from_raw_parts(ptr, len) };
        self.working.extend_from_slice(slice);
    }
}

pub struct TreeWriter<'a, File, Map>
where
    File: Write,
    Map: std::ops::Index<Intern, Output = str>,
{
    file: &'a mut File,
    level: usize,
    map: &'a Map,
}

impl<'a, File, Map> TreeWriter<'a, File, Map>
where
    File: Write,
    Map: std::ops::Index<Intern, Output = str>,
{
    pub fn new(file: &'a mut File, map: &'a Map) -> Self {
        Self {
            file,
            level: 0,
            map,
        }
    }
}

#[allow(unused)]
mod test {
    use lasso::Key;

    use crate::{
        lex::{Intern, Token, TokenKind},
        parse::{ParseBranch, ParseBranchKind, ParseNode},
    };

    use super::*;

    #[test]
    fn build() {
        let mut builder = TreeBuilder::default();
        builder.push(Token {
            kind: TokenKind::Be,
            intern: Intern::try_from_usize(1).unwrap(),
        });
        assert_eq!(2, builder.working.len());
        builder.push(ParseBranch {
            kind: ParseBranchKind::Call,
            range: (0..1 as Index).into(),
        });
        assert_eq!(5, builder.working.len());
        let offset = 0usize;
        // Read first.
        let (node, offset) = ParseNode::read(&builder.working, offset);
        let ParseNode::Leaf(token) = node else {
            panic!()
        };
        assert_eq!(TokenKind::Be, token.kind);
        assert_eq!(2, offset);
        // Read second.
        let (node, offset) = ParseNode::read(&builder.working, offset);
        let ParseNode::Branch(branch) = node else {
            panic!()
        };
        assert!(branch.kind == ParseBranchKind::Call);
    }
}
