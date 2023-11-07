use crate::Cart;

pub fn link_modules(cart: &Cart) {
    let _ = cart;
    // TODO Find referenced uids.
    // TODO Remove unused defs. Validation should happen before removal?
    // TODO Build lookup table.
    // TODO Flatten type aliases and equivalent types.
    // TODO Merge common types into new module, pointing all types there.
    // TODO Ensure all type uids in new module are global uids.
}
