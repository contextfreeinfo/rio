// extern crate pest;
// #[macro_use]
// extern crate pest_derive;

// use pest::Parser;
use std::env;
use std::fs::File;
use std::io;
use std::io::prelude::*;
use std::str::*;

// #[derive(Parser)]
// #[grammar = "rio.pest"]
// struct RioParser;

#[derive(Clone, Copy, Debug, PartialEq)]
enum TokenState {
  Dot,
  Error,
  EscapeStart,
  Escape,
  Fraction,
  HSpace,
  Id,
  Int,
  Op1,
  Op2,
  Start,
  StringStart,
  StringStop,
  StringText,
  VSpace,
}

fn main() {
  tokenize().expect("Failed to parse");
}

#[derive(Debug)]
struct Token<'a> {
  col: usize,
  line: usize,
  state: TokenState,
  text: &'a str,
}

struct Tokenizer<'a> {
  buffer: &'a str,
  char_indices: CharIndices<'a>,
  col_index: usize,
  last_start: usize,
  line_index: usize,
  start_col: usize,
  start_line: usize,
  state: TokenState,
  string_start: char,
}

impl<'a> Tokenizer<'a> {
  fn new(buffer: &str) -> Tokenizer {
    Tokenizer {
      buffer: buffer,
      char_indices: buffer.char_indices(),
      col_index: 0,
      last_start: 0,
      line_index: 0,
      start_line: 0,
      start_col: 0,
      state: TokenState::Start,
      string_start: '?',
    }
  }
}

impl<'a> Iterator for Tokenizer<'a> {

  type Item = Token<'a>;

  fn next(&mut self) -> Option<Self::Item> {
    let mut stop_index: usize = 0;
    let last_start = self.last_start;
    let start_col = self.start_col;
    let start_line = self.start_line;
    let mut state = self.state;
    loop {
      match self.char_indices.next() {
        Some((index, char)) => {
          let new_state = match self.state {
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
          let state_changed = new_state != self.state;
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
          if state_changed && index > self.last_start {
            break;
          }
        }
        None => {
          break;
        }
      }
    }
    if stop_index > last_start {
      let text = &self.buffer[last_start..stop_index];
      Some(Token {
        col: start_col, line: start_line, state: state, text: text,
      })
    } else {
      None
    }
  }

}

fn tokenize() -> Result<(), io::Error> {
  let args: Vec<String> = env::args().collect();
  let name = &args[1];
  let mut file = File::open(name)?;
  let mut buffer = String::new();
  file.read_to_string(&mut buffer)?;
  // println!("{}", buffer);
  for token in Tokenizer::new(buffer.as_str()) {
    println!("{:?}", token);
  }
  // println!("size {}", buffer.len());
  Ok(())
}
