use std::{collections::HashMap, ops::Range};

use crate::{
    lex::{Intern, Interner, Token, TokenKind},
    tree::{BranchKind, Nod, Node, SimpleRange, TreeBuilder, Type},
    Cart,
};

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct Index(u32);

#[derive(Clone, Copy, Debug, Default, Eq, Ord, PartialEq, PartialOrd)]
pub struct ScopeEntry {
    intern: Intern,
    module: u16,
    num: u32,
}

pub struct Module {
    pub def_indices: Vec<Index>,
    pub name: Intern,
    pub num: u16,
    // TODO Find or make some abstraction for this kind of multimap?
    pub tops: Vec<ScopeEntry>,
    pub top_map: HashMap<Intern, u32>,
    pub tree: Vec<Node>,
}

impl Module {
    pub fn get_top(&self, intern: Intern) -> Option<ScopeEntry> {
        // TODO None on duplicates.
        self.top_map
            .get(&intern)
            .map(|ind| self.tops[*ind as usize])
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
    module: u16,
    scope: Vec<ScopeEntry>,
    // Single vector to support overloading while trying to limit allocations.
    // TODO Differentiate overload and full definitions.
    // TODO Find or make some abstraction for this kind of multimap?
    pub tops: Vec<ScopeEntry>,
    pub top_map: HashMap<Intern, u32>,
    types: TreeBuilder,
}

impl<'a> Runner<'a> {
    pub fn new(cart: &'a mut Cart) -> Self {
        let module = cart.modules.len() as u16 + cart.core.is_some() as u16 + 1;
        Self {
            cart,
            def_indices: vec![Index::default()],
            module,
            scope: vec![],
            tops: vec![],
            top_map: HashMap::new(),
            types: TreeBuilder::default(),
        }
    }

    pub fn run(mut self, name: Intern, tree: &mut Vec<Node>) -> Module {
        self.types.clear();
        self.convert_ids(tree);
        self.extract_top(tree);
        self.resolve(tree);
        self.type_any(tree, Type(0));
        self.append_types(tree);
        println!("Defs: {:?}", self.def_indices);
        // println!("Tops: {:?}", self.tops);
        // println!("Top map: {:?}", self.top_map);
        Module {
            def_indices: self.def_indices,
            name,
            num: self.module,
            tops: self.tops,
            top_map: self.top_map,
            tree: self.cart.tree_builder.nodes.clone(),
        }
    }

    fn append_types(&mut self, tree: &mut Vec<Node>) {
        self.builder().nodes.extend_from_slice(&tree);
        self.builder().pop();
        self.builder().pop();
        // Double-wrap to push a block.
        self.types.wrap(BranchKind::Types, 0, Type(0), 0);
        self.types.wrap(BranchKind::Types, 0, Type(0), 0);
        self.cart.tree_builder.push_tree(&self.types.nodes);
        self.builder().wrap(BranchKind::Block, 0, Type(0), 0);
        self.builder().wrap(BranchKind::Block, 0, Type(0), 0);
        self.builder().drain_into(tree);
    }

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    fn build_type(&mut self, tree: &[Node]) -> bool {
        let node = *tree.last().unwrap();
        match node.nod {
            // TODO Complex types.
            Nod::Uid { .. } => {
                // TODO Look up existing type.
                self.types.push(node);
                true
            }
            _ => false,
        }
    }

    fn type_any(&mut self, tree: &mut [Node], typ: Type) -> Option<()> {
        let node = *tree.last()?;
        let mut typ = typ.or(node.typ);
        match node.nod {
            Nod::Branch { kind, range } => {
                let range: Range<usize> = range.into();
                let handled = match kind {
                    BranchKind::Def => {
                        typ = self.type_def(&range, tree, typ);
                        true
                    }
                    BranchKind::Fun => {
                        println!("Interpret fun sig as type");
                        false
                    }
                    _ => false,
                };
                // TODO Eventually, should we handle everything above?
                if !handled {
                    for kid_index in range.clone() {
                        self.type_any(&mut tree[..=kid_index], Type::default());
                    }
                }
            }
            _ => {}
        }
        // TODO Push with new typ instead?
        tree.last_mut().unwrap().typ = typ;
        Some(())
    }

    fn type_def(&mut self, range: &Range<usize>, tree: &mut [Node], mut typ: Type) -> Type {
        let start = range.start;
        let xtype = tree[start + 1];
        let value = tree[start + 2];
        if typ.0 == 0 {
            // Prioritize previously evaluated types, first from
            // the explicit type.
            typ = xtype.typ.or(value.typ);
            if typ.0 == 0 {
                // Failing that, interpret the explicit type
                // from the tree.
                if self.build_type(&tree[..=start + 1]) {
                    typ = Type(self.types.pos());
                }
            }
        }
        // There should always be exactly 3 kids of defs.
        self.type_any(&mut tree[..=start], typ);
        // TODO Look up Type type.
        self.type_any(&mut tree[..=start + 1], Type::default());
        self.type_any(&mut tree[..=start + 2], typ);
        typ
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
        // TODO Work through imports more generally.
        if let Some(core) = &self.cart.core {
            // TODO Check overloads!
            return core.get_top(intern);
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
