use std::{iter::Peekable, slice::Iter};

use log::debug;

use crate::{
    lex::{Token, TokenKind},
    tree::{BranchKind, TreeBuilder, Type},
    Cart,
};

macro_rules! loop_some {
    {$t:tt} => {(|| -> Option<()> {
        loop {
            $t
        }
    })()};
}

type Tokens<'a> = Peekable<Iter<'a, Token>>;

pub struct Parser {
    pub cart: Cart,
}

impl Parser {
    pub fn new(cart: Cart) -> Self {
        Self { cart }
    }

    pub fn parse(&mut self, tokens: &[Token]) {
        self.builder().clear();
        let mut source = tokens.iter().peekable();
        self.block_top(&mut source);
        self.wrap(BranchKind::Block, 0);
    }

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    fn advance(&mut self, source: &mut Tokens) {
        let next = source.next();
        debug!("Advance past {:?}", next);
        if let Some(token) = next {
            self.builder().push_at(token, 0);
        }
    }

    fn atom(&mut self, source: &mut Tokens) -> Option<()> {
        debug!("atom");
        self.skip_h(source);
        match peek(source)? {
            TokenKind::Colon | TokenKind::Comma | TokenKind::VSpace => {}
            TokenKind::Be | TokenKind::CurlyOpen | TokenKind::RoundOpen => self.block(source)?,
            TokenKind::Fun => self.fun(source)?,
            TokenKind::Id => self.advance(source),
            TokenKind::StringEdge => self.string(source),
            _ => self.advance(source),
        }
        debug!("/atom");
        Some(())
    }

    fn block(&mut self, source: &mut Tokens) -> Option<()> {
        debug!("block");
        let start = self.builder().pos();
        let ender = match peek(source)? {
            TokenKind::Be => TokenKind::End,
            TokenKind::CurlyOpen => TokenKind::CurlyClose,
            TokenKind::RoundOpen => TokenKind::RoundClose,
            _ => panic!(),
        };
        self.advance(source);
        self.skip_h(source);
        if ender == TokenKind::End && peek(source)? != TokenKind::VSpace {
            // Inline be ...
            self.call(source, true);
        } else {
            // Actually wrapped block.
            loop_some!({
                self.block_content(source);
                let kind = peek(source)?;
                self.advance(source);
                if kind == ender {
                    None?
                }
            });
        }
        if self.builder().pos() > start {
            self.wrap(BranchKind::Block, start);
        }
        debug!("/block");
        Some(())
    }

    fn block_content(&mut self, source: &mut Tokens) -> Option<()> {
        debug!("block_content");
        loop_some!({
            self.skip_hv(source);
            match peek(source)? {
                TokenKind::Comma => self.advance(source),
                TokenKind::CurlyClose | TokenKind::End | TokenKind::RoundClose => None?,
                _ => {
                    self.def(source);
                }
            }
        });
        debug!("/block_content");
        self.skip_hv(source)
    }

    fn block_top(&mut self, source: &mut Tokens) {
        let start = self.builder().pos();
        loop_some!({
            debug!("block_top: {:?}", peek(source));
            self.block_content(source)?;
            if matches!(
                peek(source)?,
                TokenKind::CurlyClose | TokenKind::End | TokenKind::RoundClose,
            ) {
                // Eat trash. TODO Avoid ever getting here.
                self.advance(source);
                self.skip_hv(source);
            }
        });
        if self.builder().pos() > start {
            self.wrap(BranchKind::Block, start);
        }
    }

    fn call(&mut self, source: &mut Tokens, allow_block: bool) -> Option<()> {
        self.skip_h(source);
        let start = self.builder().pos();
        self.starred(source);
        loop {
            debug!("call: {:?}", peek(source));
            let mut post = self.builder().pos();
            if post <= start {
                break;
            }
            // TODO Can we leaving trailing hspace, or do we need to have an indicator?
            match peek(source)? {
                TokenKind::HSpace | TokenKind::Be | TokenKind::CurlyOpen => {
                    self.skip_h(source);
                    if !allow_block && matches!(peek(source)?, TokenKind::Be | TokenKind::CurlyOpen)
                    {
                        break;
                    }
                    post = self.builder().pos();
                    self.spaced(source);
                }
                TokenKind::RoundOpen => {
                    // TODO Expand paren-comma args.
                    self.advance(source);
                    self.block_content(source);
                    if peek(source)? == TokenKind::RoundClose {
                        self.advance(source);
                    }
                    self.skip_h(source);
                    if matches!(peek(source)?, TokenKind::Be | TokenKind::CurlyOpen) {
                        if !allow_block {
                            break;
                        }
                        self.block(source);
                        self.skip_h(source);
                    }
                }
                _ => break,
            }
            if self.builder().pos() > post {
                self.wrap(BranchKind::Call, start);
            }
            debug!("/call");
        }
        Some(())
    }

    fn def(&mut self, source: &mut Tokens) -> Option<()> {
        debug!("def");
        self.skip_h(source);
        let start = self.builder().pos();
        self.typed(source);
        if self.builder().pos() > start {
            self.skip_h(source);
            if peek(source)? == TokenKind::Define {
                self.advance(source);
                self.skip_hv(source);
                // Right-side descent.
                self.def(source);
                self.wrap(BranchKind::Def, start);
                self.skip_hv(source)?;
            }
        }
        debug!("/def");
        Some(())
    }

    fn fun(&mut self, source: &mut Tokens) -> Option<()> {
        debug!("fun");
        let start = self.builder().pos();
        self.advance(source);
        self.skip_h(source);
        // TODO Type params
        // In params
        let in_params_start = self.builder().pos();
        match peek(source)? {
            TokenKind::CurlyOpen | TokenKind::Id => {
                self.atom(source);
                self.skip_h(source);
            }
            TokenKind::RoundOpen => {
                self.advance(source);
                // TODO Call it Params.
                self.block_content(source);
                if peek(source)? == TokenKind::RoundClose {
                    self.advance(source);
                }
            }
            _ => return Some(()),
        }
        self.wrap(BranchKind::Params, in_params_start);
        self.skip_h(source);
        // Out params
        let has_out = peek(source)? == TokenKind::Colon;
        if has_out {
            self.advance(source);
            self.skip_hv(source);
        }
        let out_params_start = self.builder().pos();
        if has_out {
            match peek(source)? {
                TokenKind::RoundOpen => {
                    // Explicit return vars.
                    self.advance(source);
                    self.block_content(source);
                    if peek(source)? == TokenKind::RoundClose {
                        self.advance(source);
                    }
                }
                _ => {
                    // Implied anonymous return var.
                    let typed_start = self.builder().pos();
                    self.builder().push_none(0);
                    self.call(source, false);
                    self.wrap(BranchKind::Typed, typed_start);
                }
            }
        }
        self.wrap(BranchKind::Params, out_params_start);
        self.skip_h(source);
        // Body
        self.atom(source);
        self.wrap(BranchKind::Fun, start);
        debug!("/fun");
        Some(())
    }

    fn skip<F>(&mut self, source: &mut Tokens, skipping: F) -> Option<()>
    where
        F: Fn(TokenKind) -> bool,
    {
        loop {
            let token = peek_token(source)?;
            if skipping(token.kind) {
                debug!("Skipping {:?}", token);
                source.next();
                self.builder().push_at(token, 0);
            } else {
                break;
            }
        }
        Some(())
    }

    fn skip_h(&mut self, source: &mut Tokens) -> Option<()> {
        self.skip(source, |kind| {
            matches!(kind, TokenKind::Comment | TokenKind::HSpace)
        })
    }

    fn skip_hv(&mut self, source: &mut Tokens) -> Option<()> {
        self.skip(source, |kind| {
            matches!(
                kind,
                TokenKind::Comment | TokenKind::HSpace | TokenKind::VSpace
            )
        })
    }

    fn spaced(&mut self, source: &mut Tokens) -> Option<()> {
        loop {
            self.skip_h(source);
            let next = peek(source)?;
            debug!("spaced: {next:?}");
            match next {
                TokenKind::Comma
                | TokenKind::Colon
                | TokenKind::CurlyClose
                | TokenKind::Define
                | TokenKind::End
                | TokenKind::RoundClose
                | TokenKind::VSpace => None?,
                _ => self.starred(source),
            };
            debug!("/spaced");
        }
    }

    fn starred(&mut self, source: &mut Tokens) -> Option<()> {
        debug!("starred");
        let start = self.builder().pos();
        self.atom(source);
        if peek(source)? == TokenKind::Star {
            self.advance(source);
            self.wrap(BranchKind::Pub, start);
        }
        debug!("/starred");
        Some(())
    }

    fn string(&mut self, source: &mut Peekable<Iter<'_, Token>>) {
        debug!("string");
        let start = self.builder().pos();
        self.advance(source);
        loop_some!({
            let next = peek(source)?;
            self.advance(source);
            if next == TokenKind::StringEdge {
                None?;
            }
        });
        self.wrap(BranchKind::StringParts, start);
        debug!("/string");
    }

    fn typed(&mut self, source: &mut Tokens) -> Option<()> {
        debug!("typed");
        self.skip_h(source);
        let start = self.builder().pos();
        self.call(source, true);
        self.skip_h(source);
        loop {
            if peek(source)? != TokenKind::Colon {
                break;
            }
            self.advance(source);
            self.skip_hv(source);
            self.call(source, false);
            self.wrap(BranchKind::Typed, start);
        }
        debug!("/typed");
        Some(())
    }

    fn wrap(&mut self, kind: BranchKind, start: u32) {
        self.builder().wrap(kind, start, Type::default(), 0);
    }
}

fn peek(source: &mut Tokens) -> Option<TokenKind> {
    source.peek().map(|it| it.kind)
}

fn peek_token(source: &mut Tokens) -> Option<Token> {
    source.peek().copied().copied()
}
