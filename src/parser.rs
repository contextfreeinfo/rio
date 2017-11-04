use tokenizer::*;

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum ParseState {
  Error,
  Expr,
  Fraction,
  PostAtom,
  PostHSpace,
}

pub fn parse<'a>(tokens: Vec<Token<'a>>) {
  let mut state = ParseState::Expr;
  for token in tokens {
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
}
