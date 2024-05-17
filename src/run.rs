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
    pub add_fun: ScopeEntry,
    pub branch_fun: ScopeEntry,
    pub claim_type: ScopeEntry,
    pub else_fun: ScopeEntry,
    pub eq_fun: ScopeEntry,
    pub false_val: ScopeEntry,
    pub function_type: ScopeEntry,
    pub ge_fun: ScopeEntry,
    pub gt_fun: ScopeEntry,
    pub int32_type: ScopeEntry,
    pub le_fun: ScopeEntry,
    pub lt_fun: ScopeEntry,
    pub native_fun: ScopeEntry,
    pub ne_fun: ScopeEntry,
    pub pair_type: ScopeEntry,
    pub print_fun: ScopeEntry,
    pub struct_fun: ScopeEntry,
    pub sub_fun: ScopeEntry,
    pub text_type: ScopeEntry,
    pub true_val: ScopeEntry,
    pub type_type: ScopeEntry,
    pub void_type: ScopeEntry,
}

impl CoreExports {
    pub fn extract(tops: &MultiMap<Intern, ScopeEntry>, interner: &Interner) -> CoreExports {
        let get = |name: &str| {
            // If called late enough in processing, we should have these.
            tops.get(interner.get(name).unwrap())
                .next()
                .copied()
                .unwrap()
        };
        CoreExports {
            add_fun: get("add"),
            branch_fun: get("branch"),
            claim_type: get("Claim"),
            else_fun: get("else"),
            eq_fun: get("eq"),
            false_val: get("false"),
            function_type: get("Function"),
            ge_fun: get("ge"),
            gt_fun: get("gt"),
            int32_type: get("Int32"),
            le_fun: get("le"),
            lt_fun: get("lt"),
            native_fun: get("native"),
            ne_fun: get("ne"),
            pair_type: get("Pair"),
            print_fun: get("print"),
            struct_fun: get("struct"),
            sub_fun: get("sub"),
            text_type: get("Text"),
            true_val: get("true"),
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
    // TODO Differentiate overload and full definitions. Or no overloads?
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
        if self.module == 1 {
            // Get intermediate nums for internal recognition.
            self.cart.core_exports = CoreExports::extract(&self.tops, &self.cart.interner);
        }
        // TODO Multiple big rounds of resolution and typing.
        self.resolve(tree);
        self.evaluate(tree);
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
        if self.module == 1 {
            // Get final nums for use from other modules.
            self.cart.core_exports = CoreExports::extract(&self.tops, &self.cart.interner);
        }
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

    pub fn evaluate(&mut self, tree: &mut Vec<Node>) {
        // println!("evaluate");
        self.builder().clear();
        self.evaluate_at(tree, None);
        self.builder()
            .wrap(BranchKind::Block, 0, Type::default(), 0);
        self.builder().drain_into(tree);
    }

    fn evaluate_at(&mut self, tree: &[Node], override_kind: Option<BranchKind>) -> Option<()> {
        let node = *tree.last()?;
        match node.nod {
            Nod::Branch { kind, range, .. } => {
                let start = self.builder().pos();
                let range: Range<usize> = range.into();
                if kind == BranchKind::Call {
                    if let Nod::Uid { module, num, .. } = tree[range.start].nod {
                        if self.is_core(module) {
                            let core = self.cart.core_exports;
                            match () {
                                _ if num == core.struct_fun.num => {
                                    // TODO Similar for `class` or `pact` later.
                                    self.evaluate_struct_def_call(tree);
                                    return Some(());
                                }
                                _ => {}
                            }
                        }
                    }
                }
                let kind = override_kind.unwrap_or(kind);
                match kind {
                    BranchKind::Struct => {
                        for kid_index in range.clone() {
                            let kid_kind = match tree[kid_index].nod {
                                Nod::Branch {
                                    kind: BranchKind::Def | BranchKind::Field,
                                    ..
                                } => Some(BranchKind::Field),
                                _ => None,
                            };
                            self.evaluate_at(&tree[..=kid_index], kid_kind);
                        }
                    }
                    _ => {
                        for kid_index in range.clone() {
                            self.evaluate_at(&tree[..=kid_index], None);
                        }
                    }
                }
                self.builder().wrap(kind, start, node.typ, node.source);
            }
            _ => self.builder().push(node),
        }
        Some(())
    }

    fn evaluate_struct_def_call(&mut self, tree: &[Node]) -> Option<()> {
        let node = *tree.last()?;
        let Nod::Branch { kind, range } = node.nod else {
            panic!()
        };
        let start = self.builder().pos();
        let range: Range<usize> = range.into();
        match range.len() {
            2 => {
                self.evaluate_at(&tree[..=range.start], None);
                if let Nod::Branch {
                    kind: BranchKind::Struct,
                    ..
                } = tree[range.start + 1].nod
                {
                    // All this effort just to replace Struct with StructDef.
                    self.evaluate_at(&tree[..=range.start + 1], Some(BranchKind::StructDef));
                } else {
                    self.evaluate_at(&tree[..=range.start + 1], None);
                }
            }
            _ => {
                for kid_index in range.clone() {
                    self.evaluate_at(&tree[..=kid_index], None);
                }
            }
        }
        self.builder().wrap(kind, start, node.typ, node.source);
        Some(())
    }

    pub fn is_core(&self, module: u16) -> bool {
        module == 1 || self.module == 1
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
        // println!("\n--- Resolving ---\n");
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
                match kind {
                    BranchKind::Def => {
                        let entry = scope_entry(&tree[..=range.start]);
                        // Technically, don't need letrec for tops/classes.
                        // TODO Which is less surprising/confusing/annoying?
                        // let is_fun = matches!(
                        //     tree[range.end - 1].nod,
                        //     Nod::Branch {
                        //         kind: BranchKind::Fun,
                        //         ..
                        //     }
                        // );
                        // Add scope def either before or after resolving kids depending on simple fun def.
                        // This is currently the "let vs let rec" of Rio.
                        // if is_fun {
                        //     if let Some(entry) = entry {
                        //         self.scope.push(entry);
                        //     }
                        // }
                        for kid_index in range.clone() {
                            self.resolve_at(&tree[..=kid_index]);
                        }
                        if let Some(entry) = entry {
                            self.scope.push(entry);
                        }
                    }
                    BranchKind::Dot => {
                        for kid_index in range.clone() {
                            if kid_index == range.end - 1 {
                                // Resolving dot targets is different.
                                // But only bother with special treatment for simple leaves.
                                let kid = tree[kid_index];
                                if let Nod::Leaf { .. } = kid.nod {
                                    self.builder().push(kid);
                                    continue;
                                }
                            }
                            self.resolve_at(&tree[..=kid_index]);
                        }
                    }
                    _ => {
                        if matches!(kind, BranchKind::Def) {
                            if let Some(entry) = scope_entry(&tree[..=range.start]) {
                                self.scope.push(entry);
                            }
                        }
                        for kid_index in range.clone() {
                            self.resolve_at(&tree[..=kid_index]);
                        }
                    }
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
                // println!(
                //     "Checking {:?} @{}",
                //     &self.cart.interner[intern], node.source.0
                // );
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
            // println!(
            //     "   Check {:?} at {}/{}",
            //     &self.cart.interner[entry.intern], entry.module, entry.num
            // );
            if entry.intern == intern {
                // println!("   Found in scope!");
                // TODO Check overloads! Would need more context specs.
                return Some(*entry);
            }
        }
        if let Some(entry) = self.tops.get(intern).next() {
            // println!("   Found at top!");
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
