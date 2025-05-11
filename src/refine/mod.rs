use extract::Extractor;
use resolve::Resolver;

use crate::Cart;

pub mod extract;
pub mod resolve;

pub struct Refiner<'a> {
    pub cart: &'a mut Cart,
}

impl<'a> Refiner<'a> {
    pub fn new(cart: &'a mut Cart) -> Self {
        Self { cart }
    }

    pub fn refine(&mut self) {
        self.cart.defs.clear();
        // Every step can be run in a cycle, ideally until convergence.
        // TODO Flatten destructuring assignments before extract.
        Extractor::new(self.cart).extract();
        Resolver::new(self.cart).resolve();
    }
}
