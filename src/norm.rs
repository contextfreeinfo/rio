use std::ops::Range;

use crate::{
    lex::{Intern, Interner, Token, TokenKind},
    tree::{BranchKind, Nod, Node, Nody, TreeBuilder, Type},
    Cart,
};

pub struct Normer {
    pub cart: Cart,
}

/// Provide easy access for comparing resolutions to core native definitions.
#[derive(Clone, Copy, Debug, Default)]
pub struct CoreInterns {
    eq: Intern,
    ge: Intern,
    gt: Intern,
    le: Intern,
    lt: Intern,
    ne: Intern,
    pair: Intern,
}

impl CoreInterns {
    pub fn new(interner: &Interner) -> Self {
        Self {
            eq: interner.get_or_intern("eq"),
            ge: interner.get_or_intern("ge"),
            gt: interner.get_or_intern("gt"),
            le: interner.get_or_intern("le"),
            lt: interner.get_or_intern("lt"),
            ne: interner.get_or_intern("ne"),
            pair: interner.get_or_intern("Pair"),
        }
    }
}

impl Normer {
    pub fn new(cart: Cart) -> Self {
        Self { cart }
    }

    pub fn norm(&mut self, parsed_tree: &[Nod], tree: &mut Vec<Node>) {
        self.trim(parsed_tree, tree);
        self.rebranch(tree);
        self.define(tree);
    }

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    fn define(&mut self, tree: &mut Vec<Node>) {
        self.builder().clear();
        self.define_at(&tree);
        self.builder()
            .wrap(BranchKind::Block, 0, Type::default(), 0);
        self.builder().drain_into(tree);
    }

    fn define_at(&mut self, tree: &[Node]) -> Option<()> {
        let node = *tree.last()?;
        match node.nod {
            Nod::Branch { kind, range, .. } => {
                let start = self.builder().pos();
                let range: Range<usize> = range.into();
                // Maybe handle some kids in custom fashion.
                let range = match kind {
                    BranchKind::Def => {
                        let kid = tree[range.start];
                        // We can't parse a Def of range len 0.
                        match kid.nod {
                            Nod::Branch {
                                kind: BranchKind::Typed,
                                range: first_range,
                                ..
                            } => {
                                // Expand typed in place.
                                let first_range: Range<usize> = first_range.into();
                                for kid_index in first_range.clone() {
                                    // TODO Why recurse instead of special handling?
                                    self.define_at(&tree[..=kid_index]);
                                }
                                range.start + 1..range.end
                            }
                            _ => {
                                // TODO Why recurse instead of special handling?
                                self.define_at(&tree[..=range.start]);
                                // Untyped, so push empty type after id.
                                self.builder().push_none(kid.source);
                                range.start + 1..range.end
                            }
                        }
                    }
                    _ => range,
                };
                // Loop remaining kids.
                for kid_index in range.clone() {
                    // TODO If we're a params, turn kid Ids to Defs.
                    self.define_at(&tree[..=kid_index]);
                }
                // Finalize.
                let kind = match kind {
                    BranchKind::Typed => {
                        // This typed wasn't the lead of a def, or we'd have handled it in that branch.
                        // No value, so push empty value after type.
                        self.builder().push_none(node.source);
                        // And make a def out of it.
                        BranchKind::Def
                    }
                    _ => kind,
                };
                self.builder()
                    .wrap(kind, start, Type::default(), node.source);
            }
            _ => self.builder().push(node),
        }
        Some(())
    }

    fn rebranch(&mut self, tree: &mut Vec<Node>) {
        self.builder().clear();
        self.rebranch_at(&tree);
        self.builder()
            .wrap(BranchKind::Block, 0, Type::default(), 0);
        self.builder().drain_into(tree);
    }

    fn rebranch_at(&mut self, tree: &[Node]) -> Option<()> {
        let node = *tree.last()?;
        match node.nod {
            Nod::Branch { kind, range, .. } => {
                let mut kind = kind;
                let start = self.builder().pos();
                let mut range: Range<usize> = range.into();
                let done = match kind {
                    BranchKind::Block => {
                        if !range.is_empty() {
                            if let Nod::Leaf { token } = tree[range.start].nod {
                                let new_kind = match token.kind {
                                    TokenKind::CurlyOpen | TokenKind::Of => {
                                        Some(BranchKind::Struct)
                                    }
                                    TokenKind::With => Some(BranchKind::List),
                                    _ => None,
                                };
                                if let Some(new_kind) = new_kind {
                                    range = range.start + 1..range.end;
                                    kind = new_kind;
                                }
                            }
                        }
                        false
                    }
                    BranchKind::Infix => {
                        let Nod::Leaf { token: op } = tree[range.start + 1].nod else {
                            panic!()
                        };
                        (|| {
                            let (new_kind, intern) = match op.kind {
                                TokenKind::Dot => (BranchKind::Dot, None),
                                _ => {
                                    let intern = match op.kind {
                                        TokenKind::AngleClose => self.cart.core_interns.gt,
                                        TokenKind::AngleOpen => self.cart.core_interns.lt,
                                        TokenKind::Eq => self.cart.core_interns.eq,
                                        TokenKind::GreaterEq => self.cart.core_interns.ge,
                                        TokenKind::LessEq => self.cart.core_interns.le,
                                        TokenKind::NotEq => self.cart.core_interns.ne,
                                        TokenKind::To => self.cart.core_interns.pair,
                                        _ => return false,
                                    };
                                    (BranchKind::Call, Some(intern))
                                }
                            };
                            if let Some(intern) = intern {
                                self.builder()
                                    .push_at(Token::new(TokenKind::Id, intern), node.source);
                            }
                            self.rebranch_at(&tree[..=range.start]);
                            if range.len() > 2 {
                                self.rebranch_at(&tree[..range.end]);
                            }
                            self.builder()
                                .wrap(new_kind, start, Type::default(), node.source);
                            true
                        })()
                    }
                    _ => false,
                };
                if !done {
                    for kid_index in range.clone() {
                        self.rebranch_at(&tree[..=kid_index]);
                    }
                    self.builder()
                        .wrap(kind, start, Type::default(), node.source);
                }
            }
            _ => self.builder().push(node),
        }
        Some(())
    }

    fn trim(&mut self, parsed_tree: &[Nod], tree: &mut Vec<Node>) {
        self.builder().clear();
        self.trim_at(&parsed_tree, 0);
        self.builder()
            .wrap(BranchKind::Block, 0, Type::default(), tree.len());
        self.builder().drain_into(tree);
    }

    fn trim_at(&mut self, tree: &[Nod], index: usize) -> Option<()> {
        let root = *tree.last()?;
        match root {
            Nod::Branch {
                kind: BranchKind::StringParts,
                range,
            } => {
                let range: Range<usize> = range.into();
                let buffer = &mut self.cart.buffer;
                buffer.clear();
                for kid_index in range.clone() {
                    if let Nod::Leaf { token } = tree[kid_index].nod() {
                        match token {
                            Token {
                                kind: TokenKind::StringEscape,
                                intern,
                            } => {
                                let value = match self.cart.interner[intern].chars().next().unwrap()
                                {
                                    'n' => '\n',
                                    ch => ch,
                                };
                                buffer.push(value);
                            }
                            Token {
                                kind: TokenKind::String,
                                intern,
                            } => {
                                buffer.push_str(&self.cart.interner[intern]);
                            }
                            // Skip others.
                            _ => {}
                        }
                    }
                }
                let intern = self.cart.interner.get_or_intern(&buffer);
                buffer.clear();
                self.builder()
                    .push_at(Token::new(TokenKind::String, intern), tree.len());
            }
            Nod::Branch { kind, range, .. } => {
                // TODO Change Typed to Def with separate type?
                // TODO Change all Def to triples? Quads? Kids with meta lists?
                let start = self.builder().pos();
                let range: Range<usize> = range.into();
                for kid_index in range.clone() {
                    self.trim_at(&tree[..=kid_index], kid_index - range.start);
                }
                self.builder()
                    .wrap(kind, start, Type::default(), tree.len());
            }
            Nod::Leaf { token, .. } => match token.kind {
                TokenKind::Colon | TokenKind::Define => {
                    // Keep binaries in their place.
                    if index == 0 {
                        self.builder().push_none(tree.len());
                    }
                }
                TokenKind::Int => {
                    let _ = self.cart.interner[token.intern].parse().map(|value: i32| {
                        self.builder().push(Node {
                            nod: Nod::Int32 { value },
                            source: tree.len().into(),
                            typ: Type(0),
                        });
                    });
                }
                TokenKind::Be
                | TokenKind::Comma
                | TokenKind::Comment
                | TokenKind::CurlyClose
                | TokenKind::End
                | TokenKind::Fun
                | TokenKind::HSpace
                | TokenKind::RoundClose
                | TokenKind::RoundOpen
                | TokenKind::Star
                | TokenKind::VSpace => {}
                _ => self.builder().push_at(token, tree.len()),
            },
            _ => todo!(),
        }
        Some(())
    }
}
