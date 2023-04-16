import intern
import lex
import system/assertions
import std/strutils
import strformat

type
  NodeKind* = enum
    leaf
    f64
    infix
    num
    prefix
    prefixt
    space
    top
    uref

  NodeId* = int32

  NodeNum* = object
    # Technically could use multiple num nodes for a larger int.
    # Meanwhile, can't do an int64 here without throwing off 4-byte alignment.
    signed*: int32
    unsigned*: uint32

  NodeSlice* = object
    idx*: NodeId
    thru*: NodeId

  Node* {.packed.} = object
    pad1: uint8
    pad2: uint16
    case kind*: NodeKind
    of leaf:
      token*: Token
    of f64:
      # Bad for packed, but slower might be better than 16 bytes.
      f64Val*: float64
    of num, uref:
      num*: NodeNum
    else:
      kids*: NodeSlice

  Pass* = enum
    parse
    resolve
    spaceless
    # After simplified is just leaf, prefix, and top.
    simplified

  Uid* = uint32

  Tree* = ref object
    pass*: Pass
    nodes*: seq[Node]
    uid*: Uid

  Grower* = ref object
    ## Persistent buffers across uses, retained for better pre-allocation.
    ## Cleared retaining capacity for each new run.
    # Use lexer only for controlled diagnostics.
    nodes*: seq[Node]
    pool*: Pool[TextId]
    working*: seq[Node]

  Parser = object
    ## Information for a particular parse.
    ## TODO Track ids for contextual block closing?
    index: int32
    grower: Grower
    tokens: Tokens

  Parsing = ptr Parser

# Support.

func `$`*(num: NodeNum): string = fmt"({num.signed}, {num.unsigned})"

func `==`*(x, y: Node): bool =
  ## Super simple compare that's only useful if a whole node seq is equal.
  x.kind == y.kind and (
    case x.kind:
    of leaf: x.token == y.token
    of num: x.num == y.num
    else: x.kids == y.kids
  )

func len*(slice: NodeSlice): int = slice.thru - slice.idx + 1

func rootId*(tree: Tree): NodeId = NodeId tree.nodes.high

func root*(tree: Tree): Node = tree.nodes[tree.rootId]

proc printIndent(indent: int, file: File = stdout) = file.write indent.spaces

proc print*(token: Token, pool: Pool[TextId], file: File = stdout) =
    if token.text == emptyId:
      file.writeLine token.kind
    else:
      file.writeLine token.kind, ": '", pool[token.text], "'"

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
    node.token.print(pool = pool, file = file)
  of f64:
    file.writeLine "f64: ", node.f64Val
  of num:
    file.writeLine "num: ", node.num.signed, " ", node.num.unsigned
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

func peek(parsing: Parsing): TokenKind = parsing.token.kind

proc advance(parsing: var Parsing) =
  # Bounds checked access here asserts that nobody eats the eof.
  parsing.grower.working.add(Node(kind: leaf, token: parsing.token))
  parsing.index += 1

func here*(grower: Grower): NodeId = NodeId grower.working.len

func here(parsing: Parsing): NodeId = parsing.grower.here

func sliceFrom(begin: NodeId, til: NodeId): NodeSlice =
  NodeSlice(idx: begin, thru: til - 1)

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
    if parsing.peek in {roundEnd, squareEnd}:
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

proc quote(parsing: var Parsing) =
  let begin = parsing.here
  parsing.advance
  while true:
    case parsing.peek:
    of quoteDouble:
      parsing.advance
      break
    of eof, vspace:
      break
    else:
      parsing.advance
  parsing.nest(prefix, begin)

proc round(parsing: var Parsing) =
  let begin = parsing.here
  parsing.advance
  parsing.space
  parsing.group {eof, keyEnd, roundEnd}
  discard parsing.advanceIf(roundEnd)
  # Ends are whitespace after parsing and validation.
  parsing.nest(prefix, begin)

proc square(parsing: var Parsing) =
  let begin = parsing.here
  parsing.advance
  parsing.space
  parsing.group {eof, keyEnd, squareEnd}
  discard parsing.advanceIf(squareEnd)
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
    of eof, roundEnd, squareEnd: break
    of keyBe:
      parsing.bloc
      break
    of keyEnd:
      parsing.advance
      break
    of keyTo:
      # Would get here eventually, but might as well short-circuit.
      parsing.to
    of opDef: break
    else: parsing.simpleExpression
  parsing.nest(prefix, begin)

proc atom(parsing: var Parsing) =
  case parsing.peek:
  of {eof, hspace, opDef, roundEnd, squareEnd, vspace}: return
  of keyBe: parsing.bloc
  of keyFor: parsing.fun
  of keyOf: parsing.bloc
  of keyTo: parsing.to
  of quoteDouble: parsing.quote
  of roundBegin: parsing.round
  of squareBegin: parsing.square
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
  while not (parsing.peek in {eof, opDef, roundEnd, squareEnd, vspace}):
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
  if parsing.advanceIf(opDef):
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
    if parsing.peek in {keyEnd, roundEnd, squareEnd}:
      # Avoid infinite loops.
      parsing.advance
      parsing.space
    parsing.expression
    parsing.space
  parsing.nestMaybe(top, 0)
  # Final nest pushes down the outer block if it exists.
  parsing.nest(top, 0)
  Tree(pass: parse, nodes: grower.nodes, uid: 0)

proc newGrower*(pool: Pool[TextId]): Grower = Grower(
  nodes: newSeq[Node](),
  pool: pool,
  working: newSeq[Node](),
)

proc parse*(grower: var Grower, tokens: Tokens): Tree =
  var parser = Parser(index: 0, grower: grower, tokens: tokens)
  var parsing = addr parser
  parsing.parse
