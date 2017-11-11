use tokenizer::*;

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum NodeType {
  Assign,
  Block,
  Do,
  Row,
  String,
  Token,
  Top,
}

#[derive(Clone, Debug)]
pub struct Node<'a> {
  kids: Vec<Node<'a>>,
  state: NodeType,
  token: Option<&'a Token<'a>>,
}

impl<'a> Node<'a> {

  fn new(state: NodeType) -> Node<'a> {
    Node {kids: vec![], state, token: None}
  }

  fn new_token(token: &'a Token<'a>) -> Node<'a> {
    // TODO Control state? Some token state?
    Node {kids: vec![], state: NodeType::Token, token: Some(&token)}
  }

  pub fn format(&self) -> String {
    self.format_at("")
  }

  fn format_at(&self, prefix: &str) -> String {
    // Might not be the most efficient, since it doesn't work on a single big
    // buffer, but it shouldn't be needed except for debugging.
    // TODO Pass down the string to append to?
    if self.token.is_some() {
      let token = &self.token.unwrap();
      format!(
        "{}Token at {}, {}: {:?} ({})",
        prefix, token.line, token.col, token.state, token.text,
      )
    } else {
      let mut head = format!("{}{:?}\n", prefix, self.state);
      let deeper = format!("{}  ", prefix);
      let kids: Vec<_> = self.kids.iter().map(|ref kid| {
        kid.format_at(deeper.as_str())
      }).collect();
      head.push_str(kids[..].join("\n").as_str());
      head
    }
  }

  fn push(&mut self, node: Node<'a>) {
    self.kids.push(node);
  }

  fn push_if(&mut self, node: Node<'a>) {
    if !node.kids.is_empty() {
      self.push(node);
    }
  }

  fn push_token(&mut self, token: &'a Token<'a>) {
    self.push(Node::new_token(token));
  }

}

pub fn parse<'a>(tokens: &'a Vec<Token<'a>>) -> Node<'a> {
  let mut root = Node::new(NodeType::Top);
  let mut parser = Parser {index: 0, tokens: tokens};
  parser.parse_block(&mut root);
  root
}

struct Parser<'a> {
  index: usize,
  tokens: &'a Vec<Token<'a>>,
}

impl<'a> Parser<'a> {

  fn parse_assign(&mut self, parent: &mut Node<'a>) {
    let mut assign = Node::new(NodeType::Assign);
    let mut has_assign = false;
    let mut skip_vspace = false;
    loop {
      let token = self.peek();
      match token.state {
        TokenState::Assign => {
          assign.push_token(token);
          has_assign = true;
          skip_vspace = true;
          self.next();
        }
        TokenState::Comment | TokenState::HSpace => {
          assign.push_token(token);
          self.next();
        }
        TokenState::End | TokenState::Eof => break,
        TokenState::VSpace => {
          if !skip_vspace {
            break;
          }
          assign.push_token(token);
          self.next();
        }
        _ => {
          self.parse_item(&mut assign);
          skip_vspace = false;
        }
      }
    }
    if has_assign {
      parent.push(assign);
    } else {
      parent.kids.extend_from_slice(assign.kids.as_slice());
    }
  }

  fn parse_block(&mut self, parent: &mut Node<'a>) {
    loop {
      let mut row = Node::new(NodeType::Row);
      let token = self.peek();
      match token.state {
        TokenState::End => {
          // TODO Flag error.
          // TODO Also error and remain if non-matching.
          // TODO Or separate validation rules from initial parsing, maybe.
          // TODO Separation of concerns at all might keep code cleaner.
          if parent.state == NodeType::Top {
            // Keep on trucking.
            parent.push_token(&token);
            self.next();
          } else {
            break;
          }
        }
        TokenState::Eof => break,
        TokenState::VSpace => {
          parent.push_token(&token);
          self.next();
        }
        _ => self.parse_row(&mut row),
      }
      parent.push_if(row);
    }
  }

  fn parse_do(&mut self, parent: &mut Node<'a>) {
    let mut current = Node::new(NodeType::Do);
    current.push_token(self.next());
    // TODO Parse args, etc.
    loop {
      let token = self.peek();
      match token.state {
        TokenState::HSpace => {
          current.push_token(&token);
          self.next();
        }
        _ => break,
      }
    }
    // See if we have a block.
    match self.next().state {
      TokenState::VSpace => {
        self.prev();
        let mut block = Node::new(NodeType::Block);
        self.parse_block(&mut block);
        current.push(block);
        let token = self.next();
        match token.state {
          TokenState::End => {
            current.push_token(token);
          }
          _ => {}
        }
      }
      _ => {}
    }
    parent.push(current);
  }

  fn parse_item(&mut self, parent: &mut Node<'a>) {
    let token = self.peek();
    match token.state {
      TokenState::Assign | TokenState::End | TokenState::Eof => {},
      TokenState::Do => self.parse_do(parent),
      TokenState::StringStart => self.parse_string(parent),
      _ => {
        parent.push_token(token);
        self.next();
      }
    }
  }

  fn parse_row(&mut self, parent: &mut Node<'a>) {
    loop {
      match self.peek().state {
        TokenState::End | TokenState::Eof | TokenState::VSpace => break,
        _ => self.parse_assign(parent),
      }
    }
  }

  fn parse_string(&mut self, parent: &mut Node<'a>) {
    let mut string = Node::new(NodeType::String);
    loop {
      let token = self.next();
      match token.state {
        TokenState::StringStop => {
          string.push_token(token);
          break;
        }
        TokenState::Eof | TokenState::VSpace => {
          // TODO In what pass do we link juxtaposed strings?
          // TODO Look ahead now?
          self.prev();
          break;
        }
        _ => string.push_token(token),
      }
    }
    // We shouldn't be here in the first place if there weren't a start.
    parent.push(string);
  }

  fn next(&mut self) -> &'a Token<'a> {
    let index = self.index;
    if index < self.tokens.len() {
      self.index += 1;
    }
    self.tokens.get(index).unwrap()
  }

  fn peek(&mut self) -> &'a Token<'a> {
    self.tokens.get(self.index).unwrap()
  }

  fn prev(&mut self) {
    if self.index > 0 {
      self.index -= 1;
    }
  }

}
