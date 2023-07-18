use std::{iter::Peekable, str::Chars, sync::Arc};

use lasso::{Spur, ThreadedRodeo};

#[derive(Default)]
pub struct Lexer {
    buffer: String,
    interner: Interner,
    tokens: Vec<Token>,
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
    HSpace,
    Id,
}

impl Lexer {
    pub fn new(interner: Interner) -> Self {
        Self {
            interner,
            ..Default::default()
        }
    }

    pub fn lex(&mut self, source: &str) -> Vec<Token> {
        self.buffer.clear();
        self.tokens.clear();
        let mut source = source.chars().peekable();
        loop {
            match self.next(&mut source) {
                Some(c) => match c {
                    ' ' | '\t' => self.white(&mut source),
                    '=' => {}
                    _ => {}
                },
                None => break,
            }
        }
        let atom = self.interner.get_or_intern(self.buffer.as_str());
        self.tokens.push(Token::new(TokenKind::Id, atom));
        self.tokens.clone()
    }

    fn next(&mut self, source: &mut Peekable<Chars>) -> Option<char> {
        let next = source.next();
        if let Some(c) = next {
            self.buffer.push(c);
        }
        next
    }

    fn push(&mut self, kind: TokenKind) {
        let atom = self.interner.get_or_intern(self.buffer.as_str());
        self.buffer.clear();
        self.tokens.push(Token::new(kind, atom));
    }

    fn white(&mut self, source: &mut Peekable<Chars>) {
        loop {
            let next = source.peek();
            match next {
                Some(' ' | '\t') => {}
                _ => break,
            }
            self.next(source);
        }
        self.push(TokenKind::HSpace);
    }
}
