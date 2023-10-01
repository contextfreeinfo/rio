use std::ops::Range;

use crate::{
    lex::TokenKind,
    tree::{BranchKind, Nod, Node, TreeBuilder, Type},
    Cart,
};

pub struct Normer {
    pub cart: Cart,
}

impl Normer {
    pub fn new(cart: Cart) -> Self {
        Self { cart }
    }

    pub fn norm(&mut self, parsed_tree: &[Nod], tree: &mut Vec<Node>) {
        self.trim(parsed_tree, tree);
        self.define(tree);
    }

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    fn define(&mut self, tree: &mut Vec<Node>) {
        self.builder().clear();
        self.define_at(&tree);
        self.builder()
            .wrap(BranchKind::Block, 0, Type::default(), 0);
        self.builder().drain_into(tree);
    }

    fn define_at(&mut self, tree: &[Node]) -> Option<()> {
        let node = *tree.last()?;
        match node.nod {
            Nod::Branch { kind, range, .. } => {
                let start = self.builder().pos();
                let range: Range<usize> = range.into();
                // Maybe handle some kids in custom fashion.
                let range = match kind {
                    BranchKind::Def => {
                        let kid = tree[range.start];
                        // We can't parse a Def of range len 0.
                        match kid.nod {
                            Nod::Branch {
                                kind: BranchKind::Typed,
                                range: first_range,
                                ..
                            } => {
                                // Expand typed in place.
                                let first_range: Range<usize> = first_range.into();
                                for kid_index in first_range.clone() {
                                    // TODO Why recurse instead of special handling?
                                    self.define_at(&tree[..=kid_index]);
                                }
                                range.start + 1..range.end
                            }
                            _ => {
                                // TODO Why recurse instead of special handling?
                                self.define_at(&tree[..=range.start]);
                                // Untyped, so push empty type after id.
                                self.builder().push_none(kid.source);
                                range.start + 1..range.end
                            }
                        }
                    }
                    _ => range,
                };
                // Loop remaining kids.
                for kid_index in range.clone() {
                    // TODO If we're a params, turn kid Ids to Defs.
                    self.define_at(&tree[..=kid_index]);
                }
                // Finalize.
                let kind = match kind {
                    BranchKind::Typed => {
                        // This typed wasn't the lead of a def, or we'd have handled it in that branch.
                        // No value, so push empty value after type.
                        self.builder().push_none(node.source);
                        // And make a def out of it.
                        BranchKind::Def
                    }
                    _ => kind,
                };
                self.builder()
                    .wrap(kind, start, Type::default(), node.source);
            }
            _ => self.builder().push(node),
        }
        Some(())
    }

    fn trim(&mut self, parsed_tree: &[Nod], tree: &mut Vec<Node>) {
        self.builder().clear();
        self.trim_at(&parsed_tree, 0);
        self.builder()
            .wrap(BranchKind::Block, 0, Type::default(), 0);
        self.builder().drain_into(tree);
    }

    fn trim_at(&mut self, tree: &[Nod], index: usize) -> Option<()> {
        let root = *tree.last()?;
        match root {
            Nod::Branch { kind, range, .. } => {
                // TODO Change Typed to Def with separate type?
                // TODO Change all Def to triples? Quads? Kids with meta lists?
                let start = self.builder().pos();
                let range: Range<usize> = range.into();
                for kid_index in range.clone() {
                    self.trim_at(&tree[..=kid_index], kid_index - range.start);
                }
                self.builder()
                    .wrap(kind, start, Type::default(), tree.len() - 1);
            }
            Nod::Leaf { token, .. } => match token.kind {
                TokenKind::Colon | TokenKind::Define => {
                    // Keep binaries in their place.
                    if index == 0 {
                        self.builder().push_none(tree.len() - 1);
                    }
                }
                TokenKind::Be
                | TokenKind::Comma
                | TokenKind::Comment
                | TokenKind::CurlyClose
                | TokenKind::CurlyOpen
                | TokenKind::End
                | TokenKind::Fun
                | TokenKind::HSpace
                | TokenKind::RoundClose
                | TokenKind::RoundOpen
                | TokenKind::Star
                | TokenKind::VSpace => {}
                _ => self.builder().push_at(token, tree.len() - 1),
            },
            _ => todo!(),
        }
        Some(())
    }
}
