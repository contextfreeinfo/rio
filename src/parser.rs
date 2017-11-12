use tokenizer::*;

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum NodeType {
  Add,
  Assign,
  Block,
  Do,
  Number,
  Row,
  Spaced,
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

  fn parse_add(&mut self, parent: &mut Node<'a>) {
    // TODO Macro or other parameterized combo with assign, etc.
    let mut result = Node::new(NodeType::Add);
    let mut has_op = false;
    let mut skip_vspace = false;
    loop {
      let token = self.peek();
      match token.state {
        TokenState::Plus => {
          // TODO Also track whitespace so we can tell spaced ops or not?
          result.push_token(token);
          has_op = true;
          skip_vspace = true;
          self.next();
        }
        TokenState::Comment | TokenState::HSpace => {
          result.push_token(token);
          self.next();
        }
        // This knowledge of inner or outer precedence makes things a bit hard
        // to generalize.
        TokenState::Assign | TokenState::End | TokenState::Eof => break,
        TokenState::VSpace => {
          if !skip_vspace {
            break;
          }
          result.push_token(token);
          self.next();
        }
        _ => {
          self.parse_item(&mut result);
          skip_vspace = false;
        }
      }
    }
    if has_op {
      parent.push(result);
    } else {
      // TODO This multi push keeps Spaced from knowing there were multi.
      // TODO Figure out the return nodes thing?
      // TODO Make a catch-all to gather things that others don't for now?
      parent.kids.extend_from_slice(result.kids.as_slice());
    }
  }

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
          self.parse_spaced(&mut assign);
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
      // Once we handle all correct cases here, always go up on others, so we'll
      // can remove this list of ignores.
      TokenState::Assign | TokenState::End | TokenState::Eof => {},
      TokenState::Do => self.parse_do(parent),
      TokenState::Dot | TokenState::Int => self.parse_number(parent),
      TokenState::StringStart => self.parse_string(parent),
      _ => {
        parent.push_token(token);
        self.next();
      }
    }
  }

  fn parse_number(&mut self, parent: &mut Node<'a>) {
    let mut number = Node::new(NodeType::Number);
    let mut token = self.next();
    let mut has_int = false;
    match token.state {
      TokenState::Int => {
        number.push_token(token);
        has_int = true;
        token = self.next();
      }
      _ => {}
    };
    // Just a block, not a loop.
    // See https://github.com/rust-lang/rfcs/pull/2046
    loop {
      let dot = token;
      match token.state {
        TokenState::Dot => {
          number.push_token(token);
          token = self.next();
        }
        _ => {
          self.prev();
          break;
        },
      };
      match token.state {
        TokenState::Fraction => {
          number.push_token(token);
        }
        _ => {
          self.prev();
          if !has_int {
            // TODO Parse this as member access even if there was preceding
            // TODO whitespace!
            parent.push_token(dot);
            return;
          }
          break;
        },
      }
      break;
    }
    parent.push(number);
  }

  fn parse_row(&mut self, parent: &mut Node<'a>) {
    loop {
      match self.peek().state {
        TokenState::End | TokenState::Eof | TokenState::VSpace => break,
        _ => self.parse_assign(parent),
      }
    }
  }

  fn parse_spaced(&mut self, parent: &mut Node<'a>) {
    let mut result = Node::new(NodeType::Spaced);
    loop {
      let token = self.peek();
      match token.state {
        // All outer operators need to go here.
        TokenState::Assign | TokenState::End | TokenState::Eof |
        TokenState::VSpace => {
          break;
        }
        TokenState::Comment | TokenState::HSpace => {
          result.push_token(token);
          self.next();
        }
        _ => self.parse_add(&mut result),
      }
    }
    // See if we had multiple.
    let mut item_count = 0;
    for kid in &result.kids {
      let is_item = match kid.state {
        NodeType::Token => match kid.token.unwrap().state {
          // For now, comments are always until vspace, but maybe could change
          // someday, and the vspace will end things anyway.
          TokenState::Comment | TokenState::HSpace => false,
          _ => true,
        }
        _ => true,
      };
      if is_item {
        item_count += 1;
      }
    }
    // Finalize.
    if item_count > 1 {
      parent.push(result);
    } else {
      parent.kids.extend_from_slice(result.kids.as_slice());
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
