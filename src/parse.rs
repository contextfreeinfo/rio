use std::{iter::Peekable, slice::Iter};

use log::debug;

use crate::{
    lex::{Token, TokenKind},
    tree::{BranchKind, TreeBuilder, Type},
    Cart,
};

macro_rules! define_infix {
    ($name:ident, $next:ident, $must_space:expr, $pattern:pat $(if $guard:expr)? $(,)?) => {
        fn $name(&mut self, source: &mut Tokens) -> Option<bool> {
            debug!("{}", stringify!($name));
            let start = self.builder().pos();
            let mut skipped = self.$next(source)?;
            loop {
                let pre_space = self.builder().pos();
                self.skip_h(source);
                let did_space = skipped || self.builder().pos() > pre_space;
                if ($must_space && !did_space) || !matches!(peek(source)?, $pattern $(if $guard)?) {
                    debug!("/{}", stringify!($name));
                    return Some(did_space);
                }
                self.advance(source);
                self.skip_hv(source);
                let maybe_skipped = self.$next(source);
                self.wrap(BranchKind::Infix, start);
                skipped = maybe_skipped?;
            }
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
            TokenKind::Be
            | TokenKind::CurlyOpen
            | TokenKind::Of
            | TokenKind::RoundOpen
            | TokenKind::With => self.block(source)?,
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
        let ender = choose_ender(peek(source)?);
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
                if kind != TokenKind::With {
                    self.advance(source);
                }
                if kind == ender || (ender == TokenKind::End && kind == TokenKind::With) {
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
                TokenKind::AngleClose
                | TokenKind::CurlyClose
                | TokenKind::End
                | TokenKind::RoundClose
                | TokenKind::With => None?,
                _ => {
                    self.def(source);
                }
            }
        });
        debug!("/block_content");
        self.skip_hv(source);
        Some(())
    }

    fn block_top(&mut self, source: &mut Tokens) {
        let start = self.builder().pos();
        loop_some!({
            debug!("block_top: {:?}", peek(source));
            self.block_content(source)?;
            match peek(source)? {
                TokenKind::AngleClose
                | TokenKind::CurlyClose
                | TokenKind::End
                | TokenKind::RoundClose => {
                    // Eat trash. TODO Avoid ever getting here.
                    self.advance(source);
                    self.skip_hv(source);
                }
                TokenKind::With => {
                    // Also trash, but be nicer.
                    self.atom(source);
                }
                _ => {}
            }
        });
        if self.builder().pos() > start {
            self.wrap(BranchKind::Block, start);
        }
    }

    fn call(&mut self, source: &mut Tokens, allow_block: bool) -> Option<()> {
        self.call_maybe_spaced(source, allow_block)
    }

    fn call_maybe_spaced(&mut self, source: &mut Tokens, allow_block: bool) -> Option<()> {
        debug!("call");
        self.skip_h(source);
        let start = self.builder().pos();
        let had_space = self.pair(source)?;
        loop {
            debug!("call loop: {:?}", peek(source));
            let mut post = self.builder().pos();
            if post <= start {
                break;
            }
            // TODO Can we leaving trailing hspace, or do we need to have an indicator?
            let peeked = peek(source)?;
            if had_space
                || matches!(
                    peeked,
                    TokenKind::HSpace | TokenKind::Be | TokenKind::CurlyOpen | TokenKind::With
                )
            {
                self.skip_h(source);
                if !allow_block
                    && matches!(
                        peek(source)?,
                        TokenKind::Be | TokenKind::CurlyOpen | TokenKind::With
                    )
                {
                    break;
                }
                post = self.builder().pos();
                self.spaced(source);
            } else {
                break;
            }
            if self.builder().pos() <= post {
                // TODO What does this mean???
                break;
            }
            self.wrap(BranchKind::Call, start);
            debug!("/call loop");
        }
        debug!("/call");
        Some(())
    }

    fn call_tight(&mut self, source: &mut Tokens) -> Option<bool> {
        debug!("call_tight");
        let start = self.builder().pos();
        let mut skipped = self.dot(source)?;
        loop {
            let peeked = peek(source)?;
            match peeked {
                TokenKind::AngleOpen | TokenKind::RoundOpen if !skipped => {
                    // Unspaced open brackets bind tightly.
                    // self.call_maybe_spaced(source, false, false, Some(start));
                    // TODO Below instead of `call_maybe_spaced` above.
                    // TODO Expand paren-comma args.
                    self.advance(source);
                    self.block_content(source);
                    if peek(source)? == choose_ender(peeked) {
                        self.advance(source);
                    }
                }
                _ => {
                    match peek(source)? {
                        // Of blocks bind tightly.
                        TokenKind::Of => {
                            skipped = self.dot(source)?;
                        }
                        _ => break,
                    }
                }
            };
            self.wrap(BranchKind::Call, start);
        }
        debug!("/call_tight");
        Some(skipped)
    }

    define_infix!(
        compare,
        call_tight,
        true,
        TokenKind::AngleClose
            | TokenKind::AngleOpen
            | TokenKind::Eq
            | TokenKind::GreaterEq
            | TokenKind::LessEq
            | TokenKind::NotEq,
    );

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

    define_infix!(dot, starred, false, TokenKind::Dot,);

    fn fun(&mut self, source: &mut Tokens) -> Option<()> {
        debug!("fun");
        let start = self.builder().pos();
        self.advance(source);
        self.skip_h(source);
        // TODO Type params
        // In params
        let in_params_start = self.builder().pos();
        let start_kind = peek(source)?;
        match start_kind {
            TokenKind::CurlyOpen | TokenKind::Id => {
                self.atom(source);
                self.skip_h(source);
            }
            // TODO If angle, also try paren afterward. If so, nested funs.
            TokenKind::AngleOpen | TokenKind::RoundOpen => {
                self.advance(source);
                // TODO Call it Params.
                self.block_content(source);
                if peek(source)? == choose_ender(start_kind) {
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

    define_infix!(pair, compare, false, TokenKind::To);

    fn skip<F>(&mut self, source: &mut Tokens, skipping: F) -> Option<bool>
    where
        F: Fn(TokenKind) -> bool,
    {
        let mut skipped = false;
        loop {
            let token = peek_token(source)?;
            if skipping(token.kind) {
                debug!("Skipping {:?}", token);
                skipped = true;
                source.next();
                self.builder().push_at(token, 0);
            } else {
                break;
            }
        }
        Some(skipped)
    }

    fn skip_h(&mut self, source: &mut Tokens) -> Option<bool> {
        self.skip(source, |kind| {
            matches!(kind, TokenKind::Comment | TokenKind::HSpace)
        })
    }

    fn skip_hv(&mut self, source: &mut Tokens) -> Option<bool> {
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
                TokenKind::AngleClose
                | TokenKind::Comma
                | TokenKind::Colon
                | TokenKind::CurlyClose
                | TokenKind::Define
                | TokenKind::End
                | TokenKind::RoundClose
                | TokenKind::VSpace => break,
                _ => self.pair(source),
            };
            debug!("/spaced");
        }
        Some(())
    }

    fn starred(&mut self, source: &mut Tokens) -> Option<bool> {
        debug!("starred");
        let start = self.builder().pos();
        self.atom(source);
        if peek(source)? == TokenKind::Star {
            self.advance(source);
            self.wrap(BranchKind::Pub, start);
        }
        debug!("/starred");
        Some(false)
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
            debug!("typed loop: {:?}", peek(source)?);
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

fn choose_ender(token_kind: TokenKind) -> TokenKind {
    match token_kind {
        TokenKind::AngleOpen => TokenKind::AngleClose,
        TokenKind::Be | TokenKind::Of | TokenKind::With => TokenKind::End,
        TokenKind::CurlyOpen => TokenKind::CurlyClose,
        TokenKind::RoundOpen => TokenKind::RoundClose,
        _ => panic!(),
    }
}

fn peek(source: &mut Tokens) -> Option<TokenKind> {
    source.peek().map(|it| it.kind)
}

fn peek_token(source: &mut Tokens) -> Option<Token> {
    source.peek().copied().copied()
}
