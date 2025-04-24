use crate::{
    Cart,
    lex::{TOKEN_KIND_END, Token, TokenKind},
    tree::{CHUNK_SIZE, Chunk, Index, SimpleRange, TreeBuilder},
};
use log::debug;
use num_derive::FromPrimitive;
use static_assertions::const_assert;
use std::{iter::Peekable, ptr::read_unaligned, slice::Iter};
use strum::EnumCount;

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub enum ParseNode {
    Branch(ParseBranch),
    Leaf(Token),
}

impl ParseNode {
    pub fn read(codes: &[Chunk], offset: usize) -> (ParseNode, usize) {
        let codes = &codes[offset..];
        if PARSE_BRANCH_SIZE <= codes.len() {
            // Check branch.
            let node = unsafe {
                let ptr = codes.as_ptr() as *const ParseBranch;
                read_unaligned(ptr)
            };
            if node.kind as Index >= PARSE_BRANCH_KIND_START {
                return (ParseNode::Branch(node), offset + PARSE_BRANCH_SIZE);
            }
        }
        // Check token.
        assert!(TOKEN_SIZE <= codes.len());
        let node = unsafe {
            let ptr = codes.as_ptr() as *const Token;
            read_unaligned(ptr)
        };
        assert!((node.kind as Index) < TOKEN_KIND_END);
        (ParseNode::Leaf(node), offset + TOKEN_SIZE)
    }
}

const PARSE_BRANCH_SIZE: usize = size_of::<ParseBranch>() / CHUNK_SIZE;
const TOKEN_SIZE: usize = size_of::<Token>() / CHUNK_SIZE;

#[derive(Clone, Copy, Debug, EnumCount, Eq, FromPrimitive, Hash, PartialEq)]
pub enum ParseBranchKind {
    Call = PARSE_BRANCH_KIND_START as isize,
    Infix,
}

const PARSE_BRANCH_KIND_START: Index = 0x1000 as Index;
const PARSE_BRANCH_KIND_END: Index = PARSE_BRANCH_KIND_START + ParseBranchKind::COUNT as Index;
const_assert!(PARSE_BRANCH_KIND_START >= TOKEN_KIND_END);

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub struct ParseBranch {
    pub kind: ParseBranchKind,
    pub range: SimpleRange<u32>,
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
    pub token_index: usize,
}

impl<'a> Parser<'a> {
    pub fn new(cart: &'a mut Cart) -> Self {
        Self {
            cart,
            token_index: 0,
        }
    }

    pub fn parse(&mut self) {
        self.builder().clear();
        self.block_top();
        // self.wrap(BranchKind::Block, 0);
    }

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    fn advance(&mut self) {
        if self.token_index >= self.cart.tokens.len() {
            return;
        }
        let token = self.cart.tokens[self.token_index];
        debug!("Advance past {:?}", token);
        self.token_index += 1;
        self.builder().push(token);
    }

    fn block_top(&mut self) {
        let start = self.builder().pos();
        loop_some!({
            debug!("block_top: {:?}", self.peek());
            // self.block_content()?;
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
                    // self.atom();
                }
                _ => {}
            }
            self.advance();
        });
        if self.builder().pos() > start {
            // self.wrap(ParseBranchKind::Block, start);
        }
    }

    fn peek(&self) -> Option<TokenKind> {
        self.cart.tokens.get(self.token_index).map(|x| x.kind)
    }

    fn peek_token(&self) -> Option<Token> {
        self.cart.tokens.get(self.token_index).copied()
    }

    fn skip<F>(&mut self, skipping: F) -> Option<bool>
    where
        F: Fn(TokenKind) -> bool,
    {
        let mut skipped = false;
        loop {
            let token = self.peek_token()?;
            if skipping(token.kind) {
                debug!("Skipping {:?}", token);
                skipped = true;
                self.token_index += 1;
                self.builder().push(token);
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
}

type Tokens<'a> = Peekable<Iter<'a, Token>>;

fn peek(source: &mut Tokens) -> Option<TokenKind> {
    source.peek().map(|it| it.kind)
}

fn peek_token(source: &mut Tokens) -> Option<Token> {
    source.peek().copied().copied()
}
