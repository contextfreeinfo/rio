use std::{collections::HashMap, ops::Range};

use crate::{
    lex::{Intern, Token, TokenKind},
    tree::{BranchKind, Node, TreeBuilder},
};

pub struct Runner {
    pub builder: TreeBuilder,
    id_num: u32,
}

impl Runner {
    pub fn new(builder: TreeBuilder) -> Self {
        Self { builder, id_num: 1 }
    }

    pub fn run(&mut self, tree: &mut Vec<Node>) {
        self.convert_ids(tree);
        self.resolve_top(tree);
    }

    fn convert_ids(&mut self, tree: &mut Vec<Node>) {
        self.builder.clear();
        self.convert_ids_at(tree);
        self.builder.wrap(BranchKind::Block, 0);
        self.builder.drain_into(tree);
    }

    fn convert_ids_at(&mut self, tree: &[Node]) -> Option<()> {
        match *tree.last()? {
            Node::Branch { kind, range } => {
                let start = self.builder.pos();
                let range: Range<usize> = range.into();
                for kid_index in range.clone() {
                    match kind {
                        BranchKind::Def => {
                            if kid_index == range.start {
                                if let Node::Leaf {
                                    token:
                                        Token {
                                            kind: TokenKind::Id,
                                            intern,
                                        },
                                } = tree[kid_index]
                                {
                                    // Got an id assignment.
                                    // TODO Normalize fancier assignments before we get here.
                                    let num = self.id_num;
                                    self.id_num += 1;
                                    self.builder.push(Node::Id { intern, num });
                                    continue;
                                }
                            }
                        }
                        BranchKind::Fun => {
                            // TODO Go through first if group and find params.
                        }
                        _ => {}
                    }
                    self.convert_ids_at(&tree[..kid_index + 1]);
                }
                self.builder.wrap(kind, start);
            }
            node @ _ => self.builder.push(node),
        }
        Some(())
    }

    fn find_id(&mut self, tree: &[Node]) -> Option<Intern> {
        let root = *tree.last()?;
        match root {
            Node::Branch { kind, range } => {
                let start = self.builder.pos();
                let range: Range<usize> = range.into();
                for kid_index in range.clone() {
                    match self.find_id(&tree[..kid_index + 1]) {
                        result @ Some(..) => return result,
                        _ => {}
                    }
                    // self.any(&tree[..kid_index + 1], kid_index - range.start);
                }
                self.builder.wrap(kind, start);
            }
            Node::Id { intern, .. } => return Some(intern),
            _ => {}
        }
        None
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
                let start = self.builder.pos();
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
                self.builder.wrap(BranchKind::Block, start);
            }
            _ => panic!(),
        }
        println!("Scope: {:?}", scope);
        Some(())
    }
}
