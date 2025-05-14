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

pub type SizeRange = SimpleRange<usize>;

impl SizeRange {
    pub fn len(&self) -> usize {
        self.end - self.start
    }
}

impl<Idx> From<SimpleRange<Idx>> for Range<Idx> {
    fn from(value: SimpleRange<Idx>) -> Self {
        value.start..value.end
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

pub struct SimpleRangeIterator<Idx> {
    range: SimpleRange<Idx>,
    range_idx: Idx,
}

pub trait Next {
    fn next(&self) -> Self;
}

impl Next for usize {
    fn next(&self) -> Self {
        *self + 1
    }
}

impl<Idx: Copy + Next + PartialOrd> Iterator for SimpleRangeIterator<Idx> {
    type Item = Idx;

    fn next(&mut self) -> Option<Self::Item> {
        match () {
            _ if self.range_idx < self.range.end => {
                let result = Some(self.range_idx);
                self.range_idx = self.range_idx.next();
                result
            }
            _ => None,
        }
    }
}

impl<Idx: Copy + Next + PartialOrd> IntoIterator for SimpleRange<Idx> {
    type Item = Idx;

    type IntoIter = SimpleRangeIterator<Idx>;

    fn into_iter(self) -> Self::IntoIter {
        SimpleRangeIterator {
            range: self,
            range_idx: self.start,
        }
    }
}

#[derive(Default)]
pub struct TreeBuilder {
    pub applied: Vec<u8>,
    pub working: Vec<u8>,
}

// https://postcard.jamesmunns.com/wire-format.html#maximum-encoded-length
const MAX_USIZE_ENCODED_LEN: usize = 10;

impl TreeBuilder {
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

    pub fn apply_range(&mut self, start: usize) -> SizeRange {
        let applied_start = self.applied.len();
        self.applied.extend(self.working.drain(start..));
        (applied_start..self.applied.len()).into()
    }

    pub fn pos(&self) -> usize {
        self.working.len()
    }

    pub fn push<T: Serialize>(&mut self, node: T) {
        self.working = to_extend(&node, take(&mut self.working)).unwrap();
    }

    pub fn top_of(bytes: &[u8]) -> usize {
        from_bytes(&bytes[..MAX_USIZE_ENCODED_LEN]).unwrap()
    }
}

#[macro_export]
macro_rules! impl_tree_builder_wrap {
    () => {
        fn wrap<F: FnOnce(&mut Self)>(&mut self, build: F) -> SizeRange {
            let start = self.builder().pos();
            build(self);
            match () {
                _ if self.builder().pos() == start => SizeRange::default(),
                _ => self.builder().apply_range(start),
            }
        }
    };
}

pub struct TreeWriter<'a, File, Map>
where
    File: Write,
    Map: std::ops::Index<Intern, Output = str>,
{
    pub bytes: &'a [u8],
    pub file: &'a mut File,
    pub indent: usize,
    pub map: &'a Map,
}

impl<'a, File, Map> TreeWriter<'a, File, Map>
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
