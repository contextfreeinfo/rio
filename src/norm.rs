use crate::{
    Cart,
    lex::{Token, TokenKind},
    parse::{PARSE_BRANCH_KIND_END, ParseBranch, ParseBranchKind, ParseNode, ParseNodeStepper},
    tree::{Chunk, Size, SizeRange, TreeBuilder},
};
use num_derive::FromPrimitive;
use static_assertions::const_assert;
use strum::EnumCount;

macro_rules! generate_node_enums {
    // Match the input pattern: list of variants and their corresponding discriminants
    ($($variant:ident),*$(,)*) => {
        #[repr(u32)]
        #[derive(Clone, Copy, Debug, EnumCount, FromPrimitive, Eq, Hash, PartialEq)]
        enum NodeKind {
            None = NODE_KIND_START,
            $(
                $variant,
            )*
        }

        #[derive(Clone, Copy, Debug, EnumCount, Eq, Hash, PartialEq)]
        pub enum Node {
            #[allow(unused)]
            None,
            $(
                $variant($variant),
            )*
        }

        $(
            impl NodeData for $variant {
                fn kind() -> NodeKind {
                    NodeKind::$variant
                }
            }
        )*
    };
}

impl Node {
    // TODO Read
}

trait NodeData {
    fn kind() -> NodeKind;
}

// const NODE_SIZE: usize = size_of::<Node>() / CHUNK_SIZE;
const NODE_KIND_START: Size = 0x2000 as Size;
const NODE_KIND_END: Size = NODE_KIND_START + NodeKind::COUNT as Size;
const_assert!(NODE_KIND_START >= PARSE_BRANCH_KIND_END);

generate_node_enums!(Block, Call, Def, Fun, Public, Tok,);

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, Eq, Hash, PartialEq)]
pub struct NodeMeta {
    source: Size,
    typ: Size,
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
    meta: NodeMeta,
    target: Size,
    public: bool,
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
        dbg!(&self.builder().chunks);
    }

    // General helpers

    fn builder(&mut self) -> &mut TreeBuilder {
        &mut self.cart.tree_builder
    }

    fn chunks(&self) -> &[Chunk] {
        &self.cart.tree
    }

    fn push<T: NodeData>(&mut self, node: T) {
        self.builder().push(T::kind());
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
            meta: NodeMeta { source, typ: 0 },
            target,
            public: false, // to be filled in later but only matters for tops?
            value,
        };
        self.push(def);
    }

    fn node(&mut self, node: ParseNode, source: Size) {
        dbg!(node);
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
            meta: NodeMeta { source, typ: 0 },
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
            meta: NodeMeta { source, typ: 0 },
            kids,
        };
        self.push(block);
    }

    fn token(&mut self, token: Token, source: Size) {
        let tok = Tok {
            token,
            meta: NodeMeta { source, typ: 0 },
        };
        // Go straight to builder to avoid extra kind chunk.
        self.builder().push(tok);
    }
}
