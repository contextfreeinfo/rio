import intern
import lex
import parse
import std/os

proc main() =
  var lexer = newLexer()
  var grower = newGrower()
  let
    args = commandLineParams()
    source =
      if args.len > 0:
        readFile(args[0])
      else:
        "hi there"
    tokens = lexer.lex(source)
    tree = grower.parse(tokens)
  # TODO Why does this alloc extra to add nodes on existing grower???
  # for _ in 0..<10:
  #   # Costs even more on a new grower beyond the cost of grower alloc itself.
  #   # var p = newGrower()
  #   # discard p
  #   var p = grower
  #   discard p.parse(tokens)
  # Report.
  tree.print(pool = lexer.pool)
  echo("nodes: ", tree.nodes.len)
  echo("tokens: ", tokens.tokens.len)
  echo("interns: ", lexer.pool.size)
  echo("token size: ", sizeof(Token))
  echo("node size: ", sizeof(Node))

main()
