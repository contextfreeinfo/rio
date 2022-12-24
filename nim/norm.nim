import intern
import lex
import parse

type
  Normer = object
    grower: Grower
    tree: Tree

  Norming = ptr Normer

func kidAt*(nodes: seq[Node], node: Node, offset: NodeId = 0): Node =
  nodes[node.kids.idx + offset]

func kidAt*(tree: Tree, node: Node, offset: NodeId = 0): Node =
  tree.nodes.kidAt(node, offset)

func kidsLast*(node: Node): NodeId =
  case node.kind:
  of leaf, num: raise ValueError.newException "kids invalid"
  of prefixt: node.kids.thru - 1
  else: node.kids.thru

func kidIds*(node: Node): Slice[NodeId] = node.kids.idx .. node.kidsLast

func callee*(nodes: seq[Node], node: Node): Node =
  case node.kind:
  of leaf: node
  of infix: nodes.kidAt(node, 1)
  of prefix, prefixt: nodes.kidAt(node)
  # TODO Can we just panic? Should this return optional???
  else: raise ValueError.newException "callee invalid"

func callee*(tree: Tree, node: Node): Node = tree.nodes.callee(node)

func tokenKind*(node: Node): TokenKind =
  case node.kind:
  of leaf: node.token.kind
  else: other

func calleeKind*(nodes: seq[Node], node: Node): TokenKind =
  nodes.callee(node).tokenKind

func calleeKind*(tree: Tree, node: Node): TokenKind =
  tree.nodes.calleeKind(node)

proc add(norming: var Norming, node: Node) = norming.grower.working.add(node)

func here(norming: Norming): NodeId = norming.grower.here

proc nest(norming: var Norming, kind: NodeKind, begin: NodeId) =
  norming.grower.nest(kind, begin)

func kidAt(norming: Norming, node: Node, offset: NodeId = 0): Node =
  norming.tree.kidAt(node, offset)

proc nestMaybe(norming: var Norming, kind: NodeKind, begin: NodeId) =
  norming.grower.nestMaybe(kind, begin)

proc simplifyAny(norming: var Norming, node: Node)

proc simplifyGeneric(norming: var Norming, node: Node) =
  let begin = norming.here
  for kidId in node.kids.idx .. node.kids.thru:
    norming.simplifyAny(norming.tree.nodes[kidId])
  if node.kind == prefix and norming.here == begin + 1:
    return
  norming.nestMaybe(node.kind, begin)

proc simplifyDot(norming: var Norming, node: Node) =
  let callee = norming.kidAt(node)
  if callee.kids.len == 3:
    # Reorder and uplift to simple prefix.
    let begin = norming.here
    norming.simplifyAny(norming.kidAt(callee, 2))
    norming.simplifyAny(norming.kidAt(callee))
    for kidId in node.kids.idx + 1 .. node.kids.thru:
      norming.simplifyAny(norming.tree.nodes[kidId])
    norming.nest(prefix, begin)
  else:
    # Don't have the right parts for this anyway.
    norming.simplifyGeneric(node)

proc simplifyInfix(norming: var Norming, node: Node) =
  let begin = norming.here
  # Convert infix to prefix.
  norming.add(norming.kidAt(node, 1))
  norming.simplifyAny(norming.kidAt(node))
  if node.kids.len == 3:
    norming.simplifyAny(norming.kidAt(node, 2))
  norming.nest(prefix, begin)

proc simplifyOf(norming: var Norming, node: Node) =
  for kidId in node.kids.idx + 1 .. node.kids.thru:
    norming.simplifyAny(norming.tree.nodes[kidId])

proc simplifyRound(norming: var Norming, node: Node) =
  if node.kids.len == 2:
    let tail = norming.kidAt(node, 1)
    if tail.kind == leaf and tail.token.kind == roundEnd:
      # Use a single roundBegin to represent the nil tuple.
      norming.add(norming.kidAt(node))
      return
  norming.simplifyGeneric(node)

proc simplifyAny(norming: var Norming, node: Node) =
  case node.kind:
  of leaf, num:
    case node.token.kind
    # Don't need extra groupers anymore.
    of keyEnd, roundBegin, roundEnd: discard
    else: norming.add(node)
  of infix: norming.simplifyInfix(node)
  of prefix:
    let callee = norming.tree.callee(node)
    case callee.kind:
    of leaf:
      case callee.token.kind:
      # TODO Assert matching id before discard.
      of keyEnd: discard
      of keyOf: norming.simplifyOf(node)
      of roundBegin: norming.simplifyRound(node)
      else: norming.simplifyGeneric(node)
    of infix:
      case norming.kidAt(callee, 1).token.kind:
      of opDot: norming.simplifyDot(node)
      else: norming.simplifyGeneric(node)
    else: norming.simplifyGeneric(node)
  of top: norming.simplifyGeneric(node)
  # TODO Support prefixt here in the future?
  of space, prefixt: assert false

proc spacelessAction(norming: var Norming, node: Node) =
  if node.kind == space:
    return
  # Add all nonspace.
  case node.kind:
  of leaf:
    norming.add(node)
  else:
    let begin = norming.here
    for kidId in node.kids.idx .. node.kids.thru:
      norming.spacelessAction(norming.tree.nodes[kidId])
    if node.kind == prefix and norming.here == begin + 1:
      return
    norming.nestMaybe(node.kind, begin)

proc norm(
  grower: var Grower,
  pass: Pass,
  tree: Tree,
  action: proc(norming: var Norming, node: Node),
  sourceId: TextId = 0,
): Tree =
  grower.nodes.setLen(0)
  grower.working.setLen(0)
  var normer = Normer(grower: grower, tree: tree)
  var norming = addr normer
  let begin = norming.here
  echo "begin ", begin
  if sourceId != 0:
    # Tag source file.
    norming.add(Node(kind: leaf, token: Token(kind: id, text: pubId)))
    norming.add(Node(kind: leaf, token: Token(kind: id, text: sourceId)))
    norming.nest(prefix, begin)
    echo "norming.here ", norming.here
  norming.action(tree.root)
  echo "norming.here2 ", norming.here
  norming.nestMaybe(top, begin)
  Tree(pass: pass, nodes: grower.nodes, uid: 0)

proc spaceless(grower: var Grower, tree: Tree): Tree =
  grower.norm(spaceless, tree): spacelessAction

proc simplified(grower: var Grower, tree: Tree, sourceId: TextId): Tree =
  grower.norm(simplified, tree, action = simplifyAny, sourceId = sourceId)

proc normed*(grower: var Grower, tree: Tree, sourceId: TextId): Tree =
  result = tree
  result = grower.spaceless(result)
  result = grower.simplified(result, sourceId = sourceId)
