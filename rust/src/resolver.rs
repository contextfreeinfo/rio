use parser::*;
// use tokenizer::*;
use std::collections::HashMap;

#[derive(Clone, Debug)]
pub struct Context<'a> {
  entries: HashMap<&'a str, &'a Node<'a>>,
  nodes: Vec<Node<'a>>,
}

impl<'a> Context<'a> {

  pub fn new() -> Context<'a> {
    let entries = HashMap::new();
    let nodes = vec![Node::new(NodeKind::Do)];
    Context {entries, nodes}
  }

  pub fn init(&'a mut self) {
    self.nodes.push(Node::new(NodeKind::Do));
    let last = self.nodes.last().unwrap();
    self.entries.insert("print", last);
  }

}

pub fn resolve(script: &Node, context: &Context) {
  //
}
