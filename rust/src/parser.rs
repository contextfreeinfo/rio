use tokenizer::*;

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum ParseState {
  Error,
  Expr,
  Fraction,
  PostAtom,
  PostHSpace,
}

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
  let mut state = ParseState::Expr;
  let mut stack: Vec<ParseState> = vec![ParseState::Expr];
  for token in tokens {
    let parent = &stack.last().unwrap();
    let last = &root.kids.last();
    let new_state = match state {
      ParseState::Error | ParseState::Expr => {
        match token.state {
          TokenState::Dot => {
            ParseState::Fraction
          }
          TokenState::HSpace => {
            ParseState::Expr
          }
          _ => {
            ParseState::PostAtom
          }
        }
      }
      ParseState::Fraction => {
        match token.state {
          TokenState::Fraction => {
            ParseState::Expr
          }
          _ => {
            ParseState::Error
          }
        }
      }
      ParseState::PostAtom => {
        match token.state {
          TokenState::Dot => {
            // Get member.
            ParseState::Expr
          }
          TokenState::HSpace => {
            ParseState::PostHSpace
          }
          TokenState::Op1 | TokenState::Op2 => {
            ParseState::Expr
          }
          _ => {
            ParseState::Expr
          }
        }
      }
      ParseState::PostHSpace => {
        match token.state {
          _ => {
            ParseState::Expr
          }
        }
      }
    };
    println!("{:?}: {:?}", state, token);
    state = new_state;
  }
  root
}

struct Parser<'a> {
  index: usize,
  tokens: &'a Vec<Token<'a>>,
}

impl<'a> Parser<'a> {

  fn parse_block(&mut self, parent: &mut Node) {
    loop {
      let mut row = Node::new(ParseState::Expr);
      match self.peek() {
        Some(_) => {
          self.parse_row(&mut row);
        }
        None => {
          break;
        }
      }
      parent.push_if(row);
    }
  }

  fn parse_row(&mut self, parent: &mut Node) {
    loop {
      match self.next() {
        Some(ref token) => {
          match token.state {
            TokenState::VSpace => break,
            _ => {
              parent.kids.push(Node::new_token(&token));
            }
          }
        }
        None => break,
      }
    }
  }

  fn next(&mut self) -> Option<&Token> {
    let index = self.index;
    if index < self.tokens.len() {
      self.index += 1;
    }
    self.tokens.get(index)
  }

  fn peek(&mut self) -> Option<&Token> {
    self.tokens.get(self.index)
  }

}
