use std::ops::Range;

use crate::{
    lex::TokenKind,
    tree::{BranchKind, Node, TreeBuilder},
    Cart,
};

pub struct Normer {
    pub cart: Cart,
}

impl Normer {
    pub fn new(cart: Cart) -> Self {
        Self { cart }
    }

    pub fn norm(&mut self, tree: &mut Vec<Node>) {
        self.builder().clear();
        self.any(&tree, 0);
        self.builder().wrap(BranchKind::Block, 0);
        self.builder().drain_into(tree);
    }

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    fn any(&mut self, tree: &[Node], index: usize) -> Option<()> {
        let root = *tree.last()?;
        match root {
            Node::Branch { kind, range } => {
                // TODO Change Typed to Def with separate type?
                // TODO Change all Def to triples? Quads? Kids with meta lists?
                let start = self.builder().pos();
                let range: Range<usize> = range.into();
                for kid_index in range.clone() {
                    self.any(&tree[..kid_index + 1], kid_index - range.start);
                }
                self.builder().wrap(kind, start);
            }
            Node::IdDef { .. } => self.builder().push(root),
            Node::Leaf { token } => match token.kind {
                TokenKind::Colon | TokenKind::Define => {
                    // Keep binaries in their place.
                    if index == 0 {
                        self.builder().push_none();
                    }
                }
                TokenKind::Comma
                | TokenKind::Comment
                | TokenKind::CurlyClose
                | TokenKind::CurlyOpen
                | TokenKind::Fun
                | TokenKind::HSpace
                | TokenKind::RoundClose
                | TokenKind::RoundOpen
                | TokenKind::VSpace => {}
                _ => self.builder().push(root),
            },
        }
        Some(())
    }
}
