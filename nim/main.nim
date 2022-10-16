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
  # TODO Why does this alloc extra to add nodes on existing parser???
  # for _ in 0..<10:
  #   # Costs even more on a new parser beyond the cost of parser alloc itself.
  #   # var p = newParser()
  #   # discard p
  #   var p = parser
  #   discard p.parse(tokens)
  # Report.
  tree.print(pool = lexer.pool)
  echo("nodes: ", tree.nodes.len)
  echo("tokens: ", tokens.tokens.len)
  echo("interns: ", lexer.pool.size)
  echo("token size: ", sizeof(Token))
  echo("node size: ", sizeof(Node))

main()
