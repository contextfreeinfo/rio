use std::{io::Write, ops::Range};

use anyhow::Result;

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

impl SimpleRange<Size> {
    pub fn len(&self) -> usize {
        (self.end - self.start).try_into().unwrap()
    }
}

impl<Idx> From<SimpleRange<Idx>> for Range<Idx> {
    fn from(value: SimpleRange<Idx>) -> Self {
        value.start..value.end
    }
}

impl From<SimpleRange<Size>> for Range<usize> {
    fn from(value: SimpleRange<Size>) -> Self {
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

impl From<Range<usize>> for SimpleRange<Size> {
    fn from(value: Range<usize>) -> Self {
        (value.start as Size..value.end as Size).into()
    }
}

#[derive(Default)]
pub struct TreeBuilder {
    pub chunks: Vec<Chunk>,
    pub working: Vec<Chunk>,
}

pub type Chunk = u32;
pub const CHUNK_SIZE: usize = size_of::<Chunk>();

pub type Size = u32;

impl TreeBuilder {
    pub fn clear(&mut self) {
        self.chunks.clear();
        self.working.clear();
    }

    pub fn apply_range(&mut self, start: Size) -> SimpleRange<Size> {
        let start = start as usize;
        let applied_start = self.chunks.len();
        self.chunks.extend(self.working.drain(start..));
        (applied_start..self.chunks.len()).into()
    }

    pub fn drain_into(&mut self, tree: &mut Vec<Chunk>) {
        tree.clone_from(&self.chunks);
        self.clear();
    }

    pub fn pos(&self) -> Size {
        self.working.len() as Size
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
    pub chunks: &'a [Chunk],
    pub file: &'a mut File,
    pub map: &'a Map,
}

impl<'a, File, Map> TreeWriter<'a, File, Map>
where
    File: Write,
    Map: std::ops::Index<Intern, Output = str>,
{
    pub fn new(chunks: &'a [Chunk], file: &'a mut File, map: &'a Map) -> Self {
        Self { chunks, file, map }
    }

    pub fn indent(&mut self, indent: usize) -> Result<()> {
        write!(self.file, "{: <1$}", "", indent)?;
        Ok(())
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
            range: (0..1 as Size).into(),
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
