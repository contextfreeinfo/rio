use std::ops::Range;

use crate::{
    lex::TokenKind,
    parse::{BranchKind, Node, TreeBuilder},
};

pub struct Normer {
    pub builder: TreeBuilder,
}

impl Normer {
    pub fn new(builder: TreeBuilder) -> Self {
        Self { builder }
    }

    pub fn norm(&mut self, tree: &[Node]) -> Vec<Node> {
        self.builder.clear();
        self.any(&tree);
        self.builder.wrap(BranchKind::Block, 0);
        self.builder.extract()
    }

    fn any(&mut self, tree: &[Node]) -> Option<()> {
        let root = *tree.last()?;
        match root {
            Node::Branch { kind, range } => {
                let start = self.builder.pos();
                let range: Range<usize> = range.into();
                for index in range.clone() {
                    self.any(&tree[..index + 1]);
                }
                self.builder.wrap(kind, start);
            }
            Node::Leaf { token } => match token.kind {
                TokenKind::Colon
                | TokenKind::Comma
                | TokenKind::CurlyClose
                | TokenKind::CurlyOpen
                | TokenKind::Define
                | TokenKind::Fun
                | TokenKind::HSpace
                | TokenKind::RoundClose
                | TokenKind::RoundOpen
                | TokenKind::VSpace => {}
                _ => self.builder.push(root),
            },
        }
        Some(())
    }
}
