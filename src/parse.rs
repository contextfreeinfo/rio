use std::ops::Range;

use crate::lex::Token;

pub enum Node {
    Branch {
        kind: BranchKind,
        range: Range<u32>,
        source: u32,
    },
    Leaf {
        token: Token,
    },
}

pub enum BranchKind {
    Call,
    Def,
    Function,
}
