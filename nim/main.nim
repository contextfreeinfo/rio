import intern
import lex
import parse
import std/os
import std/strformat

proc main() =
  let
    kind = TokenKind.comment
    kind2 = NodeKind.comment
    comment = 3
  var lexer = newLexer()
  let
    args = commandLineParams()
    source =
      if args.len > 0:
        readFile(args[0])
      else:
        "hi there"
    tokens = lexer.lex(source)
  for token in tokens:
    echo(fmt"{token.kind}: '{lexer.pool[token.text]}'")
  echo(fmt"interns: {lexer.pool.size}")

main()
