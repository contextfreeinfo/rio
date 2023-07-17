use std::sync::Arc;

use lasso::{Spur, ThreadedRodeo};

#[derive(Default)]
pub struct Lexer {
    interner: Interner,
    buffer: String,
}

pub type Intern = Spur;
pub type Interner = Arc<ThreadedRodeo>;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Token {
    pub kind: TokenKind,
    pub intern: Intern,
}

impl Token {
    pub fn new(kind: TokenKind, atom: Intern) -> Self {
        Self { kind, intern: atom }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum TokenKind {
    Id,
}

impl Lexer {
    pub fn new(interner: Interner) -> Self {
        Self {
            interner,
            ..Default::default()
        }
    }

    pub fn lex(&mut self, source: &str) {
        self.buffer.clear();
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
        let atom = self.interner.get_or_intern(self.buffer.as_str());
        tokens.push(Token::new(TokenKind::Id, atom));
        for token in tokens {
            let atom = token.intern;
            println!("{:?} {:?}", token, self.interner.resolve(&atom));
        }
    }
}
