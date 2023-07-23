use std::{iter::Peekable, slice::Iter};

use crate::lex::Token;

#[derive(Clone, Copy, Eq, PartialEq)]
pub enum Node {
    Branch {
        kind: BranchKind,
        range: SimpleRange<u32>,
    },
    Leaf {
        token: Token,
    },
}

#[derive(Clone, Copy, Eq, PartialEq)]
pub enum BranchKind {
    Call,
    Def,
    Function,
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

#[derive(Default)]
pub struct Parser {
    nodes: Vec<Node>,
}

impl Parser {
    pub fn new() -> Self {
        Default::default()
    }

    pub fn parse(&mut self, source: &[Token]) -> Vec<Node> {
        self.nodes.clear();
        let mut source = source.iter().peekable();
        loop {
            match source.peek() {
                None => break,
                _ => {
                    self.next(&mut source);
                }
            }
        }
        self.nodes.clone()
    }

    fn next(&mut self, source: &mut Peekable<Iter<Token>>) -> Option<Token> {
        let next = source.next();
        next.copied()
    }
}
