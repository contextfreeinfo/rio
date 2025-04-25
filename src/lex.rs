use std::{fmt::Debug, sync::Arc};

use lasso::{Spur, ThreadedRodeo};
use num_derive::FromPrimitive;
use strum::EnumCount;

use crate::{Cart, tree::Size};

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

#[derive(Clone, Copy, Debug, EnumCount, Eq, FromPrimitive, Hash, PartialEq)]
pub enum TokenKind {
    Also = TOKEN_KIND_START as isize,
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

pub const TOKEN_KIND_START: Size = 0;
pub const TOKEN_KIND_END: Size = TOKEN_KIND_START + TokenKind::COUNT as Size;

pub struct Lexer<'a> {
    pub cart: &'a mut Cart,
    pub source_index: usize,
    pub token_start: usize,
}

impl<'a> Lexer<'a> {
    pub fn new(cart: &'a mut Cart) -> Self {
        Self {
            cart,
            source_index: 0,
            token_start: 0,
        }
    }

    pub fn lex(&mut self) {
        self.tokens().clear();
        while let Some(c) = self.peek() {
            match c {
                ' ' | '\t' => self.hspace(),
                '\n' => {
                    self.trim();
                    self.push(TokenKind::VSpace);
                }
                '\r' => {
                    self.trim();
                    if self.peek() == Some('\n') {
                        self.next();
                    }
                    self.push(TokenKind::VSpace);
                }
                '#' => self.comment(),
                '"' => self.string(),
                ':' => self.trim_push(TokenKind::Colon),
                ',' | ';' => self.trim_push(TokenKind::Comma),
                '<' => {
                    self.trim();
                    match self.peek() {
                        Some('=') => {
                            self.next();
                            self.push(TokenKind::LessEq);
                        }
                        _ => self.push(TokenKind::AngleOpen),
                    }
                }
                '>' => {
                    self.trim();
                    match self.peek() {
                        Some('=') => {
                            self.next();
                            self.push(TokenKind::GreaterEq);
                        }
                        _ => self.push(TokenKind::AngleClose),
                    }
                }
                '{' => self.trim_push(TokenKind::CurlyOpen),
                '}' => self.trim_push(TokenKind::CurlyClose),
                '(' => self.trim_push(TokenKind::RoundOpen),
                ')' => self.trim_push(TokenKind::RoundClose),
                '.' => self.trim_push(TokenKind::Dot),
                '=' => {
                    self.trim();
                    match self.peek() {
                        Some('=') => {
                            self.next();
                            self.push(TokenKind::Eq);
                        }
                        _ => self.push(TokenKind::Define),
                    }
                }
                '!' => {
                    self.trim();
                    if let Some('=') = self.peek() {
                        self.next();
                        self.push(TokenKind::NotEq);
                    }
                }
                '*' => self.trim_push(TokenKind::Star),
                '+' => self.trim_push(TokenKind::Plus),
                '-' | '0'..='9' if self.token_text().is_empty() => self.number(),
                _ => {
                    self.next();
                }
            }
        }
        self.trim();
    }

    fn token_text(&self) -> &str {
        &self.cart.text[self.token_start..self.source_index]
    }

    fn tokens(&mut self) -> &mut Vec<Token> {
        &mut self.cart.tokens
    }

    fn comment(&mut self) {
        self.trim();
        loop {
            match self.peek() {
                Some('\r' | '\n') | None => break,
                _ => {}
            }
            self.next();
        }
        self.push(TokenKind::Comment);
    }

    fn hspace(&mut self) {
        self.trim();
        while let Some(' ' | '\t') = self.peek() {
            self.next();
        }
        self.push(TokenKind::HSpace);
    }

    fn next(&mut self) -> Option<char> {
        let c = self.peek()?;
        self.source_index += c.len_utf8();
        Some(c)
    }

    fn number(&mut self) {
        let negative = self.peek().unwrap() == '-';
        self.trim();
        while let Some('0'..='9') = self.peek() {
            self.next();
        }
        if negative && self.token_text().len() < 2 {
            // Disconnected minus.
            self.push(TokenKind::Minus);
            return;
        }
        self.push(TokenKind::Int);
    }

    fn peek(&mut self) -> Option<char> {
        self.cart.text[self.source_index..].chars().next()
    }

    fn push(&mut self, kind: TokenKind) {
        let val = self.token_text();
        let intern = self.cart.interner.get_or_intern(val);
        self.token_start = self.source_index;
        self.tokens().push(Token::new(kind, intern));
    }

    fn push_maybe(&mut self, kind: TokenKind) {
        if !self.token_text().is_empty() {
            self.push(kind);
        }
    }

    fn string(&mut self) {
        self.trim_push(TokenKind::StringEdge);
        // TODO Better string lexing.
        loop {
            match self.peek() {
                Some('\\') => {
                    self.push_maybe(TokenKind::String);
                    self.next();
                    self.push(TokenKind::StringEscaper);
                    if self.peek().is_some() {
                        self.next();
                        self.push(TokenKind::StringEscape);
                    }
                }
                Some('"') | None => {
                    self.push_maybe(TokenKind::String);
                    self.next();
                    break;
                }
                _ => {
                    self.next();
                }
            }
        }
        self.push(TokenKind::StringEdge);
    }

    fn trim(&mut self) {
        if !self.token_text().is_empty() {
            let kind = match self.token_text() {
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
        self.next();
    }

    fn trim_push(&mut self, kind: TokenKind) {
        self.trim();
        self.push(kind);
    }
}
