extern crate rio;

use rio::process;
use std::env;

fn main() {
  let args: Vec<String> = env::args().collect();
  let name = &args[1];
  process(name).expect("Failed to parse");
}
