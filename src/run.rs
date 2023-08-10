use std::{collections::HashMap, ops::Range};

use crate::{
    lex::{Intern, Token, TokenKind},
    tree::{BranchKind, Nod, Node, TreeBuilder, Type},
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
        let node = *tree.last()?;
        match node.nod {
            Nod::Branch { kind, range, .. } => {
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
                self.builder().wrap(kind, start, node.typ, node.source);
            }
            _ => {
                if let Nod::IdDef { num, .. } = node.nod {
                    self.id_defs[num as usize] = tree.len() as u32 - 1;
                }
                self.builder().push_at(node, tree.len() - 1);
            }
        }
        Some(())
    }

    fn push_id(&mut self, node: Node, index: u32, intern: Intern) {
        let num = self.id_defs.len() as u32;
        self.id_defs.push(index);
        self.builder().push(Node {
            nod: Nod::IdDef { intern, num },
            ..node
        });
    }

    fn push_id_maybe(&mut self, node: Node, index: u32) -> bool {
        match node.nod {
            Nod::Leaf {
                token:
                    Token {
                        kind: TokenKind::Id,
                        intern,
                    },
                ..
            } => {
                self.push_id(node, index, intern);
                true
            }
            _ => false,
        }
    }

    fn extract_top(&mut self, tree: &[Node]) -> Option<()> {
        let root = *tree.last()?;
        self.tops.clear();
        match root.nod {
            Nod::Branch {
                kind: BranchKind::Block,
                range,
                ..
            } => {
                let range: Range<usize> = range.into();
                for kid_index in range.clone() {
                    let kid = tree[kid_index];
                    if let Nod::Branch {
                        kind: BranchKind::Def,
                        range: kid_range,
                        ..
                    } = kid.nod
                    {
                        if let Nod::IdDef { intern, num, .. } = tree[kid_range.start as usize].nod {
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
