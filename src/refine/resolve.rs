use serde::{Deserialize, Serialize};

use crate::{
    Cart,
    lex::Intern,
    norm::{Block, Call, Def, Fun, Node, NodeData, NodeStepper, Public, Structured, Typed},
    tree::{SizeRange, TreeBuilder},
};

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct UidInfo {
    pub intern: Intern,
    pub module: usize,
    pub num: usize,
}

pub struct Resolver<'a> {
    pub cart: &'a mut Cart,
}

impl<'a> Resolver<'a> {
    pub fn new(cart: &'a mut Cart) -> Self {
        Self { cart }
    }

    pub fn resolve(&mut self) {
        self.builder().clear();
        self.cart.scope.clear();
        let top = self
            .wrap(|s| s.resolve_at(TreeBuilder::top_of(&s.cart.tree)))
            .start;
        self.cart.tree_builder.drain_into(&mut self.cart.tree, top);
    }

    // General helpers
    // TODO Move these to Cart or some central helper?

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

    // TODO Especially move this one to a common place, since it's not just a one-liner.
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

    // Resolve

    fn resolve_at(&mut self, idx: usize) {
        let node = self.read(idx);
        match node {
            Node::Block(block) => {
                let kids = self.wrap(|s| {
                    let mut stepper = NodeStepper::new(block.kids);
                    while let Some((_, idx)) = stepper.next_idx(&s.cart.tree) {
                        s.resolve_at(idx);
                    }
                });
                self.push(Block { kids, ..block });
            }
            Node::Call(call) => {
                let fun = self.wrap_one(|s| s.resolve_at(call.fun));
                let args = self.wrap(|s| {
                    let mut stepper = NodeStepper::new(call.args);
                    while let Some((_, idx)) = stepper.next_idx(&s.cart.tree) {
                        s.resolve_at(idx);
                    }
                });
                self.push(Call { fun, args, ..call });
            }
            Node::Def(def) => {
                // New def just for target.
                let target = self.wrap_one(|s| s.resolve_at(def.target));
                let value = self.wrap_one(|s| s.resolve_at(def.value));
                self.push(Def {
                    target,
                    value,
                    ..def
                });
            }
            Node::Fun(fun) => {
                let params = self.wrap(|s| {
                    let mut stepper = NodeStepper::new(fun.params);
                    while let Some((_, idx)) = stepper.next_idx(&s.cart.tree) {
                        s.resolve_at(idx);
                    }
                });
                let returning = self.wrap_one(|s| s.resolve_at(fun.returning));
                let body = self.wrap_one(|s| s.resolve_at(fun.body));
                self.push(Fun {
                    params,
                    returning,
                    body,
                    ..fun
                });
            }
            Node::Public(public) => {
                // Forward def.
                let kid = self.wrap_one(|s| s.resolve_at(public.kid));
                self.push(Public { kid, ..public });
            }
            Node::Structured(structured) => {
                let defs = self.wrap(|s| {
                    let mut stepper = NodeStepper::new(structured.defs);
                    while let Some((_, idx)) = stepper.next_idx(&s.cart.tree) {
                        s.resolve_at(idx);
                    }
                });
                self.push(Structured { defs, ..structured });
            }
            Node::Tok(_) => self.push_node(node),
            Node::Typed(typed) => {
                // Forward def just for target.
                let target = self.wrap_one(|s| s.resolve_at(typed.target));
                let typ = self.wrap_one(|s| s.resolve_at(typed.typ));
                self.push(Typed {
                    target,
                    typ,
                    ..typed
                });
            }
            Node::None | Node::Uid(_) => self.push_node(node),
        }
    }
}
