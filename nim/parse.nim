import intern
import lex
import system/assertions
import std/strutils

type
  NodeKind* = enum
    leaf,
    group,
    prefix,
    infix,
    space

  NodeId* = int32

  NodeSlice* = object
    idx: NodeId
    thru: NodeId

  Node* = object
    case kind*: NodeKind
    of leaf:
      token*: Token
    else:
      kids*: NodeSlice

  Pass* = enum
    parse

  Tree* = ref object
    pass*: Pass
    nodes*: seq[Node]

  Grower* = ref object
    ## Persistent buffers across uses, retained for better pre-allocation.
    ## Cleared retaining capacity for each new run.
    nodes: seq[Node]
    working: seq[Node]

  Parsing = ref object
    ## Information for a particular parse.
    index: int32
    grower: Grower
    tokens: Tokens

# Support.

func len(slice: NodeSlice): int = slice.thru - slice.idx + 1

proc printIndent(indent: int) = stdout.write indent.spaces

proc print(tree: Tree, nodeId: NodeId, pool: Pool[TextId], indent: int) =
  let node = tree.nodes[nodeId]
  printIndent(2 * indent)
  case node.kind:
  of leaf:
    echo node.token.kind, ": '", pool[node.token.text], "'"
  else:
    echo node.kind
    for kid in node.kids.idx .. node.kids.thru:
      tree.print(kid, pool = pool, indent = indent + 1)
    if node.kids.len > 1:
      printIndent(2 * indent)
      echo "/", node.kind

proc print*(tree: Tree, pool: Pool[TextId]) =
  tree.print(NodeId tree.nodes.high, pool = pool, indent = 0)

func token(parsing: Parsing): Token = parsing.tokens.tokens[parsing.index]

proc advance(parsing: var Parsing) =
  # Bounds checked access here asserts that nobody eats the eof.
  parsing.grower.working.add(Node(kind: leaf, token: parsing.token))
  parsing.index += 1

func here(parsing: Parsing): int32 = int32 parsing.grower.working.len

func sliceFrom(begin: NodeId, til: NodeId): NodeSlice =
  NodeSlice(idx: begin, thru: til - 1)

func peek(parsing: Parsing): TokenKind = parsing.token.kind

proc advanceIf(parsing: var Parsing, tokenKinds: set[TokenKind]): bool =
  let match = parsing.peek in tokenKinds
  if match:
    parsing.advance
  match

proc advanceIf(parsing: var Parsing, tokenKind: TokenKind): bool =
  parsing.advanceIf({tokenKind})

proc nest(parsing: var Parsing, kind: NodeKind, begin: NodeId) =
  # TODO Any way to define a type that statically asserts this?
  assert kind != leaf
  let
    grower = addr parsing.grower
    nodesBegin = NodeId grower.nodes.len
  grower.nodes.add(grower.working[begin ..< ^0])
  grower.working.setLen(begin)
  {.cast(uncheckedAssign).}: # For `kind` that's asserted above.
    let parent = Node(
      kind: kind,
      kids: sliceFrom(nodesBegin, til = NodeId grower.nodes.len),
    )
  grower.working.add(parent)

proc nestMaybe(parsing: var Parsing, kind: NodeKind, begin: NodeId) =
  if parsing.here > begin:
    parsing.nest(kind, begin)

proc nest(parsing: var Parsing, kind: NodeKind, ifWhile: set[TokenKind]) =
  let begin = parsing.here
  while parsing.peek in ifWhile:
    parsing.advance
  parsing.nestMaybe(kind, begin)

# Parsing rules.

proc expression(parsing: var Parsing)

proc hspace(parsing: var Parsing) =
  parsing.nest(space, ifWhile = {TokenKind.comment, hspace})

proc space(parsing: var Parsing) =
  parsing.nest(space, ifWhile = {TokenKind.comment, hspace, vspace})

proc group(parsing: var Parsing, until: set[TokenKind]) =
  parsing.expression
  parsing.space
  while not (parsing.peek in until):
    if parsing.peek == roundEnd:
      # Avoid infinite loops.
      parsing.advance
      parsing.space
    parsing.expression
    parsing.space

proc ender(parsing: var Parsing) =
  let begin = parsing.here
  if parsing.peek == keyEnd:
    parsing.advance
    parsing.hspace
    if parsing.peek in idLike:
      parsing.advance
      parsing.nest(prefix, begin)

proc bloc(parsing: var Parsing) =
  let begin = parsing.here
  parsing.advance
  parsing.hspace
  if parsing.peek == vspace:
    # Actual block.
    # Can put a second space node in a row, but eh.
    parsing.space
    parsing.group {eof, keyEnd}
    parsing.ender
  else:
    # Single nested expression.
    parsing.expression
  parsing.nest(group, begin)

proc round(parsing: var Parsing) =
  let begin = parsing.here
  parsing.advance
  parsing.group {eof, keyEnd, roundEnd}
  discard parsing.advanceIf(roundEnd)
  parsing.nest(group, begin)

proc atom(parsing: var Parsing) =
  case parsing.peek:
  of {eof, hspace, opIs, roundEnd, vspace}:
    return
  of keyBe, keyOf:
    parsing.bloc
  of roundBegin:
    parsing.round
  else:
    parsing.advance

proc infix(
  parsing: var Parsing,
  ops: set[TokenKind],
  next: proc(parsing: var Parsing)
) =
  let begin = parsing.here
  parsing.next
  parsing.hspace
  while parsing.advanceIf(ops):
    parsing.space
    parsing.next
    parsing.nest infix, begin

proc addSub(parsing: var Parsing) = parsing.infix {opAdd, opSub}: atom

proc compare(parsing: var Parsing) =
  parsing.infix {opEq, opGe, opGt, opLe, opLt, opNe}: addSub

proc isType(parsing: var Parsing) = parsing.infix {keyIs}: compare

proc colon(parsing: var Parsing) = parsing.infix {opColon}: isType

proc call(parsing: var Parsing) =
  let begin = parsing.here
  parsing.colon
  parsing.hspace
  while not (parsing.peek in {eof, opIs, roundEnd, vspace}):
    parsing.colon
    parsing.hspace
  parsing.nestMaybe(prefix, begin)

proc define(parsing: var Parsing) =
  let begin = parsing.here
  parsing.call
  parsing.hspace
  if parsing.advanceIf(opIs):
    parsing.space
    # Right associative.
    parsing.define
    parsing.nest infix, begin

proc expression(parsing: var Parsing) = parsing.define

# Top level.

proc parse(parsing: var Parsing): Tree =
  let grower = addr parsing.grower
  grower.nodes.setLen(0)
  grower.working.setLen(0)
  while parsing.peek != eof:
    if parsing.peek in {keyEnd, roundEnd}:
      # Avoid infinite loops.
      parsing.advance
      parsing.space
    parsing.expression
    parsing.space
  parsing.nestMaybe(group, 0)
  # Final nest pushes down the outer block if it exists.
  parsing.nest(group, 0)
  Tree(pass: parse, nodes: grower.nodes)

proc newGrower*(): Grower = Grower(
  nodes: newSeq[Node](),
  working: newSeq[Node](),
)

proc parse*(grower: var Grower, tokens: Tokens): Tree =
  var parsing = Parsing(index: 0, grower: grower, tokens: tokens)
  parsing.parse
