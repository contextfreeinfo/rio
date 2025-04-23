use crate::{
    Cart,
    lex::{TOKEN_KIND_END, Token, TokenKind},
    tree::{CHUNK_SIZE, Chunk, Index, SimpleRange},
};
use lasso::Iter;
use num_derive::FromPrimitive;
use static_assertions::const_assert;
use std::{iter::Peekable, ptr::read_unaligned};
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

pub struct Parser {
    pub cart: Cart,
}

impl Parser {
    pub fn new(cart: Cart) -> Self {
        Self { cart }
    }
}

type Tokens<'a> = Peekable<Iter<'a, Token>>;
