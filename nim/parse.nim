import lex

type
  NodeKind* = enum
    comment,
    define,
    leaf,
    space

  NodeId* = int32

  NodeSlice* = object
    idx: NodeId
    len: NodeId

  Node* = object
    case kind: NodeKind
    of leaf:
      token: Token
    else:
      kids: NodeSlice

  Tree* = ref object
    nodes*: seq[Node]

  Parser* = ref object
    ## Persistent buffers across uses, retained for better pre-allocation.
    ## Cleared for each new run.
    nodes: seq[Node]
    working: seq[Node]

  Parsing = ref object
    ## Information for a particular parse.
    index: int32
    parser: Parser
    tokens: Tokens

# Support.

proc advance(parsing: var Parsing) =
  # Presume we have an eof at the end, so don't go past it.
  if parsing.index < parsing.tokens.tokens.len - 1:
    parsing.index += 1

func here(parsing: Parsing): int32 = int32 parsing.parser.nodes.len

func peek(parsing: Parsing): TokenKind =
  parsing.tokens.tokens[parsing.index].kind

proc advanceIf(parsing: var Parsing, tokenKind: TokenKind): bool =
  let match = parsing.peek == tokenKind
  if match:
    parsing.advance
  match

proc nest(parsing: var Parsing, kind: NodeKind, begin: NodeId) =
  discard

proc nestMaybe(parsing: var Parsing, kind: NodeKind, begin: NodeId) =
  if parsing.here > begin:
    parsing.nest(kind, begin)

proc nestIfWhile(parsing: var Parsing, tokenKinds: set[TokenKind]) =
  let begin = parsing.here
  while parsing.peek in tokenKinds:
    parsing.advance
  parsing.nestMaybe(space, begin)

# Parsing rules.

proc call(parsing: var Parsing) =
  discard parsing

proc compare(parsing: var Parsing) =
  parsing.call

proc hspace(parsing: var Parsing) =
  parsing.nestIfWhile {TokenKind.comment, hspace}

proc space(parsing: var Parsing) =
  parsing.nestIfWhile {TokenKind.comment, hspace, vspace}

proc define(parsing: var Parsing) =
  let begin = parsing.here
  parsing.compare
  parsing.hspace
  if parsing.advanceIf(opIs):
    parsing.space
    parsing.define
    parsing.nest(NodeKind.define, begin)

proc expression(parsing: var Parsing) = parsing.define

# Top level.

proc parse(parsing: var Parsing): Tree =
  let parser = addr parsing.parser
  parser.nodes.setLen(0)
  parser.working.setLen(0)
  while true:
    parsing.expression
    # TODO Parse.
    break
  Tree(nodes: parser.nodes)

proc newParser*(): Parser = Parser(
  nodes: newSeq[Node](),
  working: newSeq[Node](),
)

proc parse*(parser: var Parser, tokens: Tokens): Tree =
  var parsing = Parsing(index: 0, parser: parser, tokens: tokens)
  parsing.parse
