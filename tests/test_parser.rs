extern crate rio;

use rio::*;

#[test]
fn test_parse() {
  let source = include_str!("test.rio");
  let out = include_str!("test.rio.tree.txt");
  let tokens = tokenize(source);
  let tree = parse(&tokens);
  // println!("{}", tree.format());
  assert_eq!(out, tree.format());
}
