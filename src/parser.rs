use tokenizer::*;

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum NodeType {
  Add,
  Assign,
  Block,
  Do,
  Multiply,
  None,
  Number,
  Other,
  Row,
  Spaced,
  String,
  Token,
  Top,
}

#[derive(Clone, Debug)]
pub struct Node<'a> {
  kids: Vec<Node<'a>>,
  kind: NodeType,
  token: Option<&'a Token<'a>>,
}

impl<'a> Node<'a> {

  fn new(kind: NodeType) -> Node<'a> {
    Node {kids: vec![], kind, token: None}
  }

  fn new_token(token: &'a Token<'a>) -> Node<'a> {
    // TODO Control kind? Some token kind?
    Node {kids: vec![], kind: NodeType::Token, token: Some(&token)}
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
      let mut head = format!("{}{:?}\n", prefix, self.kind);
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

  fn push_token(&mut self, token: &'a Token<'a>) {
    self.push(Node::new_token(token));
  }

}

pub fn parse<'a>(tokens: &'a Vec<Token<'a>>) -> Node<'a> {
  let mut parser = Parser {
    found_index: 0, index: 0, last_index: 0, tokens: tokens,
  };
  parser.parse_expr(0)
}

struct Parser<'a> {
  found_index: usize,
  index: usize,
  last_index: usize,
  tokens: &'a Vec<Token<'a>>,
}

impl<'a> Parser<'a> {

  fn focus(&mut self) {
    self.index = self.found_index;
  }

  fn next(&mut self) -> &'a Token<'a> {
    self.next_skipv(false)
  }

  fn next_skipv(&mut self, skipv: bool) -> &'a Token<'a> {
    let mut index = self.index;
    self.last_index = index;
    let mut token;
    loop {
      token = self.tokens.get(index).unwrap();
      match token.state {
        TokenState::Comment | TokenState::HSpace => index += 1,
        TokenState::VSpace => if skipv {
          index += 1;
        } else {
          break;
        }
        _ => break,
      }
    }
    self.found_index = index;
    if index < self.tokens.len() {
      index += 1;
    }
    self.index = index;
    token
  }

  fn parse_expr(&mut self, precedence: u8) -> Node<'a> {
    // Node::new(NodeType::Top)
    let mut expr = self.parse_prefix();
    let mut first = true;
    let mut last_precedence = precedence;
    loop {
      let token = self.peek();
      if token.state == TokenState::Eof {
        break;
      }
      // TODO Not infix if postfix instead (attached then space).
      let infix = token.infix();
      let node_kind: NodeType;
      let op_precedence = if infix {
        node_kind = token_to_node_kind(token.state);
        token.precedence()
      } else {
        // TODO Check if spaced or directly attached.
        node_kind = NodeType::Spaced;
        TokenState::HSpace.precedence()
      };
      // println!("Peeked at {:?}, infix: {}", token, infix);
      if precedence >= op_precedence {
        // Go to outer level.
        break;
      }
      // In https://github.com/KeepCalmAndLearnRust/pratt-parser they always
      // nest binary ops left to right, so there's no need for this, but I
      // want a flatter tree here, and I also want to keep skip tokens in
      // the tree, so I can't always just nest.
      // So we have to build out manually when precedence falls.
      // If precedence rises, we'll go deeper into the call stack.
      // The precedence issue here is more for reset situations like new blocks
      // and such.
      if first || last_precedence > op_precedence {
        // println!("Got one inside");
        // TODO Figure out types.
        let mut outer = Node::new(node_kind);
        outer.push(expr);
        expr = outer;
        // Past first.
        first = false;
      }
      if infix {
        expr.push(Node::new_token(self.next()));
        // We can skip vertical after an infix operator.
        // This includes skipping other empty rows of vspace.
        self.next_skipv(true);
      }
      // Look at the next nonskippable.
      self.focus();
      // println!("Post at {:?}", self.peek());
      let kid = self.parse_expr(op_precedence);
      expr.push(kid);
      // Reset expectations for next time.
      last_precedence = op_precedence;
    }
    expr
  }

  fn parse_number(&mut self) -> Node<'a> {
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
            return Node::new_token(dot);
          }
          break;
        },
      }
      break;
    }
    number
  }

  fn parse_prefix(&mut self) -> Node<'a> {
    match self.peek().state {
      TokenState::Dot | TokenState::Int => self.parse_number(),
      TokenState::Eof => Node::new_token(self.peek()),
      TokenState::StringStart => self.parse_string(),
      // TODO Add others that are always infix.
      TokenState::VSpace => Node::new(NodeType::None),
      // TODO Branch on paren, do, string, number, ...
      _ => Node::new_token(self.next()),
    }
  }

  fn parse_string(&mut self) -> Node<'a> {
    let mut node = Node::new(NodeType::String);
    // StringStart, then loop through contents.
    node.push_token(self.next());
    loop {
      match self.peek().state {
        TokenState::StringStop => {
          node.push_token(self.next());
          break;
        }
        TokenState::Eof | TokenState::VSpace => {
          break;
        }
        _ => node.push_token(self.next()),
      }
    }
    node
  }

  fn peek(&mut self) -> &'a Token<'a> {
    let token = self.next();
    self.prev();
    token
  }

  fn prev(&mut self) {
    // TODO This results in iterating over space multiple times.
    // TODO Instead remember where the next is.
    self.index = self.last_index;
  }

}

fn token_to_node_kind(token_state: TokenState) -> NodeType {
  match token_state {
    TokenState::HSpace => NodeType::Spaced,
    TokenState::Plus => NodeType::Add,
    TokenState::Times => NodeType::Multiply,
    TokenState::VSpace => NodeType::Block,
    _ => NodeType::Other,
  }
}
