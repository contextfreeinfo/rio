use crate::{
    Cart,
    lex::TokenKind,
    norm::{Block, Call, Def, Fun, Node, NodeData, NodeStepper, Public, Structured, Typed, Uid},
    tree::{SizeRange, TreeBuilder},
};

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
        // Convert def ids to uids.
        self.builder().clear();
        let top = TreeBuilder::top_of(&self.cart.tree);
        let top = self.wrap(|s| s.convert_def_ids(s.read(top), false)).start;
        self.cart.tree_builder.drain_into(&mut self.cart.tree, top);
        // Extract defs.
        self.extract_defs(self.read(top));
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

    // Extract indices

    fn extract_defs(&mut self, top: Node) {
        let Node::Block(top) = top else {
            return;
        };
        self.cart.tops.clear();
        let mut stepper = NodeStepper::new(top.kids);
        while let Some((kid, idx)) = stepper.next_idx(&self.cart.tree) {
            let Some(uid) = Node::uid(&self.cart.tree, kid) else {
                continue;
            };
            // TODO Report error if duplicate values
            self.cart.tops.insert(uid.intern, uid.num);
            self.cart.defs[uid.num] = idx;
        }
        dbg!(&self.cart.tops);
        // TODO Go deep through tree.
        dbg!(&self.cart.defs);
    }
}
