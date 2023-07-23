use std::{fmt::Debug, iter::Peekable, ops::Range, slice::Iter};

use crate::lex::{Token, TokenKind};

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

impl Debug for Node {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Branch { kind, range } => f.write_fmt(format_args!("{kind:?}({range:?})")),
            Self::Leaf { token } => f.write_fmt(format_args!("{token:?}")),
        }
    }
}

impl From<Token> for Node {
    fn from(token: Token) -> Self {
        Node::Leaf { token }
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
}

impl TreeBuilder {
    pub fn clear(&mut self) {
        self.nodes.clear();
        self.working.clear();
    }

    pub fn extract(&mut self) -> Vec<Node> {
        self.nodes.clone()
    }

    pub fn pos(&self) -> u32 {
        self.working.len() as u32
    }

    pub fn push<N>(&mut self, node: N)
    where
        N: Into<Node>,
    {
        self.working.push(node.into());
    }

    pub fn wrap(&mut self, kind: BranchKind) {
        let old = self.nodes.len();
        self.nodes.extend(self.working.drain(..));
        self.working.push(Node::Branch {
            kind,
            range: (old..self.nodes.len()).into(),
        });
    }
}

type Tokens<'a> = Peekable<Iter<'a, Token>>;

#[derive(Default)]
pub struct Parser {
    builder: TreeBuilder,
}

impl Parser {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn parse(&mut self, source: &[Token]) -> Vec<Node> {
        self.builder.clear();
        let mut source = source.iter().peekable();
        self.block(&mut source);
        self.builder.wrap(BranchKind::Block);
        self.builder.extract()
    }

    fn block(&mut self, source: &mut Tokens) {
        let old = self.builder.pos();
        loop {
            self.skip_h(source);
            match peek(source) {
                Some(token) => match token {
                    _ => {
                        self.next(source);
                    }
                },
                None => break,
            }
        }
        self.skip_h(source);
        // TODO Refine wrapping conditions.
        if self.builder.pos() > old {
            self.builder.wrap(BranchKind::Block);
        }
    }

    fn next(&mut self, source: &mut Tokens) -> Option<Token> {
        let next = source.next();
        if let Some(token) = next {
            self.builder.push(token);
        }
        next.copied()
    }

    fn skip_h(&mut self, source: &mut Tokens) {
        loop {
            match peek(source) {
                Some(
                    token @ Token {
                        kind: TokenKind::HSpace,
                        ..
                    },
                ) => {
                    source.next();
                    self.builder.push(token);
                }
                _ => break,
            }
        }
    }
}

fn peek(source: &mut Tokens) -> Option<Token> {
    source.peek().copied().copied()
}
