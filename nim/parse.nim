import lex
import system/assertions

type
  NodeKind* = enum
    leaf,
    bloc, # because block is keyword
    call,
    comment,
    define,
    space

  NodeId* = int32

  NodeSlice* = object
    idx: NodeId
    len: NodeId

  Node* = object
    case kind*: NodeKind
    of leaf:
      token*: Token
    else:
      kids*: NodeSlice

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

func token(parsing: Parsing): Token = parsing.tokens.tokens[parsing.index]

proc advance(parsing: var Parsing) =
  # Bounds checked access asserts that nobody eats the eof.
  parsing.parser.working.add(Node(kind: leaf, token: parsing.token))
  parsing.index += 1

func here(parsing: Parsing): int32 = int32 parsing.parser.working.len

func sliceFrom(begin: NodeId, til: NodeId): NodeSlice =
  NodeSlice(idx: begin, len: til - begin)

func peek(parsing: Parsing): TokenKind = parsing.token.kind

proc advanceIf(parsing: var Parsing, tokenKind: TokenKind): bool =
  let match = parsing.peek == tokenKind
  if match:
    parsing.advance
  match

proc nest(parsing: var Parsing, kind: NodeKind, begin: NodeId) =
  # TODO Any way to define a type that statically asserts this?
  assert kind != leaf
  let
    parser = addr parsing.parser
    nodesBegin = NodeId parser.nodes.len
  parser.nodes.add(parser.working[begin ..< ^0])
  parser.working.setLen(begin)
  {.cast(uncheckedAssign).}: # For `kind` that's asserted above.
    let parent = Node(
      kind: kind,
      kids: sliceFrom(nodesBegin, til = NodeId parser.nodes.len),
    )
  parser.working.add(parent)

proc nestMaybe(parsing: var Parsing, kind: NodeKind, begin: NodeId) =
  if parsing.here > begin:
    parsing.nest(kind, begin)

proc nest(parsing: var Parsing, kind: NodeKind, ifWhile: set[TokenKind]) =
  let begin = parsing.here
  while parsing.peek in ifWhile:
    parsing.advance
  parsing.nestMaybe(kind, begin)

# Parsing rules.

proc call(parsing: var Parsing) =
  # TODO Real parsing.
  parsing.nest(
    call, ifWhile = {TokenKind.low..TokenKind.high} - {eof, opIs, vspace}
  )

proc compare(parsing: var Parsing) =
  parsing.call

proc hspace(parsing: var Parsing) =
  parsing.nest(space, ifWhile = {TokenKind.comment, hspace})

proc space(parsing: var Parsing) =
  parsing.nest(space, ifWhile = {TokenKind.comment, hspace, vspace})

proc define(parsing: var Parsing) =
  let begin = parsing.here
  parsing.compare
  parsing.hspace
  if parsing.advanceIf(opIs):
    parsing.space
    parsing.define
    parsing.nest define, begin

proc expression(parsing: var Parsing) = parsing.define

# Top level.

proc parse(parsing: var Parsing): Tree =
  let parser = addr parsing.parser
  parser.nodes.setLen(0)
  parser.working.setLen(0)
  while parsing.peek != eof:
    parsing.expression
    parsing.space
  parsing.nestMaybe(bloc, 0)
  Tree(nodes: parser.nodes)

proc newParser*(): Parser = Parser(
  nodes: newSeq[Node](),
  working: newSeq[Node](),
)

proc parse*(parser: var Parser, tokens: Tokens): Tree =
  var parsing = Parsing(index: 0, parser: parser, tokens: tokens)
  parsing.parse
