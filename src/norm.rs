use std::io::Write;

use crate::{
    Cart, TreeIdx, impl_tree_builder_wrap,
    intern::Intern,
    lex::{Token, TokenKind},
    parse::{ParseBranch, ParseBranchKind, ParseNode, ParseNodeStepper},
    tree::{SizeRange, TreeBuilder, TreeWriter},
};
use anyhow::Result;
use postcard::take_from_bytes;
use serde::{Deserialize, Serialize};

// Unify enum data with enum numbers with supporting struct values.
// Lots of this could be avoided if Rust enums were different.
macro_rules! generate_node_enums {
    ($($variant:ident),*$(,)*) => {
        #[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
        pub enum NodeKind {
            None,
            $(
                $variant,
            )*
        }

        #[derive(Clone, Copy, Debug, Default, Deserialize, Eq, Hash, PartialEq, Serialize)]
        pub enum Node {
            #[allow(unused)]
            #[default]
            None,
            $(
                $variant($variant),
            )*
        }

        $(
            impl NodeData for $variant {
                const KIND: NodeKind = NodeKind::$variant;

                fn as_node(&self) -> Node {
                    Node::$variant(*self)
                }
            }
        )*
    };
}

impl Node {
    pub fn read(bytes: &[u8], offset: usize) -> (Node, usize) {
        if offset == 0 {
            return (Node::None, 0);
        }
        let (node, unused) = take_from_bytes(&bytes[offset..]).unwrap();
        (node, bytes.len() - unused.len())
    }

    pub fn uid(bytes: &[u8], node: Node) -> Option<Uid> {
        match node {
            Node::Def(def) => Node::uid(bytes, Node::read(bytes, def.target).0),
            Node::Uid(uid) => Some(uid),
            _ => None,
        }
    }
}

pub trait NodeData {
    const KIND: NodeKind;

    fn as_node(&self) -> Node;

    fn kind(&self) -> NodeKind {
        Self::KIND
    }
}

pub struct NodeStepper {
    start: usize,
    end: usize,
}

impl NodeStepper {
    pub fn new(range: SizeRange) -> Self {
        Self {
            start: range.start,
            end: range.end,
        }
    }

    pub fn next(&mut self, chunks: &[u8]) -> Option<Node> {
        self.next_idx(chunks).map(|x| x.0)
    }

    pub fn next_idx(&mut self, chunks: &[u8]) -> Option<(Node, usize)> {
        let idx = self.start;
        let (node, offset) = match () {
            _ if self.start < self.end => Node::read(chunks, self.start),
            _ => return None,
        };
        self.start = offset;
        Some((node, idx))
    }
}

generate_node_enums!(Block, Call, Def, Dot, Fun, Structured, Tok, Typed, Uid);

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct NodeMeta {
    source: usize,
    typ: usize,
}

impl NodeMeta {
    pub fn at(source: usize) -> Self {
        Self { source, typ: 0 }
    }
}

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct Block {
    pub meta: NodeMeta,
    pub kids: SizeRange,
}

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct Call {
    pub meta: NodeMeta,
    pub fun: TreeIdx,
    pub args: SizeRange,
}

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct Def {
    pub meta: NodeMeta,
    pub target: TreeIdx,
    pub public: bool,
    /// Explicitly specified type information.
    /// Might be cleared after full typing.
    pub typ: TreeIdx,
    pub value: TreeIdx,
}

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct Dot {
    pub meta: NodeMeta,
    pub scope: TreeIdx,
    pub member: TreeIdx,
}

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct Fun {
    pub meta: NodeMeta,
    pub params: SizeRange,
    pub returning: TreeIdx,
    pub body: TreeIdx,
}

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct Structured {
    pub meta: NodeMeta,
    pub defs: SizeRange,
}

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct Tok {
    pub meta: NodeMeta,
    pub token: Token,
}

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
/// Reserve this just for actual casts.
pub struct Typed {
    pub meta: NodeMeta,
    pub target: TreeIdx,
    pub typ: TreeIdx,
}

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct Uid {
    pub meta: NodeMeta,
    pub intern: Intern,
    pub module: TreeIdx,
    pub num: TreeIdx,
}

#[allow(unused)]
const SIZE_FOR_EASY_VIEW_IN_EDITOR: usize = size_of::<Def>();

pub struct Normer<'a> {
    pub cart: &'a mut Cart,
}

impl<'a> Normer<'a> {
    pub fn new(cart: &'a mut Cart) -> Self {
        Self { cart }
    }

    pub fn norm(&mut self) {
        self.builder().clear();
        let top = TreeBuilder::top_of(&self.cart.bytes);
        let top = self.wrap(|s| s.top(top)).start;
        self.cart.tree_builder.drain_into(&mut self.cart.bytes, top);
    }

    // General helpers

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    fn push(&mut self, node: Node) {
        self.builder().push(node);
    }

    fn read(&self, offset: usize) -> (ParseNode, usize) {
        ParseNode::read(&self.cart.bytes, offset)
    }

    impl_tree_builder_wrap!();

    fn wrap_node(&mut self, node: ParseNode, source: usize) -> usize {
        self.wrap(|s| s.node(node, source)).start
    }

    // Processors

    fn block(&mut self, branch: ParseBranch, source: usize) {
        let mut stepper = ParseNodeStepper::new(branch.range);
        let (open, _) = stepper.next(&self.cart.bytes).unwrap();
        let ParseNode::Leaf(open) = open else {
            panic!()
        };
        match open.kind {
            TokenKind::Also => {}
            TokenKind::Be => self.block_be(stepper, source),
            TokenKind::CurlyOpen => self.structured(stepper, source),
            TokenKind::Of => {}
            TokenKind::RoundOpen => self.round(stepper, source),
            _ => panic!("{open:?}"),
        }
    }

    fn block_be(&mut self, mut stepper: ParseNodeStepper, source: usize) {
        let kids = self.wrap(|s| {
            while let Some((kid, kid_source)) = stepper.next(&s.cart.bytes) {
                if matches!(kid, ParseNode::Leaf(token) if token.kind == TokenKind::End) {
                    // TODO Tagged ends?
                    break;
                }
                s.node(kid, kid_source);
            }
        });
        let block = Block {
            meta: NodeMeta::at(source),
            kids,
        };
        self.push(block.as_node());
    }

    #[allow(dead_code)]
    fn block_of(&mut self, mut stepper: ParseNodeStepper) {
        while let Some((kid, kid_source)) = stepper.next(&self.cart.bytes) {
            if matches!(kid, ParseNode::Leaf(token) if token.kind == TokenKind::End) {
                // TODO Tagged ends?
                break;
            }
            self.node(kid, kid_source);
        }
    }

    fn call(&mut self, branch: ParseBranch, source: usize) {
        let mut stepper = ParseNodeStepper::new(branch.range);
        let fun = stepper
            .next(&self.cart.bytes)
            .map(|(kid, kid_source)| self.wrap_node(kid, kid_source))
            .unwrap_or(0);
        let args = self.wrap(|s| {
            let Some((block_kid, _)) = stepper.next(&s.cart.bytes) else {
                return;
            };
            let ParseNode::Branch(branch) = block_kid else {
                return;
            };
            if branch.kind != ParseBranchKind::Block {
                return;
            }
            let mut stepper = ParseNodeStepper::new(branch.range);
            while let Some((kid, kid_source)) = stepper.next(&s.cart.bytes) {
                match kid {
                    ParseNode::Leaf(Token {
                        kind: TokenKind::RoundClose | TokenKind::RoundOpen,
                        ..
                    }) => {}
                    _ => {
                        s.node(kid, kid_source);
                    }
                }
            }
        });
        let call = Call {
            meta: NodeMeta::at(source),
            fun,
            args,
        };
        self.push(call.as_node());
    }

    fn infix(&mut self, branch: ParseBranch, source: usize) {
        let mut stepper = ParseNodeStepper::new(branch.range);
        let (a, a_source) = stepper.next(&self.cart.bytes).unwrap();
        let (fun, fun_source) = stepper.next(&self.cart.bytes).unwrap();
        // Special-case dot operations.
        if let ParseNode::Leaf(Token {
            kind: TokenKind::Dot,
            ..
        }) = fun
        {
            let a = self.wrap_node(a, a_source);
            let b = match stepper.next(&self.cart.bytes) {
                Some((b, b_source)) => self.wrap_node(b, b_source),
                _ => 0,
            };
            let dot = Dot {
                meta: NodeMeta::at(source),
                scope: a,
                member: b,
            };
            self.push(dot.as_node());
            return;
        }
        // Otherwise treat as calls.
        let Some(fun) = self.cart.core_interns.token_to_intern(fun) else {
            // Presumably only happens on bad macros?
            return;
        };
        let fun = self
            .wrap(|s| {
                let tok = Tok {
                    meta: NodeMeta::at(fun_source),
                    token: Token {
                        kind: TokenKind::Id,
                        intern: fun,
                    },
                };
                s.push(tok.as_node());
            })
            .start;
        let args = self.wrap(|s| {
            s.node(a, a_source);
            if let Some((b, b_source)) = stepper.next(&s.cart.bytes) {
                s.node(b, b_source)
            }
        });
        // TODO Make this into a dot call instead? Except for pair?
        let call = Call {
            meta: NodeMeta::at(source),
            fun,
            args,
        };
        self.push(call.as_node());
    }

    fn def(&mut self, branch: ParseBranch, source: usize) {
        let mut stepper = ParseNodeStepper::new(branch.range);
        let (target, target_source) = stepper.next(&self.cart.bytes).unwrap();
        let mut kid = target;
        let target = match kid {
            ParseNode::Leaf(token) if token.kind == TokenKind::Define => 0,
            _ => {
                let target = self.wrap_node(target, target_source);
                // commit and remember idx
                kid = stepper.next(&self.cart.bytes).unwrap().0;
                target
            }
        };
        let public = matches!(
            kid,
            ParseNode::Leaf(Token {
                kind: TokenKind::DefinePub,
                ..
            })
        );
        let value = stepper
            .next(&self.cart.bytes)
            .map(|(node, source)| self.wrap_node(node, source))
            .unwrap_or(0);
        let def = Def {
            meta: NodeMeta::at(source),
            target,
            public,
            typ: 0,
            value,
        };
        self.push(def.as_node());
    }

    fn fun(&mut self, branch: ParseBranch, source: usize) {
        let mut stepper = ParseNodeStepper::new(branch.range);
        let (kid, _) = stepper.next(&self.cart.bytes).unwrap();
        assert!(matches!(kid, ParseNode::Leaf(token) if token.kind == TokenKind::Fun));
        let mut kiddles = stepper.next(&self.cart.bytes);
        let params = match kiddles {
            Some((kid, _)) => match kid {
                ParseNode::Branch(branch) if branch.kind == ParseBranchKind::Params => {
                    let params = self.wrap(|s| s.params(branch));
                    kiddles = stepper.next(&self.cart.bytes);
                    params
                }
                _ => SizeRange::default(),
            },
            None => SizeRange::default(),
        };
        let returning = match kiddles {
            Some((
                ParseNode::Branch(ParseBranch {
                    kind: ParseBranchKind::Typed,
                    range,
                }),
                _,
            )) => {
                // Special typed handling here for return types.
                let mut kid_stepper = ParseNodeStepper::new(range);
                let returning = match kid_stepper.next(&self.cart.bytes) {
                    Some((kid, kid_source)) => self.wrap_node(kid, kid_source),
                    None => 0,
                };
                kiddles = stepper.next(&self.cart.bytes);
                returning
            }
            _ => 0,
        };
        let body = match kiddles {
            Some((kid, kid_source)) => self.wrap_node(kid, kid_source),
            None => 0,
        };
        let fun = Fun {
            meta: NodeMeta::at(source),
            params,
            returning,
            body,
        };
        self.push(fun.as_node());
    }

    fn node(&mut self, node: ParseNode, source: usize) {
        // dbg!(node);
        match node {
            ParseNode::Branch(branch) => match branch.kind {
                ParseBranchKind::Block => self.block(branch, source),
                ParseBranchKind::Call => self.call(branch, source),
                ParseBranchKind::Infix => self.infix(branch, source),
                ParseBranchKind::Def => self.def(branch, source),
                ParseBranchKind::Params => panic!(),
                ParseBranchKind::Typed => self.typed(branch, source),
                ParseBranchKind::Fun => self.fun(branch, source),
                ParseBranchKind::StringParts => self.string_parts(branch, source),
                ParseBranchKind::TypCall => todo!(),
            },
            ParseNode::Leaf(token) => self.token(token, source),
        }
    }

    fn params(&mut self, branch: ParseBranch) {
        let mut stepper = ParseNodeStepper::new(branch.range);
        while let Some((kid, source)) = stepper.next(&self.cart.bytes) {
            match kid {
                ParseNode::Leaf(Token {
                    kind: TokenKind::RoundOpen | TokenKind::RoundClose,
                    ..
                }) => {}
                ParseNode::Leaf(_) => {
                    let def = Def {
                        meta: NodeMeta::at(source),
                        target: self.wrap_node(kid, source),
                        public: false,
                        typ: 0,
                        value: 0,
                    };
                    self.push(def.as_node());
                }
                ParseNode::Branch(kid) if kid.kind == ParseBranchKind::Typed => {
                    let (target, typ) = self.typed_data(kid);
                    let def = Def {
                        meta: NodeMeta::at(source),
                        target,
                        public: false,
                        typ,
                        // typ: 0,
                        value: 0,
                    };
                    self.push(def.as_node());
                }
                _ => self.node(kid, source),
            }
        }
    }

    fn round(&mut self, mut stepper: ParseNodeStepper, _source: usize) {
        // TODO If empty, store a unit/null/void indicator?
        if let Some((kid, kid_source)) = stepper.next(&self.cart.bytes)
            && !matches!(kid, ParseNode::Leaf(token) if token.kind == TokenKind::RoundClose)
        {
            self.node(kid, kid_source);
        }
    }

    fn string_parts(&mut self, branch: ParseBranch, source: usize) {
        let mut stepper = ParseNodeStepper::new(branch.range);
        let (kid, _) = stepper.next(&self.cart.bytes).unwrap();
        assert!(matches!(kid, ParseNode::Leaf(token) if token.kind == TokenKind::StringEdge));
        while let Some((kid, _)) = stepper.next(&self.cart.bytes) {
            match kid {
                ParseNode::Leaf(token) if token.kind == TokenKind::String => {
                    self.cart
                        .text
                        .push_str(&self.cart.interner.borrow()[token.intern]);
                }
                ParseNode::Leaf(token) if token.kind == TokenKind::StringEscape => {
                    let value = match self.cart.interner.borrow()[token.intern]
                        .chars()
                        .next()
                        .unwrap()
                    {
                        'n' => '\n',
                        ch => ch,
                    };
                    self.cart.text.push(value);
                }
                _ => {}
            }
        }
        let intern = self.cart.interner.borrow_mut().intern(&self.cart.text);
        self.cart.text.clear();
        let tok = Tok {
            meta: NodeMeta::at(source),
            token: Token {
                kind: TokenKind::String,
                intern,
            },
        };
        self.push(tok.as_node());
    }

    fn structured(&mut self, mut stepper: ParseNodeStepper, source: usize) {
        let defs = self.wrap(|s| {
            while let Some((kid, kid_source)) = stepper.next(&s.cart.bytes) {
                if matches!(
                    kid,
                    ParseNode::Leaf(Token {
                        kind: TokenKind::CurlyClose | TokenKind::End,
                        ..
                    })
                ) {
                    // TODO Tagged ends?
                    break;
                }
                match kid {
                    ParseNode::Leaf(_) => {
                        let target = s.wrap_node(kid, kid_source);
                        let def = Def {
                            meta: NodeMeta::at(kid_source),
                            target,
                            public: false,
                            typ: 0,
                            value: target,
                        };
                        s.push(def.as_node());
                    }
                    ParseNode::Branch(branch) if branch.kind != ParseBranchKind::Def => {
                        // TODO Dig into dots and calls. Easier later in normed mode pass?
                        s.node(kid, kid_source)
                    }
                    _ => s.node(kid, kid_source),
                }
            }
        });
        let structured = Structured {
            meta: NodeMeta::at(source),
            defs,
        };
        self.push(structured.as_node());
    }

    fn top(&mut self, source: usize) {
        let ParseNode::Branch(branch) = self.read(source).0 else {
            panic!()
        };
        let kids = self.wrap(|s| {
            let mut stepper = ParseNodeStepper::new(branch.range);
            while let Some((kid, source)) = stepper.next(&s.cart.bytes) {
                s.node(kid, source);
            }
        });
        let block = Block {
            meta: NodeMeta::at(source),
            kids,
        };
        self.push(block.as_node());
    }

    fn token(&mut self, token: Token, source: usize) {
        let tok = Tok {
            token,
            meta: NodeMeta::at(source),
        };
        // Go straight to builder to avoid extra kind chunk.
        self.builder().push(tok.as_node());
    }

    fn typed(&mut self, branch: ParseBranch, source: usize) {
        // println!("typed");
        let (target, typ) = self.typed_data(branch);
        let node = Typed {
            meta: NodeMeta::at(source),
            target,
            typ,
        };
        self.push(node.as_node());
    }

    /// Returns the node instead of pushing it.
    fn typed_data(&mut self, branch: ParseBranch) -> (TreeIdx, TreeIdx) {
        let mut stepper = ParseNodeStepper::new(branch.range);
        let (target, target_source) = stepper.next(&self.cart.bytes).unwrap();
        let target = self.wrap_node(target, target_source);
        let typ = stepper
            .next(&self.cart.bytes)
            .map(|(node, source)| self.wrap_node(node, source))
            .unwrap_or(0);
        (target, typ)
    }
}

pub fn write_tree<File, Map>(writer: &mut TreeWriter<'_, File, Map>) -> Result<()>
where
    File: Write,
    Map: std::ops::Index<Intern, Output = str>,
{
    let bytes = writer.bytes;
    let (top, end) = Node::read(bytes, TreeBuilder::top_of(bytes));
    assert_eq!(bytes.len(), { end });
    let context = TreeWriting {
        node: top,
        ..Default::default()
    };
    write_tree_at(writer, context)?;
    Ok(())
}

#[derive(Default)]
pub struct TreeWriting {
    indent: usize,
    inline: bool,
    label: &'static str,
    node: Node,
}

/// Returns the number of lines written.
pub fn write_tree_at<File, Map>(
    writer: &mut TreeWriter<'_, File, Map>,
    context: TreeWriting,
) -> Result<usize>
where
    File: Write,
    Map: std::ops::Index<Intern, Output = str>,
{
    if matches!(context.node, Node::None) {
        return Ok(0);
    }
    let mut result = 1usize;
    if !context.inline {
        writer.indent(context.indent)?;
    }
    if !context.label.is_empty() {
        write!(writer.file, "{} = ", context.label)?;
    }
    let indented = TreeWriting {
        indent: context.indent + writer.indent,
        ..Default::default()
    };
    let write_range = |writer: &mut TreeWriter<'_, File, Map>,
                       label: &'static str,
                       range: SizeRange|
     -> Result<usize> {
        let mut result = 0;
        let range_kind = match range.start {
            0 => "None",
            _ => "Range",
        };
        writer.indent(context.indent + writer.indent)?;
        writeln!(writer.file, "{label} = {range_kind}")?;
        let indented_more = TreeWriting {
            indent: indented.indent + writer.indent,
            ..indented
        };
        let mut count = 0;
        let mut stepper = NodeStepper::new(range);
        while let Some(node) = stepper.next(writer.bytes) {
            count += write_tree_at(
                writer,
                TreeWriting {
                    node,
                    ..indented_more
                },
            )?;
        }
        result += count;
        if count > 1 {
            writer.indent(indented.indent)?;
            writeln!(writer.file, "/{range_kind}")?;
            result += 1;
        }
        Ok(result)
    };
    match context.node {
        Node::None => writeln!(writer.file, "{:?}", NodeKind::None)?,
        Node::Block(block) => {
            writeln!(writer.file, "{:?}", block.kind())?;
            let mut count = 0;
            let mut stepper = NodeStepper::new(block.kids);
            while let Some(node) = stepper.next(writer.bytes) {
                count += write_tree_at(writer, TreeWriting { node, ..indented })?;
            }
            result += count;
            if count > 1 {
                writer.indent(context.indent)?;
                writeln!(writer.file, "/{:?}", block.kind())?;
                result += 1;
            }
        }
        Node::Call(call) => {
            writeln!(writer.file, "{:?}", call.kind())?;
            result += write_tree_at(
                writer,
                TreeWriting {
                    label: "fun",
                    node: Node::read(writer.bytes, call.fun).0,
                    ..indented
                },
            )?;
            result += write_range(writer, "args", call.args)?;
            // Close
            writer.indent(context.indent)?;
            writeln!(writer.file, "/{:?}", call.kind())?;
        }
        Node::Dot(dot) => {
            writeln!(writer.file, "{:?}", dot.kind())?;
            result += write_tree_at(
                writer,
                TreeWriting {
                    label: "scope",
                    node: Node::read(writer.bytes, dot.scope).0,
                    ..indented
                },
            )?;
            result += write_tree_at(
                writer,
                TreeWriting {
                    label: "member",
                    node: Node::read(writer.bytes, dot.member).0,
                    ..indented
                },
            )?;
            writer.indent(context.indent)?;
            writeln!(writer.file, "/{:?}", dot.kind())?;
            result += 1;
        }
        Node::Def(def) => {
            writeln!(writer.file, "{:?}", def.kind())?;
            result += write_tree_at(
                writer,
                TreeWriting {
                    label: "target",
                    node: Node::read(writer.bytes, def.target).0,
                    ..indented
                },
            )?;
            if def.public {
                writer.indent(indented.indent)?;
                writeln!(writer.file, "public = true")?;
            }
            result += write_tree_at(
                writer,
                TreeWriting {
                    label: "type",
                    node: Node::read(writer.bytes, def.typ).0,
                    ..indented
                },
            )?;
            result += write_tree_at(
                writer,
                TreeWriting {
                    label: "value",
                    node: Node::read(writer.bytes, def.value).0,
                    ..indented
                },
            )?;
            writer.indent(context.indent)?;
            writeln!(writer.file, "/{:?}", def.kind())?;
            result += 1;
        }
        Node::Fun(fun) => {
            writeln!(writer.file, "{:?}", fun.kind())?;
            // Params
            result += write_range(writer, "params", fun.params)?;
            // Returning
            write_tree_at(
                writer,
                TreeWriting {
                    label: "returning",
                    node: Node::read(writer.bytes, fun.returning).0,
                    ..indented
                },
            )?;
            result += 2;
            // Body
            write_tree_at(
                writer,
                TreeWriting {
                    label: "body",
                    node: Node::read(writer.bytes, fun.body).0,
                    ..indented
                },
            )?;
            // Close
            writer.indent(context.indent)?;
            writeln!(writer.file, "/{:?}", fun.kind())?;
        }
        Node::Structured(structured) => {
            writeln!(writer.file, "{:?}", structured.kind())?;
            let mut count = 0;
            let mut stepper = NodeStepper::new(structured.defs);
            while let Some(node) = stepper.next(writer.bytes) {
                count += write_tree_at(writer, TreeWriting { node, ..indented })?;
            }
            result += count;
            if count > 1 {
                writer.indent(context.indent)?;
                writeln!(writer.file, "/{:?}", structured.kind())?;
                result += 1;
            }
        }
        Node::Tok(tok) => writeln!(
            writer.file,
            "{:?}: {:?}",
            tok.token.kind, &writer.map[tok.token.intern]
        )?,
        Node::Typed(typed) => {
            writeln!(writer.file, "{:?}", typed.kind())?;
            result += write_tree_at(
                writer,
                TreeWriting {
                    label: "target",
                    node: Node::read(writer.bytes, typed.target).0,
                    ..indented
                },
            )?;
            result += write_tree_at(
                writer,
                TreeWriting {
                    label: "type",
                    node: Node::read(writer.bytes, typed.typ).0,
                    ..indented
                },
            )?;
            writer.indent(context.indent)?;
            writeln!(writer.file, "/{:?}", typed.kind())?;
            result += 1;
        }
        Node::Uid(uid) => {
            writeln!(
                writer.file,
                "{:?} {}/{}: {:?}",
                uid.kind(),
                uid.module,
                uid.num,
                &writer.map[uid.intern]
            )?;
            result += 1;
        }
    }
    Ok(result)
}
