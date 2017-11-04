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

pub fn parse<'a>(tokens: &'a Vec<Token<'a>>) -> Node<'a> {
  let mut root = Node {kids: vec![], state: ParseState::Expr, token: None};
  let mut parser = Parser {index: 0, tokens: tokens};
  parser.parse_block(&root);
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

  fn parse_block(&mut self, parent: &Node) {
    //
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
