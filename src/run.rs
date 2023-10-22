use std::{collections::HashMap, ops::Range};

use crate::{
    lex::{Intern, Interner, Token, TokenKind},
    tree::{BranchKind, Nod, Node, TreeBuilder, Type},
    Cart,
};

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct Index(pub u32);

#[derive(Clone, Copy, Debug, Default, Eq, Ord, PartialEq, PartialOrd)]
pub struct ScopeEntry {
    intern: Intern,
    module: u16,
    num: u32,
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
    any_change: bool,
    pub cart: &'a mut Cart,
    pub def_indices: Vec<Index>,
    module: u16,
    scope: Vec<ScopeEntry>,
    // Single vector to support overloading while trying to limit allocations.
    // TODO Differentiate overload and full definitions.
    // TODO Find or make some abstraction for this kind of multimap?
    pub tops: Vec<ScopeEntry>,
    pub top_map: HashMap<Intern, u32>,
    type_refs: Vec<Type>,
    types: TreeBuilder,
}

impl<'a> Runner<'a> {
    pub fn new(cart: &'a mut Cart) -> Self {
        let module = cart.modules.len() as u16 + 1;
        Self {
            any_change: false,
            cart,
            def_indices: vec![Index::default()],
            module,
            scope: vec![],
            tops: vec![],
            top_map: HashMap::new(),
            type_refs: vec![],
            types: TreeBuilder::default(),
        }
    }

    pub fn run(mut self, name: Intern, tree: &mut Vec<Node>) -> Module {
        self.types.clear();
        self.convert_ids(tree);
        self.update_def_inds(tree);
        self.extract_top(tree);
        println!("Defs of {}: {:?}", tree.len(), self.def_indices);
        // TODO Multiple big rounds of resolution and typing.
        self.resolve(tree);
        if !tree.is_empty() {
            let end = tree.len() - 1;
            // 2 rounds of typing lets us get back references.
            for _ in 0..2 {
                self.any_change = false;
                // Keep full tree for typing or eval so we can reference anywhere.
                self.type_any(tree, end, Type(0));
                if !self.any_change {
                    break;
                }
            }
        }
        self.append_types(tree);
        self.update_def_inds(tree);
        // println!("Tops: {:?}", self.tops);
        // println!("Top map: {:?}", self.top_map);
        Module {
            def_indices: self.def_indices,
            name,
            num: self.module,
            tops: self.tops,
            top_map: self.top_map,
            tree: tree.clone(),
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

    fn build_type(&mut self, tree: &[Node]) -> Option<Type> {
        let node = tree.last().unwrap();
        match node.nod {
            // TODO Complex types.
            Nod::Uid { .. } => {
                // TODO Look up existing type.
                if node.typ.0 == 0 {
                    self.types.push(*node);
                    Some(Type(self.types.pos()))
                } else {
                    Some(node.typ)
                }
            }
            _ => None,
        }
    }

    fn set_type(&mut self, node: &mut Node, typ: Type) {
        if node.typ != typ {
            node.typ = typ;
            self.any_change = true;
        }
    }

    fn type_any(&mut self, tree: &mut [Node], at: usize, typ: Type) -> Type {
        let node = tree[at];
        let mut typ = typ.or(node.typ);
        match node.nod {
            Nod::Branch { kind, range } => {
                let range: Range<usize> = range.into();
                let handled = match kind {
                    BranchKind::Def => {
                        typ = self.type_def(tree, &range, typ);
                        true
                    }
                    BranchKind::Fun => {
                        typ = self.type_fun(tree, at, typ);
                        true
                    }
                    _ => false,
                };
                // TODO Eventually, should we handle everything above?
                if !handled {
                    for kid_index in range.clone() {
                        self.type_any(tree, kid_index, Type::default());
                    }
                }
            }
            Nod::Leaf { token } => match token.kind {
                TokenKind::String => {
                    if typ.0 == 0 {
                        typ = self
                            .build_type(&[self.cart.core_exports.text_type.into()])
                            .unwrap();
                    }
                }
                _ => {}
            },
            Nod::Uid { module, num, .. } => {
                if module == 0 || module == self.module {
                    // Dig from this module
                    let def_typ = tree[self.def_indices[num as usize].0 as usize].typ;
                    if def_typ.0 != 0 {
                        typ = def_typ;
                    }
                } else {
                    // Dig from other modules
                    let other = &self.cart.modules[module as usize - 1];
                    let other_node = other.tree[(other.def_indices[num as usize].0 as usize)];
                    let def_typ = other_node.typ;
                    if def_typ.0 != 0 {
                        // TODO Copy type into our types.
                        println!("Found {def_typ:?} in module {module}");
                    }
                }
            }
            _ => {}
        }
        // Assign on existing structure lets us go in arbitrary order easier.
        self.set_type(&mut tree[at], typ);
        typ
    }

    fn type_def(&mut self, tree: &mut [Node], range: &Range<usize>, mut typ: Type) -> Type {
        let start = range.start;
        let xtype = tree[start + 1];
        let value = tree[start + 2];
        if typ.0 == 0 {
            // Prioritize previously evaluated types, first from the explicit
            // type.
            typ = xtype.typ.or(value.typ);
            if typ.0 == 0 {
                // Failing that, interpret the explicit type from the tree.
                if let Some(built_typ) = self.build_type(&tree[..=start + 1]) {
                    typ = built_typ;
                }
            }
        }
        // There should always be exactly 3 kids of defs: id, xtype, value.
        // Check value first in case we want the type from it.
        self.type_any(tree, start + 2, typ);
        if typ.0 == 0 {
            // We did't have type yet, so try the value's type.
            typ = tree[start + 2].typ;
        }
        self.type_any(tree, start, typ);
        // TODO Look up Type type.
        self.type_any(tree, start + 1, Type::default());
        typ
    }

    fn type_fun(&mut self, tree: &mut [Node], at: usize, typ: Type) -> Type {
        let node = tree[at];
        let Nod::Branch { kind: BranchKind::Fun, range } = node.nod else { panic!() };
        // Kids
        let range: Range<usize> = range.into();
        // Should always be in-params, out(-params), body.
        let Range { start, .. } = range;
        // Params
        let Nod::Branch { range: params_range, .. } = tree[start].nod else { panic!() };
        let params_range: Range<usize> = params_range.into();
        let ref_start = self.type_refs.len();
        let mut any_change = false;
        for param_index in params_range.clone() {
            let old_type = tree[param_index].typ;
            let param_type = self.type_any(tree, param_index, Type::default());
            self.type_refs.push(param_type);
            any_change |= param_type != old_type;
        }
        // Return
        let Nod::Branch { range: out_range, .. } = tree[start + 1].nod else { panic!() };
        let out_range: Range<usize> = out_range.into();
        let old_out_type = if node.typ.0 == 0 {
            Type(0)
        } else {
            self.types.working[node.typ.0 as usize - 1].typ
        };
        let mut out_type = if out_range.is_empty() {
            old_out_type.or(
                // See if we're expecting a particular return type.
                if typ.0 != 0 {
                    self.types.working[typ.0 as usize - 1].typ
                } else {
                    Type(0)
                },
            )
        } else {
            self.type_any(tree, out_range.start, old_out_type)
        };
        // Body
        if range.len() > 2 {
            let body_type = self.type_any(tree, start + 2, out_type.or(old_out_type));
            if out_type.0 == 0 && tree[out_range.start].typ.0 == 0 {
                // Infer return type from body.
                self.set_type(&mut tree[out_range.start], body_type);
                out_type = body_type;
            }
        } else if out_type.0 == 0 {
            // Infer void for empty body. TODO Infer by context with default return value?
            out_type = match old_out_type.0 {
                0 => self
                    .build_type(&[self.cart.core_exports.void_type.into()])
                    .unwrap(),
                _ => old_out_type,
            }
        }
        any_change |= out_type != old_out_type;
        if node.typ.0 == 0 || any_change {
            // After digging subtrees, we're ready to push type refs.
            let types_start = self.types.pos();
            for param_type in self.type_refs.drain(ref_start..) {
                self.types.push(Node {
                    typ: param_type,
                    source: 0.into(),
                    nod: Nod::Branch {
                        kind: BranchKind::None,
                        range: (0u32..0u32).into(),
                    },
                });
            }
            // Function
            self.types
                .wrap(BranchKind::FunType, types_start, out_type, 0);
            Type(self.types.pos())
        } else {
            node.typ
        }
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
                    self.def_indices[num as usize] = Index(at as u32);
                }
            }
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
