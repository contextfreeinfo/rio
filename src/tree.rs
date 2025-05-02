use std::{io::Write, mem::take, ops::Range};

use anyhow::Result;
use postcard::{from_bytes, to_extend, to_slice};
use serde::{Deserialize, Serialize};

use crate::lex::Intern;

// Duplicated from std Range so it can be Copy.
// See: https://github.com/rust-lang/rust/pull/27186#issuecomment-123390413
#[derive(Clone, Copy, Debug, Default, Deserialize, PartialEq, Eq, Hash, Serialize)]
pub struct SimpleRange<Idx> {
    /// The lower bound of the range (inclusive).
    pub start: Idx,
    /// The upper bound of the range (exclusive).
    pub end: Idx,
}

pub type SizeRange = SimpleRange<Size>;

impl SizeRange {
    pub fn len(&self) -> usize {
        (self.end - self.start).try_into().unwrap()
    }
}

impl<Idx> From<SimpleRange<Idx>> for Range<Idx> {
    fn from(value: SimpleRange<Idx>) -> Self {
        value.start..value.end
    }
}

impl From<SizeRange> for Range<usize> {
    fn from(value: SizeRange) -> Self {
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

impl From<Range<usize>> for SizeRange {
    fn from(value: Range<usize>) -> Self {
        (value.start as Size..value.end as Size).into()
    }
}

#[derive(Default)]
pub struct TreeBytes {
    pub applied: Vec<u8>,
    pub working: Vec<u8>,
}

// https://postcard.jamesmunns.com/wire-format.html#maximum-encoded-length
const MAX_USIZE_ENCODED_LEN: usize = 10;

impl TreeBytes {
    pub fn clear(&mut self) {
        self.applied.clear();
        self.working.clear();
        // Avoid 0 pointers, and reserve aligned space for start index.
        self.applied.extend([0; MAX_USIZE_ENCODED_LEN]);
    }

    pub fn drain_into(&mut self, tree: &mut Vec<u8>, top: usize) {
        tree.clear();
        tree.append(&mut self.applied);
        to_slice(&top, &mut tree[..MAX_USIZE_ENCODED_LEN]).unwrap();
    }

    pub fn apply_range(&mut self, start: Size) -> SizeRange {
        let start = start as usize;
        let applied_start = self.applied.len();
        self.applied.extend(self.working.drain(start..));
        (applied_start..self.applied.len()).into()
    }

    pub fn pos(&self) -> Size {
        self.working.len() as Size
    }

    pub fn push<T: Serialize>(&mut self, node: T) {
        self.working = to_extend(&node, take(&mut self.working)).unwrap();
    }

    pub fn top_of(bytes: &[u8]) -> usize {
        from_bytes(&bytes[..MAX_USIZE_ENCODED_LEN]).unwrap()
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
        // Avoid 0 pointers, so burn the first chunk.
        self.chunks.push(0);
    }

    pub fn apply_range(&mut self, start: Size) -> SizeRange {
        let start = start as usize;
        let applied_start = self.chunks.len();
        self.chunks.extend(self.working.drain(start..));
        (applied_start..self.chunks.len()).into()
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
    pub indent: usize,
    pub map: &'a Map,
}

impl<'a, File, Map> TreeWriter<'a, File, Map>
where
    File: Write,
    Map: std::ops::Index<Intern, Output = str>,
{
    pub fn new(chunks: &'a [Chunk], file: &'a mut File, map: &'a Map) -> Self {
        Self {
            chunks,
            file,
            indent: 2,
            map,
        }
    }

    pub fn indent(&mut self, indent: usize) -> Result<()> {
        write!(self.file, "{: <1$}", "", indent)?;
        Ok(())
    }
}

pub struct TreeBytesWriter<'a, File, Map>
where
    File: Write,
    Map: std::ops::Index<Intern, Output = str>,
{
    pub bytes: &'a [u8],
    pub file: &'a mut File,
    pub indent: usize,
    pub map: &'a Map,
}

impl<'a, File, Map> TreeBytesWriter<'a, File, Map>
where
    File: Write,
    Map: std::ops::Index<Intern, Output = str>,
{
    pub fn new(bytes: &'a [u8], file: &'a mut File, map: &'a Map) -> Self {
        Self {
            bytes,
            file,
            indent: 2,
            map,
        }
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
