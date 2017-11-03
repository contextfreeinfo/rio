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

#[derive(Debug, PartialEq)]
enum TokenState {
  Any,
  Error,
  HSpace,
  Id,
  VSpace,
}

fn main() {
  parse().expect("Failed to parse");
}

fn parse() -> Result<(), io::Error> {
  let args: Vec<String> = env::args().collect();
  let name = &args[1];
  let mut file = File::open(name)?;
  let mut buffer = String::new();
  file.read_to_string(&mut buffer)?;
  // println!("{}", buffer);
  let mut last_start = 0;
  let mut state = TokenState::Any;
  // let mut token = String::new();
  // TODO How to avoid copies and use chars.as_str instead?
  // let mut iter = buffer.chars();
  for (index, char) in buffer.char_indices() {
    let new_state = match state {
      _ => {
        match char {
          ' ' | '\t' => TokenState::HSpace,
          'A'...'Z' | 'a'...'z' => TokenState::Id,
          '\n' | '\r' => TokenState::VSpace,
          _ => TokenState::Error,
        }
      }
    };
    if new_state != state {
      if index > last_start {
        let token = &buffer[last_start..index];
        println!(
          "Change from {:?} ({}) to {:?} at {}", state, token, new_state, index,
        );
      }
      state = new_state;
      last_start = index;
    }
  }
  Ok(())
}
