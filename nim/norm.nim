import lex
import parse

type
  Normer = object
    grower: Grower
    tree: Tree

  Norming = ptr Normer

proc add(norming: var Norming, node: Node) = norming.grower.working.add(node)

func here(norming: Norming): NodeId = norming.grower.here

proc nest(norming: var Norming, kind: NodeKind, begin: NodeId) =
  norming.grower.nest(kind, begin)

func kidAt(norming: Norming, node: Node, offset: NodeId = 0): Node =
  norming.tree.nodes[node.kids.idx + offset]

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

proc simplifyInfix(norming: var Norming, node: Node) =
  let begin = norming.here
  # Convert infix to prefix.
  norming.add(norming.kidAt(node, 1))
  norming.simplifyAny(norming.kidAt(node))
  if node.kids.len == 3:
    norming.simplifyAny(norming.kidAt(node, 2))
  norming.nest(prefix, begin)

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
  of leaf:
    case node.token.kind
    # Don't need extra groupers anymore.
    of keyEnd, roundBegin, roundEnd: discard
    else: norming.add(node)
  of infix: norming.simplifyInfix(node)
  of prefix:
    let callee = norming.kidAt(node)
    case callee.kind:
    of leaf:
      case callee.token.kind:
      # TODO Assert matching id before discard.
      of keyEnd: discard
      of roundBegin: norming.simplifyRound(node)
      else: norming.simplifyGeneric(node)
    of infix:
      # TODO Check if kid 2 is opDot
      norming.simplifyGeneric(node)
    else: norming.simplifyGeneric(node)
  of top: norming.simplifyGeneric(node)
  of space:
    assert false

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
  action: proc(norming: var Norming, node: Node)
): Tree =
  grower.nodes.setLen(0)
  grower.working.setLen(0)
  var normer = Normer(grower: grower, tree: tree)
  var norming = addr normer
  let begin = norming.here
  norming.action(tree.root)
  norming.nestMaybe(top, begin)
  Tree(pass: pass, nodes: grower.nodes)

proc spaceless*(grower: var Grower, tree: Tree): Tree =
  grower.norm(spaceless, tree): spacelessAction

proc simplified*(grower: var Grower, tree: Tree): Tree =
  grower.norm(simplified, tree): simplifyAny
