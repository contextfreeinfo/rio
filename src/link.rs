use std::ops::Range;

use crate::{
    tree::{BranchKind, Nod, TreeBuilder},
    Cart,
};

pub fn link_modules(cart: &Cart) {
    let nodes = TreeBuilder::default();
    // Presume we might add more before types sometime.
    fun_name(nodes, cart);
    // TODO Find referenced uids.
    // TODO Remove unused defs. Validation should happen before removal?
    // TODO Build lookup table.
    // TODO Flatten type aliases and equivalent types.
    // TODO Merge common types into new module, pointing all types there.
    // TODO Ensure all type uids in new module are global uids.
}

fn fun_name(nodes: TreeBuilder, cart: &Cart) {
    let types_start = nodes.pos();
    let _ = types_start;
    for module in &cart.modules {
        let Nod::Branch { range, .. } = module.tree.last().unwrap().nod else {
            panic!()
        };
        let range: Range<usize> = range.into();
        for index in range {
            let node = module.tree[index];
            match node.nod {
                Nod::Branch {
                    kind: BranchKind::Types,
                    range: types_range,
                } => {
                    let _ = types_range;
                    // TODO Extract each type into `types`.
                }
                _ => {}
            }
        }
    }
}
