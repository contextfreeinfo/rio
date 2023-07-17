use std::sync::Arc;

use lasso::{Spur, ThreadedRodeo};

#[derive(Default)]
pub struct Lexer {
    pub rodeo: Arc<ThreadedRodeo>,
    pub buffer: String,
}

pub type Atom = Spur;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Token {
    pub kind: TokenKind,
    pub atom: Atom,
}

impl Token {
    pub fn new(kind: TokenKind, atom: Atom) -> Self {
        Self { kind, atom }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum TokenKind {
    Id,
}

impl Lexer {
    pub fn new(rodeo: Arc<ThreadedRodeo>) -> Self {
        Self {
            rodeo,
            ..Default::default()
        }
    }

    pub fn lex(&mut self, source: &str) {
        let mut source = source.chars().peekable();
        let mut tokens = vec![];
        loop {
            let next = source.next();
            match next {
                Some(c) => {
                    self.buffer.push(c);
                }
                None => break,
            }
        }
        let atom = self.rodeo.get_or_intern(self.buffer.as_str());
        tokens.push(Token::new(TokenKind::Id, atom));
        for token in tokens {
            let atom = token.atom;
            println!("{:?} {:?}", atom, self.rodeo.resolve(&atom));
        }
    }
}
