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
    Also,
    AngleClose,
    AngleOpen,
    Be,
    Colon,
    Comma,
    Comment,
    CurlyClose,
    CurlyOpen,
    Define,
    Dot,
    End,
    Eq,
    Fun,
    GreaterEq,
    HSpace,
    Id,
    Int,
    LessEq,
    Minus,
    // None,
    NotEq,
    Of,
    Plus,
    RoundClose,
    RoundOpen,
    Star,
    // TODO String parts and lex mode stack. Is call stack good enough?
    String,
    StringEdge,
    StringEscape,
    StringEscaper,
    To,
    VSpace,
    With,
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
        while let Some(c) = source.peek() {
            match c {
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
                '<' => {
                    self.trim(&mut source);
                    match source.peek() {
                        Some('=') => {
                            self.next(&mut source);
                            self.push(TokenKind::LessEq);
                        }
                        _ => self.push(TokenKind::AngleOpen),
                    }
                }
                '>' => {
                    self.trim(&mut source);
                    match source.peek() {
                        Some('=') => {
                            self.next(&mut source);
                            self.push(TokenKind::GreaterEq);
                        }
                        _ => self.push(TokenKind::AngleClose),
                    }
                }
                '{' => self.trim_push(&mut source, TokenKind::CurlyOpen),
                '}' => self.trim_push(&mut source, TokenKind::CurlyClose),
                '(' => self.trim_push(&mut source, TokenKind::RoundOpen),
                ')' => self.trim_push(&mut source, TokenKind::RoundClose),
                '.' => self.trim_push(&mut source, TokenKind::Dot),
                '=' => {
                    self.trim(&mut source);
                    match source.peek() {
                        Some('=') => {
                            self.next(&mut source);
                            self.push(TokenKind::Eq);
                        }
                        _ => self.push(TokenKind::Define),
                    }
                }
                '!' => {
                    self.trim(&mut source);
                    if let Some('=') = source.peek() {
                        self.next(&mut source);
                        self.push(TokenKind::NotEq);
                    }
                }
                '*' => self.trim_push(&mut source, TokenKind::Star),
                '+' => self.trim_push(&mut source, TokenKind::Plus),
                '-' | '0'..='9' if self.buffer().is_empty() => self.number(&mut source),
                _ => {
                    self.next(&mut source);
                }
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
        while let Some(' ' | '\t') = source.peek() {
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

    fn number(&mut self, source: &mut Peekable<Chars>) {
        let negative = *source.peek().unwrap() == '-';
        self.trim(source);
        while let Some('0'..='9') = source.peek() {
            self.next(source);
        }
        if negative && self.buffer().len() < 2 {
            // Disconnected minus.
            self.push(TokenKind::Minus);
            return;
        }
        self.push(TokenKind::Int);
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
                    if source.peek().is_some() {
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
                "also" => TokenKind::Also,
                "be" => TokenKind::Be,
                "end" => TokenKind::End,
                "for" => TokenKind::Fun,
                "of" => TokenKind::Of,
                "to" => TokenKind::To,
                "with" => TokenKind::With,
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
