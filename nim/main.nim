import intern
import lex
import parse
import std/os

proc main() =
  var lexer = newLexer()
  var parser = newParser()
  let
    args = commandLineParams()
    source =
      if args.len > 0:
        readFile(args[0])
      else:
        "hi there"
    tokens = lexer.lex(source)
    tree = parser.parse(tokens)
  discard tree
  for token in tokens.tokens:
    echo(token.kind, ": '", lexer.pool[token.text], "'")
  echo("nodes: ", tree.nodes.len)
  echo("interns: ", lexer.pool.size)
  echo("token size: ", sizeof(Token))
  echo("node size: ", sizeof(Node))

main()
