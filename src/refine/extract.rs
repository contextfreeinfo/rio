use crate::{
    Cart, impl_tree_builder_wrap,
    lex::TokenKind,
    norm::{
        Block, Call, Def, Dot, Fun, Node, NodeData, NodeStepper, Public, Structured, Typed, Uid,
    },
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
        self.convert_def_ids();
        self.update_defs();
        self.update_tops();
    }

    pub fn update_defs(&mut self) {
        self.update_defs_at(TreeBuilder::top_of(&self.cart.tree), 0);
        // dbg!(&self.cart.defs);
    }

    pub fn update_tops(&mut self) {
        let Node::Block(top) = self.top() else {
            panic!()
        };
        self.cart.tops.clear();
        let mut stepper = NodeStepper::new(top.kids);
        while let Some(kid) = stepper.next(&self.cart.tree) {
            let Some(uid) = Node::uid(&self.cart.tree, kid) else {
                continue;
            };
            // TODO Report error if duplicates.
            self.cart.tops.insert(uid.intern, uid.num);
        }
        // dbg!(&self.cart.tops);
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

    fn top(&self) -> Node {
        self.read(TreeBuilder::top_of(&self.cart.tree))
    }

    impl_tree_builder_wrap!();

    fn wrap_one<F: FnOnce(&mut Self)>(&mut self, build: F) -> usize {
        self.wrap(build).start
    }

    // Convert ids

    fn convert_def_ids(&mut self) {
        // Avoid zeros.
        if self.cart.defs.is_empty() {
            self.cart.defs.push(Default::default());
        }
        // Convert def ids to uids.
        self.builder().clear();
        let top = TreeBuilder::top_of(&self.cart.tree);
        let top = self
            .wrap(|s| s.convert_def_ids_at(s.read(top), false))
            .start;
        self.cart.tree_builder.drain_into(&mut self.cart.tree, top);
    }

    fn convert_def_ids_at(&mut self, node: Node, def: bool) {
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
        // TODO Reduce boilerplate.
        match node {
            Node::Block(block) => {
                let kids = self.wrap(|s| {
                    let mut stepper = NodeStepper::new(block.kids);
                    while let Some(kid) = stepper.next(&s.cart.tree) {
                        s.convert_def_ids_at(kid, false);
                    }
                });
                self.push(Block { kids, ..block });
            }
            Node::Call(call) => {
                let fun = self.wrap_one(|s| s.convert_def_ids_at(s.read(call.fun), false));
                let args = self.wrap(|s| {
                    let mut stepper = NodeStepper::new(call.args);
                    while let Some(arg) = stepper.next(&s.cart.tree) {
                        s.convert_def_ids_at(arg, false);
                    }
                });
                self.push(Call { fun, args, ..call });
            }
            Node::Dot(dot) => {
                // New def just for target.
                let scope = self.wrap_one(|s| s.convert_def_ids_at(s.read(dot.scope), true));
                let member = self.wrap_one(|s| s.convert_def_ids_at(s.read(dot.member), false));
                self.push(Dot {
                    scope,
                    member,
                    ..dot
                });
            }
            Node::Def(def) => {
                // New def just for target.
                // TODO Special-case target for structured!
                let target = self.wrap_one(|s| s.convert_def_ids_at(s.read(def.target), true));
                let value = self.wrap_one(|s| s.convert_def_ids_at(s.read(def.value), false));
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
                        s.convert_def_ids_at(param, false);
                    }
                });
                let returning =
                    self.wrap_one(|s| s.convert_def_ids_at(s.read(fun.returning), false));
                let body = self.wrap_one(|s| s.convert_def_ids_at(s.read(fun.body), false));
                self.push(Fun {
                    params,
                    returning,
                    body,
                    ..fun
                });
            }
            Node::Public(public) => {
                // Forward def.
                let kid = self.wrap_one(|s| s.convert_def_ids_at(s.read(public.kid), def));
                self.push(Public { kid, ..public });
            }
            Node::Structured(structured) => {
                let defs = self.wrap(|s| {
                    let mut stepper = NodeStepper::new(structured.defs);
                    while let Some(def) = stepper.next(&s.cart.tree) {
                        s.convert_def_ids_at(def, false);
                    }
                });
                self.push(Structured { defs, ..structured });
            }
            Node::Typed(typed) => {
                // Forward def just for target.
                let target = self.wrap_one(|s| s.convert_def_ids_at(s.read(typed.target), def));
                let typ = self.wrap_one(|s| s.convert_def_ids_at(s.read(typed.typ), false));
                self.push(Typed {
                    target,
                    typ,
                    ..typed
                });
            }
            Node::None | Node::Tok(_) | Node::Uid(_) => self.push_node(node),
        }
    }

    // Update defs

    pub fn update_defs_at(&mut self, idx: usize, def: usize) {
        let update_defs_at_range = |s: &mut Self, range: SizeRange| {
            let mut stepper = NodeStepper::new(range);
            while let Some((_, idx)) = stepper.next_idx(&s.cart.tree) {
                s.update_defs_at(idx, 0);
            }
        };
        match self.read(idx) {
            Node::Block(block) => update_defs_at_range(self, block.kids),
            Node::Call(call) => {
                self.update_defs_at(call.fun, 0);
                update_defs_at_range(self, call.args);
            }
            Node::Def(def) => {
                self.update_defs_at(def.target, idx);
                self.update_defs_at(def.value, 0);
            }
            Node::Dot(dot) => {
                self.update_defs_at(dot.scope, idx);
                self.update_defs_at(dot.member, 0);
            }
            Node::Fun(fun) => {
                update_defs_at_range(self, fun.params);
                self.update_defs_at(fun.returning, 0);
                self.update_defs_at(fun.body, 0);
            }
            Node::Public(public) => self.update_defs_at(public.kid, def),
            Node::Structured(structured) => update_defs_at_range(self, structured.defs),
            Node::Typed(typed) => {
                self.update_defs_at(typed.target, def);
                self.update_defs_at(typed.typ, 0);
            }
            Node::Uid(uid) => {
                if def != 0 {
                    self.cart.defs[uid.num] = def;
                }
            }
            Node::None | Node::Tok(_) => {}
        }
    }
}
