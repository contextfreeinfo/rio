use tokenizer::*;

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum NodeKind {
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
  Stray,
  String,
  Token,
  Top,
}

#[derive(Clone, Debug)]
pub struct Node<'a> {
  kids: Vec<Node<'a>>,
  kind: NodeKind,
  token: Option<&'a Token<'a>>,
}

impl<'a> Node<'a> {

  fn new(kind: NodeKind) -> Node<'a> {
    // Vec shouldn't allocate on empty.
    Node {kids: vec![], kind, token: None}
  }

  fn new_token(token: &'a Token<'a>) -> Node<'a> {
    Node {kids: vec![], kind: NodeKind::Token, token: Some(&token)}
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
      let mut group: Vec<String> = vec![format!("{}{:?}", prefix, self.kind)];
      let deeper = format!("{}  ", prefix);
      group.extend(self.kids.iter().map(|ref kid| {
        kid.format_at(deeper.as_str())
      }));
      group[..].join("\n")
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

  fn parse_do(&mut self) -> Node<'a> {
    // Build the do.
    // println!("start do");
    let mut node = Node::new(NodeKind::Do);
    node.push_token(self.next());
    // Go inside. TODO Skip adding if none.
    let content = self.parse_expr(TokenState::End.precedence());
    node.push(content);
    // See where we ended.
    // println!("after do at {:?}", self.peek());
    match self.peek().state {
      TokenState::End => node.push_token(self.next()),
      // Must be eof. Just move on.
      _ => (),
    }
    // println!("then at {:?}", self.peek());
    node
  }

  fn parse_expr(&mut self, precedence: u8) -> Node<'a> {
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
      let mut op_precedence = token.precedence();
      // println!(
      //   "Peeked at {:?}, {} vs {}, infix: {}",
      //   token, precedence, op_precedence, infix,
      // );
      if precedence >= op_precedence {
        // Go to outer level.
        break;
      }
      let node_kind = if infix {
        token_to_node_kind(token.state)
      } else {
        op_precedence = TokenState::HSpace.precedence();
        NodeKind::Spaced
      };
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
        expr.push_token(self.next());
        // We can skip vertical after an infix operator.
        // This includes skipping other empty rows of vspace.
        self.next_skipv(true);
      } else if token.close() {
        // We should have broken out on a close.
        // Must be a stray inside something larger, like a paren before end, or
        // end at top file level.
        // TODO Track stack of what opens we have, so we can break even better
        // TODO than just looking at precedence.
        let mut stray = Node::new(NodeKind::Stray);
        stray.push_token(self.next());
        expr.push(stray);
        // Try again to see what we can see.
        continue;
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
    let mut number = Node::new(NodeKind::Number);
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
      TokenState::Do => self.parse_do(),
      TokenState::Dot | TokenState::Int => self.parse_number(),
      TokenState::End | TokenState::VSpace => Node::new(NodeKind::None),
      TokenState::Eof => Node::new_token(self.peek()),
      TokenState::StringStart => self.parse_string(),
      // TODO Add others that are always infix.
      // TODO Branch on paren, do, string, number, ...
      _ => Node::new_token(self.next()),
    }
  }

  fn parse_string(&mut self) -> Node<'a> {
    let mut node = Node::new(NodeKind::String);
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

fn token_to_node_kind(token_state: TokenState) -> NodeKind {
  match token_state {
    TokenState::HSpace => NodeKind::Spaced,
    TokenState::Plus => NodeKind::Add,
    TokenState::Times => NodeKind::Multiply,
    TokenState::VSpace => NodeKind::Block,
    _ => NodeKind::Other,
  }
}
