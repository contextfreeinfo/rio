extern crate rio;

use rio::*;
use std::env::*;
use std::io;
use std::io::prelude::*;
use std::fs::*;
use std::path::*;

fn main() {
  let dir =
    Path::new(var("CARGO_MANIFEST_DIR").unwrap().as_str()).join("tests");
  // println!("{:?}", dir);
  for entry in read_dir(dir).unwrap() {
    let path = entry.unwrap().path();
    // println!("{:?}", path);
    match path.extension() {
      Some(ext) => {
        if ext == "rio" {
          process(path.as_os_str().to_str().unwrap()).unwrap();
        }
      }
      _ => {}
    }
  }
}

fn process(path: &str) -> Result<(), io::Error> {
  // Read.
  let mut file = File::open(path)?;
  let mut source = String::new();
  file.read_to_string(&mut source)?;
  let tokens = tokenize(source.as_str());
  let tree = parse(&tokens);
  // Write.
  let out_path = format!("{}.tree.txt", path);
  println!("Write: {}", out_path);
  let mut out = File::create(out_path)?;
  write!(out, "{}\n", tree.format())?;
  // Done.
  Ok(())
}
