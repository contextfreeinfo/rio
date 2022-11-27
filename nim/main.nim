import intern
import lex
import norm
import parse
import run
import std/os

type
  Engine = ref object
    grower: Grower
    lexer: Lexer
    outDir: string

  Module = ref object
    # TODO Links from resolved to parsed.
    parsed: Tree
    resolved: Tree

proc process(
  engine: Engine,
  imports: seq[Module] = @[],
  source: string,
  sourceName: string,
): Module =
  discard imports
  var grower = engine.grower
  let
    outDir = engine.outDir
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
  Module(parsed: parsed, resolved: resolved)

const
  coreName = "core.rio"
  coreSource = staticRead(coreName)

proc main() =
  let args = commandLineParams()
  if args.len < 2:
    raise ValueError.newException "Usage for now: rio infile outdir"
  var
    # TODO Real arg parsing.
    outDir = args[1]
    lexer = newLexer()
    engine = Engine(grower: newGrower(), lexer: lexer, outDir: outDir)
  let
    sourcePath = args[0]
    sourceName = sourcePath.extractFilename
    source = readFile(sourcePath)
    core = engine.process(source = coreSource, sourceName = coreName)
    module = engine.process(
      imports = @[core], source = source, sourceName = sourceName
    )
  discard module

main()
