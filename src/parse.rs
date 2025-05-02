use crate::{
    Cart,
    lex::{Intern, TOKEN_KIND_END, TOKEN_KIND_START, Token, TokenKind},
    tree::{
        CHUNK_SIZE, Chunk, SimpleRange, Size, SizeRange, TreeBuilder, TreeBytes, TreeBytesWriter,
        TreeWriter,
    },
};
use anyhow::{Ok, Result};
use log::debug;
use num_derive::FromPrimitive;
use postcard::take_from_bytes;
use serde::{Deserialize, Serialize};
use static_assertions::{const_assert, const_assert_eq};
use std::{io::Write, ops::Range};
use strum::EnumCount;

// TODO Combine multiple files into one parse tree.
// TODO Track the node ranges for each file.
#[derive(Clone, Copy, Debug, Deserialize, Eq, Hash, PartialEq, Serialize)]
pub enum ParseNode {
    Branch(ParseBranch),
    Leaf(Token),
}

impl ParseNode {
    pub fn read(chunks: &[Chunk], offset: usize) -> (ParseNode, usize) {
        // Seems less bother than Option<NonZeroU32> or whatever everywhere.
        assert_ne!(0, offset);
        let chunks = &chunks[offset..];
        let kind = chunks[0];
        match kind {
            PARSE_BRANCH_KIND_START..PARSE_BRANCH_KIND_END => {
                assert!(chunks.len() >= PARSE_BRANCH_SIZE);
                let node = unsafe {
                    let ptr = chunks.as_ptr() as *const ParseBranch;
                    std::ptr::read(ptr)
                };
                (ParseNode::Branch(node), offset + PARSE_BRANCH_SIZE)
            }
            TOKEN_KIND_START..TOKEN_KIND_END => {
                assert!(chunks.len() >= TOKEN_SIZE);
                let node = unsafe {
                    let ptr = chunks.as_ptr() as *const Token;
                    std::ptr::read(ptr)
                };
                (ParseNode::Leaf(node), offset + TOKEN_SIZE)
            }
            _ => panic!(),
        }
    }

    pub fn read_bytes(bytes: &[u8], offset: usize) -> (ParseNode, usize) {
        assert_ne!(0, offset);
        let (node, unused) = take_from_bytes(&bytes[offset..]).unwrap();
        (node, bytes.len() - unused.len())
    }
}

pub struct ParseNodeStepper {
    start: usize,
    end: usize,
    bytes_start: usize,
    bytes_end: usize,
}

impl ParseNodeStepper {
    pub fn new(range: SizeRange) -> Self {
        Self {
            start: range.start as usize,
            end: range.end as usize,
            bytes_start: range.start as usize,
            bytes_end: range.end as usize,
        }
    }

    pub fn next(&mut self, chunks: &[Chunk]) -> Option<(ParseNode, Size)> {
        let mut node: Option<ParseNode> = None;
        let mut source: Size = 0;
        while self.start < self.end {
            // Guaranteed to cast since start and end were both originally Size.
            source = self.start as Size;
            let (next, offset) = ParseNode::read(chunks, self.start);
            node = match next {
                ParseNode::Leaf(Token {
                    // TODO What else to skip?
                    kind: TokenKind::HSpace | TokenKind::VSpace,
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

    pub fn next_bytes(&mut self, chunks: &[u8]) -> Option<(ParseNode, Size)> {
        let mut node: Option<ParseNode> = None;
        let mut source: Size = 0;
        while self.start < self.end {
            // Guaranteed to cast since start and end were both originally Size.
            source = self.start as Size;
            let (next, offset) = ParseNode::read_bytes(chunks, self.start);
            node = match next {
                ParseNode::Leaf(Token {
                    // TODO What else to skip?
                    kind: TokenKind::HSpace | TokenKind::VSpace,
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

const PARSE_BRANCH_SIZE: usize = size_of::<ParseBranch>() / CHUNK_SIZE;
pub const TOKEN_SIZE: usize = size_of::<Token>() / CHUNK_SIZE;

#[repr(C)]
#[derive(
    Clone, Copy, Debug, Deserialize, EnumCount, Eq, FromPrimitive, Hash, PartialEq, Serialize,
)]
pub enum ParseBranchKind {
    Block = PARSE_BRANCH_KIND_START as isize,
    Call,
    Infix,
    Def,
    Params,
    Typed,
    Fun,
    Pub,
    StringParts,
}

pub const PARSE_BRANCH_KIND_START: Size = 0x1000 as Size;
pub const PARSE_BRANCH_KIND_END: Size = PARSE_BRANCH_KIND_START + ParseBranchKind::COUNT as Size;
const_assert!(PARSE_BRANCH_KIND_START >= TOKEN_KIND_END);
const_assert_eq!(0, std::mem::offset_of!(ParseBranch, kind));
const_assert_eq!(CHUNK_SIZE, align_of::<ParseNode>());

#[repr(C)]
#[derive(Clone, Copy, Debug, Deserialize, Eq, Hash, PartialEq, Serialize)]
pub struct ParseBranch {
    pub kind: ParseBranchKind,
    pub range: SimpleRange<u32>,
}

macro_rules! define_infix {
    ($name:ident, $next:ident, $must_space:expr, $pattern:pat $(if $guard:expr)? $(,)?) => {
        fn $name(&mut self) -> Option<bool> {
            debug!("{}", stringify!($name));
            let start = self.builder().pos();
            let start_bytes = self.cart.tree_bytes_builder.pos();
            let mut skipped = self.$next()?;
            loop {
                let pre_space = self.builder().pos();
                let pre_space_bytes = self.cart.tree_bytes_builder.pos();
                self.skip_h();
                let did_space = skipped || self.builder().pos() > pre_space;
                if ($must_space && !did_space) || !matches!(self.peek()?, $pattern $(if $guard)?) {
                    debug!("/{}", stringify!($name));
                    return Some(did_space);
                }
                self.advance();
                self.skip_hv();
                let maybe_skipped = self.$next();
                self.wrap(ParseBranchKind::Infix, start);
                self.wrap_bytes(ParseBranchKind::Infix, start_bytes);
                skipped = maybe_skipped?;
            }
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
    pub tokens_index: usize,
}

impl<'a> Parser<'a> {
    pub fn new(cart: &'a mut Cart) -> Self {
        Self {
            cart,
            tokens_index: 0,
        }
    }

    pub fn parse(&mut self) {
        self.builder().clear();
        self.cart.tree_bytes_builder.clear();
        self.block_top();
        // Finish top and drain tree.
        let bytes_top = self.cart.tree_bytes_builder.applied.len();
        // The end of the last range is naturally the start of the last node,
        // because the kid range ends that way.
        self.wrap(ParseBranchKind::Block, 0);
        self.wrap_bytes(ParseBranchKind::Block, 0);
        self.cart.tree.clear();
        self.cart.tree.append(&mut self.cart.tree_builder.chunks);
        self.cart.tree_bytes.clear();
        self.cart
            .tree_bytes_builder
            .drain_into(&mut self.cart.tree_bytes, bytes_top);
    }

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    fn advance(&mut self) {
        if self.tokens_index >= self.cart.tokens.len() {
            return;
        }
        let (token, unused) =
            take_from_bytes::<Token>(&self.cart.tokens[self.tokens_index..]).unwrap();
        debug!("Advance past {:?}", token);
        self.tokens_index = self.cart.tokens.len() - unused.len();
        self.builder().push(token);
        self.cart.tree_bytes_builder.push(ParseNode::Leaf(token));
    }

    define_infix!(add, typed, true, TokenKind::Minus | TokenKind::Plus);

    fn atom(&mut self) -> Option<()> {
        debug!("atom");
        self.skip_h();
        match self.peek()? {
            TokenKind::Colon | TokenKind::Comma | TokenKind::VSpace => {}
            TokenKind::Be
            | TokenKind::CurlyOpen
            | TokenKind::Of
            | TokenKind::RoundOpen
            | TokenKind::With => self.block()?,
            TokenKind::Fun => self.fun()?,
            TokenKind::Id => self.advance(),
            TokenKind::StringEdge => self.string(),
            _ => self.advance(),
        }
        debug!("/atom");
        Some(())
    }

    fn block(&mut self) -> Option<()> {
        debug!("block");
        let start = self.builder().pos();
        let start_bytes = self.cart.tree_bytes_builder.pos();
        let ender = choose_ender(self.peek()?);
        self.advance();
        self.skip_h();
        if ender == TokenKind::End && self.peek()? != TokenKind::VSpace {
            // Inline be ...
            self.call(true);
        } else {
            // Actually wrapped block.
            loop_some!({
                self.block_content();
                let kind = self.peek()?;
                if kind != TokenKind::With {
                    self.advance();
                }
                if kind == ender || (ender == TokenKind::End && kind == TokenKind::With) {
                    None?
                }
            });
        }
        if self.builder().pos() > start {
            self.wrap(ParseBranchKind::Block, start);
            self.wrap_bytes(ParseBranchKind::Block, start_bytes);
        }
        debug!("/block");
        Some(())
    }

    fn block_content(&mut self) -> Option<()> {
        debug!("block_content");
        loop_some!({
            self.skip_hv();
            match self.peek()? {
                TokenKind::Comma => self.advance(),
                TokenKind::AngleClose
                | TokenKind::CurlyClose
                | TokenKind::End
                | TokenKind::RoundClose
                | TokenKind::With => None?,
                _ => {
                    self.def();
                }
            }
        });
        debug!("/block_content");
        self.skip_hv();
        Some(())
    }

    fn block_top(&mut self) {
        let start = self.builder().pos();
        let start_bytes = self.cart.tree_bytes_builder.pos();
        loop_some!({
            debug!("block_top: {:?}", self.peek());
            self.block_content()?;
            match self.peek()? {
                TokenKind::AngleClose
                | TokenKind::CurlyClose
                | TokenKind::End
                | TokenKind::RoundClose => {
                    // Eat trash. TODO Avoid ever getting here?
                    self.advance();
                    self.skip_hv();
                }
                TokenKind::With => {
                    // Also trash, but be nicer.
                    self.atom();
                }
                _ => {}
            }
        });
        if self.builder().pos() > start {
            self.wrap(ParseBranchKind::Block, start);
            self.wrap_bytes(ParseBranchKind::Block, start_bytes);
        }
    }

    fn call(&mut self, allow_block: bool) -> Option<()> {
        self.call_maybe_spaced(allow_block)
    }

    fn call_maybe_spaced(&mut self, allow_block: bool) -> Option<()> {
        debug!("call");
        self.skip_h();
        let start = self.builder().pos();
        let start_bytes = self.cart.tree_bytes_builder.pos();
        let had_space = self.pair()?;
        loop {
            debug!("call loop: {:?}", self.peek());
            let mut post = self.builder().pos();
            if post <= start {
                break;
            }
            // TODO Can we leaving trailing hspace, or do we need to have an indicator?
            let peeked = self.peek()?;
            if had_space
                || matches!(
                    peeked,
                    TokenKind::HSpace | TokenKind::Be | TokenKind::CurlyOpen | TokenKind::With
                )
            {
                self.skip_h();
                if !allow_block
                    && matches!(
                        self.peek()?,
                        TokenKind::Be | TokenKind::CurlyOpen | TokenKind::With
                    )
                {
                    break;
                }
                post = self.builder().pos();
                self.spaced();
            } else {
                break;
            }
            if self.builder().pos() <= post {
                // TODO What does this mean???
                break;
            }
            self.wrap(ParseBranchKind::Call, start);
            self.wrap_bytes(ParseBranchKind::Call, start_bytes);
            debug!("/call loop");
        }
        debug!("/call");
        Some(())
    }

    fn call_tight(&mut self) -> Option<bool> {
        debug!("call_tight");
        let start = self.builder().pos();
        let start_bytes = self.cart.tree_bytes_builder.pos();
        let mut skipped = self.dot()?;
        #[allow(clippy::while_let_loop)] // because maybe include `with` here
        loop {
            match self.peek()? {
                // Of blocks bind tightly. TODO Also `with`?
                TokenKind::Of => {
                    skipped = self.dot()?;
                }
                _ => break,
            }
            self.wrap(ParseBranchKind::Call, start);
            self.wrap_bytes(ParseBranchKind::Call, start_bytes);
        }
        debug!("/call_tight");
        Some(skipped)
    }

    define_infix!(
        compare,
        add,
        true,
        TokenKind::AngleClose
            | TokenKind::AngleOpen
            | TokenKind::Eq
            | TokenKind::GreaterEq
            | TokenKind::LessEq
            | TokenKind::NotEq,
    );

    fn def(&mut self) -> Option<()> {
        debug!("def");
        self.skip_h();
        let start = self.builder().pos();
        let start_bytes = self.cart.tree_bytes_builder.pos();
        self.call(true);
        if self.builder().pos() > start {
            self.skip_h();
            if self.peek()? == TokenKind::Define {
                self.advance();
                self.skip_hv();
                // Right-side descent.
                self.def();
                self.wrap(ParseBranchKind::Def, start);
                self.wrap_bytes(ParseBranchKind::Def, start_bytes);
                self.skip_hv()?;
            }
        }
        debug!("/def");
        Some(())
    }

    define_infix!(dot, starred, false, TokenKind::Dot,);

    fn fun(&mut self) -> Option<()> {
        debug!("fun");
        let start = self.builder().pos();
        let start_bytes = self.cart.tree_bytes_builder.pos();
        self.advance();
        self.skip_h();
        // TODO Type params
        // In params
        let in_params_start = self.builder().pos();
        let in_params_start_bytes = self.cart.tree_bytes_builder.pos();
        loop {
            let start_kind = self.peek()?;
            match start_kind {
                TokenKind::Be => break,
                TokenKind::Colon | TokenKind::CurlyOpen | TokenKind::Id | TokenKind::RoundOpen => {
                    // TODO Really only expect typed.
                    self.pair();
                    self.skip_h();
                }
                // TODO Fix angle handling. Maybe just before in params?
                // TODO If angle, also try paren afterward. If so, nested funs.
                TokenKind::AngleOpen => {
                    self.advance();
                    // TODO Call it Params?
                    self.block_content();
                    if self.peek()? == choose_ender(start_kind) {
                        self.advance();
                    }
                }
                _ => return Some(()),
            }
        }
        self.wrap(ParseBranchKind::Params, in_params_start);
        self.wrap_bytes(ParseBranchKind::Params, in_params_start_bytes);
        self.skip_h();
        // Body
        self.atom();
        self.wrap(ParseBranchKind::Fun, start);
        self.wrap_bytes(ParseBranchKind::Fun, start_bytes);
        debug!("/fun");
        Some(())
    }

    define_infix!(pair, compare, false, TokenKind::To);

    fn peek(&self) -> Option<TokenKind> {
        self.peek_token().map(|x| x.kind)
    }

    fn peek_token(&self) -> Option<Token> {
        self.peek_token_step().map(|x| x.0)
    }

    fn peek_token_step(&self) -> Option<(Token, usize)> {
        match () {
            _ if self.tokens_index < self.cart.tokens.len() => {
                let (token, unused) =
                    take_from_bytes(&self.cart.tokens[self.tokens_index..]).unwrap();
                Some((token, self.cart.tokens.len() - unused.len()))
            }
            _ => None,
        }
    }

    fn skip<F>(&mut self, skipping: F) -> Option<bool>
    where
        F: Fn(TokenKind) -> bool,
    {
        let mut skipped = false;
        loop {
            let (token, next) = self.peek_token_step()?;
            if skipping(token.kind) {
                debug!("Skipping {:?}", token);
                skipped = true;
                self.tokens_index = next;
                self.builder().push(token);
                self.cart.tree_bytes_builder.push(ParseNode::Leaf(token));
            } else {
                break;
            }
        }
        Some(skipped)
    }

    fn skip_h(&mut self) -> Option<bool> {
        self.skip(|kind| matches!(kind, TokenKind::Comment | TokenKind::HSpace))
    }

    fn skip_hv(&mut self) -> Option<bool> {
        self.skip(|kind| {
            matches!(
                kind,
                TokenKind::Comment | TokenKind::HSpace | TokenKind::VSpace
            )
        })
    }

    fn spaced(&mut self) -> Option<()> {
        loop {
            self.skip_h();
            let next = self.peek()?;
            debug!("spaced: {next:?}");
            match next {
                TokenKind::AngleClose
                | TokenKind::Comma
                | TokenKind::Colon
                | TokenKind::CurlyClose
                | TokenKind::Define
                | TokenKind::End
                | TokenKind::RoundClose
                | TokenKind::VSpace => break,
                _ => self.pair(),
            };
            debug!("/spaced");
        }
        Some(())
    }

    fn starred(&mut self) -> Option<bool> {
        debug!("starred");
        let start = self.builder().pos();
        let start_bytes = self.cart.tree_bytes_builder.pos();
        self.atom();
        if self.peek()? == TokenKind::Star {
            self.advance();
            self.wrap(ParseBranchKind::Pub, start);
            self.wrap_bytes(ParseBranchKind::Pub, start_bytes);
        }
        debug!("/starred");
        Some(false)
    }

    fn string(&mut self) {
        debug!("string");
        let start = self.builder().pos();
        let start_bytes = self.cart.tree_bytes_builder.pos();
        self.advance();
        loop_some!({
            let next = self.peek()?;
            self.advance();
            if next == TokenKind::StringEdge {
                None?;
            }
        });
        self.wrap(ParseBranchKind::StringParts, start);
        self.wrap_bytes(ParseBranchKind::StringParts, start_bytes);
        debug!("/string");
    }

    fn typed(&mut self) -> Option<bool> {
        let start = self.builder().pos();
        let start_bytes = self.cart.tree_bytes_builder.pos();
        let mut skipped = self.call_tight()?;
        let pre_space = self.builder().pos();
        self.skip_h();
        skipped |= self.builder().pos() > pre_space;
        if self.peek()? == TokenKind::Colon {
            self.advance();
            self.skip_hv();
            let maybe_skipped = self.call_tight();
            self.wrap(ParseBranchKind::Typed, start);
            self.wrap_bytes(ParseBranchKind::Typed, start_bytes);
            skipped = maybe_skipped?;
        }
        Some(skipped)
    }

    fn wrap(&mut self, kind: ParseBranchKind, start: Size) {
        let range = self.builder().apply_range(start);
        let branch = ParseBranch { kind, range };
        self.builder().push(branch);
    }

    fn wrap_bytes(&mut self, kind: ParseBranchKind, start: Size) {
        let range = self.cart.tree_bytes_builder.apply_range(start);
        let branch = ParseBranch { kind, range };
        self.cart.tree_bytes_builder.push(ParseNode::Branch(branch));
    }
}

fn choose_ender(token_kind: TokenKind) -> TokenKind {
    match token_kind {
        TokenKind::AngleOpen => TokenKind::AngleClose,
        TokenKind::Be | TokenKind::Of | TokenKind::With => TokenKind::End,
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
    let chunks = writer.chunks;
    let (top, end) = ParseNode::read(chunks, *chunks.last().unwrap() as usize);
    assert_eq!(chunks.len(), end);
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
                let (node, next_offset) = ParseNode::read(writer.chunks, offset);
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

pub fn write_parse_tree_bytes<File, Map>(writer: &mut TreeBytesWriter<'_, File, Map>) -> Result<()>
where
    File: Write,
    Map: std::ops::Index<Intern, Output = str>,
{
    let bytes = writer.bytes;
    let (top, end) = ParseNode::read_bytes(bytes, TreeBytes::top_of(bytes));
    assert_eq!(bytes.len(), end);
    write_parse_tree_bytes_at(writer, top, 0)
}

pub fn write_parse_tree_bytes_at<File, Map>(
    writer: &mut TreeBytesWriter<'_, File, Map>,
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
                let (node, next_offset) = ParseNode::read_bytes(writer.bytes, offset);
                write_parse_tree_bytes_at(writer, node, indent + writer.indent)?;
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
