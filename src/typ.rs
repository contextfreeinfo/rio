use std::ops::Range;

use crate::{
    lex::TokenKind,
    run::Runner,
    tree::{BranchKind, Nod, Node, TreeBuilder, Type},
};

pub struct Typer {
    pub any_change: bool,
    pub type_refs: Vec<Type>,
    pub types: TreeBuilder,
}

impl Typer {
    pub fn new() -> Self {
        Self {
            any_change: false,
            type_refs: vec![],
            types: TreeBuilder::default(),
        }
    }

    fn ingest_type(&mut self, other: &[Node], typ: Type) -> Type {
        // TODO Or make a node for foreign reference?
        // TODO At least fix type refs, so simple copy probably no good.
        if typ.0 == 0 {
            return typ;
        }
        let node = other[typ.0 as usize - 1];
        let new_ref = match node.typ.0 {
            0 => node.typ,
            _ => self.ingest_type(other, node.typ),
        };
        match node.nod {
            Nod::Branch {
                kind: BranchKind::None,
                ..
            } => {
                // All we need is the ref ingestion from above.
            }
            Nod::Branch { kind, range } => {
                let range: Range<usize> = range.into();
                let ref_start = self.type_refs.len();
                for kid_index in range.clone() {
                    let kid_type = self.ingest_type(other, Type(kid_index as u32 + 1));
                    self.type_refs.push(kid_type);
                }
                let start = self.types.pos();
                self.push_type_refs(ref_start);
                self.types.wrap(kind, start, new_ref, 0);
            }
            Nod::Uid { .. } => {
                self.types.push(Node {
                    typ: new_ref,
                    ..node
                });
            }
            _ => {
                self.types.push_tree(&other[..typ.0 as usize - 1]);
            }
        }
        Type(self.types.pos())
    }

    fn push_type_refs(&mut self, ref_start: usize) {
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
    }
}

pub fn type_tree(runner: &mut Runner, tree: &mut [Node]) {
    if !tree.is_empty() {
        let end = tree.len() - 1;
        // I've seen it need 3 to percolate some things back and forth.
        for _ in 0..3 {
            runner.any_change = false;
            // Keep full tree for typing or eval so we can reference anywhere.
            type_any(runner, tree, end, Type(0));
            if !runner.any_change {
                break;
            }
        }
    }
}

pub fn append_types(runner: &mut Runner, tree: &mut Vec<Node>) {
    runner.builder().nodes.extend_from_slice(&tree);
    runner.builder().pop();
    runner.builder().pop();
    // Double-wrap to push a block.
    runner.typer.types.wrap(BranchKind::Types, 0, Type(0), 0);
    runner.typer.types.wrap(BranchKind::Types, 0, Type(0), 0);
    runner
        .cart
        .tree_builder
        .push_tree(&runner.typer.types.nodes);
    // Find new types offset in tree.
    let Nod::Branch { range, .. } = runner.builder().working.last().unwrap().nod else { panic!() };
    let types_offset = range.start;
    runner.builder().wrap(BranchKind::Block, 0, Type(0), 0);
    runner.builder().wrap(BranchKind::Block, 0, Type(0), 0);
    runner.builder().drain_into(tree);
    // Now point directly to tree position.
    for node in tree {
        if node.typ.0 != 0 {
            node.typ.0 += types_offset;
        }
    }
}

fn build_type(runner: &mut Runner, tree: &[Node]) -> Option<Type> {
    let node = tree.last().unwrap();
    match node.nod {
        // TODO Complex types.
        Nod::Uid { .. } => {
            // TODO Try to look up existing type after pushing.
            if node.typ.0 == 0 {
                runner.typer.types.push(*node);
                Some(Type(runner.typer.types.pos()))
            } else {
                Some(node.typ)
            }
        }
        _ => None,
    }
}

fn set_type(runner: &mut Runner, node: &mut Node, typ: Type) {
    if node.typ != typ && typ.0 != 0 {
        // if node.typ.0 != 0 {
        //     println!("aha {node:?}: {:?} -> {:?}", node.typ, typ);
        // }
        node.typ = typ;
        runner.any_change = true;
    }
}

fn type_any(runner: &mut Runner, tree: &mut [Node], at: usize, typ: Type) -> Type {
    let node = tree[at];
    let mut typ = typ.or(node.typ);
    match node.nod {
        Nod::Branch { kind, range } => {
            let range: Range<usize> = range.into();
            let handled = match kind {
                BranchKind::Block => {
                    typ = type_block(runner, tree, &range, typ);
                    true
                }
                BranchKind::Call => {
                    typ = type_call(runner, tree, &range, typ);
                    true
                }
                BranchKind::Def => {
                    typ = type_def(runner, tree, &range, typ);
                    true
                }
                BranchKind::Fun => {
                    typ = type_fun(runner, tree, at, typ);
                    true
                }
                _ => false,
            };
            // TODO Eventually, should we handle everything above?
            if !handled {
                for kid_index in range.clone() {
                    type_any(runner, tree, kid_index, Type::default());
                }
            }
        }
        Nod::Leaf { token } => match token.kind {
            TokenKind::String => {
                if typ.0 == 0 {
                    typ = build_type(runner, &[runner.cart.core_exports.text_type.into()]).unwrap();
                }
            }
            _ => {}
        },
        Nod::Uid { module, num, .. } => {
            if module == 0 || module == runner.module {
                // Dig from this module using indirect indices.
                let def_typ = tree[runner.def_indices[num as usize].0 as usize - 1].typ;
                if def_typ.0 != 0 {
                    typ = def_typ;
                }
            } else if typ.0 == 0 {
                // Dig from other modules using direct indices.
                let other = &runner.cart.modules[module as usize - 1];
                let other_node = other.tree[num as usize - 1];
                let def_typ = other_node.typ;
                if def_typ.0 != 0 {
                    // Copy type into our types.
                    runner.typer.ingest_type(&other.tree, def_typ);
                    typ = Type(runner.typer.types.pos());
                }
            }
        }
        _ => {}
    }
    // Assign on existing structure lets us go in arbitrary order easier.
    set_type(runner, &mut tree[at], typ);
    typ
}

fn type_block(runner: &mut Runner, tree: &mut [Node], range: &Range<usize>, mut typ: Type) -> Type {
    // Block type is last type.
    for kid_index in range.clone() {
        let last = kid_index == range.end - 1;
        let expected = if last { typ } else { Type::default() };
        let found = type_any(runner, tree, kid_index, expected);
        if last {
            typ = found;
        }
    }
    typ
}

fn type_call(runner: &mut Runner, tree: &mut [Node], range: &Range<usize>, mut typ: Type) -> Type {
    for (local_index, kid_index) in range.clone().enumerate() {
        let kid = tree[kid_index];
        if local_index == 0 && kid.typ.0 != 0 {
            // TODO Also grab all expected param types for later kids.
            let return_type = runner.typer.types.working[kid.typ.0 as usize - 1].typ;
            if return_type.0 != 0 {
                typ = return_type;
            }
        }
        type_any(runner, tree, kid_index, Type::default());
    }
    typ
}

fn type_def(runner: &mut Runner, tree: &mut [Node], range: &Range<usize>, mut typ: Type) -> Type {
    let start = range.start;
    let xtype = tree[start + 1];
    let value = tree[start + 2];
    if typ.0 == 0 {
        // Prioritize previously evaluated types, first from the explicit
        // type.
        typ = xtype.typ.or(value.typ);
        if typ.0 == 0 {
            // Failing that, interpret the explicit type from the tree.
            if let Some(built_typ) = build_type(runner, &tree[..=start + 1]) {
                typ = built_typ;
            }
        }
    }
    // There should always be exactly 3 kids of defs: id, xtype, value.
    // Check value first in case we want the type from it.
    type_any(runner, tree, start + 2, typ);
    if typ.0 == 0 {
        // We did't have type yet, so try the value's type.
        typ = tree[start + 2].typ;
    }
    type_any(runner, tree, start, typ);
    // TODO Look up Type type.
    type_any(runner, tree, start + 1, Type::default());
    typ
}

fn type_fun(runner: &mut Runner, tree: &mut [Node], at: usize, typ: Type) -> Type {
    let node = tree[at];
    let Nod::Branch { kind: BranchKind::Fun, range } = node.nod else { panic!() };
    // Kids
    let range: Range<usize> = range.into();
    // Should always be in-params, out(-params), body.
    let Range { start, .. } = range;
    // Params
    let Nod::Branch { range: params_range, .. } = tree[start].nod else { panic!() };
    let params_range: Range<usize> = params_range.into();
    let ref_start = runner.typer.type_refs.len();
    let mut any_change = false;
    for param_index in params_range.clone() {
        let old_type = tree[param_index].typ;
        let param_type = type_any(runner, tree, param_index, Type::default());
        runner.typer.type_refs.push(param_type);
        any_change |= param_type != old_type;
    }
    // Return
    let Nod::Branch { range: out_range, .. } = tree[start + 1].nod else { panic!() };
    let out_range: Range<usize> = out_range.into();
    let old_out_type = if node.typ.0 == 0 {
        Type(0)
    } else {
        runner.typer.types.working[node.typ.0 as usize - 1].typ
    };
    let mut out_type = if out_range.is_empty() {
        old_out_type.or(
            // See if we're expecting a particular return type.
            if typ.0 != 0 {
                runner.typer.types.working[typ.0 as usize - 1].typ
            } else {
                Type(0)
            },
        )
    } else {
        type_any(runner, tree, out_range.start, old_out_type)
    };
    // Body
    if range.len() > 2 {
        // TODO If we have an out type that was missing parts that the new one has, use the new one.
        let body_type = type_any(runner, tree, start + 2, out_type.or(old_out_type));
        if out_type.0 == 0 && (out_range.is_empty() || tree[out_range.start].typ.0 == 0) {
            // Infer return type from body.
            if !out_range.is_empty() {
                set_type(runner, &mut tree[out_range.start], body_type);
            }
            out_type = body_type;
        }
    } else if out_type.0 == 0 {
        // Infer void for empty body. TODO Infer by context with default return value?
        out_type = match old_out_type.0 {
            0 => build_type(runner, &[runner.cart.core_exports.void_type.into()]).unwrap(),
            _ => old_out_type,
        }
    }
    any_change |= out_type != old_out_type;
    if node.typ.0 == 0 || any_change {
        // After digging subtrees, we're ready to push type refs.
        let types_start = runner.typer.types.pos();
        runner.typer.push_type_refs(ref_start);
        // Function
        runner
            .typer
            .types
            .wrap(BranchKind::FunType, types_start, out_type, 0);
        Type(runner.typer.types.pos())
    } else {
        runner.typer.type_refs.drain(ref_start..);
        node.typ
    }
}
