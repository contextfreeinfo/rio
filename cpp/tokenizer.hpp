#pragma once

#include <string_view>

namespace rio {

enum struct TokenState {
  Assign,
  Comment,
  Do,
  Dot,
  End,
  Eof,
  Error,
  EscapeStart,
  Escape,
  Fraction,
  HSpace,
  Id,
  Int,
  Op,
  Op1,
  Op2,
  ParenOpen,
  ParenClose,
  Plus,
  Start,
  StringStart,
  StringStop,
  StringText,
  Times,
  VSpace,
};

bool closing(TokenState state) {
  switch (state) {
    case TokenState::End: {
      return true;
    }
    default: {
      return false;
    }
  }
}

bool infix(TokenState state) {
  switch (state) {
    case TokenState::Assign:
    case TokenState::Op:
    case TokenState::Op1:
    case TokenState::Op2:
    case TokenState::Plus:
    case TokenState::Times:
    case TokenState::VSpace: {
      return true;
    }
    default: {
      return false;
    }
  }
}

int precedence(TokenState state) {
  switch (state) {
    case TokenState::Eof: return 0;
    case TokenState::End: return 5;
    // TODO Same precedence as End but with open-token stack tracking?
    case TokenState::ParenClose: return 7;
    case TokenState::Comment:
    case TokenState::VSpace: return 10;
    case TokenState::HSpace: return 20;
    case TokenState::Plus: return 30;
    case TokenState::Times: return 40;
    default: return 20;
  }
}

struct Token {

  // Fields.

  int line;

  // Col in code points.
  int col;

  // Index in bytes.
  int index;

  TokenState state;

  std::string_view text;

  // Functions.

  bool closing() {
    return ::rio::closing(state);
  }

  bool infix() {
    return ::rio::infix(state);
  }

  bool precedence() {
    return ::rio::precedence(state);
  }

};

# if 0
// TODO Prefer impl Iterator<type = Token<'a>> when impl trait is done.
pub fn tokenize<'a>(buffer: &'a str) -> Vec<Token<'a>> {
  Tokenizer::new(buffer).collect()
}

struct Tokenizer<'a> {
  buffer: &'a str,
  char_indices: CharIndices<'a>,
  col_index: usize,
  gave_eof: bool,
  last_start: usize,
  line_index: usize,
  start_col: usize,
  start_line: usize,
  state: TokenState,
  string_start: char,
}

impl<'a> Tokenizer<'a> {

  pub fn new(buffer: &str) -> Tokenizer {
    Tokenizer {
      buffer,
      char_indices: buffer.char_indices(),
      col_index: 0,
      gave_eof: false,
      last_start: 0,
      line_index: 0,
      start_line: 0,
      start_col: 0,
      state: TokenState::Start,
      string_start: '?',
    }
  }

  fn find_key(&self, text: &str) -> TokenState {
    let mut chars = text.chars();
    match chars.next() {
      Some('d') => match chars.next() {
        Some('o') => match chars.next() {
          None => TokenState::Do,
          _ => TokenState::Id,
        }
        _ => TokenState::Id,
      }
      Some('e') => match chars.next() {
        Some('n') => match chars.next() {
          Some('d') => match chars.next() {
            None => TokenState::End,
            _ => TokenState::Id,
          }
          _ => TokenState::Id,
        }
        _ => TokenState::Id,
      }
      _ => TokenState::Id,
    }
  }

  fn find_op(&self, text: &str) -> TokenState {
    let mut chars = text.chars();
    match chars.next() {
      Some('=') => match chars.next() {
        None => TokenState::Assign,
        _ => TokenState::Op,
      }
      Some('(') => TokenState::ParenOpen,
      Some(')') => TokenState::ParenClose,
      Some('+') | Some('-') => match chars.next() {
        None => TokenState::Plus,
        _ => TokenState::Op,
      }
      Some('*') | Some('/') => match chars.next() {
        None => TokenState::Times,
        _ => TokenState::Op,
      }
      _ => TokenState::Op,
    }
  }

}

impl<'a> Iterator for Tokenizer<'a> {

  type Item = Token<'a>;

  fn next(&mut self) -> Option<Self::Item> {
    let mut stop_index: usize;
    let last_start = self.last_start;
    let start_col = self.start_col;
    let start_line = self.start_line;
    let mut state = self.state;
    loop {
      match self.char_indices.next() {
        Some((index, char)) => {
          let new_state = match self.state {
            TokenState::Comment => {
              match char {
                '\n' | '\r' => TokenState::VSpace,
                _ => TokenState::Comment,
              }
            }
            TokenState::Escape => {
              TokenState::StringText
            }
            TokenState::EscapeStart => {
              TokenState::Escape
            }
            TokenState::StringStart | TokenState::StringText => {
              match char {
                '\\' => TokenState::EscapeStart,
                '\'' | '"' => {
                  if char == self.string_start {
                    TokenState::StringStop
                  } else {
                    TokenState::StringText
                  }
                },
                '\n' | '\r' => TokenState::VSpace,
                _ => TokenState::StringText,
              }
            },
            _ => {
              match char {
                '#' => TokenState::Comment,
                ' ' | '\t' => TokenState::HSpace,
                'A'...'Z' | 'a'...'z' | '_' => TokenState::Id,
                '0'...'9' => {
                  match self.state {
                    TokenState::Dot => TokenState::Fraction,
                    TokenState::Fraction => TokenState::Fraction,
                    TokenState::Id => TokenState::Id,
                    _ => TokenState::Int,
                  }
                },
                '.' => TokenState::Dot,
                ',' | ';' | ':' | '(' | ')' | '[' | ']' | '{' | '}' => {
                  // Parens!!!
                  TokenState::Op1
                },
                '+' | '-' | '*' | '/' | '=' => {
                  // TODO Deal with compound operators on these.
                  TokenState::Op2
                },
                '\'' | '"' => {
                  self.string_start = char;
                  TokenState::StringStart
                },
                '\n' | '\r' => TokenState::VSpace,
                _ => TokenState::Error,
              }
            }
          };
          stop_index = index;
          state = self.state;
          let state_changed = new_state != state;
          if state_changed {
            self.state = new_state;
            self.last_start = index;
            self.start_line = self.line_index;
            self.start_col = self.col_index;
          }
          // Count rows and cols.
          self.col_index += 1;
          if char == '\n' {
            self.line_index += 1;
            self.col_index = 0;
          }
          // If the first char, we might need to keep going.
          if state_changed && index > last_start {
            break;
          }
        }
        None => {
          stop_index = self.buffer.len();
          self.last_start = stop_index;
          break;
        }
      }
    }
    if stop_index == last_start {
      if self.gave_eof {
        return None;
      }
      // Eof token lets parser ignore None case better.
      state = TokenState::Eof;
      self.gave_eof = true;
    }
    // We have a token to give.
    let text = &self.buffer[last_start..stop_index];
    state = match state {
      TokenState::Id => self.find_key(text),
      TokenState::Op1 | TokenState::Op2 => self.find_op(text),
      _ => state,
    };
    Some(Token {
      line: start_line + 1,
      col: start_col + 1,
      index: last_start,
      state: state,
      text: text,
    })
  }

}
#endif

}
