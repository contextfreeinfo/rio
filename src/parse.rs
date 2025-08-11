use std::{io::Write, ops::Range};

use anyhow::Result;
use postcard::take_from_bytes;
use serde::{Deserialize, Serialize};

use crate::{
    Cart,
    intern::Intern,
    lex::{Token, TokenKind},
    tree::{SizeRange, TreeBuilder, TreeWriter},
};

// TODO Combine multiple files into one parse tree.
// TODO Track the node ranges for each file.
#[derive(Clone, Copy, Debug, Deserialize, Eq, Hash, PartialEq, Serialize)]
pub enum ParseNode {
    Branch(ParseBranch),
    Leaf(Token),
}

impl Default for ParseNode {
    fn default() -> Self {
        ParseNode::Leaf(Default::default())
    }
}

impl ParseNode {
    pub fn read(bytes: &[u8], offset: usize) -> (ParseNode, usize) {
        assert_ne!(0, offset);
        let (node, unused) = take_from_bytes(&bytes[offset..]).unwrap();
        (node, bytes.len() - unused.len())
    }
}

pub struct ParseNodeStepper {
    start: usize,
    end: usize,
}

impl ParseNodeStepper {
    pub fn new(range: SizeRange) -> Self {
        Self {
            start: range.start,
            end: range.end,
        }
    }

    pub fn next(&mut self, chunks: &[u8]) -> Option<(ParseNode, usize)> {
        let mut node: Option<ParseNode> = None;
        let mut source = 0;
        while self.start < self.end {
            // Guaranteed to cast since start and end were both originally Size.
            source = self.start;
            let (next, offset) = ParseNode::read(chunks, self.start);
            node = match next {
                ParseNode::Leaf(Token {
                    // TODO What else to skip?
                    kind:
                        TokenKind::Comma | TokenKind::Comment | TokenKind::HSpace | TokenKind::VSpace,
                    ..
                }) => None,
                _ => Some(next),
            };
            self.start = offset;
            if node.is_some() {
                break;
            }
        }
        node.map(|node| (node, source))
    }
}

#[derive(Clone, Copy, Debug, Deserialize, Eq, Hash, PartialEq, Serialize)]
pub enum ParseBranchKind {
    Block,
    Call,
    Infix,
    Def,
    Params,
    Typed,
    Fun,
    Pub,
    StringParts,
    TypCall,
}

#[derive(Clone, Copy, Debug, Deserialize, Eq, Hash, PartialEq, Serialize)]
pub struct ParseBranch {
    pub kind: ParseBranchKind,
    pub range: SizeRange,
}

macro_rules! define_infix {
    ($name:ident, $next:ident, $pattern:pat $(if $guard:expr)? $(,)?) => {
        fn $name(&mut self) -> Option<()> {
            let start = self.pos();
            self.$next()?;
            loop_some!({
                if !matches!(self.peek()?, $pattern $(if $guard)?) {
                    return Some(());
                }
                self.advance();
                self.skip_hv()?;
                let progress = self.try_progress(|s| s.$next());
                self.wrap(ParseBranchKind::Infix, start);
                progress?;
            })
        }
    };
}

macro_rules! loop_some {
    {$t:tt} => {(|| -> Option<()> {
        loop {
            $t
        }
    })()};
}

pub struct Parser<'a> {
    pub cart: &'a mut Cart,
    peeked: Option<(Token, usize)>,
    tokens_index: usize,
}

impl<'a> Parser<'a> {
    pub fn new(cart: &'a mut Cart) -> Self {
        Self {
            cart,
            peeked: None,
            tokens_index: 0,
        }
    }

    pub fn parse(&mut self) {
        self.builder().clear();
        self.block_top();
        // Finish top and drain tree.
        let bytes_top = self.builder().applied.len();
        self.wrap(ParseBranchKind::Block, 0);
        self.cart
            .tree_builder
            .drain_into(&mut self.cart.bytes, bytes_top);
    }

    define_infix!(add, multiply, TokenKind::Minus | TokenKind::Plus);

    fn advance(&mut self) {
        let Some((token, next)) = self.peek_token_step() else {
            return;
        };
        self.advance_without_peek(token, next);
    }

    fn advance_without_peek(&mut self, token: Token, next: usize) {
        self.tokens_index = next;
        self.builder().push(ParseNode::Leaf(token));
        self.peeked = None;
    }

    fn atom(&mut self) -> Option<()> {
        match self.peek()? {
            TokenKind::RoundOpen => {
                // TODO For round open, mode stack for ignoring vspace before infix.
                self.block()?
            }
            TokenKind::Be | TokenKind::CurlyOpen | TokenKind::Of => self.block()?,
            TokenKind::Fun => self.fun()?,
            TokenKind::Id | TokenKind::Int => self.advance(),
            TokenKind::StringEdge => self.string(),
            _ => {}
        }
        Some(())
    }

    fn block(&mut self) -> Option<()> {
        let start = self.pos();
        let ender = choose_ender(self.peek()?);
        self.advance();
        if ender == TokenKind::End && self.peek()? != TokenKind::VSpace {
            // Inline be ...
            self.def()?;
        } else {
            // Actually wrapped block.
            loop_some!({
                let progress = self.try_progress(|s| s.block_content());
                let kind = self.peek()?;
                if kind == ender {
                    self.advance();
                    None?
                }
                progress?;
            });
        }
        if self.pos() > start {
            self.wrap(ParseBranchKind::Block, start);
        }
        Some(())
    }

    fn block_content(&mut self) -> Option<()> {
        loop_some!({
            self.skip_hv();
            match self.peek()? {
                TokenKind::Comma => self.advance(),
                TokenKind::CurlyClose
                | TokenKind::End
                | TokenKind::RoundClose
                | TokenKind::SquareClose => None?,
                _ => self.try_progress(|s| s.def())?,
            }
        });
        self.skip_hv();
        Some(())
    }

    fn block_top(&mut self) {
        let start = self.pos();
        loop_some!({
            self.block_content()?;
            match self.peek()? {
                TokenKind::Greater
                | TokenKind::CurlyClose
                | TokenKind::End
                | TokenKind::RoundClose => {
                    // Eat trash. TODO Avoid ever getting here?
                    self.advance();
                    self.skip_hv();
                }
                _ => {}
            }
            self.advance();
        });
        if self.pos() > start {
            self.wrap(ParseBranchKind::Block, start);
        }
    }

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    fn call(&mut self) -> Option<()> {
        let start = self.pos();
        self.typ_call()?;
        if self.peek()? == TokenKind::RoundOpen {
            self.block();
            self.wrap(ParseBranchKind::Call, start);
        }
        Some(())
    }

    define_infix!(
        compare,
        add,
        TokenKind::Eq
            | TokenKind::Greater
            | TokenKind::GreaterEq
            | TokenKind::Less
            | TokenKind::LessEq
            | TokenKind::NotEq,
    );

    fn def(&mut self) -> Option<()> {
        // debug!("def");
        let start = self.pos();
        self.compare();
        if self.pos() > start && matches!(self.peek()?, TokenKind::Define | TokenKind::DefinePub) {
            self.advance();
            self.skip_hv();
            // Right-side descent.
            // TODO Error on nested assignment later?
            self.def();
            self.wrap(ParseBranchKind::Def, start);
            self.skip_hv()?;
        }
        Some(())
    }

    fn fun(&mut self) -> Option<()> {
        let start = self.pos();
        self.advance();
        self.skip_hv();
        // TODO Type params
        // In params
        let in_params_start = self.pos();
        match self.peek() {
            Some(TokenKind::Id) => {
                self.atom();
            }
            Some(TokenKind::RoundOpen) => {
                let block_start = self.pos();
                self.advance();
                loop_some!({
                    let param_start = self.pos();
                    let progress = self.try_progress(|s| s.def());
                    if !matches!(
                        self.peek(),
                        Some(TokenKind::Comma | TokenKind::RoundClose | TokenKind::VSpace)
                    ) {
                        self.def();
                        self.wrap(ParseBranchKind::Typed, param_start);
                    }
                    self.skip(|k| matches!(k, TokenKind::Comma | TokenKind::VSpace));
                    if self.peek() == Some(TokenKind::RoundClose) {
                        self.advance();
                        None?
                    }
                    progress?
                });
                self.wrap(ParseBranchKind::Block, block_start);
                // self.skip_hv();
                if self.peek() != Some(TokenKind::Be) {
                    // if self.peek() == Some(TokenKind::As) {
                    //     self.advance();
                    // }
                    self.def();
                    self.wrap(ParseBranchKind::Typed, block_start);
                    // self.skip_hv();
                }
            }
            _ => {}
        }
        self.wrap(ParseBranchKind::Params, in_params_start);
        // Body
        self.def();
        self.wrap(ParseBranchKind::Fun, start);
        Some(())
    }

    define_infix!(multiply, call, TokenKind::Slash | TokenKind::Star);

    fn peek(&mut self) -> Option<TokenKind> {
        self.peek_token_step().map(|it| it.0.kind)
    }

    fn peek_token_step(&mut self) -> Option<(Token, usize)> {
        match self.peeked {
            // Use cached when available to avoid redecoding cost.
            Some(_) => self.peeked,
            _ => {
                while self.tokens_index < self.cart.bytes.len() {
                    let (token, unused) =
                        take_from_bytes::<Token>(&self.cart.bytes[self.tokens_index..]).unwrap();
                    let next = self.cart.bytes.len() - unused.len();
                    // Eat hspace, so only stop looping on other.
                    if !matches!(token.kind, TokenKind::Comment | TokenKind::HSpace) {
                        self.peeked = Some((token, next));
                        break;
                    }
                    self.advance_without_peek(token, next);
                }
                // dbg!(self.peeked);
                self.peeked
            }
        }
    }

    fn pos(&mut self) -> usize {
        self.builder().pos()
    }

    fn skip<F>(&mut self, skipping: F) -> Option<()>
    where
        F: Fn(TokenKind) -> bool,
    {
        loop_some!({
            if skipping(self.peek()?) {
                // debug!("Skipping {:?}", token);
                self.advance();
            } else {
                return Some(());
            }
        })?;
        Some(())
    }

    fn skip_hv(&mut self) -> Option<()> {
        self.skip(|kind| matches!(kind, TokenKind::VSpace))
    }

    fn string(&mut self) {
        let start = self.pos();
        self.advance();
        loop_some!({
            let next = self.peek()?;
            self.advance();
            if next == TokenKind::StringEdge {
                None?;
            }
        });
        self.wrap(ParseBranchKind::StringParts, start);
    }

    /// None if done or if no advance happened. Option<()> elsewhere has None
    /// to mean the input is done.
    fn try_progress<F>(&mut self, f: F) -> Option<()>
    where
        F: FnOnce(&mut Self) -> Option<()>,
    {
        let start = self.tokens_index;
        f(self)?;
        match () {
            _ if self.tokens_index == start => None,
            _ => Some(()),
        }
    }

    fn typ_call(&mut self) -> Option<()> {
        let start = self.pos();
        self.atom()?;
        if self.peek()? == TokenKind::SquareOpen {
            self.block();
            self.wrap(ParseBranchKind::TypCall, start);
        }
        Some(())
    }

    fn wrap(&mut self, kind: ParseBranchKind, start: usize) {
        let range = self.builder().apply_range(start);
        let branch = ParseBranch { kind, range };
        self.builder().push(ParseNode::Branch(branch));
    }
}

fn choose_ender(token_kind: TokenKind) -> TokenKind {
    match token_kind {
        TokenKind::Be | TokenKind::Of => TokenKind::End,
        TokenKind::CurlyOpen => TokenKind::CurlyClose,
        TokenKind::RoundOpen => TokenKind::RoundClose,
        _ => panic!(),
    }
}

pub fn write_parse_tree<File, Map>(writer: &mut TreeWriter<'_, File, Map>) -> Result<()>
where
    File: Write,
    Map: std::ops::Index<Intern, Output = str>,
{
    let bytes = writer.bytes;
    let (top, end) = ParseNode::read(bytes, TreeBuilder::top_of(bytes));
    assert_eq!(bytes.len(), end);
    write_parse_tree_at(writer, top, 0)
}

pub fn write_parse_tree_at<File, Map>(
    writer: &mut TreeWriter<'_, File, Map>,
    node: ParseNode,
    indent: usize,
) -> Result<()>
where
    File: Write,
    Map: std::ops::Index<Intern, Output = str>,
{
    writer.indent(indent)?;
    match node {
        ParseNode::Branch(branch) => {
            writeln!(writer.file, "{:?}", branch.kind)?;
            let range: Range<usize> = branch.range.into();
            let mut offset = range.start;
            let mut count = 0;
            while offset < range.end {
                let (node, next_offset) = ParseNode::read(writer.bytes, offset);
                write_parse_tree_at(writer, node, indent + writer.indent)?;
                offset = next_offset;
                count += 1;
            }
            if count > 1 {
                writer.indent(indent)?;
                writeln!(writer.file, "/{:?}", branch.kind)?;
            }
        }
        ParseNode::Leaf(token) => writeln!(
            writer.file,
            "{:?}: {:?}",
            token.kind, &writer.map[token.intern]
        )?,
    }
    Ok(())
}
