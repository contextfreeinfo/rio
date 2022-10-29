import intern
import lex
import norm
import parse
import std/os

proc main() =
  let args = commandLineParams()
  if args.len < 2:
    raise ValueError.newException "Usage for now: rio infile outdir"
  var lexer = newLexer()
  var grower = newGrower()
  let
    # TODO Real arg parsing.
    sourcePath = args[0]
    sourceName = sourcePath.extractFilename
    source = readFile(sourcePath)
    outDir = args[1]
    tokens = lexer.lex(source)
    parsed = grower.parse(tokens)
    spaceless = grower.spaceless(parsed)
    tree = spaceless
  # TODO Why does this alloc extra to add nodes on existing grower???
  # for _ in 0..<10:
  #   # Costs even more on a new grower beyond the cost of grower alloc itself.
  #   # var p = newGrower()
  #   # discard p
  #   var p = grower
  #   discard p.parse(tokens)
  # Write.
  outDir.createDir
  block:
    let file = open(outDir / sourceName.changeFileExt "parsed.txt", fmWrite)
    defer: file.close
    parsed.print(file = file, pool = lexer.pool)
    file.writeLine("nodes: ", parsed.nodes.len)
  block:
    let file = open(outDir / sourceName.changeFileExt ".normed.txt", fmWrite)
    defer: file.close
    tree.print(file = file, pool = lexer.pool)
    file.writeLine("nodes: ", tree.nodes.len)
    file.writeLine("tokens: ", tokens.tokens.len)
    file.writeLine("interns: ", lexer.pool.size)
    file.writeLine("token size: ", sizeof(Token))
    file.writeLine("node size: ", sizeof(Node))

main()
