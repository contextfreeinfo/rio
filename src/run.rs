use std::{collections::HashMap, ops::Range};

use crate::{
    lex::{Intern, Token, TokenKind},
    tree::{BranchKind, Node, TreeBuilder, Type},
    Cart,
};

pub struct Runner {
    pub cart: Cart,
    id_defs: Vec<u32>,
    tops: HashMap<Intern, u32>,
}

impl Runner {
    pub fn new(cart: Cart) -> Self {
        Self {
            cart,
            id_defs: vec![0],
            tops: HashMap::new(),
        }
    }

    pub fn run(&mut self, tree: &mut Vec<Node>) {
        self.convert_ids(tree);
        self.extract_top(tree);
        println!("Defs: {:?}", self.id_defs);
        println!("Tops: {:?}", self.tops);
    }

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    fn convert_ids(&mut self, tree: &mut Vec<Node>) {
        self.id_defs.fill(0);
        self.builder().clear();
        self.convert_ids_at(tree);
        self.builder()
            .wrap(BranchKind::Block, 0, Type::default(), 0);
        self.builder().drain_into(tree);
    }

    fn convert_ids_at(&mut self, tree: &[Node]) -> Option<()> {
        match *tree.last()? {
            Node::Branch { kind, range, typ } => {
                let start = self.builder().pos();
                let range: Range<usize> = range.into();
                for kid_index in range.clone() {
                    match kind {
                        BranchKind::Def => {
                            if kid_index == range.start {
                                if self.push_id_maybe(tree[kid_index], tree.len() as u32 - 1) {
                                    continue;
                                }
                            }
                        }
                        _ => {}
                    }
                    self.convert_ids_at(&tree[..kid_index + 1]);
                }
                self.builder().wrap(kind, start, typ, tree.len() - 1);
            }
            node @ _ => {
                if let Node::IdDef { num, .. } = node {
                    self.id_defs[num as usize] = tree.len() as u32 - 1;
                }
                self.builder().push(node, tree.len() - 1);
            }
        }
        Some(())
    }

    fn push_id(&mut self, intern: lasso::Spur, index: u32) {
        let num = self.id_defs.len() as u32;
        self.id_defs.push(index);
        self.builder().push(
            Node::IdDef {
                intern,
                num,
                typ: Type::default(),
            },
            index as usize,
        );
    }

    fn push_id_maybe(&mut self, node: Node, index: u32) -> bool {
        match node {
            Node::Leaf {
                token:
                    Token {
                        kind: TokenKind::Id,
                        intern,
                    },
                ..
            } => {
                self.push_id(intern, index);
                true
            }
            _ => false,
        }
    }

    fn extract_top(&mut self, tree: &[Node]) -> Option<()> {
        let root = *tree.last()?;
        self.tops.clear();
        match root {
            Node::Branch {
                kind: BranchKind::Block,
                range,
                ..
            } => {
                let range: Range<usize> = range.into();
                for kid_index in range.clone() {
                    let kid = tree[kid_index];
                    if let Node::Branch {
                        kind: BranchKind::Def,
                        range: kid_range,
                        ..
                    } = kid
                    {
                        if let Node::IdDef { intern, num, .. } = tree[kid_range.start as usize] {
                            self.tops.insert(intern, num);
                        }
                    }
                }
            }
            _ => panic!(),
        }
        Some(())
    }
}
