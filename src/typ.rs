use std::{
    cell::{Ref, RefCell, RefMut},
    collections::HashMap,
    hash::Hash,
    ops::Range,
    rc::Rc,
};

use crate::{
    lex::{Intern, TokenKind},
    run::{CoreExports, Runner},
    tree::{tree_hash_with, BranchKind, Nod, Node, Source, TreeBuilder, Type},
};

pub struct Typer {
    any_change: bool,
    map: HashMap<TypeTree, Type>,
    type_refs: Vec<Type>,
    // Need Rc so we can also store refs inside the map keys.
    types: Rc<RefCell<TreeBuilder>>,
}

pub struct TypeTree {
    typ: Type,
    types: Rc<RefCell<TreeBuilder>>,
}

impl PartialEq for TypeTree {
    fn eq(&self, other: &Self) -> bool {
        let result = self
            .types
            .borrow()
            .working_tree_eq(self.typ.0 - 1, other.typ.0 - 1);
        // println!("eq: {result}");
        result
        // TypeTree::types_eq(&self.types.borrow(), self.typ, other.typ)
    }
}

impl Eq for TypeTree {}

impl Hash for TypeTree {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        // let h = self.types.borrow().working_tree_hash(self.typ.0 - 1);
        // println!("hash: {h} for {:?}", self.typ);
        let types = self.types.borrow();
        if let Some(node) = types.working.get(self.typ.0 as usize - 1) {
            tree_hash_with(state, node, &types.nodes);
        }
    }
}

impl Typer {
    pub fn new() -> Self {
        let types = Rc::new(RefCell::new(TreeBuilder::default()));
        Self {
            any_change: false,
            map: HashMap::new(),
            type_refs: vec![],
            types: types.clone(),
        }
    }

    pub fn clear(&mut self) {
        self.types.borrow_mut().clear();
    }

    fn types_ref(&self) -> Ref<'_, TreeBuilder> {
        self.types.borrow()
    }

    fn types_mut(&mut self) -> RefMut<'_, TreeBuilder> {
        self.types.borrow_mut()
    }

    fn ingest_type(&mut self, other: &[Node], typ: Type) -> Type {
        // Ingest from another module.
        // println!("Ingest {typ:?}");
        if typ.0 == 0 {
            return typ;
        }
        let node = other[typ.0 as usize - 1];
        let new_ref = match node.typ.0 {
            0 => node.typ,
            _ => self.ingest_type(other, node.typ),
        };
        let result = match node.nod {
            Nod::Branch {
                kind: BranchKind::None,
                ..
            } => {
                // All we need is the ref ingestion from above.
                new_ref
            }
            Nod::Branch { kind, range } => {
                let range: Range<usize> = range.into();
                let ref_start = self.type_refs.len();
                for kid_index in range.clone() {
                    let kid_type = self.ingest_type(other, Type(kid_index as u32 + 1));
                    // println!("  kid type: {kid_type:?}");
                    self.type_refs.push(kid_type);
                }
                let start = self.types_ref().pos();
                self.push_type_refs(ref_start);
                self.types_mut().wrap(kind, start, new_ref, 0);
                self.unify()
            }
            Nod::Uid { .. } => {
                self.types_mut().push(Node {
                    typ: new_ref,
                    ..node
                });
                self.unify()
            }
            _ => {
                self.types_mut().push_tree(&other[..typ.0 as usize - 1]);
                self.unify()
            }
        };
        // println!("/Ingest {typ:?} {result:?}");
        result
    }

    fn more_precise(&self, a: Type, b: Type) -> bool {
        a.0 != 0
            && (b.0 == 0
                // We store return types in this spot for fun types, and this
                // seems likely to be meaningful in a general sense.
                || (self.types_ref().working[a.0 as usize - 1].typ.0 != 0
                    && self.types_ref().working[b.0 as usize - 1].typ.0 == 0))
    }

    fn push_type_refs(&mut self, ref_start: usize) {
        for type_ref in self.type_refs.drain(ref_start..) {
            // println!("push type ref: {type_ref:?}");
            self.types.borrow_mut().push(Node {
                typ: type_ref,
                source: 0.into(),
                nod: Nod::Branch {
                    kind: BranchKind::None,
                    range: (0u32..0u32).into(),
                },
            });
            // Already shallow refs, so no need to unify.
        }
    }

    fn unify(&mut self) -> Type {
        // Ensure all sources are 0 for type nodes so we're consistent for
        // hashing and comparison.
        self.types_mut().working.last_mut().unwrap().source = Source(0);
        let typ = Type(self.types_ref().pos());
        let key = TypeTree {
            typ,
            types: self.types.clone(),
        };
        let typ = match self.map.get(&key).copied() {
            Some(typ) => {
                // Don't need that one anymore.
                self.types_mut().pop_working_tree();
                typ
            }
            None => {
                self.map.insert(key, typ);
                typ
            }
        };
        typ
    }
}

pub fn type_tree(runner: &mut Runner, tree: &mut [Node]) {
    // println!("\n\n--- Start typing ---\n");
    if !tree.is_empty() {
        let end = tree.len() - 1;
        // I've seen it need 4 or 5 to percolate some things back and forth.
        // So allow at least some bonus beyond that.
        for _i in 0..10 {
            // println!("Super loop {_i}");
            runner.typer.any_change = false;
            // Keep full tree for typing or eval so we can reference anywhere.
            type_any(runner, tree, end, Type(0));
            if !runner.typer.any_change {
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
    runner
        .typer
        .types_mut()
        .wrap(BranchKind::Types, 0, Type(0), 0);
    runner
        .typer
        .types_mut()
        .wrap(BranchKind::Types, 0, Type(0), 0);
    runner
        .cart
        .tree_builder
        .push_tree(&runner.typer.types_ref().nodes);
    // Find new types offset in tree.
    // TODO Also remove unused types. Needs an offset lookup vector?
    // TODO We can have unused types from looped type refinement.
    let Nod::Branch { range, .. } = runner.builder().working.last().unwrap().nod else {
        panic!()
    };
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
            let node = Node {
                // Only function types should have types on them so far.
                typ: Type(0),
                ..*node
            };
            runner.typer.types_mut().push(node);
            // Try to look up existing type after pushing.
            Some(runner.typer.unify())
        }
        _ => None,
    }
}

type NodeAccess<'a> = (&'a [Node], usize);

/// Return the tree and and the index in the tree pointed at by the ref, if it's
/// a Uid.
fn get_node_access<'a>(runner: &'a Runner, tree: &'a [Node], ref_: Node) -> Option<NodeAccess<'a>> {
    let Nod::Uid { module, num, .. } = ref_.nod else {
        return None;
    };
    let access = if module == 0 || module == runner.module {
        (
            tree.as_ref(),
            runner.def_indices[num as usize].0 as usize - 1,
        )
    } else {
        if num == 0 {
            return None;
        }
        (
            runner.cart.modules[module as usize - 1].tree.as_slice(),
            num as usize - 1,
        )
    };
    Some(access)
}

fn set_type(runner: &mut Runner, node: &mut Node, typ: Type) {
    if node.typ != typ && typ.0 != 0 {
        node.typ = typ;
        runner.typer.any_change = true;
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
                BranchKind::Def | BranchKind::Field => {
                    typ = type_def(runner, tree, &range, typ);
                    true
                }
                BranchKind::Dot => {
                    typ = type_dot(runner, tree, &range, typ);
                    true
                }
                BranchKind::Fun => {
                    typ = type_fun(runner, tree, at, typ);
                    true
                }
                BranchKind::Struct => {
                    // We don't use struct contents to infer struct type.
                    // TODO Should we?
                    // println!("type struct at: {at}");
                    type_struct(runner, tree, &range, typ);
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
        Nod::Int32 { .. } => {
            if typ.0 == 0 {
                typ = build_type(runner, &[runner.cart.core_exports.int32_type.into()]).unwrap();
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
                    // TODO Keep a map from uids to local types to save time?
                    typ = runner.typer.ingest_type(&other.tree, def_typ);
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
    let mut arg_types_range: Option<Range<usize>> = None;
    let mut next_kid_typ = Type::default();
    let mut type_branch = false;
    for (local_index, kid_index) in range.clone().enumerate() {
        let kid = tree[kid_index];
        if local_index == 0 {
            let CoreExports {
                branch_fun,
                type_type,
                ..
            } = runner.cart.core_exports;
            if kid.typ.0 != 0 {
                let callee_type = runner.typer.types_ref().working[kid.typ.0 as usize - 1];
                if let Nod::Branch {
                    kind: BranchKind::FunType,
                    range: fun_type_range,
                } = callee_type.nod
                {
                    arg_types_range = Some(fun_type_range.into());
                }
                let return_type = callee_type.typ;
                if return_type.0 != 0 {
                    typ = return_type;
                } else if let Nod::Uid { module, num, .. } = callee_type.nod {
                    if runner.is_core(module) && num == type_type.num {
                        if let Some(built_type) = build_type(runner, &tree[..=kid_index]) {
                            typ = built_type;
                            next_kid_typ = typ;
                            continue;
                        }
                    }
                }
            } else if let Nod::Uid { module, num, .. } = kid.nod {
                if runner.is_core(module) {
                    match () {
                        _ if num == branch_fun.num => {
                            // Special-case some until we process type args.
                            type_branch = true;
                        }
                        _ => {}
                    }
                }
            }
        }
        if next_kid_typ == Type::default() {
            // Use param types if we have those.
            if let Some(arg_types_range) = arg_types_range.clone() {
                if local_index - 1 < arg_types_range.len() {
                    let arg_type_index = arg_types_range.start + local_index - 1;
                    next_kid_typ = runner.typer.types_ref().nodes[arg_type_index].typ;
                }
            }
        }
        type_any(runner, tree, kid_index, next_kid_typ);
        next_kid_typ = Type::default();
    }
    if type_branch && range.len() == 2 {
        if let Nod::Branch {
            kind: BranchKind::List,
            range: cases_range,
        } = tree[range.start + 1].nod
        {
            // With kids typed, we should be able to see the type of the branch.
            // TODO Do we want this generally because of generics, or should we just be patient on rounds?
            let cases_range: Range<usize> = cases_range.into();
            if !cases_range.is_empty() {
                // Just go with the first case for now.
                // It ought to be a pair, but don't bother checking that for now.
                if let Nod::Branch {
                    range: case_range, ..
                } = tree[cases_range.start].nod
                {
                    let case_range: Range<usize> = case_range.into();
                    if case_range.len() > 1 {
                        // Then look at the last thing in each case.
                        typ = tree[case_range.end - 1].typ;
                    }
                }
            }
        }
    }
    typ
}

// fn is_none(nod: Nod) -> bool {
//     matches!(nod, Nod::Branch { kind: BranchKind::None, .. })
// }

fn type_def(runner: &mut Runner, tree: &mut [Node], range: &Range<usize>, mut typ: Type) -> Type {
    let start = range.start;
    // let xtype_given = !is_none(tree[start + 1].nod);
    let value = tree[start + 2];
    if typ.0 == 0 {
        // Prioritize previously evaluated types, first from the explicit
        // type.
        // TODO Is xtype typ always Type?
        typ = value.typ;
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
    let value = tree[start + 2];
    // TODO If typ is FunType but has nones where value type doesn't, also use value type!
    if runner.typer.more_precise(value.typ, typ) {
        // Keep the better type from value.
        typ = value.typ;
    }
    type_any(runner, tree, start, typ);
    if runner.typer.more_precise(typ, tree[start].typ) {
        tree[start].typ = typ;
    }
    // TODO Look up Type type.
    type_any(runner, tree, start + 1, Type::default());
    typ
}

fn type_dot(runner: &mut Runner, tree: &mut [Node], range: &Range<usize>, typ: Type) -> Type {
    // Just recurse until below is worked out.
    // TODO When should we actually be doing this?
    for kid_index in range.clone() {
        type_any(runner, tree, kid_index, Type::default());
    }
    // Validate that expected kids exist.
    if range.len() != 2 {
        return typ;
    }
    // Get the field id.
    let Nod::Leaf { token } = tree[range.start + 1].nod else {
        return typ;
    };
    // Get the def node.
    let Some((def_tree, def_index)) = find_id_type_node_access(runner, tree, tree[range.start].typ)
    else {
        return typ;
    };
    // println!(
    //     "dot: {:?} {:?} {:?}",
    //     tree[range.start].typ, struct_type.nod, token
    // );
    // TODO Modify leaf to Sid or Uid.
    let Some(uid) = find_field_def(runner, def_tree, def_index, token.intern) else {
        return typ;
    };
    tree[range.start + 1] = uid;
    uid.typ
}

fn find_id_type_node_access<'a>(
    runner: &'a Runner,
    tree: &'a [Node],
    struct_type_ref: Type,
) -> Option<NodeAccess<'a>> {
    let struct_type_index = struct_type_ref.0 as usize;
    if struct_type_index == 0 {
        return None;
    }
    let struct_type = runner.typer.types_ref().working[struct_type_index - 1];
    get_node_access(runner, tree, struct_type)
}

fn type_struct(runner: &mut Runner, tree: &mut [Node], range: &Range<usize>, typ: Type) -> Type {
    // Do simple inference for starters.
    // TODO Avoid simple if we work out name-correlated typing down below.
    for kid_index in range.clone() {
        type_any(runner, tree, kid_index, Type::default());
    }
    if typ.0 == 0 {
        return typ;
    }
    // Because of borrows, we can't safely modify our tree in place.
    // So gather these up for later use.
    // TODO Instead of Runner, pass in something else that has a reusable buffer?
    // TODO Instead module id and def_index for NodeAccess? Then no need to retain refs?
    let mut uids = Vec::<Option<Node>>::new();
    // Get the struct def node.
    let Some((def_tree, def_index)) = find_id_type_node_access(runner, tree, typ) else {
        // Loop the kids.
        return typ;
    };
    // println!("found struct");
    for kid_index in range.clone() {
        let mut uid: Option<Node> = None;
        // Use one-pass loop for local block break.
        loop {
            let Nod::Branch {
                kind: BranchKind::Field,
                range: kid_range,
            } = tree[kid_index].nod
            else {
                break;
            };
            let kid_range: Range<usize> = kid_range.into();
            let intern = match tree[kid_range.start].nod {
                Nod::Sid { .. } => {
                    // Already passed this way.
                    return typ;
                }
                Nod::Uid { intern, .. } => intern,
                // TODO Will they always be Uid by this point?
                // Nod::Leaf { token } => token.intern,
                _ => break,
            };
            // TODO Better than O(n^2).
            uid = find_field_def(runner, def_tree, def_index, intern);
            break;
        }
        uids.push(uid);
    }
    // println!("--> Aligning struct with kids starting at: {}", range.start);
    // Above borrow should be done, so redo the loop with known alignment.
    for (kid_index, uid) in range.clone().zip(uids) {
        let Some(uid) = uid else {
            continue;
        };
        let Nod::Branch {
            kind: BranchKind::Field,
            range: kid_range,
        } = tree[kid_index].nod
        else {
            panic!();
        };
        // if kid_index == 88 || kid_index == 89 {
        //     println!("    Whoa from {:?}", uid.nod);
        // }
        let kid_range: Range<usize> = kid_range.into();
        // Point the uid to original field uid.
        let instance_uid = &mut tree[kid_range.start];
        instance_uid.nod = uid.nod;
        instance_uid.typ = uid.typ;
        // Retype the kid based on the field type.
        if tree[kid_range.end - 1].typ.0 == 0 {
            type_any(runner, tree, kid_range.end - 1, uid.typ);
        }
    }
    typ
}

fn find_field_def(runner: &Runner, tree: &[Node], at: usize, key: Intern) -> Option<Node> {
    let Nod::Branch {
        kind: BranchKind::Def,
        range,
    } = tree[at].nod
    else {
        return None;
    };
    // TODO Validate elsewhere that all Def ranges have len == 3?
    let Nod::Branch {
        kind: BranchKind::Call,
        range,
    } = tree[range.end as usize - 1].nod
    else {
        return None;
    };
    // Expect a simple call to struct.
    if range.len() != 2 {
        return None;
    }
    // TODO Defer call to struct as a validation thing?
    let Nod::Uid { module, num, .. } = tree[range.start as usize].nod else {
        return None;
    };
    let CoreExports { struct_fun, .. } = runner.cart.core_exports;
    if !(module == struct_fun.module && num == struct_fun.num) {
        return None;
    }
    // TODO Also support classes?
    // println!("struct call: {:?}", tree[range.start as usize + 1].nod);
    let Nod::Branch {
        kind: BranchKind::StructDef,
        range,
    } = tree[range.start as usize + 1].nod
    else {
        return None;
    };
    // Loop kid defs.
    let range: Range<usize> = range.into();
    for (local_index, kid_index) in range.clone().enumerate() {
        let _ = local_index;
        // TODO Use local_index for Sid?
        // TODO Change to Uid for finished modules?
        // TODO How do Uids work for local var references already?
        // TODO Just try Uid to start with?
        let Nod::Branch {
            kind: BranchKind::Def,
            range: field_range,
        } = tree[kid_index].nod
        else {
            continue;
        };
        let field_id = tree[field_range.start as usize];
        let Nod::Uid { intern, .. } = field_id.nod else {
            continue;
        };
        if intern == key {
            // println!("  found field: {:?} {:?}", intern, tree[kid_index].typ);
            return Some(Node {
                nod: Nod::Sid {
                    intern,
                    num: local_index as u32,
                },
                source: Source::default(),
                typ: field_id.typ,
            });
        }
    }
    None
}

fn type_fun(runner: &mut Runner, tree: &mut [Node], at: usize, typ: Type) -> Type {
    let node = tree[at];
    // println!("Type fun @{at}: {node:?}");
    let Nod::Branch {
        kind: BranchKind::Fun,
        range,
    } = node.nod
    else {
        panic!()
    };
    // Kids
    let range: Range<usize> = range.into();
    // Should always be in-params, out(-params), body.
    let Range { start, .. } = range;
    // Params
    let Nod::Branch {
        range: params_range,
        ..
    } = tree[start].nod
    else {
        panic!()
    };
    let params_range: Range<usize> = params_range.into();
    let ref_start = runner.typer.type_refs.len();
    let mut any_change = false;
    for param_index in params_range.clone() {
        let old_type = tree[param_index].typ;
        let param_type = type_any(runner, tree, param_index, Type::default());
        // println!("param {param_type:?}");
        runner.typer.type_refs.push(param_type);
        any_change |= param_type != old_type;
    }
    // Return
    let Nod::Branch {
        range: out_range, ..
    } = tree[start + 1].nod
    else {
        panic!()
    };
    let out_range: Range<usize> = out_range.into();
    let old_out_type = if node.typ.0 == 0 {
        Type(0)
    } else {
        runner.typer.types_ref().working[node.typ.0 as usize - 1].typ
    };
    let mut out_type = if out_range.is_empty() {
        old_out_type.or(
            // See if we're expecting a particular return type.
            if typ.0 != 0 {
                runner.typer.types_ref().working[typ.0 as usize - 1].typ
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
    if any_change || node.typ.0 == 0 || (out_type.0 != 0 && old_out_type.0 == 0) {
        // After digging subtrees, we're ready to push type refs.
        let types_start = runner.typer.types_ref().pos();
        runner.typer.push_type_refs(ref_start);
        // Function
        runner
            .typer
            .types_mut()
            .wrap(BranchKind::FunType, types_start, out_type, 0);
        runner.typer.unify()
    } else {
        runner.typer.type_refs.drain(ref_start..);
        node.typ
    }
}
