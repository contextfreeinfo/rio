mod parser;
mod tokenizer;

use parser::*;
use std::env;
use std::fs::File;
use std::io;
use std::io::prelude::*;
use tokenizer::*;

fn main() {
  let args: Vec<String> = env::args().collect();
  let name = &args[1];
  process(name).expect("Failed to parse");
}

fn process(name: &str) -> Result<(), io::Error> {
  let mut file = File::open(name)?;
  let mut buffer = String::new();
  file.read_to_string(&mut buffer)?;
  parse(tokenize(buffer.as_str()));
  Ok(())
}
