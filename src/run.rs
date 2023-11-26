use std::ops::Range;

use crate::{
    lex::{Intern, Interner, Token, TokenKind},
    tree::{BranchKind, Nod, Node, TreeBuilder, Type},
    typ::{append_types, type_tree, Typer},
    util::{MultiMap, MultiMapEntry},
    Cart,
};

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct Index(pub u32);

#[derive(Clone, Copy, Debug, Default, Eq, Ord, PartialEq, PartialOrd)]
pub struct ScopeEntry {
    pub intern: Intern,
    pub module: u16,
    pub num: u32,
}

impl From<ScopeEntry> for Node {
    fn from(value: ScopeEntry) -> Self {
        let ScopeEntry {
            intern,
            module,
            num,
        } = value;
        Node {
            typ: 0.into(),
            source: 0.into(),
            nod: Nod::Uid {
                intern,
                module,
                num,
            },
        }
    }
}

pub struct Module {
    pub name: Intern,
    pub num: u16,
    pub tops: MultiMap<Intern, ScopeEntry>,
    pub tree: Vec<Node>,
}

impl MultiMapEntry<Intern> for ScopeEntry {
    fn key(&self) -> Intern {
        self.intern
    }
}

impl Module {
    pub fn get_top(&self, intern: Intern) -> Option<ScopeEntry> {
        // TODO None on duplicates with type info.
        self.tops.get(intern).next().copied()
    }
}

/// Provide easy access for comparing resolutions to core native definitions.
#[derive(Clone, Copy, Debug, Default)]
pub struct CoreExports {
    pub function_type: ScopeEntry,
    pub native_fun: ScopeEntry,
    pub print_fun: ScopeEntry,
    pub text_type: ScopeEntry,
    pub type_type: ScopeEntry,
    pub void_type: ScopeEntry,
}

impl CoreExports {
    pub fn extract(core: &Module, interner: &Interner) -> CoreExports {
        let get = |name: &str| {
            // If called at the right point in processing, we should have these.
            core.get_top(interner.get(name).unwrap()).unwrap()
        };
        CoreExports {
            function_type: get("Function"),
            native_fun: get("native"),
            print_fun: get("print"),
            text_type: get("Text"),
            type_type: get("Type"),
            void_type: get("Void"),
        }
    }
}

pub struct Runner<'a> {
    pub cart: &'a mut Cart,
    pub def_indices: Vec<Index>,
    pub module: u16,
    pending: Option<Node>,
    scope: Vec<ScopeEntry>,
    // TODO Differentiate overload and full definitions.
    pub tops: MultiMap<Intern, ScopeEntry>,
    pub typer: Typer,
}

impl<'a> Runner<'a> {
    pub fn new(cart: &'a mut Cart) -> Self {
        let module = cart.modules.len() as u16 + 1;
        Self {
            cart,
            def_indices: vec![Index::default()],
            pending: None,
            module,
            scope: vec![],
            tops: MultiMap::new(),
            typer: Typer::new(),
        }
    }

    pub fn run(mut self, name: Intern, tree: &mut Vec<Node>) -> Module {
        // println!("---");
        // println!("hash: {}", tree_hash(&tree));
        self.typer.clear();
        self.pending = Some(Node {
            typ: 0.into(),
            source: 0.into(),
            nod: Nod::ModuleId {
                name,
                module: self.module,
            },
        });
        self.convert_ids(tree);
        self.update_def_inds(tree);
        self.extract_top(tree);
        // TODO Multiple big rounds of resolution and typing.
        self.resolve(tree);
        type_tree(&mut self, tree);
        // Finalize things.
        append_types(&mut self, tree);
        // Appending types shifted indices, so update them.
        self.update_def_inds(tree);
        // println!("Defs of {}: {:?}", tree.len(), self.def_indices);
        // Then replace indirect indices with direct.
        self.update_uids_at_end(tree);
        // println!("Tops: {:?}", self.tops);
        // println!("Top map: {:?}", self.top_map);
        Module {
            name,
            num: self.module,
            tops: self.tops,
            tree: tree.clone(),
        }
    }

    pub fn builder(&mut self) -> &mut TreeBuilder {
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
                if let Some(pending) = self.pending {
                    self.builder().push(pending);
                    self.pending = None;
                }
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
            _ => self.builder().push(node),
        }
        Some(())
    }

    fn push_id(&mut self, node: Node, intern: Intern, r#pub: bool) {
        let module = if r#pub { self.module } else { 0 };
        let num = self.def_indices.len() as u32;
        // Fix indices later.
        self.def_indices.push(Index(0));
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
                self.push_id(node, intern, r#pub);
                true
            }
            _ => false,
        }
    }

    fn extract_top(&mut self, tree: &[Node]) -> Option<()> {
        let root = *tree.last()?;
        self.tops.values.clear();
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
                            Some(entry) => self.tops.values.push(entry),
                            None => (),
                        }
                    }
                }
            }
            _ => panic!(),
        }
        // Sort then map.
        self.tops.update(true);
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
        if let Some(entry) = self.tops.get(intern).next() {
            // TODO Check overloads!
            return Some(*entry);
        }
        // TODO Work through imports more generally.
        if let Some(core) = self.cart.modules.get(0) {
            // TODO Check overloads!
            return core.get_top(intern);
        }
        None
    }

    /// Update def indices for current tree.
    fn update_def_inds(&mut self, tree: &mut [Node]) {
        for (at, node) in tree.iter().enumerate() {
            if let Nod::Branch {
                kind: BranchKind::Def,
                range,
            } = node.nod
            {
                let range: Range<usize> = range.into();
                if let Nod::Uid { num, .. } = tree[range.start].nod {
                    self.def_indices[num as usize] = Index(at as u32 + 1);
                }
            }
        }
    }

    fn update_uids_at_end(&mut self, tree: &mut [Node]) {
        // Tree
        for node in tree {
            if let Nod::Uid {
                intern,
                module,
                num,
            } = node.nod
            {
                if module == 0 || module == self.module {
                    node.nod = Nod::Uid {
                        intern,
                        module,
                        num: self.def_indices[num as usize].0,
                    };
                }
            }
        }
        // Tops
        for top in self.tops.values.iter_mut() {
            top.num = self.def_indices[top.num as usize].0;
        }
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
