mod parser;
mod tokenizer;

pub use parser::*;
pub use tokenizer::*;

use std::fs::File;
use std::io;
use std::io::prelude::*;

pub fn process(name: &str) -> Result<(), io::Error> {
  let mut file = File::open(name)?;
  let mut buffer = String::new();
  file.read_to_string(&mut buffer)?;
  let tokens = tokenize(buffer.as_str());
  let tree = parse(&tokens);
  tree.print();
  Ok(())
}
