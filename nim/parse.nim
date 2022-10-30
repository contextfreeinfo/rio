import intern
import lex
import system/assertions
import std/strutils

type
  NodeKind* = enum
    leaf,
    prefix,
    infix,
    space,
    top,

  NodeId* = int32

  NodeSlice* = object
    idx*: NodeId
    thru*: NodeId

  Node* = object
    case kind*: NodeKind
    of leaf:
      token*: Token
    else:
      kids*: NodeSlice

  Pass* = enum
    parse
    spaceless
    simplified

  Tree* = ref object
    pass*: Pass
    nodes*: seq[Node]

  Grower* = ref object
    ## Persistent buffers across uses, retained for better pre-allocation.
    ## Cleared retaining capacity for each new run.
    nodes*: seq[Node]
    working*: seq[Node]

  Parser = object
    ## Information for a particular parse.
    ## TODO Track ids for contextual block closing?
    index: int32
    grower: Grower
    tokens: Tokens

  Parsing = ptr Parser

# Support.

func len*(slice: NodeSlice): int = slice.thru - slice.idx + 1

func rootId*(tree: Tree): NodeId = NodeId tree.nodes.high

func root*(tree: Tree): Node = tree.nodes[tree.rootId]

proc printIndent(indent: int, file: File = stdout) = file.write indent.spaces

proc print(
  tree: Tree,
  nodeId: NodeId,
  pool: Pool[TextId],
  indent: int,
  file: File = stdout,
) =
  let node = tree.nodes[nodeId]
  printIndent(2 * indent, file = file)
  case node.kind:
  of leaf:
    file.writeLine node.token.kind, ": '", pool[node.token.text], "'"
  else:
    file.writeLine node.kind
    for kid in node.kids.idx .. node.kids.thru:
      tree.print(kid, file = file, pool = pool, indent = indent + 1)
    if node.kids.len > 1:
      printIndent(2 * indent, file = file)
      file.writeLine "/", node.kind

proc print*(tree: Tree, pool: Pool[TextId], file: File = stdout) =
  tree.print(tree.rootId, file = file, pool = pool, indent = 0)

func token(parsing: Parsing): Token = parsing.tokens.tokens[parsing.index]

proc advance(parsing: var Parsing) =
  # Bounds checked access here asserts that nobody eats the eof.
  parsing.grower.working.add(Node(kind: leaf, token: parsing.token))
  parsing.index += 1

func here*(grower: Grower): NodeId = NodeId grower.working.len

func here(parsing: Parsing): NodeId = parsing.grower.here

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

proc nest*(grower: var Grower, kind: NodeKind, begin: NodeId) =
  # TODO Any way to define a type that statically asserts this?
  assert kind != leaf
  let
    nodesBegin = NodeId grower.nodes.len
  grower.nodes.add(grower.working[begin ..< ^0])
  grower.working.setLen(begin)
  {.cast(uncheckedAssign).}: # For `kind` that's asserted above.
    let parent = Node(
      kind: kind,
      kids: sliceFrom(nodesBegin, til = NodeId grower.nodes.len),
    )
  grower.working.add(parent)

proc nestMaybe*(grower: var Grower, kind: NodeKind, begin: NodeId) =
  if grower.here > begin:
    grower.nest(kind, begin)

proc nest(parsing: var Parsing, kind: NodeKind, begin: NodeId) =
  parsing.grower.nest(kind, begin)

proc nestMaybe(parsing: var Parsing, kind: NodeKind, begin: NodeId) =
  parsing.grower.nestMaybe(kind, begin)

proc nest(parsing: var Parsing, kind: NodeKind, ifWhile: set[TokenKind]) =
  let begin = parsing.here
  while parsing.peek in ifWhile:
    parsing.advance
  parsing.nestMaybe(kind, begin)

# Parsing rules.

proc expression(parsing: var Parsing)

proc simpleExpression(parsing: var Parsing)

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
  # Ends are whitespace after parsing and validation.
  parsing.nest(prefix, begin)

proc round(parsing: var Parsing) =
  let begin = parsing.here
  parsing.advance
  parsing.space
  parsing.group {eof, keyEnd, roundEnd}
  discard parsing.advanceIf(roundEnd)
  # Ends are whitespace after parsing and validation.
  parsing.nest(prefix, begin)

proc to(parsing: var Parsing) =
  let begin = parsing.here
  # Always starts a prefix with up to 1 expression.
  parsing.advance
  parsing.space
  parsing.simpleExpression
  parsing.nest(prefix, begin)

proc fun(parsing: var Parsing) =
  let begin = parsing.here
  parsing.advance
  while true:
    parsing.space
    case parsing.peek
    of eof:
      break
    of keyBe:
      parsing.bloc
      break
    of keyEnd:
      parsing.advance
      break
    of keyTo:
      # Would get here eventually, but might as well short-circuit.
      parsing.to
    else:
      parsing.simpleExpression
  parsing.nest(prefix, begin)

proc atom(parsing: var Parsing) =
  case parsing.peek:
  of {eof, hspace, opIs, roundEnd, vspace}: return
  of keyBe: parsing.bloc
  of keyFor: parsing.fun
  of keyOf: parsing.bloc
  of keyTo: parsing.to
  of roundBegin: parsing.round
  else: parsing.advance

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
    parsing.nest(infix, begin)

proc dot(parsing: var Parsing) = parsing.infix {opDot}: atom

proc addSub(parsing: var Parsing) = parsing.infix {opAdd, opSub}: dot

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
  if parsing.here == begin + 1:
    # See if we can get away with `(a)` and `a` meaning the same thing.
    # Maybe `a _` or some such is a nullary call?
    return
  parsing.nestMaybe(prefix, begin)

proc define(parsing: var Parsing) =
  let begin = parsing.here
  parsing.call
  parsing.hspace
  if parsing.advanceIf(opIs):
    parsing.space
    # Right associative.
    parsing.define
    parsing.nest(infix, begin)

proc expression(parsing: var Parsing) = parsing.define

proc simpleExpression(parsing: var Parsing) = parsing.colon

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
  parsing.nestMaybe(top, 0)
  # Final nest pushes down the outer block if it exists.
  parsing.nest(top, 0)
  Tree(pass: parse, nodes: grower.nodes)

proc newGrower*(): Grower = Grower(
  nodes: newSeq[Node](),
  working: newSeq[Node](),
)

proc parse*(grower: var Grower, tokens: Tokens): Tree =
  var parser = Parser(index: 0, grower: grower, tokens: tokens)
  var parsing = addr parser
  parsing.parse
