use std::{
    fmt::Debug,
    iter::Peekable,
    ops::{Index, Range},
    slice::Iter,
};

use crate::lex::{Intern, Token, TokenKind};

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

pub fn print_tree<Map>(nodes: &[Node], map: &Map)
where
    Map: Index<Intern, Output = str>,
{
    print_at(nodes, map, nodes.len() - 1, 0);
}

fn print_at<Map>(nodes: &[Node], map: &Map, index: usize, indent: usize)
where
    Map: Index<Intern, Output = str>,
{
    let node = nodes[index];
    print!("{: <1$}", "", indent);
    match node {
        Node::Branch { kind, range } => {
            println!("{kind:?}");
            let range: Range<usize> = range.into();
            for index in range {
                print_at(nodes, map, index, indent + 2);
            }
        }
        Node::Leaf { token } => println!("{:?} {:?}", token.kind, &map[token.intern]),
    }
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
    Fun,
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

    pub fn wrap(&mut self, kind: BranchKind, start: u32) {
        let start = start as usize;
        let nodes_start = self.nodes.len();
        self.nodes.extend(self.working.drain(start..));
        self.working.push(Node::Branch {
            kind,
            range: (nodes_start..self.nodes.len()).into(),
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
        self.block_top(&mut source);
        self.builder.wrap(BranchKind::Block, 0);
        self.builder.extract()
    }

    fn advance(&mut self, source: &mut Tokens) {
        let next = source.next();
        if let Some(token) = next {
            self.builder.push(token);
        }
    }

    fn atom(&mut self, source: &mut Tokens) {
        self.skip_h(source);
        // let old = self.builder.pos();
        match peek(source) {
            Some(token) => match token.kind {
                TokenKind::Comma | TokenKind::VSpace => {}
                TokenKind::CurlyOpen | TokenKind::RoundOpen => self.block(source),
                TokenKind::Id => self.advance(source),
                TokenKind::String => self.advance(source),
                _ => self.advance(source),
            },
            None => {}
        }
    }

    fn block(&mut self, source: &mut Tokens) {
        let start = self.builder.pos();
        let ender = match peek(source) {
            Some(token) => match token.kind {
                TokenKind::CurlyOpen => TokenKind::CurlyClose,
                TokenKind::RoundOpen => TokenKind::RoundClose,
                _ => return,
            },
            None => return,
        };
        self.advance(source);
        loop {
            self.block_content(source);
            match peek(source) {
                Some(token) => {
                    self.advance(source);
                    match () {
                        _ if token.kind == ender => break,
                        _ => {}
                    }
                }
                None => break,
            }
        }
        if self.builder.pos() > start {
            self.builder.wrap(BranchKind::Block, start);
        }
    }

    fn block_content(&mut self, source: &mut Tokens) {
        loop {
            self.skip_hv(source);
            match peek(source) {
                Some(token) => match token.kind {
                    TokenKind::Comma => self.advance(source),
                    TokenKind::CurlyClose | TokenKind::RoundClose => break,
                    _ => {
                        self.def(source);
                    }
                },
                None => break,
            }
        }
        self.skip_hv(source);
    }

    fn block_top(&mut self, source: &mut Tokens) {
        let start = self.builder.pos();
        loop {
            self.block_content(source);
            if let None = peek(source) {
                break;
            }
        }
        if self.builder.pos() > start {
            self.builder.wrap(BranchKind::Block, start);
        }
    }

    fn call(&mut self, source: &mut Tokens) {
        self.skip_h(source);
        let start = self.builder.pos();
        self.atom(source);
        loop {
            let mut post = self.builder.pos();
            if post > start {
                // TODO Can we leaving trailing hspace, or do we need to have an indicator?
                match peek(source) {
                    Some(token) => match token.kind {
                        TokenKind::HSpace | TokenKind::CurlyOpen => {
                            self.skip_h(source);
                            post = self.builder.pos();
                            self.spaced(source);
                        }
                        TokenKind::RoundOpen => {
                            // TODO Expand paren-comma args.
                            self.advance(source);
                            self.block_content(source);
                            match peek(source) {
                                Some(Token {
                                    kind: TokenKind::RoundClose,
                                    ..
                                }) => {
                                    self.advance(source);
                                }
                                _ => {}
                            }
                            self.skip_h(source);
                            match peek(source) {
                                Some(Token {
                                    kind: TokenKind::CurlyOpen,
                                    ..
                                }) => {
                                    self.block(source);
                                    self.skip_h(source);
                                }
                                _ => {}
                            }
                        }
                        _ => break,
                    },
                    None => break,
                }
                if self.builder.pos() > post {
                    self.builder.wrap(BranchKind::Call, start);
                }
            }
        }
    }

    fn def(&mut self, source: &mut Tokens) {
        self.skip_h(source);
        let start = self.builder.pos();
        self.call(source);
        if self.builder.pos() > start {
            self.skip_h(source);
            match peek(source) {
                Some(token) => match token.kind {
                    TokenKind::Define => {
                        self.advance(source);
                        self.skip_hv(source);
                        // Right-side descent.
                        self.def(source);
                        self.builder.wrap(BranchKind::Def, start);
                        self.skip_hv(source);
                    }
                    _ => {}
                },
                None => {}
            }
        }
    }

    fn skip<F>(&mut self, source: &mut Tokens, skipping: F)
    where
        F: Fn(TokenKind) -> bool,
    {
        loop {
            match peek(source) {
                Some(token) => {
                    if skipping(token.kind) {
                        source.next();
                        self.builder.push(token);
                    } else {
                        break;
                    }
                }
                _ => break,
            }
        }
    }

    fn skip_h(&mut self, source: &mut Tokens) {
        self.skip(source, |kind| kind == TokenKind::HSpace);
    }

    fn skip_hv(&mut self, source: &mut Tokens) {
        self.skip(source, |kind| {
            kind == TokenKind::HSpace || kind == TokenKind::VSpace
        });
    }

    fn spaced(&mut self, source: &mut Tokens) {
        loop {
            self.skip_h(source);
            match peek(source) {
                Some(token) => match token.kind {
                    TokenKind::Comma
                    | TokenKind::CurlyClose
                    | TokenKind::Define
                    | TokenKind::RoundClose
                    | TokenKind::VSpace => break,
                    _ => self.atom(source),
                },
                None => break,
            }
        }
    }
}

fn peek(source: &mut Tokens) -> Option<Token> {
    source.peek().copied().copied()
}
