use std::{fmt::Debug, iter::Peekable, str::Chars, sync::Arc};

use lasso::{Spur, ThreadedRodeo};

use crate::Cart;

pub type Intern = Spur;
pub type Interner = Arc<ThreadedRodeo>;

#[derive(Clone, Copy, Eq, Hash, PartialEq)]
pub struct Token {
    pub kind: TokenKind,
    pub intern: Intern,
}

impl Debug for Token {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        f.write_fmt(format_args!("{:?}:{}", self.kind, self.intern.into_inner()))
    }
}

impl Token {
    pub fn new(kind: TokenKind, intern: Intern) -> Self {
        Self { kind, intern }
    }
}

#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
pub enum TokenKind {
    Be,
    Colon,
    Comma,
    Comment,
    CurlyClose,
    CurlyOpen,
    Define,
    End,
    Fun,
    HSpace,
    Id,
    None,
    RoundClose,
    RoundOpen,
    Star,
    // TODO String parts and lex mode stack. Is call stack good enough?
    String,
    StringEdge,
    StringEscape,
    StringEscaper,
    VSpace,
}

pub struct Lexer<'a> {
    pub cart: &'a mut Cart,
    pub tokens: Vec<Token>,
}

impl<'a> Lexer<'a> {
    pub fn new(cart: &'a mut Cart) -> Self {
        Self {
            cart,
            tokens: vec![],
        }
    }

    pub fn lex(&mut self, source: &str) {
        self.buffer().clear();
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
                    '#' => self.comment(&mut source),
                    '"' => self.string(&mut source),
                    ':' => self.trim_push(&mut source, TokenKind::Colon),
                    ',' | ';' => self.trim_push(&mut source, TokenKind::Comma),
                    '{' => self.trim_push(&mut source, TokenKind::CurlyOpen),
                    '}' => self.trim_push(&mut source, TokenKind::CurlyClose),
                    '(' => self.trim_push(&mut source, TokenKind::RoundOpen),
                    ')' => self.trim_push(&mut source, TokenKind::RoundClose),
                    '=' => self.trim_push(&mut source, TokenKind::Define),
                    '*' => self.trim_push(&mut source, TokenKind::Star),
                    _ => {
                        self.next(&mut source);
                    }
                },
                None => break,
            }
        }
        self.trim(&mut source);
    }

    fn buffer(&mut self) -> &mut String {
        &mut self.cart.buffer
    }

    fn comment(&mut self, source: &mut Peekable<Chars>) {
        self.trim(source);
        loop {
            match source.peek() {
                Some('\r' | '\n') | None => break,
                _ => {}
            }
            self.next(source);
        }
        self.push(TokenKind::Comment);
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
            self.buffer().push(c);
        }
        next
    }

    fn push(&mut self, kind: TokenKind) {
        let val = self.cart.buffer.as_str();
        let intern = self.cart.interner.get_or_intern(val);
        self.buffer().clear();
        self.tokens.push(Token::new(kind, intern));
    }

    fn push_maybe(&mut self, kind: TokenKind) {
        if !self.buffer().is_empty() {
            self.push(kind);
        }
    }

    fn string(&mut self, source: &mut Peekable<Chars>) {
        self.trim_push(source, TokenKind::StringEdge);
        // TODO Better string lexing.
        loop {
            match source.peek() {
                Some('\\') => {
                    self.push_maybe(TokenKind::String);
                    self.next(source);
                    self.push(TokenKind::StringEscaper);
                    if let Some(_) = source.peek() {
                        self.next(source);
                        self.push(TokenKind::StringEscape);
                    }
                }
                Some('"') | None => {
                    self.push_maybe(TokenKind::String);
                    self.next(source);
                    break;
                }
                _ => {
                    self.next(source);
                }
            }
        }
        self.push(TokenKind::StringEdge);
    }

    fn trim(&mut self, source: &mut Peekable<Chars>) {
        if !self.buffer().is_empty() {
            let kind = match self.buffer().as_str() {
                "be" => TokenKind::Be,
                "end" => TokenKind::End,
                "for" => TokenKind::Fun,
                _ => TokenKind::Id,
            };
            self.push(kind);
        }
        self.next(source);
    }

    fn trim_push(&mut self, source: &mut Peekable<Chars>, kind: TokenKind) {
        self.trim(source);
        self.push(kind);
    }
}
