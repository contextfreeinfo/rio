use std::{collections::HashMap, ops::Range};

use crate::{
    lex::{Intern, Token, TokenKind},
    tree::{BranchKind, Node, TreeBuilder},
    Cart,
};

pub struct Runner {
    pub cart: Cart,
    id_num: u32,
}

impl Runner {
    pub fn new(cart: Cart) -> Self {
        Self { cart, id_num: 1 }
    }

    pub fn run(&mut self, tree: &mut Vec<Node>) {
        self.convert_ids(tree);
        self.resolve_top(tree);
    }

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    fn convert_ids(&mut self, tree: &mut Vec<Node>) {
        self.builder().clear();
        self.convert_ids_at(tree);
        self.builder().wrap(BranchKind::Block, 0);
        self.builder().drain_into(tree);
    }

    fn convert_ids_at(&mut self, tree: &[Node]) -> Option<()> {
        match *tree.last()? {
            Node::Branch { kind, range } => {
                let start = self.builder().pos();
                let range: Range<usize> = range.into();
                for kid_index in range.clone() {
                    match kind {
                        BranchKind::Def => {
                            if kid_index == range.start {
                                if self.push_id_maybe(tree[kid_index]) {
                                    continue;
                                }
                            }
                        }
                        BranchKind::Fun => {
                            if kid_index == range.start {
                                self.convert_params_ids(&tree[..kid_index + 1]);
                                continue;
                            }
                        }
                        _ => {}
                    }
                    self.convert_ids_at(&tree[..kid_index + 1]);
                }
                self.builder().wrap(kind, start);
            }
            node @ _ => self.builder().push(node),
        }
        Some(())
    }

    fn convert_param_ids(&mut self, tree: &[Node]) -> Option<()> {
        match *tree.last()? {
            // Expect param, typed or not. TODO Always enforce typed before here?
            Node::Branch {
                kind: BranchKind::Typed,
                range,
            } => {
                let start = self.builder().pos();
                let range: Range<usize> = range.into();
                // Loop params.
                for kid_index in range.clone() {
                    let kid = tree[kid_index];
                    if kid_index == range.start {
                        if kid_index == range.start {
                            if self.push_id_maybe(tree[kid_index]) {
                                continue;
                            }
                        }
                    }
                    self.builder().push(kid);
                }
                self.builder().wrap(BranchKind::Typed, start);
            }
            node @ _ => self.builder().push(node),
        }
        Some(())
    }

    fn convert_params_ids(&mut self, tree: &[Node]) -> Option<()> {
        match *tree.last()? {
            // Expect param group. TODO Macros?
            Node::Branch {
                kind: BranchKind::Params,
                range,
            } => {
                let start = self.builder().pos();
                let range: Range<usize> = range.into();
                // Loop params.
                for kid_index in range.clone() {
                    match tree[kid_index] {
                        Node::Branch { .. } => {
                            self.convert_param_ids(&tree[..kid_index + 1]);
                        }
                        node @ _ => {
                            // Maybe Lonely id.
                            if !self.push_id_maybe(node) {
                                self.builder().push(node)
                            }
                        }
                    }
                }
                self.builder().wrap(BranchKind::Params, start);
            }
            // TODO Report error. Store error?
            node @ _ => self.builder().push(node),
        }
        Some(())
    }

    fn find_id(&mut self, tree: &[Node]) -> Option<Intern> {
        let root = *tree.last()?;
        match root {
            Node::Branch { kind, range } => {
                let start = self.builder().pos();
                let range: Range<usize> = range.into();
                for kid_index in range.clone() {
                    match self.find_id(&tree[..kid_index + 1]) {
                        result @ Some(..) => return result,
                        _ => {}
                    }
                    // self.any(&tree[..kid_index + 1], kid_index - range.start);
                }
                self.builder().wrap(kind, start);
            }
            Node::IdDef { intern, .. } => return Some(intern),
            _ => {}
        }
        None
    }

    fn push_id(&mut self, intern: lasso::Spur) {
        let num = self.id_num;
        self.id_num += 1;
        self.builder().push(Node::IdDef { intern, num });
    }

    fn push_id_maybe(&mut self, node: Node) -> bool {
        match node {
            Node::Leaf {
                token:
                    Token {
                        kind: TokenKind::Id,
                        intern,
                    },
            } => {
                self.push_id(intern);
                true
            }
            _ => false,
        }
    }

    fn resolve_top(&mut self, tree: &[Node]) -> Option<()> {
        let root = *tree.last()?;
        // TODO Need both num and index in value!
        let mut scope = HashMap::<Intern, usize>::new();
        match root {
            Node::Branch {
                kind: BranchKind::Block,
                range,
            } => {
                let start = self.builder().pos();
                let range: Range<usize> = range.into();
                for kid_index in range.clone() {
                    let kid = tree[kid_index];
                    if let Node::Branch {
                        kind: BranchKind::Def,
                        ..
                    } = kid
                    {
                        if let Some(id) = self.find_id(&tree[..kid_index + 1]) {
                            scope.insert(id, kid_index);
                        }
                    }
                    // self.any(&tree[..kid_index + 1], kid_index - range.start);
                }
                self.builder().wrap(BranchKind::Block, start);
            }
            _ => panic!(),
        }
        println!("Scope: {:?}", scope);
        Some(())
    }
}
