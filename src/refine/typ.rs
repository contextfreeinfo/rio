use serde::{Deserialize, Serialize};

use crate::{
    impl_tree_builder_wrap, lex::{Intern, TokenKind}, norm::{
        Block, Call, Def, Dot, Fun, Node, NodeData, NodeStepper, Public, Structured, Typed, Uid,
    }, tree::{SizeRange, TreeBuilder}, Cart
};

use super::resolve::UidInfo;

// TODO Just values instead of types?

#[derive(Clone, Debug, Default, Deserialize, Eq, Hash, PartialEq, Serialize)]
pub enum Typ {
    #[default]
    None,
    Simple(UidInfo),
    Function(FunctionTyp),
    Bound(BoundTyp),
    Generic(GenericTyp),
}

#[derive(Clone, Debug, Default, Deserialize, Eq, Hash, PartialEq, Serialize)]
pub enum GenericBase {
    #[default]
    None,
    Simple(UidInfo),
    /// For partial application. Needed?
    Bound(Box<BoundTyp>),
}

#[derive(Clone, Debug, Default, Deserialize, Eq, Hash, PartialEq, Serialize)]
pub enum TypArg {
    #[default]
    None,
    Typ(Typ),
    UInt(usize),
}

#[derive(Clone, Debug, Default, Deserialize, Eq, Hash, PartialEq, Serialize)]
pub enum TypParamKind {
    #[default]
    None,
    /// The arg is for the bound, allowing None.
    Typ(Typ),
    UInt,
}

#[derive(Clone, Debug, Default, Deserialize, Eq, Hash, PartialEq, Serialize)]
pub struct FunctionTyp {
    params: Vec<Typ>,
    returning: Box<Typ>,
}

#[derive(Clone, Debug, Default, Deserialize, Eq, Hash, PartialEq, Serialize)]
pub struct TypParam {
    /// For messaging only.
    id: Intern,
    kind: TypParamKind,
}

#[derive(Clone, Debug, Default, Deserialize, Eq, Hash, PartialEq, Serialize)]
pub struct BoundTyp {
    typ: GenericTyp,
    args: Vec<TypArg>,
}

#[derive(Clone, Debug, Default, Deserialize, Eq, Hash, PartialEq, Serialize)]
pub struct GenericTyp {
    typ: GenericBase,
    params: Vec<TypParam>,
}

pub struct Typer<'a> {
    pub cart: &'a mut Cart,
}

impl<'a> Typer<'a> {
    pub fn new(cart: &'a mut Cart) -> Self {
        Self { cart }
    }

    pub fn typ(&mut self) {
        self.builder().clear();
        self.cart.scope.clear();
        let top = self
            .wrap(|s| s.resolve_at(TreeBuilder::top_of(&s.cart.tree), 0, false))
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

    impl_tree_builder_wrap!();

    fn wrap_one<F: FnOnce(&mut Self)>(&mut self, build: F) -> usize {
        self.wrap(build).start
    }

    // Resolve

    fn resolve_at(&mut self, idx: usize, depth: usize, structured: bool) {
        let node = self.read(idx);
        match node {
            Node::Block(block) => {
                let scope_start = self.cart.scope.len();
                let kids = self.wrap(|s| {
                    let mut stepper = NodeStepper::new(block.kids);
                    while let Some((_, idx)) = stepper.next_idx(&s.cart.tree) {
                        s.resolve_at(idx, depth + 1, false);
                    }
                });
                self.cart.scope.truncate(scope_start);
                self.push(Block { kids, ..block });
            }
            Node::Call(call) => {
                let fun = self.wrap_one(|s| s.resolve_at(call.fun, depth + 1, false));
                let args = self.wrap(|s| {
                    let mut stepper = NodeStepper::new(call.args);
                    while let Some((_, idx)) = stepper.next_idx(&s.cart.tree) {
                        s.resolve_at(idx, depth + 1, false);
                    }
                });
                self.push(Call { fun, args, ..call });
            }
            Node::Def(def) => {
                if depth > 1 && !structured {
                    if let Some(uid) = Node::uid(&self.cart.tree, node) {
                        self.cart.scope.push(UidInfo {
                            intern: uid.intern,
                            module: uid.module,
                            num: uid.num,
                        });
                    }
                }
                let target = self.wrap_one(|s| s.resolve_at(def.target, depth + 1, false));
                let value = self.wrap_one(|s| s.resolve_at(def.value, depth + 1, false));
                self.push(Def {
                    target,
                    value,
                    ..def
                });
            }
            Node::Dot(dot) => {
                let scope = self.wrap_one(|s| s.resolve_at(dot.scope, depth + 1, false));
                let member = match self.read(dot.member) {
                    // TODO Special-case member resolution.
                    tok @ Node::Tok(_) => self.wrap_one(|s| s.push_node(tok)),
                    _ => self.wrap_one(|s| s.resolve_at(dot.member, depth + 1, false)),
                };
                self.push(Dot {
                    scope,
                    member,
                    ..dot
                });
            }
            Node::Fun(fun) => {
                let params = self.wrap(|s| {
                    let mut stepper = NodeStepper::new(fun.params);
                    while let Some((_, idx)) = stepper.next_idx(&s.cart.tree) {
                        s.resolve_at(idx, depth + 1, false);
                    }
                });
                let returning = self.wrap_one(|s| s.resolve_at(fun.returning, depth + 1, false));
                let body = self.wrap_one(|s| s.resolve_at(fun.body, depth + 1, false));
                self.push(Fun {
                    params,
                    returning,
                    body,
                    ..fun
                });
            }
            Node::Public(public) => {
                let kid = self.wrap_one(|s| s.resolve_at(public.kid, depth + 1, false));
                self.push(Public { kid, ..public });
            }
            Node::Structured(structured) => {
                let defs = self.wrap(|s| {
                    let mut stepper = NodeStepper::new(structured.defs);
                    while let Some((_, idx)) = stepper.next_idx(&s.cart.tree) {
                        // TODO Special-case structured definition.
                        s.resolve_at(idx, depth + 1, true);
                    }
                });
                self.push(Structured { defs, ..structured });
            }
            Node::Tok(tok) => {
                // Skip resolving if not an id.
                if tok.token.kind != TokenKind::Id {
                    // TODO Should this ever happen in normalized trees?
                    self.push_node(node);
                    return;
                }
                // Walk nested scope.
                for entry in self.cart.scope.iter().rev() {
                    if entry.intern == tok.token.intern {
                        let uid = Uid {
                            meta: tok.meta,
                            intern: entry.intern,
                            module: entry.module,
                            num: entry.num,
                        };
                        self.push(uid);
                        return;
                    }
                }
                // Check tops.
                if let Some(entry) = self.cart.tops.get(&tok.token.intern) {
                    let entry = self.read(self.cart.defs[*entry]);
                    if let Some(entry) = Node::uid(&self.cart.tree, entry) {
                        let uid = Uid {
                            meta: tok.meta,
                            ..entry
                        };
                        self.push(uid);
                        return;
                    }
                }
                // If we didn't find anything, just keep the token.
                self.push_node(node);
            }
            Node::Typed(typed) => {
                let target = self.wrap_one(|s| s.resolve_at(typed.target, depth + 1, false));
                let typ = self.wrap_one(|s| s.resolve_at(typed.typ, depth + 1, false));
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
