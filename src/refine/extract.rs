use std::{collections::HashMap, hash::Hash, mem::replace};

use crate::{
    Cart,
    lex::TokenKind,
    norm::{
        Block, Call, Def, Fun, Node, NodeData, NodeMeta, NodeStepper, Public, Structured, Typed,
        Uid,
    },
    tree::{SizeRange, TreeBuilder},
};

/// While inserted, the index shouldn't change, which is an index into the
/// values vec, even when new values are assigned.
#[derive(Debug, Default)]
pub struct IdxMap<K, V> {
    pub map: HashMap<K, usize>,
    pub values: Vec<V>,
}

impl<K, V> IdxMap<K, V> {
    pub fn get(&mut self, key: &K) -> Option<&V>
    where
        K: Eq + Hash,
    {
        self.get_idx(key).map(|idx| &self.values[idx])
    }

    pub fn get_idx(&mut self, key: &K) -> Option<usize>
    where
        K: Eq + Hash,
    {
        self.map.get(key).copied()
    }

    /// Returns the index and the old value if any.
    pub fn insert(&mut self, key: K, value: V) -> (usize, Option<V>)
    where
        K: Eq + Hash,
    {
        match self.map.get(&key) {
            Some(idx) => {
                let old = replace(&mut self.values[*idx], value);
                (*idx, Some(old))
            }
            None => {
                let idx = self.values.len();
                self.values.push(value);
                self.map.insert(key, idx);
                (idx, None)
            }
        }
    }
}

pub struct Extractor<'a> {
    pub cart: &'a mut Cart,
}

impl<'a> Extractor<'a> {
    pub fn new(cart: &'a mut Cart) -> Self {
        Self { cart }
    }

    pub fn extract(&mut self) {
        // Avoid zeros.
        if self.cart.defs.is_empty() {
            self.cart.defs.push(Default::default());
        }
        if self.cart.tops.values.is_empty() {
            self.cart.tops.values.push(Default::default());
        }
        // Convert def ids to uids.
        self.builder().clear();
        let top = TreeBuilder::top_of(&self.cart.tree);
        let top = self.wrap(|s| s.convert_def_ids(s.read(top), false)).start;
        self.cart.tree_builder.drain_into(&mut self.cart.tree, top);
        // Extract top defs.
        self.extract_tops(Node::read(&self.cart.tree, top).0);
    }

    // General helpers

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    fn push(&mut self, node: impl NodeData) {
        self.push_node(node.as_node());
    }

    fn push_node(&mut self, node: Node) {
        self.builder().push(node);
    }

    fn read(&self, idx: usize) -> Node {
        Node::read(&self.cart.tree, idx).0
    }

    fn wrap<F: FnOnce(&mut Self)>(&mut self, build: F) -> SizeRange {
        let start = self.builder().pos();
        build(self);
        let result = match () {
            _ if self.builder().pos() == start => SizeRange::default(),
            _ => {
                let range = self.builder().apply_range(start);
                range
            }
        };
        result
    }

    fn wrap_one<F: FnOnce(&mut Self)>(&mut self, build: F) -> usize {
        self.wrap(build).start
    }

    // Convert ids

    fn convert_def_ids(&mut self, node: Node, def: bool) {
        if let Node::Tok(tok) = node {
            if def && tok.token.kind == TokenKind::Id {
                let uid = Uid {
                    meta: tok.meta,
                    intern: tok.token.intern,
                    module: 0, // TODO
                    num: self.cart.defs.len(),
                };
                self.push(uid);
                // Figure out idx later.
                self.cart.defs.push(0);
                return;
            }
        }
        // TODO Reduce boilerplate here.
        match node {
            Node::Block(block) => {
                let kids = self.wrap(|s| {
                    let mut stepper = NodeStepper::new(block.kids);
                    while let Some(kid) = stepper.next(&s.cart.tree) {
                        s.convert_def_ids(kid, false);
                    }
                });
                self.push(Block { kids, ..block });
            }
            Node::Call(call) => {
                let fun = self.wrap_one(|s| s.convert_def_ids(s.read(call.fun), false));
                let args = self.wrap(|s| {
                    let mut stepper = NodeStepper::new(call.args);
                    while let Some(arg) = stepper.next(&s.cart.tree) {
                        s.convert_def_ids(arg, false);
                    }
                });
                self.push(Call { fun, args, ..call });
            }
            Node::Def(def) => {
                // New def just for target.
                let target = self.wrap_one(|s| s.convert_def_ids(s.read(def.target), true));
                let value = self.wrap_one(|s| s.convert_def_ids(s.read(def.value), false));
                self.push(Def {
                    target,
                    value,
                    ..def
                });
            }
            Node::Fun(fun) => {
                let params = self.wrap(|s| {
                    let mut stepper = NodeStepper::new(fun.params);
                    while let Some(param) = stepper.next(&s.cart.tree) {
                        s.convert_def_ids(param, false);
                    }
                });
                let returning = self.wrap_one(|s| s.convert_def_ids(s.read(fun.returning), false));
                let body = self.wrap_one(|s| s.convert_def_ids(s.read(fun.body), false));
                self.push(Fun {
                    params,
                    returning,
                    body,
                    ..fun
                });
            }
            Node::Public(public) => {
                // Forward def.
                let kid = self.wrap_one(|s| s.convert_def_ids(s.read(public.kid), def));
                self.push(Public { kid, ..public });
            }
            Node::Structured(structured) => {
                let defs = self.wrap(|s| {
                    let mut stepper = NodeStepper::new(structured.defs);
                    while let Some(def) = stepper.next(&s.cart.tree) {
                        s.convert_def_ids(def, false);
                    }
                });
                self.push(Structured { defs, ..structured });
            }
            Node::Typed(typed) => {
                // Forward def just for target.
                let target = self.wrap_one(|s| s.convert_def_ids(s.read(typed.target), def));
                let typ = self.wrap_one(|s| s.convert_def_ids(s.read(typed.typ), false));
                self.push(Typed {
                    target,
                    typ,
                    ..typed
                });
            }
            Node::None | Node::Tok(_) | Node::Uid(_) => self.push_node(node),
        }
    }

    // Extract tops

    fn extract_tops(&mut self, top: Node) {
        let Node::Block(top) = top else {
            return;
        };
        // First gather all tops, clearing out old indices.
        for top in &mut self.cart.tops.values {
            *top = 0;
        }
        let mut stepper = NodeStepper::new(top.kids);
        while let Some((kid, idx)) = stepper.next_idx(&self.cart.tree) {
            // TODO Recognize both uids and ids.
            // TODO Get source/type along with id?
            // TODO Rather just recurse with context until we get to id?
            let Some(id) = Node::id(&self.cart.tree, kid) else {
                continue;
            };
            // TODO Report error if old value nonzero.
            // TODO Store better def entries than just idx?
            // TODO Convert ids in separate pass.
            let num = self.cart.tops.insert(id, idx).0;
            let uid = Uid {
                meta: NodeMeta::at(0),
                intern: id,
                module: 0,
                num,
            };
            // dbg!(uid);
            // TODO Replace ids with uids.
        }
        // Then recurse.
        // dbg!(&self.cart.tops);
    }
}
