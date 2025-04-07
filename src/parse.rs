use crate::{
    Cart,
    lex::{Token, TokenKind},
    tree::SimpleRange,
};
use lasso::Iter;
use num_derive::FromPrimitive;
use num_traits::FromPrimitive;
use static_assertions::const_assert;
use std::{iter::Peekable, ptr::read_unaligned};

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub enum ParseNode {
    Branch(ParseBranch),
    Leaf(Token),
}

impl ParseNode {
    pub fn read(codes: &[u32], offset: usize) -> (ParseNode, usize) {
        if offset + PARSE_BRANCH_SIZE <= codes.len() {
            // Check branch.
            let node = unsafe {
                let ptr = codes.as_ptr().add(offset) as *const ParseBranch;
                read_unaligned(ptr)
            };
            if node.kind as u32 >= ParseBranchKind::First as u32 {
                return (ParseNode::Branch(node), offset + PARSE_BRANCH_SIZE);
            }
        }
        // Check token.
        assert!(offset + TOKEN_SIZE <= codes.len());
        let node = unsafe {
            let ptr = codes.as_ptr().add(offset) as *const Token;
            read_unaligned(ptr)
        };
        assert!(node.kind as u32 <= TokenKind::Last as u32);
        (ParseNode::Leaf(node), offset + TOKEN_SIZE)
    }
}

const PARSE_BRANCH_SIZE: usize = size_of::<ParseBranch>() / 4;
const TOKEN_SIZE: usize = size_of::<Token>() / 4;

#[derive(Clone, Copy, Debug, Eq, FromPrimitive, Hash, PartialEq)]
pub enum ParseBranchKind {
    First = 0x1000,
    Call,
    Infix,
}

const_assert!(ParseBranchKind::First as u32 > TokenKind::Last as u32);

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
