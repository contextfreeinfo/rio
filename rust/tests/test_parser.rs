extern crate difference;
extern crate rio;

use difference::*;
use rio::*;

#[test]
fn test_parse() {
  let source = include_str!("test.rio");
  let expected = include_str!("test.rio.tree.txt").replace("\r\n", "\n");
  let tokens = tokenize(source);
  let tree = parse(&tokens);
  // println!("{}", tree.format());
  let result = tree.format().replace("\r\n", "\n");
  // println!("{}", result);
  // TODO Better diffing and/or output.
  let changeset = Changeset::new(expected.trim(), result.trim(), "");
  for diff in changeset.diffs {
    match diff {
      Difference::Add(x) => {
        assert!(false, format!("Add({})", x));
      }
      Difference::Rem(x) => {
        assert!(false, format!("Rem({})", x));
      }
      Difference::Same(_x) => {
        // println!("Same({})", _x);
      }
    }
  }
}
