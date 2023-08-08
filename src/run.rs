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
        self.extract_top(tree);
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

    fn extract_top(&mut self, tree: &[Node]) -> Option<()> {
        let root = *tree.last()?;
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
                        range: kid_range,
                    } = kid
                    {
                        if let Node::IdDef { intern, .. } = tree[kid_range.start as usize] {
                            scope.insert(intern, kid_index);
                        }
                    }
                }
                self.builder().wrap(BranchKind::Block, start);
            }
            _ => panic!(),
        }
        println!("Scope: {:?}", scope);
        Some(())
    }
}
