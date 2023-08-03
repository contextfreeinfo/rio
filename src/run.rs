use crate::parse::{TreeBuilder, Node};

pub struct Runner {
    pub builder: TreeBuilder,
}

impl Runner {
    pub fn new(builder: TreeBuilder) -> Self {
        Self { builder }
    }

    pub fn run(&mut self, tree: &[Node]) -> Vec<Node> {
        self.builder.clear();
        self.resolve(tree);
        self.builder.extract()
    }

    fn resolve(&self, tree: &[Node]) -> Option<()> {
        Some(())
    }
}
