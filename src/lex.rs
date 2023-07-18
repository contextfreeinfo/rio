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
    CurlyClose,
    CurlyOpen,
    Define,
    HSpace,
    Id,
    RoundClose,
    RoundOpen,
    String,
    VSpace,
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
            match source.peek() {
                Some(c) => match c {
                    ' ' | '\t' => self.hspace(&mut source),
                    '\n' => {
                        self.trim(&mut source);
                        self.push(TokenKind::VSpace);
                    }
                    '\r' => {
                        self.trim(&mut source);
                        if source.peek() == Some(&'\n') {
                            self.next(&mut source);
                        }
                        self.push(TokenKind::VSpace);
                    }
                    '"' => self.string(&mut source),
                    '{' => self.trim_push(&mut source, TokenKind::CurlyOpen),
                    '}' => self.trim_push(&mut source, TokenKind::CurlyClose),
                    '(' => self.trim_push(&mut source, TokenKind::RoundOpen),
                    ')' => self.trim_push(&mut source, TokenKind::RoundClose),
                    '=' => self.trim_push(&mut source, TokenKind::Define),
                    _ => {
                        self.next(&mut source);
                    }
                },
                None => break,
            }
        }
        self.trim(&mut source);
        self.tokens.clone()
    }

    fn hspace(&mut self, source: &mut Peekable<Chars>) {
        self.trim(source);
        loop {
            match source.peek() {
                Some(' ' | '\t') => {}
                _ => break,
            }
            self.next(source);
        }
        self.push(TokenKind::HSpace);
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

    fn string(&mut self, source: &mut Peekable<Chars>) {
        self.trim(source);
        loop {
            match source.peek() {
                Some('"') => {
                    self.next(source);
                    break;
                }
                None => break,
                _ => {}
            }
            self.next(source);
        }
        self.push(TokenKind::String);
    }

    fn trim(&mut self, source: &mut Peekable<Chars>) {
        if !self.buffer.is_empty() {
            self.push(TokenKind::Id);
        }
        self.next(source);
    }

    fn trim_push(&mut self, source: &mut Peekable<Chars>, kind: TokenKind) {
        self.trim(source);
        self.push(kind);
    }
}
