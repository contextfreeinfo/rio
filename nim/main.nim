import intern
import lex
import norm
import parse
import run
import std/os

type
  Engine = ref object
    lexer: Lexer
    grower: Grower

proc process(
  engine: Engine, source: string, sourceName: string, outDir: string
) =
  var grower = engine.grower
  let
    lexer = engine.lexer
    tokens = engine.lexer.lex(source)
    parsed = grower.parse(tokens)
    normed = grower.normed(parsed)
    resolved = grower.resolve(normed)
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
    file.writeLine("tokens: ", tokens.tokens.len)
    file.writeLine("interns: ", lexer.pool.size)
    file.writeLine("token size: ", sizeof(Token))
    file.writeLine("node size: ", sizeof(Node))
  block:
    let file = open(outDir / sourceName.changeFileExt ".normed.txt", fmWrite)
    defer: file.close
    normed.print(file = file, pool = lexer.pool)
    file.writeLine("nodes: ", normed.nodes.len)
  block:
    let file = open(outDir / sourceName.changeFileExt ".resolved.txt", fmWrite)
    defer: file.close
    resolved.print(file = file, pool = lexer.pool)
    file.writeLine("nodes: ", resolved.nodes.len)

const
  coreName = "core.rio"
  coreSource = staticRead(coreName)

proc main() =
  let args = commandLineParams()
  if args.len < 2:
    raise ValueError.newException "Usage for now: rio infile outdir"
  var
    lexer = newLexer()
    engine = Engine(lexer: lexer, grower: newGrower())
  let
    # TODO Real arg parsing.
    sourcePath = args[0]
    sourceName = sourcePath.extractFilename
    source = readFile(sourcePath)
    outDir = args[1]
  engine.process(source = coreSource, sourceName = coreName, outDir = outDir)
  engine.process(source = source, sourceName = sourceName, outDir = outDir)

main()
