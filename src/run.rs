use std::{collections::HashMap, ops::Range};

use crate::{
    lex::{Intern, Token, TokenKind},
    parse::{BranchKind, Node, TreeBuilder},
};

pub struct Runner {
    pub builder: TreeBuilder,
}

impl Runner {
    pub fn new(builder: TreeBuilder) -> Self {
        Self { builder }
    }

    pub fn run(&mut self, tree: &[Node]) -> Vec<Node> {
        self.builder.clear();
        self.resolve_top(tree);
        self.builder.extract()
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
            Node::Leaf {
                token:
                    Token {
                        kind: TokenKind::Id,
                        intern,
                    },
            } => return Some(intern),
            _ => {}
        }
        None
    }

    fn resolve_top(&mut self, tree: &[Node]) -> Option<()> {
        let root = *tree.last()?;
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
