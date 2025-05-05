use std::io::Write;

use crate::{
    Cart,
    lex::{Intern, Token, TokenKind},
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
        enum NodeKind {
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
}

trait NodeData {
    const KIND: NodeKind;

    fn as_node(&self) -> Node;

    fn kind(&self) -> NodeKind {
        Self::KIND
    }
}

generate_node_enums!(Block, Call, Def, Fun, Public, Tok, Typed);

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
    meta: NodeMeta,
    kids: SizeRange,
}

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct Call {
    meta: NodeMeta,
    fun: usize,
    args: SizeRange,
}

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct Def {
    meta: NodeMeta, // typ eventually implies type
    target: usize,  // uid eventually implies pub
    value: usize,
}

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct Fun {
    meta: NodeMeta,
    params: SizeRange,
    returning: usize,
    body: usize,
}

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct Public {
    meta: NodeMeta,
    kid: usize,
}

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct Tok {
    meta: NodeMeta,
    token: Token,
}

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct Typed {
    meta: NodeMeta,
    target: usize,
    typ: usize,
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
        let source = TreeBuilder::top_of(&self.cart.tree);
        // Finish top and drain tree.
        let bytes_top = self.wrap(|s| s.top(source)).start;
        self.cart
            .tree_builder
            .drain_into(&mut self.cart.tree, bytes_top);
    }

    // General helpers

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    fn push(&mut self, node: Node) {
        self.builder().push(node);
    }

    fn read(&self, offset: usize) -> (ParseNode, usize) {
        ParseNode::read(&self.cart.tree, offset)
    }

    fn wrap<F: FnOnce(&mut Self)>(&mut self, build: F) -> SizeRange {
        let start = self.builder().pos();
        build(self);
        let result = match () {
            _ if self.builder().pos() == start => SizeRange::default(),
            _ => {
                let range = self.builder().apply_range(start);
                range
            }
        };
        result
    }

    fn wrap_node(&mut self, node: ParseNode, source: usize) -> usize {
        self.wrap(|s| s.node(node, source)).start
    }

    // Processors

    fn block(&mut self, branch: ParseBranch, source: usize) {
        let mut stepper = ParseNodeStepper::new(branch.range);
        let (open, _) = stepper.next(&self.cart.tree).unwrap();
        let ParseNode::Leaf(open) = open else {
            panic!()
        };
        match open.kind {
            TokenKind::Also => {}
            TokenKind::Be => self.block_be(stepper, source),
            TokenKind::CurlyOpen | TokenKind::With => {}
            TokenKind::Of => {}
            TokenKind::RoundOpen => self.round(stepper, source),
            _ => panic!("{open:?}"),
        }
    }

    fn block_be(&mut self, mut stepper: ParseNodeStepper, source: usize) {
        let kids = self.wrap(|s| {
            while let Some((kid, kid_source)) = stepper.next(&s.cart.tree) {
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

    fn block_of(&mut self, mut stepper: ParseNodeStepper) {
        while let Some((kid, kid_source)) = stepper.next(&self.cart.tree) {
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
            .next(&self.cart.tree)
            .map(|(kid, kid_source)| self.wrap_node(kid, kid_source))
            .unwrap_or(0);
        let args = self.wrap(|s| {
            while let Some((kid, kid_source)) = stepper.next(&s.cart.tree) {
                match kid {
                    ParseNode::Branch(branch) if branch.kind == ParseBranchKind::Block => {
                        let mut stepper = ParseNodeStepper::new(branch.range);
                        let (open, _) = stepper.next(&s.cart.tree).unwrap();
                        let ParseNode::Leaf(open) = open else {
                            panic!()
                        };
                        match open.kind {
                            TokenKind::Also | TokenKind::Of => s.block_of(stepper),
                            _ => s.node(kid, kid_source),
                        }
                    }
                    _ => s.node(kid, kid_source),
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

    fn def(&mut self, branch: ParseBranch, source: usize) {
        let mut stepper = ParseNodeStepper::new(branch.range);
        let (target, target_source) = stepper.next(&self.cart.tree).unwrap();
        let mut kid = target;
        let target = match kid {
            ParseNode::Leaf(token) if token.kind == TokenKind::Define => 0,
            _ => {
                let target = self.wrap_node(target, target_source);
                // commit and remember idx
                kid = stepper.next(&self.cart.tree).unwrap().0;
                target
            }
        };
        assert!(matches!(kid, ParseNode::Leaf(token) if token.kind == TokenKind::Define));
        let value = stepper
            .next(&self.cart.tree)
            .map(|(node, source)| self.wrap_node(node, source))
            .unwrap_or(0);
        let def = Def {
            meta: NodeMeta::at(source),
            target,
            value,
        };
        self.push(def.as_node());
    }

    fn fun(&mut self, branch: ParseBranch, source: usize) {
        let mut stepper = ParseNodeStepper::new(branch.range);
        let (kid, _) = stepper.next(&self.cart.tree).unwrap();
        assert!(matches!(kid, ParseNode::Leaf(token) if token.kind == TokenKind::Fun));
        let mut kiddles = stepper.next(&self.cart.tree);
        let mut returning = 0usize;
        let params = match kiddles {
            Some((kid, _)) => match kid {
                ParseNode::Branch(branch) if branch.kind == ParseBranchKind::Params => {
                    let params = self.wrap(|s| returning = s.params(branch));
                    kiddles = stepper.next(&self.cart.tree);
                    params
                }
                _ => SizeRange::default(),
            },
            None => SizeRange::default(),
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
                ParseBranchKind::Infix => {}
                ParseBranchKind::Def => self.def(branch, source),
                ParseBranchKind::Params => panic!(), // handled elsewhere
                ParseBranchKind::Typed => self.typed(branch, source),
                ParseBranchKind::Fun => self.fun(branch, source),
                ParseBranchKind::Pub => self.public(branch, source),
                ParseBranchKind::StringParts => self.string_parts(branch, source),
            },
            ParseNode::Leaf(token) => self.token(token, source),
        }
    }

    /// Returns the index for the return type, possibly 0.
    fn params(&mut self, branch: ParseBranch) -> usize {
        let mut stepper = ParseNodeStepper::new(branch.range);
        // TODO More syntax/nesting for params vs return type?
        let mut last: Option<Typed> = None;
        while let Some((kid, source)) = stepper.next(&self.cart.tree) {
            if let Some(last) = last.take() {
                self.push(last.as_node());
            }
            match kid {
                ParseNode::Branch(kid) if kid.kind == ParseBranchKind::Typed => {
                    let kid = self.typed_data(kid, source);
                    match kid.target {
                        0 => last = Some(kid),
                        _ => self.push(kid.as_node()),
                    }
                }
                _ => self.node(kid, source),
            }
        }
        last.map(|last| last.typ).unwrap_or(0)
    }

    fn public(&mut self, branch: ParseBranch, source: usize) {
        let mut stepper = ParseNodeStepper::new(branch.range);
        let (kid, kid_source) = stepper.next(&self.cart.tree).unwrap();
        let kid = match kid {
            ParseNode::Leaf(token) if token.kind == TokenKind::Star => 0,
            _ => self.wrap_node(kid, kid_source),
        };
        let public = Public {
            meta: NodeMeta::at(source),
            kid,
        };
        self.push(public.as_node());
    }

    fn round(&mut self, mut stepper: ParseNodeStepper, _source: usize) {
        // TODO If empty, store a unit/null/void indicator?
        if let Some((kid, kid_source)) = stepper.next(&self.cart.tree) {
            if !matches!(kid, ParseNode::Leaf(token) if token.kind == TokenKind::RoundClose) {
                self.node(kid, kid_source);
            }
        }
    }

    fn string_parts(&mut self, branch: ParseBranch, source: usize) {
        let mut stepper = ParseNodeStepper::new(branch.range);
        let (kid, _) = stepper.next(&self.cart.tree).unwrap();
        assert!(matches!(kid, ParseNode::Leaf(token) if token.kind == TokenKind::StringEdge));
        while let Some((kid, _)) = stepper.next(&self.cart.tree) {
            match kid {
                ParseNode::Leaf(token) if token.kind == TokenKind::String => {
                    self.cart.text.push_str(&self.cart.interner[token.intern]);
                }
                ParseNode::Leaf(token) if token.kind == TokenKind::StringEscape => {
                    let value = match self.cart.interner[token.intern].chars().next().unwrap() {
                        'n' => '\n',
                        ch => ch,
                    };
                    self.cart.text.push(value);
                }
                _ => {}
            }
        }
        let intern = self.cart.interner.get_or_intern(&self.cart.text);
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

    fn top(&mut self, source: usize) {
        let ParseNode::Branch(branch) = self.read(source).0 else {
            panic!()
        };
        let kids = self.wrap(|s| {
            let mut stepper = ParseNodeStepper::new(branch.range);
            while let Some((kid, source)) = stepper.next(&s.cart.tree) {
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
        let node = self.typed_data(branch, source);
        self.push(node.as_node());
    }

    /// Returns the node instead of pushing it.
    fn typed_data(&mut self, branch: ParseBranch, source: usize) -> Typed {
        let mut stepper = ParseNodeStepper::new(branch.range);
        let (target, target_source) = stepper.next(&self.cart.tree).unwrap();
        let mut kid = target;
        let target = match kid {
            ParseNode::Leaf(token) if token.kind == TokenKind::Colon => 0,
            _ => {
                let target = self.wrap_node(target, target_source);
                // commit and remember idx
                kid = stepper.next(&self.cart.tree).unwrap().0;
                target
            }
        };
        assert!(matches!(kid, ParseNode::Leaf(token) if token.kind == TokenKind::Colon));
        let typ = stepper
            .next(&self.cart.tree)
            .map(|(node, source)| self.wrap_node(node, source))
            .unwrap_or(0);
        Typed {
            meta: NodeMeta::at(source),
            target,
            typ,
        }
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
        let params_start = match range.start {
            0 => "None",
            _ => "Range",
        };
        writer.indent(context.indent + writer.indent)?;
        writeln!(writer.file, "{label} = {params_start}")?;
        let indented_more = TreeWriting {
            indent: indented.indent + writer.indent,
            ..indented
        };
        let mut offset = range.start;
        let mut count = 0;
        while offset < range.end {
            let (node, next_offset) = Node::read(writer.bytes, offset);
            count += write_tree_at(
                writer,
                TreeWriting {
                    node,
                    ..indented_more
                },
            )?;
            offset = next_offset;
        }
        result += count;
        if count > 1 {
            writer.indent(indented.indent)?;
            writeln!(writer.file, "/{params_start}")?;
            result += 1;
        }
        Ok(result)
    };
    match context.node {
        Node::None => writeln!(writer.file, "{:?}", NodeKind::None)?,
        Node::Block(block) => {
            writeln!(writer.file, "{:?}", block.kind())?;
            let range = block.kids;
            let mut offset = range.start;
            let mut count = 0;
            while offset < range.end {
                let (node, next_offset) = Node::read(writer.bytes, offset);
                count += write_tree_at(writer, TreeWriting { node, ..indented })?;
                offset = next_offset;
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
        Node::Public(public) => {
            write!(writer.file, "{:?} ", public.kind())?;
            result += write_tree_at(
                writer,
                TreeWriting {
                    inline: true,
                    node: Node::read(writer.bytes, public.kid).0,
                    ..indented
                },
            )? - 1;
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
    }
    Ok(result)
}
