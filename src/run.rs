use std::ops::Range;

use crate::{
    lex::{Intern, Token, TokenKind},
    tree::{BranchKind, Nod, Node, TreeBuilder, Type},
    Cart,
};

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct Index(u32);

#[derive(Clone, Copy, Debug, Default, Eq, Ord, PartialEq, PartialOrd)]
pub struct DefNum(u32);

type ScopeEntry = (Intern, DefNum);

pub struct Runner {
    pub cart: Cart,
    def_indices: Vec<Index>,
    scope: Vec<ScopeEntry>,
    // Single vector to support overloading while trying to limit allocations.
    tops: Vec<ScopeEntry>,
}

impl Runner {
    pub fn new(cart: Cart) -> Self {
        Self {
            cart,
            def_indices: vec![Index::default()],
            scope: vec![],
            tops: vec![],
        }
    }

    pub fn run(&mut self, tree: &mut Vec<Node>) {
        self.convert_ids(tree);
        self.extract_top(tree);
        self.tops.sort();
        self.resolve(tree);
        println!("Defs: {:?}", self.def_indices);
        println!("Tops: {:?}", self.tops);
    }

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    fn convert_ids(&mut self, tree: &mut Vec<Node>) {
        self.def_indices.fill(Index::default());
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
                    self.convert_ids_at(&tree[..=kid_index]);
                }
                self.builder().wrap(kind, start, node.typ, node.source);
            }
            _ => {
                if let Nod::IdDef { num, .. } = node.nod {
                    self.def_indices[num as usize] = Index(tree.len() as u32 - 1);
                }
                self.builder().push_at(node, tree.len() - 1);
            }
        }
        Some(())
    }

    fn push_id(&mut self, node: Node, index: u32, intern: Intern) {
        let num = self.def_indices.len() as u32;
        self.def_indices.push(Index(index));
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
                            self.tops.push((intern, DefNum(num)));
                        }
                    }
                }
            }
            _ => panic!(),
        }
        Some(())
    }

    fn resolve(&mut self, tree: &mut Vec<Node>) {
        self.builder().clear();
        self.resolve_at(tree);
        self.builder()
            .wrap(BranchKind::Block, 0, Type::default(), 0);
        self.builder().drain_into(tree);
    }

    fn resolve_at(&mut self, tree: &[Node]) -> Option<()> {
        let node = *tree.last()?;
        match node.nod {
            Nod::Branch { kind, range, .. } => {
                let scope_start = self.scope.len();
                let start = self.builder().pos();
                let range: Range<usize> = range.into();
                for kid_index in range.clone() {
                    if let Nod::IdDef { intern, num } = tree[kid_index].nod {
                        self.scope.push((intern, DefNum(num)));
                    }
                    self.resolve_at(&tree[..=kid_index]);
                }
                match node.nod {
                    Nod::Branch {
                        kind: BranchKind::Params,
                        ..
                    } => {}
                    _ => if scope_start < self.scope.len() {
                        // Reset scope if we're not in params.
                        println!("Popping from: {:?}", self.scope);
                        self.scope.resize(scope_start, ScopeEntry::default());
                    }
                }
                self.builder().wrap(kind, start, node.typ, node.source);
            }
            _ => {
                self.builder().push_at(node, tree.len() - 1);
            }
        }
        Some(())
    }
}
