use postcard::take_from_bytes;

use crate::{Cart, lex::Token};

pub struct Parser<'a> {
    pub cart: &'a mut Cart,
    pub tokens_index: usize,
}

impl<'a> Parser<'a> {
    pub fn new(cart: &'a mut Cart) -> Self {
        Self {
            cart,
            tokens_index: 0,
        }
    }

    pub fn parse(&mut self) {
        while self.tokens_index < self.cart.bytes.len() {
            let Some((token, next)) = self.peek_token_step() else {
                break;
            };
            self.tokens_index = next;
            // dbg!(token, next, self.tokens_index);
            dbg!(token);
        }
    }

    fn peek_token_step(&self) -> Option<(Token, usize)> {
        match () {
            _ if self.tokens_index < self.cart.bytes.len() => {
                // dbg!(self.cart.bytes[self.tokens_index..].len());
                let (token, unused) =
                    take_from_bytes(&self.cart.bytes[self.tokens_index..]).unwrap();
                // dbg!(unused.len());
                Some((token, self.cart.bytes.len() - unused.len()))
            }
            _ => None,
        }
    }
}
