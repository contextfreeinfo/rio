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

generate_node_enums!(Block, Call, Def, Fun, Tok,);

// const NODE_SIZE: usize = size_of::<Node>() / CHUNK_SIZE;
const NODE_KIND_START: Size = 0x2000 as Size;
const NODE_KIND_END: Size = NODE_KIND_START + NodeKind::COUNT as Size;
const_assert!(NODE_KIND_START >= PARSE_BRANCH_KIND_END);

trait NodeData {
    fn kind() -> NodeKind;
}

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
    items: SizeRange,
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
    id: Tok,
    public: bool,
    type_spec: Size,
    value: SizeRange,
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
        self.top(*self.chunks().first().unwrap());
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

    // Processors

    fn def(&mut self, branch: ParseBranch) {
        let mut stepper = ParseNodeStepper::new(branch.range);
        let mut kid = stepper.next(self.chunks()).unwrap();
        let id = match kid {
            ParseNode::Leaf(token) if token.kind == TokenKind::Define => None,
            _ => {
                let first = kid;
                kid = stepper.next(self.chunks()).unwrap();
                match first {
                    ParseNode::Leaf(token) if matches!(token.kind, TokenKind::Id) => Some(Tok {
                        meta: NodeMeta {
                            source: Default::default(), // TODO source!
                            typ: Default::default(),
                        },
                        token,
                    }),
                    ParseNode::Branch(branch) => match branch.kind {
                        ParseBranchKind::Pub => {
                            // TODO Handle typed case.
                            None
                        }
                        ParseBranchKind::Typed => {
                            // TODO Handle typed case.
                            None
                        }
                        _ => None,
                    },
                    _ => None,
                }
            }
        };
        assert!(matches!(kid, ParseNode::Leaf(token) if token.kind == TokenKind::Define));
        if let Some(node) = stepper.next(self.chunks()) {
            // TODO What?
            self.node(node);
        }
        let def = Def {
            meta: Default::default(),
            id: id.unwrap_or_default(),
            public: false,
            type_spec: Default::default(),
            value: Default::default(),
        };
        self.push(def);
        dbg!(def);
        dbg!(&self.builder().working);
    }

    fn node(&mut self, node: ParseNode) {
        dbg!(node);
        match node {
            ParseNode::Branch(branch) => match branch.kind {
                ParseBranchKind::Block => {}
                ParseBranchKind::Call => {}
                ParseBranchKind::Infix => {}
                ParseBranchKind::Def => self.def(branch),
                ParseBranchKind::Params => {}
                ParseBranchKind::Typed => {}
                ParseBranchKind::Fun => {}
                ParseBranchKind::Pub => {}
                ParseBranchKind::StringParts => {}
            },
            ParseNode::Leaf(token) => {}
        }
    }

    fn top(&mut self, offset: Size) {
        let ParseNode::Branch(branch) = self.read(offset).0 else {
            panic!()
        };
        let mut offset = branch.range.start;
        while offset < branch.range.end {
            let (kid, next) = self.read(offset);
            self.node(kid);
            offset = next;
        }
    }
}
