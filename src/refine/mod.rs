use extract::Extractor;

use crate::Cart;

pub mod extract;

pub struct Refiner<'a> {
    pub cart: &'a mut Cart,
}

impl<'a> Refiner<'a> {
    pub fn new(cart: &'a mut Cart) -> Self {
        Self { cart }
    }

    pub fn refine(&mut self) {
        Extractor::new(self.cart).extract();
        // self.builder().clear();
        // let source = TreeBuilder::top_of(&self.cart.tree);
        // Finish top and drain tree.
        // let top = self.wrap(|s| s.top(source)).start;
        // self.cart.tree_builder.drain_into(&mut self.cart.tree, top);
    }
}
