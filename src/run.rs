use std::{collections::HashMap, ops::Range};

use crate::{
    lex::{Intern, Token, TokenKind},
    tree::{BranchKind, Nod, Node, TreeBuilder, Type},
    Cart,
};

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct Index(u32);

#[derive(Clone, Copy, Debug, Default, Eq, Ord, PartialEq, PartialOrd)]
struct ScopeEntry {
    intern: Intern,
    module: u16,
    num: u32,
}

pub struct Runner {
    pub cart: Cart,
    def_indices: Vec<Index>,
    module: u16,
    scope: Vec<ScopeEntry>,
    // Single vector to support overloading while trying to limit allocations.
    // TODO Differentiate overload and full definitions.
    // TODO Find or make some abstraction for this kind of multimap?
    tops: Vec<ScopeEntry>,
    top_map: HashMap<Intern, u32>,
}

impl Runner {
    pub fn new(cart: Cart) -> Self {
        Self {
            cart,
            def_indices: vec![Index::default()],
            module: 1, // TODO Differentiate!
            scope: vec![],
            tops: vec![],
            top_map: HashMap::new(),
        }
    }

    pub fn run(&mut self, tree: &mut Vec<Node>) {
        self.convert_ids(tree);
        self.extract_top(tree);
        self.resolve(tree);
        println!("Defs: {:?}", self.def_indices);
        // println!("Tops: {:?}", self.tops);
        // println!("Top map: {:?}", self.top_map);
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
                                if self.push_id_maybe(
                                    &tree[..=kid_index],
                                    tree.len() as u32 - 1,
                                    false,
                                ) {
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
                if let Nod::Uid { num, .. } = node.nod {
                    // TODO Check if external module!
                    self.def_indices[num as usize] = Index(tree.len() as u32 - 1);
                }
                self.builder().push(node);
            }
        }
        Some(())
    }

    fn push_id(&mut self, node: Node, index: u32, intern: Intern, r#pub: bool) {
        let module = if r#pub { self.module } else { 0 };
        let num = self.def_indices.len() as u32;
        self.def_indices.push(Index(index));
        self.builder().push(Node {
            nod: Nod::Uid {
                intern,
                module,
                num,
            },
            ..node
        });
    }

    fn push_id_maybe(&mut self, tree: &[Node], index: u32, r#pub: bool) -> bool {
        let node = *tree.last().unwrap();
        match node.nod {
            Nod::Branch {
                kind: BranchKind::Pub,
                range,
            } => {
                let range: Range<usize> = range.into();
                self.push_id_maybe(&tree[..=range.start], index, true)
            }
            Nod::Leaf {
                token:
                    Token {
                        kind: TokenKind::Id,
                        intern,
                    },
                ..
            } => {
                self.push_id(node, index, intern, r#pub);
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
                        match scope_entry(&tree[..=kid_range.start as usize]) {
                            Some(entry) => self.tops.push(entry),
                            None => (),
                        }
                    }
                }
            }
            _ => panic!(),
        }
        // Sort then map.
        self.tops.sort();
        self.top_map.clear();
        let mut last = Intern::default();
        for (index, ScopeEntry { intern, .. }) in self.tops.iter().enumerate() {
            if *intern != last {
                self.top_map.insert(*intern, index as u32);
                last = *intern
            }
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
                for (local_index, kid_index) in range.clone().enumerate() {
                    if kind == BranchKind::Def && local_index == 0 {
                        match scope_entry(&tree[..=kid_index]) {
                            Some(entry) => self.scope.push(entry),
                            None => (),
                        }
                    }
                    self.resolve_at(&tree[..=kid_index]);
                }
                match node.nod {
                    Nod::Branch {
                        kind: BranchKind::Def | BranchKind::Params,
                        ..
                    } => {}
                    _ => {
                        if scope_start < self.scope.len() {
                            // Reset scope if we're not in params.
                            // println!("Popping from: {:?}", self.scope);
                            self.scope.resize(scope_start, ScopeEntry::default());
                        }
                    }
                }
                self.builder().wrap(kind, start, node.typ, node.source);
            }
            Nod::Leaf {
                token:
                    Token {
                        kind: TokenKind::Id,
                        intern,
                    },
            } => {
                let node = match self.resolve_def(intern) {
                    Some(entry) => Node {
                        nod: Nod::Uid {
                            intern,
                            module: entry.module,
                            num: entry.num,
                        },
                        ..node
                    },
                    _ => node,
                };
                self.builder().push(node);
            }
            _ => {
                self.builder().push(node);
            }
        }
        Some(())
    }

    fn resolve_def(&mut self, intern: Intern) -> Option<ScopeEntry> {
        for entry in self.scope.iter().rev() {
            // println!("Check {intern:?} vs {entry:?}");
            if entry.intern == intern {
                // TODO Check overloads! Would need more context specs.
                return Some(*entry);
            }
        }
        if let Some(index) = self.top_map.get(&intern) {
            // TODO Check overloads!
            return Some(self.tops[*index as usize]);
        }
        None
    }
}

fn scope_entry(tree: &[Node]) -> Option<ScopeEntry> {
    match tree.last().unwrap().nod {
        Nod::Uid {
            intern,
            module,
            num,
        } => Some(ScopeEntry {
            intern,
            module,
            num,
        }),
        _ => None,
    }
}
