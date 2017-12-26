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
  Parens,
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

  pub fn new(kind: NodeKind) -> Node<'a> {
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
    found_index: 0, index: 0, last_index: 0, last_skipped: 0, tokens: tokens,
  };
  parser.parse_expr(0)
}

struct Parser<'a> {
  found_index: usize,
  index: usize,
  last_index: usize,
  last_skipped: usize,
  tokens: &'a Vec<Token<'a>>,
}

impl<'a> Parser<'a> {

  fn chew(&mut self, parent: &mut Node<'a>, index: usize) {
    // Push all skipped tokens.
    if self.last_skipped < index {
      for token in &self.tokens[self.last_skipped..(index - 1)] {
        parent.push_token(token);
      }
    }
    self.last_skipped = index;
  }

  fn focus(&mut self, parent: &mut Node<'a>) {
    self.index = self.found_index;
    let index = self.index;
    self.chew(parent, index + 1);
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
    self.push_next(&mut node);
    // Go inside.
    let content = self.parse_expr(TokenState::End.precedence());
    self.push(&mut node, content);
    // See where we ended.
    // println!("after do at {:?}", self.peek());
    match self.peek().state {
      TokenState::End => self.push_next(&mut node),
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
        self.push(&mut outer, expr);
        expr = outer;
        // Past first.
        first = false;
      }
      if infix {
        self.push_next(&mut expr);
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
        self.push_next(&mut stray);
        self.push(&mut expr, stray);
        // Try again to see what we can see.
        continue;
      }
      // Look at the next nonskippable.
      self.focus(&mut expr);
      // println!("Post at {:?}", self.peek());
      let kid = self.parse_expr(op_precedence);
      self.push(&mut expr, kid);
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
        self.push_token(&mut number, token);
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
          self.push_token(&mut number, token);
          token = self.next();
        }
        _ => {
          self.prev();
          break;
        },
      };
      match token.state {
        TokenState::Fraction => {
          self.push_token(&mut number, token);
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

  fn parse_parens(&mut self) -> Node<'a> {
    // Build the parent.
    let mut node = Node::new(NodeKind::Parens);
    self.push_next(&mut node);
    // Go inside.
    let content = self.parse_expr(TokenState::ParenClose.precedence());
    self.push(&mut node, content);
    // See where we ended.
    match self.peek().state {
      TokenState::ParenClose => self.push_next(&mut node),
      // Must be end or eof. Just move on.
      _ => (),
    }
    // println!("then at {:?}", self.peek());
    node
  }

  fn parse_prefix(&mut self) -> Node<'a> {
    match self.peek().state {
      TokenState::Do => self.parse_do(),
      TokenState::Dot | TokenState::Int => self.parse_number(),
      TokenState::ParenClose | TokenState::End | TokenState::VSpace => {
        Node::new(NodeKind::None)
      }
      TokenState::Eof => Node::new_token(self.peek()),
      TokenState::ParenOpen => self.parse_parens(),
      TokenState::StringStart => self.parse_string(),
      // TODO Add others that are always infix.
      // TODO Branch on paren, do, string, number, ...
      _ => Node::new_token(self.next()),
    }
  }

  fn parse_string(&mut self) -> Node<'a> {
    let mut node = Node::new(NodeKind::String);
    // StringStart, then loop through contents.
    self.push_next(&mut node);
    loop {
      match self.peek().state {
        TokenState::StringStop => {
          self.push_next(&mut node);
          break;
        }
        TokenState::Eof | TokenState::VSpace => {
          break;
        }
        _ => self.push_next(&mut node),
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

  fn push(&mut self, parent: &mut Node<'a>, node: Node<'a>) {
    let index = self.index;
    self.chew(parent, index);
    if node.kind != NodeKind::None {
      // Now push the requested node.
      parent.push(node);
    }
  }

  fn push_next(&mut self, parent: &mut Node<'a>) {
    let token = self.next();
    self.push_token(parent, token);
  }

  fn push_token(&mut self, parent: &mut Node<'a>, token: &'a Token<'a>) {
    self.push(parent, Node::new_token(token));
  }

}

fn token_to_node_kind(token_state: TokenState) -> NodeKind {
  match token_state {
    TokenState::Assign => NodeKind::Assign,
    TokenState::HSpace => NodeKind::Spaced,
    TokenState::Plus => NodeKind::Add,
    TokenState::Times => NodeKind::Multiply,
    TokenState::VSpace => NodeKind::Block,
    _ => NodeKind::Other,
  }
}
