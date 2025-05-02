use std::io::Write;

use crate::{
    Cart,
    lex::{Intern, Token, TokenKind},
    parse::{ParseBranch, ParseBranchKind, ParseNode, ParseNodeStepper},
    tree::{Size, SizeRange, TreeBuilder, TreeWriter},
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
            }
        )*
    };
}

impl Node {
    pub fn read(bytes: &[u8], offset: Size) -> (Node, Size) {
        if offset == 0 {
            return (Node::None, 0);
        }
        let (node, unused) = take_from_bytes(&bytes[offset as usize..]).unwrap();
        (node, (bytes.len() - unused.len()).try_into().unwrap())
    }
}

trait NodeData {
    const KIND: NodeKind;
    fn kind(&self) -> NodeKind {
        Self::KIND
    }
}

generate_node_enums!(Block, Call, Def, Fun, Public, Tok,);

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct NodeMeta {
    source: Size,
    typ: Size,
}

impl NodeMeta {
    pub fn at(source: Size) -> Self {
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
    fun: Size,
    args: SizeRange,
}

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct Def {
    meta: NodeMeta, // typ eventually implies type
    target: Size,   // uid eventually implies pub
    value: Size,
}

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct Fun {
    meta: NodeMeta,
    params: SizeRange,
    return_type: Size,
    body: Size,
}

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct Public {
    meta: NodeMeta,
    kid: Size,
}

#[derive(Clone, Copy, Debug, Deserialize, Default, Eq, Hash, PartialEq, Serialize)]
pub struct Tok {
    meta: NodeMeta,
    token: Token,
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
        let source = TreeBuilder::top_of(&self.cart.tree).try_into().unwrap();
        // Finish top and drain tree.
        let bytes_top = self.wrap(|s| s.top(source)).start;
        self.cart
            .tree_builder
            .drain_into(&mut self.cart.tree, bytes_top.try_into().unwrap());
    }

    // General helpers

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    fn push(&mut self, node: Node) {
        self.builder().push(node);
    }

    fn read(&self, offset: Size) -> (ParseNode, Size) {
        let pair = ParseNode::read(&self.cart.tree, offset as usize);
        // TODO Let ParseNode::read work with Size directly?
        (pair.0, pair.1.try_into().unwrap())
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

    fn wrap_node(&mut self, node: ParseNode, source: Size) -> SizeRange {
        self.wrap(|s| s.node(node, source))
    }

    // Processors

    fn def(&mut self, branch: ParseBranch, source: Size) {
        let mut stepper = ParseNodeStepper::new(branch.range);
        let (target, target_source) = stepper.next(&self.cart.tree).unwrap();
        let mut kid = target;
        let target = match kid {
            ParseNode::Leaf(token) if token.kind == TokenKind::Define => 0,
            _ => {
                let target = self.wrap_node(target, target_source).start;
                // commit and remember idx
                kid = stepper.next(&self.cart.tree).unwrap().0;
                target
            }
        };
        assert!(matches!(kid, ParseNode::Leaf(token) if token.kind == TokenKind::Define));
        let value = stepper
            .next(&self.cart.tree)
            .map(|(node, source)| self.wrap_node(node, source).start)
            .unwrap_or(0);
        let def = Def {
            meta: NodeMeta::at(source),
            target,
            value,
        };
        self.push(Node::Def(def));
    }

    fn fun(&mut self, branch: ParseBranch, source: Size) {
        let mut stepper = ParseNodeStepper::new(branch.range);
        let (kid, kid_source) = stepper.next(&self.cart.tree).unwrap();
        assert!(matches!(kid, ParseNode::Leaf(token) if token.kind == TokenKind::Fun));
        // TODO
    }

    fn node(&mut self, node: ParseNode, source: Size) {
        // dbg!(node);
        match node {
            ParseNode::Branch(branch) => match branch.kind {
                ParseBranchKind::Block => {}
                ParseBranchKind::Call => {}
                ParseBranchKind::Infix => {}
                ParseBranchKind::Def => self.def(branch, source),
                ParseBranchKind::Params => {}
                ParseBranchKind::Typed => {}
                ParseBranchKind::Fun => self.fun(branch, source),
                ParseBranchKind::Pub => self.public(branch, source),
                ParseBranchKind::StringParts => {}
            },
            ParseNode::Leaf(token) => self.token(token, source),
        }
    }

    fn public(&mut self, branch: ParseBranch, source: u32) {
        let mut stepper = ParseNodeStepper::new(branch.range);
        let (kid, kid_source) = stepper.next(&self.cart.tree).unwrap();
        let kid = match kid {
            ParseNode::Leaf(token) if token.kind == TokenKind::Star => 0,
            _ => self.wrap_node(kid, kid_source).start,
        };
        let public = Public {
            meta: NodeMeta::at(source),
            kid,
        };
        self.push(Node::Public(public));
    }

    fn top(&mut self, source: Size) {
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
        self.push(Node::Block(block));
    }

    fn token(&mut self, token: Token, source: Size) {
        let tok = Tok {
            token,
            meta: NodeMeta::at(source),
        };
        // Go straight to builder to avoid extra kind chunk.
        self.builder().push(Node::Tok(tok));
    }
}

pub fn write_tree<File, Map>(writer: &mut TreeWriter<'_, File, Map>) -> Result<()>
where
    File: Write,
    Map: std::ops::Index<Intern, Output = str>,
{
    let bytes = writer.bytes;
    let (top, end) = Node::read(bytes, TreeBuilder::top_of(bytes).try_into().unwrap());
    assert_eq!(bytes.len(), end as usize);
    let context = TreeWriting {
        node: top,
        ..Default::default()
    };
    write_tree_at(writer, context)
}

#[derive(Default)]
pub struct TreeWriting {
    indent: usize,
    inline: bool,
    label: &'static str,
    node: Node,
}

pub fn write_tree_at<File, Map>(
    writer: &mut TreeWriter<'_, File, Map>,
    context: TreeWriting,
) -> Result<()>
where
    File: Write,
    Map: std::ops::Index<Intern, Output = str>,
{
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
    match context.node {
        Node::None => writeln!(writer.file, "{:?}", NodeKind::None)?,
        Node::Block(block) => {
            writeln!(writer.file, "{:?}", block.kind())?;
            let range = block.kids;
            let mut offset = range.start;
            let mut count = 0;
            while offset < range.end {
                let (node, next_offset) = Node::read(writer.bytes, offset);
                write_tree_at(writer, TreeWriting { node, ..indented })?;
                offset = next_offset;
                count += 1;
            }
            if count > 1 {
                writer.indent(context.indent)?;
                writeln!(writer.file, "/{:?}", block.kind())?;
            }
        }
        Node::Call(call) => {
            writeln!(writer.file, "{:?}", Call::KIND)?;
        }
        Node::Def(def) => {
            writeln!(writer.file, "{:?}", Def::KIND)?;
            write_tree_at(
                writer,
                TreeWriting {
                    label: "target",
                    node: Node::read(writer.bytes, def.target).0,
                    ..indented
                },
            )?;
            write_tree_at(
                writer,
                TreeWriting {
                    label: "value",
                    node: Node::read(writer.bytes, def.value).0,
                    ..indented
                },
            )?;
            writer.indent(context.indent)?;
            writeln!(writer.file, "/{:?}", def.kind())?;
        }
        Node::Fun(fun) => {
            writeln!(writer.file, "{:?}", Fun::KIND)?;
        }
        Node::Public(public) => {
            write!(writer.file, "{:?} ", Public::KIND)?;
            write_tree_at(
                writer,
                TreeWriting {
                    inline: true,
                    node: Node::read(writer.bytes, public.kid).0,
                    ..indented
                },
            )?;
        }
        Node::Tok(tok) => writeln!(
            writer.file,
            "{:?}: {:?}",
            tok.token.kind, &writer.map[tok.token.intern]
        )?,
    }
    Ok(())
}
