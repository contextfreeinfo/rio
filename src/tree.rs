use std::ops::Range;

// Duplicated from std Range so it can be Copy.
// See: https://github.com/rust-lang/rust/pull/27186#issuecomment-123390413
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Hash)]
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

#[derive(Default)]
pub struct TreeBuilder {
    pub nodes: Vec<u32>,
    pub working: Vec<u32>,
}

impl TreeBuilder {
    pub fn clear(&mut self) {
        self.nodes.clear();
        self.working.clear();
    }

    pub fn drain_into(&mut self, tree: &mut Vec<u32>) {
        tree.clone_from(&self.nodes);
        self.clear();
    }

    pub fn pos(&self) -> u32 {
        self.working.len() as u32
    }

    pub fn push<T>(&mut self, node: T) {
        let ptr = &node as *const T as *const u32;
        assert!(std::mem::size_of::<T>() % 4 == 0);
        let len = std::mem::size_of::<T>() / 4;
        let slice = unsafe { std::slice::from_raw_parts(ptr, len) };
        self.working.extend_from_slice(slice);
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
            range: (0..1u32).into(),
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
