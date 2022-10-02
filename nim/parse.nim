import lex

type
  NodeKind* = enum
    comment,
    define,
    leaf

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
    index: int
    parser: Parser
    tokens: Tokens

proc parse(parsing: var Parsing): Tree =
  parsing.parser.nodes.setLen(0)
  parsing.parser.working.setLen(0)
  while true:
    # TODO Parse.
    break
  Tree(nodes: parsing.parser.nodes)

proc newParser*(): Parser = Parser(
  nodes: newSeq[Node](),
  working: newSeq[Node](),
)

proc parse*(parser: var Parser, tokens: Tokens): Tree =
  var parsing = Parsing(index: 0, parser: parser, tokens: tokens)
  parsing.parse
