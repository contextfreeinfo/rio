use std::io::Write;

use crate::{
    Cart,
    lex::{Intern, TOKEN_KIND_END, TOKEN_KIND_START, Token, TokenKind},
    parse::{PARSE_BRANCH_KIND_END, ParseBranch, ParseBranchKind, ParseNode, ParseNodeStepper},
    tree::{CHUNK_SIZE, Chunk, Size, SizeRange, TreeBuilder, TreeWriter},
};
use anyhow::Result;
use num_derive::FromPrimitive;
use num_traits::FromPrimitive;
use static_assertions::{const_assert, const_assert_eq};
use strum::EnumCount;

// Unify enum data with enum numbers with supporting struct values.
// Lots of this could be avoided if Rust enums were different.
macro_rules! generate_node_enums {
    ($($variant:ident),*$(,)*) => {
        #[repr(u32)]
        #[derive(Clone, Copy, Debug, EnumCount, FromPrimitive, Eq, Hash, PartialEq)]
        enum NodeKind {
            None = NODE_KIND_START,
            $(
                $variant,
            )*
        }

        #[derive(Clone, Copy, Debug, Default, EnumCount, Eq, Hash, PartialEq)]
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
                const CHUNK_SIZE: Size = (size_of::<$variant>() / CHUNK_SIZE) as Size;
                const KIND: NodeKind = NodeKind::$variant;
            }
        )*
    };
}

macro_rules! read_node {
    ($Ty:ident, $chunks:expr, $offset:expr) => {{
        assert!($chunks.len() >= $Ty::CHUNK_SIZE as usize);
        let data = unsafe {
            let ptr = $chunks.as_ptr() as *const $Ty;
            std::ptr::read(ptr)
        };
        // Access enum and struct impl both.
        (Node::$Ty(data), $offset + $Ty::CHUNK_SIZE + 1)
    }};
}

impl Node {
    pub fn read(chunks: &[Chunk], offset: Size) -> (Node, Size) {
        if offset == 0 {
            return (Node::None, 0);
        }
        let chunks = &chunks[offset as usize..];
        let kind = chunks[0];
        match kind {
            NODE_KIND_START..NODE_KIND_END => {
                let chunks = &chunks[1..];
                match NodeKind::from_u32(kind).unwrap() {
                    NodeKind::None => panic!(),
                    NodeKind::Block => read_node!(Block, chunks, offset),
                    NodeKind::Call => read_node!(Call, chunks, offset),
                    NodeKind::Def => read_node!(Def, chunks, offset),
                    NodeKind::Fun => read_node!(Fun, chunks, offset),
                    NodeKind::Public => read_node!(Public, chunks, offset),
                    NodeKind::Tok => read_node!(Tok, chunks, offset),
                }
            }
            TOKEN_KIND_START..TOKEN_KIND_END => {
                // dbg!(&chunks[..Tok::CHUNK_SIZE as usize]);
                read_node!(Tok, chunks, offset)
            }
            _ => panic!(),
        }
    }
}

trait NodeData {
    const CHUNK_SIZE: Size;
    const KIND: NodeKind;
    fn kind(&self) -> NodeKind {
        Self::KIND
    }
}

// const NODE_SIZE: usize = size_of::<Node>() / CHUNK_SIZE;
const NODE_KIND_START: Size = 0x2000 as Size;
const NODE_KIND_END: Size = NODE_KIND_START + NodeKind::COUNT as Size;
const_assert!(NODE_KIND_START >= PARSE_BRANCH_KIND_END);
const_assert_eq!(CHUNK_SIZE, align_of::<Node>());

generate_node_enums!(Block, Call, Def, Fun, Public, Tok,);

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, Eq, Hash, PartialEq)]
pub struct NodeMeta {
    source: Size,
    typ: Size,
}

impl NodeMeta {
    pub fn at(source: Size) -> Self {
        Self { source, typ: 0 }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, Eq, Hash, PartialEq)]
pub struct Block {
    meta: NodeMeta,
    kids: SizeRange,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, Eq, Hash, PartialEq)]
pub struct Call {
    meta: NodeMeta,
    fun: Size,
    args: SizeRange,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, Eq, Hash, PartialEq)]
pub struct Def {
    meta: NodeMeta, // typ eventually implies type
    target: Size,   // uid eventually implies pub
    value: Size,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, Eq, Hash, PartialEq)]
pub struct Fun {
    meta: NodeMeta,
    params: SizeRange,
    return_type: Size,
    body: Size,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, Eq, Hash, PartialEq)]
pub struct Public {
    meta: NodeMeta,
    kid: Size,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, Eq, Hash, PartialEq)]
pub struct Tok {
    // Token first so we can use the builtin kind.
    token: Token,
    meta: NodeMeta,
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
        self.wrap(|s| s.top(*s.chunks().last().unwrap()));
        self.cart.tree.clear();
        self.cart.tree.append(&mut self.cart.tree_builder.chunks);
        // dbg!(&self.cart.tree);
    }

    // General helpers

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    fn chunks(&self) -> &[Chunk] {
        &self.cart.tree
    }

    fn push<T: NodeData>(&mut self, node: T) {
        self.builder().push(T::KIND);
        self.builder().push(node);
    }

    fn read(&self, offset: Size) -> (ParseNode, Size) {
        let pair = ParseNode::read(self.chunks(), offset as usize);
        // TODO Let ParseNode::read work with Size directly?
        (pair.0, pair.1.try_into().unwrap())
    }

    fn wrap<F: FnOnce(&mut Self)>(&mut self, build: F) -> SizeRange {
        let start = self.builder().pos();
        build(self);
        self.builder().apply_range(start)
    }

    fn wrap_node(&mut self, node: ParseNode, source: Size) -> SizeRange {
        self.wrap(|s| s.node(node, source))
    }

    // Processors

    fn def(&mut self, branch: ParseBranch, source: Size) {
        let mut stepper = ParseNodeStepper::new(branch.range);
        let (target, target_source) = stepper.next(self.chunks()).unwrap();
        let mut kid = target;
        let target = match kid {
            ParseNode::Leaf(token) if token.kind == TokenKind::Define => 0,
            _ => {
                let target = self.wrap_node(target, target_source).start;
                // commit and remember idx
                kid = stepper.next(self.chunks()).unwrap().0;
                target
            }
        };
        assert!(matches!(kid, ParseNode::Leaf(token) if token.kind == TokenKind::Define));
        let value = stepper
            .next(self.chunks())
            .map(|(node, source)| self.wrap_node(node, source).start)
            .unwrap_or(0);
        let def = Def {
            meta: NodeMeta::at(source),
            target,
            value,
        };
        self.push(def);
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
                ParseBranchKind::Fun => {}
                ParseBranchKind::Pub => self.public(branch, source),
                ParseBranchKind::StringParts => {}
            },
            ParseNode::Leaf(token) => self.token(token, source),
        }
    }

    fn public(&mut self, branch: ParseBranch, source: u32) {
        let mut stepper = ParseNodeStepper::new(branch.range);
        let (kid, kid_source) = stepper.next(self.chunks()).unwrap();
        let kid = match kid {
            ParseNode::Leaf(token) if token.kind == TokenKind::Star => 0,
            _ => self.wrap_node(kid, kid_source).start,
        };
        let public = Public {
            meta: NodeMeta::at(source),
            kid,
        };
        self.push(public);
    }

    fn top(&mut self, source: Size) {
        let ParseNode::Branch(branch) = self.read(source).0 else {
            panic!()
        };
        let kids = self.wrap(|s| {
            let mut stepper = ParseNodeStepper::new(branch.range);
            while let Some((kid, source)) = stepper.next(s.chunks()) {
                s.node(kid, source);
            }
        });
        let block = Block {
            meta: NodeMeta::at(source),
            kids,
        };
        self.push(block);
    }

    fn token(&mut self, token: Token, source: Size) {
        let tok = Tok {
            token,
            meta: NodeMeta::at(source),
        };
        // Go straight to builder to avoid extra kind chunk.
        self.builder().push(tok);
    }
}

pub fn write_tree<File, Map>(writer: &mut TreeWriter<'_, File, Map>) -> Result<()>
where
    File: Write,
    Map: std::ops::Index<Intern, Output = str>,
{
    let chunks = writer.chunks;
    let (top, end) = Node::read(chunks, *chunks.last().unwrap());
    assert_eq!(chunks.len(), end as usize);
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
                let (node, next_offset) = Node::read(writer.chunks, offset);
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
                    node: Node::read(writer.chunks, def.target).0,
                    ..indented
                },
            )?;
            write_tree_at(
                writer,
                TreeWriting {
                    label: "value",
                    node: Node::read(writer.chunks, def.value).0,
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
                    node: Node::read(writer.chunks, public.kid).0,
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
