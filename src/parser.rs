use tokenizer::*;

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum ParseState {
  Error,
  Expr,
  Fraction,
  PostAtom,
  PostHSpace,
}

#[derive(Clone, Debug)]
pub struct Node<'a> {
  kids: Vec<Node<'a>>,
  state: ParseState,
  token: Option<&'a Token<'a>>,
}

impl<'a> Node<'a> {

  fn new(state: ParseState) -> Node<'a> {
    Node {kids: vec![], state, token: None}
  }

  fn new_token(token: &'a Token<'a>) -> Node<'a> {
    // TODO Control state? Some token state?
    Node {kids: vec![], state: ParseState::Expr, token: Some(&token)}
  }

  pub fn print(&self) {
    self.print_at("  ");
  }

  fn print_at(&self, prefix: &str) {
    if self.token.is_some() {
      let token = &self.token.unwrap();
      println!(
        "{}token at {}, {}: ({})", prefix, token.line, token.col, token.text,
      );
    } else {
      println!("{}{:?}", prefix, self.state);
      let deeper = format!("{}  ", prefix);
      for kid in &self.kids {
        kid.print_at(deeper.as_str());
      }
    }
  }

  fn push_if(&mut self, node: Node<'a>) {
    if !node.kids.is_empty() {
      self.kids.push(node);
    }
  }

}

pub fn parse<'a>(tokens: &'a Vec<Token<'a>>) -> Node<'a> {
  let mut root = Node::new(ParseState::Expr);
  let mut parser = Parser {index: 0, tokens: tokens};
  parser.parse_block(&mut root);
  root
}

struct Parser<'a> {
  index: usize,
  tokens: &'a Vec<Token<'a>>,
}

impl<'a> Parser<'a> {

  fn parse_block(&mut self, parent: &mut Node<'a>) {
    loop {
      let mut row = Node::new(ParseState::Expr);
      match self.peek() {
        Some(ref token) => match token.state {
          TokenState::VSpace => {
            parent.kids.push(Node::new_token(&token));
            self.next();
          }
          _ => self.parse_row(&mut row),
        }
        None => break,
      }
      parent.push_if(row);
    }
  }

  fn parse_row(&mut self, parent: &mut Node<'a>) {
    loop {
      match self.next() {
        Some(ref token) => match token.state {
          TokenState::VSpace => {
            self.prev();
            break;
          }
          _ => parent.kids.push(Node::new_token(&token)),
        }
        None => break,
      }
    }
  }

  fn next(&mut self) -> Option<&'a Token<'a>> {
    let index = self.index;
    if index < self.tokens.len() {
      self.index += 1;
    }
    self.tokens.get(index)
  }

  fn peek(&mut self) -> Option<&'a Token<'a>> {
    self.tokens.get(self.index)
  }

  fn prev(&mut self) {
    if self.index > 0 {
      self.index -= 1;
    }
  }

}
