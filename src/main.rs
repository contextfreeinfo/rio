// extern crate pest;
// #[macro_use]
// extern crate pest_derive;

// use pest::Parser;
use std::env;
use std::fs::File;
use std::io;
use std::io::prelude::*;

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

fn tokenize() -> Result<(), io::Error> {
  let args: Vec<String> = env::args().collect();
  let name = &args[1];
  let mut file = File::open(name)?;
  let mut buffer = String::new();
  file.read_to_string(&mut buffer)?;
  // println!("{}", buffer);
  let mut col_index = 0;
  let mut line_index = 0;
  let mut last_start: usize = 0;
  let mut start_line = 0;
  let mut start_col = 0;
  let mut state = TokenState::Start;
  let mut string_start = '?';
  for (index, char) in buffer.char_indices() {
    // println!("{}, {}", index, char);
    let new_state = match state {
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
            if char == string_start {
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
            match state {
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
            string_start = char;
            TokenState::StringStart
          },
          '\n' | '\r' => TokenState::VSpace,
          _ => TokenState::Error,
        }
      }
    };
    if new_state != state {
      if index > last_start {
        let token = &buffer[last_start..index];
        println!(
          "Token {:?} ({}) at {}, {} (char {})",
          state, token, start_line + 1, start_col + 1, last_start,
        );
      }
      state = new_state;
      last_start = index;
      start_line = line_index;
      start_col = col_index;
    }
    // Count rows and cols.
    col_index += 1;
    if char == '\n' {
      line_index += 1;
      col_index = 0;
    }
  }
  // Get the last token, which we should always have if it's not an empty file.
  if buffer.len() > last_start {
    let token = &buffer[last_start..];
    println!(
      "Token {:?} ({}) at {}, {} (char {})",
      state, token, start_line + 1, start_col + 1, last_start,
    );
  }
  // println!("size {}", buffer.len());
  Ok(())
}
