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
  println!("{}", buffer);
  let mut index = 0;
  let mut state = TokenState::Any;
  let chars = buffer.chars();
  let mut token = String::new();
  for char in chars {
    let new_state = match state {
      _ => {
        // TODO Match char on the outside and only on state as needed?
        match char {
          ' ' | '\t' => TokenState::HSpace,
          'a'...'z' => TokenState::Id,
          '\n' | '\r' => TokenState::VSpace,
          _ => TokenState::Error,
        }
      }
    };
    if new_state != state {
      println!("Change from {:?} to {:?} at {}: {}", state, new_state, index, token);
      state = new_state;
      token.clear();
    }
    token.push(char);
    index += 1;
  }
  // let pairs = RioParser::parse_str(Rule::rows, buffer.as_str()).unwrap_or_else(|e| panic!("{}", e));

  // // Because ident_list is silent, the iterator will contain idents
  // for pair in pairs {
  //   // A pair is a combination of the rule which matched and a span of input
  //   println!("Rule: {:?}", pair.as_rule());
  //   println!("Span: {:?}", pair.clone().into_span());
  //   println!("Text: {}", pair.clone().into_span().as_str());

  //   // A pair can be converted to an iterator of the tokens which make it up:
  //   for inner_pair in pair.into_inner() {
  //     println!("  Rule: {:?}", inner_pair.as_rule());
  //     println!("  Span: {:?}", inner_pair.clone().into_span());
  //     println!("  Text: {}", inner_pair.clone().into_span().as_str());
  //   //   match inner_pair.as_rule() {
  //   //     Rule::alpha => println!("Letter:  {}", inner_pair.into_span().as_str()),
  //   //     Rule::digit => println!("Digit:   {}", inner_pair.into_span().as_str()), 
  //   //     _ => unreachable!()
  //   //   };
  //   }
  // };
  Ok(())
}
