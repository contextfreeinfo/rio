use std::ops::Range;

use crate::{
    lex::{Token, TokenKind},
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
        self.trim(tree);
        self.define(tree);
    }

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    fn define(&mut self, tree: &mut Vec<Node>) {
        self.builder().clear();
        self.define_at(&tree);
        self.builder().wrap(BranchKind::Block, 0);
        self.builder().drain_into(tree);
    }

    fn define_at(&mut self, tree: &[Node]) -> Option<()> {
        match *tree.last()? {
            Node::Branch { kind, range } => {
                let start = self.builder().pos();
                let range: Range<usize> = range.into();
                // Maybe handle some kids in custom fashion.
                let range = match kind {
                    BranchKind::Def => {
                        // We can't parse a Def of range len 0.
                        match tree[range.start] {
                            Node::Branch {
                                kind: BranchKind::Typed,
                                range: first_range,
                            } => {
                                // Expand typed in place.
                                let first_range: Range<usize> = first_range.into();
                                for kid_index in first_range.clone() {
                                    self.define_at(&tree[..kid_index + 1]);
                                }
                                range.start + 1..range.end
                            }
                            Node::Leaf {
                                token:
                                    token @ Token {
                                        kind: TokenKind::Id,
                                        ..
                                    },
                            } => {
                                self.builder().push(token);
                                // Untyped, so push empty type after id.
                                self.builder().push_none();
                                range.start + 1..range.end
                            }
                            _ => range,
                        }
                    }
                    _ => range,
                };
                // Loop remaining kids.
                for kid_index in range.clone() {
                    // TODO If we're a params, turn kid Ids to Defs.
                    self.define_at(&tree[..kid_index + 1]);
                }
                // Finalize.
                let kind = match kind {
                    BranchKind::Typed => {
                        // This typed wasn't the lead of a def, or we'd have handled it in that branch.
                        // No value, so push empty value after type.
                        self.builder().push_none();
                        // And make a def out of it.
                        BranchKind::Def
                    }
                    _ => kind,
                };
                self.builder().wrap(kind, start);
            }
            node @ _ => self.builder().push(node),
        }
        Some(())
    }

    fn trim(&mut self, tree: &mut Vec<Node>) {
        self.builder().clear();
        self.trim_at(&tree, 0);
        self.builder().wrap(BranchKind::Block, 0);
        self.builder().drain_into(tree);
    }

    fn trim_at(&mut self, tree: &[Node], index: usize) -> Option<()> {
        let root = *tree.last()?;
        match root {
            Node::Branch { kind, range } => {
                // TODO Change Typed to Def with separate type?
                // TODO Change all Def to triples? Quads? Kids with meta lists?
                let start = self.builder().pos();
                let range: Range<usize> = range.into();
                for kid_index in range.clone() {
                    self.trim_at(&tree[..kid_index + 1], kid_index - range.start);
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
