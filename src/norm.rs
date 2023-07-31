use crate::parse::{TreeBuilder, Node};

#[derive(Default)]
pub struct Normer {
    pub builder: TreeBuilder,
}

impl Normer {
    pub fn new(builder: TreeBuilder) -> Self {
        Self { builder }
    }

    pub fn norm(&mut self, tree: &[Node]) -> Vec<Node> {
        self.builder.clear();
        // let mut source = source.iter().peekable();
        // self.block_top(&mut source);
        // self.builder.wrap(BranchKind::Block, 0);
        self.builder.extract()
    }
}
