use postcard::take_from_bytes;
use serde::{Deserialize, Serialize};

use crate::{
    Cart,
    lex::{Token, TokenKind},
    tree::{SizeRange, TreeBuilder},
};

// TODO Combine multiple files into one parse tree.
// TODO Track the node ranges for each file.
#[derive(Clone, Copy, Debug, Deserialize, Eq, Hash, PartialEq, Serialize)]
pub enum ParseNode {
    Branch(ParseBranch),
    Leaf(Token),
}

#[derive(Clone, Copy, Debug, Deserialize, Eq, Hash, PartialEq, Serialize)]
pub enum ParseBranchKind {
    Block,
    Call,
    Infix,
    Def,
    Params,
    Typed,
    Fun,
    Pub,
    StringParts,
}

#[derive(Clone, Copy, Debug, Deserialize, Eq, Hash, PartialEq, Serialize)]
pub struct ParseBranch {
    pub kind: ParseBranchKind,
    pub range: SizeRange,
}

macro_rules! define_infix {
    ($name:ident, $next:ident, $pattern:pat $(if $guard:expr)? $(,)?) => {
        fn $name(&mut self) -> Option<()> {
            let start = self.builder().pos();
            loop_some!({
                self.skip_h()?;
                if !matches!(self.peek()?, $pattern $(if $guard)?) {
                    return Some(());
                }
                self.advance();
                self.skip_hv()?;
                self.$next()?;
                self.wrap(ParseBranchKind::Infix, start);
            })
        }
    };
}

macro_rules! loop_some {
    {$t:tt} => {(|| -> Option<()> {
        loop {
            $t
        }
    })()};
}

pub struct Parser<'a> {
    pub cart: &'a mut Cart,
    peeked: Option<(Token, usize)>,
    tokens_index: usize,
}

impl<'a> Parser<'a> {
    pub fn new(cart: &'a mut Cart) -> Self {
        Self {
            cart,
            peeked: None,
            tokens_index: 0,
        }
    }

    pub fn parse(&mut self) {
        self.builder().clear();
        self.block_top();
        // Finish top and drain tree.
        let bytes_top = self.builder().applied.len();
        self.wrap(ParseBranchKind::Block, 0);
        self.cart
            .tree_builder
            .drain_into(&mut self.cart.bytes, bytes_top);
    }

    define_infix!(add, atom, TokenKind::Minus | TokenKind::Plus);

    fn advance(&mut self) {
        let Some((token, next)) = self.peek_token_step() else {
            return;
        };
        self.advance_without_peek(token, next);
    }

    fn advance_without_peek(&mut self, token: Token, next: usize) {
        self.tokens_index = next;
        self.builder().push(ParseNode::Leaf(token));
        self.peeked = None;
    }

    fn atom(&mut self) -> Option<()> {
        self.skip_h();
        match self.peek()? {
            TokenKind::Colon | TokenKind::Comma | TokenKind::VSpace => {}
            // TokenKind::Be
            // | TokenKind::CurlyOpen
            // | TokenKind::Of
            // | TokenKind::RoundOpen => self.block()?,
            // TokenKind::Fun => self.fun()?,
            TokenKind::Id => self.advance(),
            // TokenKind::StringEdge => self.string(),
            _ => self.advance(),
        }
        Some(())
    }

    fn block_content(&mut self) -> Option<()> {
        loop_some!({
            self.skip_hv();
            match self.peek()? {
                TokenKind::Comma => self.advance(),
                TokenKind::CurlyClose
                | TokenKind::End
                | TokenKind::RoundClose
                | TokenKind::SquareClose => None?,
                _ => self.try_advance(|s| s.def())?,
            }
        });
        self.skip_hv();
        Some(())
    }

    fn block_top(&mut self) {
        let start = self.pos();
        loop_some!({
            self.block_content()?;
            match self.peek()? {
                TokenKind::Greater
                | TokenKind::CurlyClose
                | TokenKind::End
                | TokenKind::RoundClose => {
                    // Eat trash. TODO Avoid ever getting here?
                    self.advance();
                    self.skip_hv();
                }
                _ => {}
            }
            self.advance();
        });
        if self.builder().pos() > start {
            self.wrap(ParseBranchKind::Block, start);
        }
    }

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    define_infix!(
        compare,
        add,
        TokenKind::Eq
            | TokenKind::Greater
            | TokenKind::GreaterEq
            | TokenKind::Less
            | TokenKind::LessEq
            | TokenKind::NotEq,
    );

    fn def(&mut self) -> Option<()> {
        // debug!("def");
        self.skip_h();
        let start = self.builder().pos();
        self.compare();
        if self.builder().pos() > start {
            self.skip_h();
            if self.peek()? == TokenKind::Define {
                self.advance();
                self.skip_hv();
                // Right-side descent.
                // TODO Error on nested assignment later?
                self.def();
                self.wrap(ParseBranchKind::Def, start);
                self.skip_hv()?;
            }
        }
        // debug!("/def");
        Some(())
    }

    fn peek(&mut self) -> Option<TokenKind> {
        self.peek_token_step().map(|it| it.0.kind)
    }

    fn peek_token_step(&mut self) -> Option<(Token, usize)> {
        match self.peeked {
            // Use cached when available to avoid redecoding cost.
            Some(_) => self.peeked,
            _ => {
                while self.tokens_index < self.cart.bytes.len() {
                    let (token, unused) =
                        take_from_bytes::<Token>(&self.cart.bytes[self.tokens_index..]).unwrap();
                    let next = self.cart.bytes.len() - unused.len();
                    // Eat hspace, so only stop looping on other.
                    if !matches!(token.kind, TokenKind::Comment | TokenKind::HSpace) {
                        self.peeked = Some((token, next));
                        break;
                    }
                    self.advance_without_peek(token, next);
                }
                // dbg!(self.peeked);
                self.peeked
            }
        }
    }

    fn pos(&mut self) -> usize {
        self.builder().pos()
    }

    fn skip<F>(&mut self, skipping: F) -> Option<()>
    where
        F: Fn(TokenKind) -> bool,
    {
        loop_some!({
            if skipping(self.peek()?) {
                // debug!("Skipping {:?}", token);
                self.advance();
            } else {
                None?;
            }
        })?;
        Some(())
    }

    /// Might call manually to control where whitespace gathers.
    fn skip_h(&mut self) -> Option<()> {
        self.peek_token_step().map(|_| ())
    }

    fn skip_hv(&mut self) -> Option<()> {
        self.skip(|kind| matches!(kind, TokenKind::VSpace))
    }

    /// None if done or if no advance happened.
    fn try_advance<F>(&mut self, f: F) -> Option<()>
    where
        F: FnOnce(&mut Self) -> Option<()>,
    {
        let start = self.tokens_index;
        f(self)?;
        match () {
            _ if self.tokens_index == start => None,
            _ => Some(()),
        }
    }

    fn wrap(&mut self, kind: ParseBranchKind, start: usize) {
        let range = self.builder().apply_range(start);
        let branch = ParseBranch { kind, range };
        self.builder().push(ParseNode::Branch(branch));
    }
}
