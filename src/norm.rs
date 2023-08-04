use std::ops::Range;

use crate::{
    lex::{Intern, Token, TokenKind},
    tree::{BranchKind, Node, TreeBuilder},
};

pub struct Normer {
    pub builder: TreeBuilder,
    none: Token,
}

impl Normer {
    pub fn new(builder: TreeBuilder, none_key: Intern) -> Self {
        Self {
            builder,
            none: Token::new(TokenKind::None, none_key),
        }
    }

    pub fn norm(&mut self, tree: &[Node]) -> Vec<Node> {
        self.builder.clear();
        self.any(&tree, 0);
        self.builder.wrap(BranchKind::Block, 0);
        self.builder.extract()
    }

    fn any(&mut self, tree: &[Node], index: usize) -> Option<()> {
        let root = *tree.last()?;
        match root {
            Node::Branch { kind, range } => {
                let start = self.builder.pos();
                let range: Range<usize> = range.into();
                for kid_index in range.clone() {
                    self.any(&tree[..kid_index + 1], kid_index - range.start);
                }
                self.builder.wrap(kind, start);
            }
            Node::Id { .. } => self.builder.push(root),
            Node::Leaf { token } => match token.kind {
                TokenKind::Colon | TokenKind::Define => {
                    // Keep binaries in their place.
                    if index == 0 {
                        self.builder.push(self.none);
                    }
                }
                TokenKind::Comma
                | TokenKind::CurlyClose
                | TokenKind::CurlyOpen
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
