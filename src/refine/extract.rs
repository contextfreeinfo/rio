use std::{collections::HashMap, hash::Hash, mem::replace};

use crate::{
    Cart,
    norm::{Block, Node, NodeData, NodeMeta, NodeStepper},
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
        self.builder().clear();
        if self.cart.tops.values.is_empty() {
            // Avoid zeros.
            self.cart.tops.values.push(Default::default());
        }
        let top = TreeBuilder::top_of(&self.cart.tree);
        self.top(Node::read(&self.cart.tree, top).0);
        // Finish top and drain tree.
        // TODO For real.
        // let top = self.wrap(|s| s.top(source)).start;
        let placeholder = Block {
            meta: NodeMeta::at(0),
            kids: SizeRange { start: 0, end: 0 },
        };
        self.push(placeholder.as_node());
        let top = self.builder().applied.len();
        self.builder().apply_range(0);
        self.cart.tree_builder.drain_into(&mut self.cart.tree, top);
    }

    // General helpers

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    fn push(&mut self, node: Node) {
        self.builder().push(node);
    }

    fn top(&mut self, top: Node) {
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
            let Some(id) = Node::id(&self.cart.tree, kid) else {
                continue;
            };
            // TODO Report error if old value nonzero.
            // TODO Store better def entries than just idx?
            self.cart.tops.insert(id, idx);
            // TODO Replace ids with uids. In place?
        }
        // Then recurse.
        dbg!(&self.cart.tops);
    }
}
