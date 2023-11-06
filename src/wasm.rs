use wasm_encoder::TypeSection;

use crate::Cart;

pub fn write_wasm(cart: &Cart) {
    let mut module = wasm_encoder::Module::new();
    let _ = cart;
    let types = TypeSection::new();
    module.section(&types);
}
