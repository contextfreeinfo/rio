import lex
import parse

type
  Normer = object
    grower: Grower
    tree: Tree

  Norming = ptr Normer

func kidAt*(tree: Tree, node: Node, offset: NodeId = 0): Node =
  tree.nodes[node.kids.idx + offset]

func callee*(tree: Tree, node: Node): Node =
  case node.kind:
  of leaf: node
  of infix, pretype: tree.kidAt(node, 1)
  of prefix: tree.kidAt(node)
  # TODO Can we just panic? Should this return optional???
  else: raise ValueError.newException "callee invalid"

func calleeKind*(tree: Tree, node: Node): TokenKind =
  let callee = tree.callee(node)
  case callee.kind:
  of leaf: callee.token.kind
  else: other

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
  # TODO Support pretype here in the future?
  of space, pretype: assert false

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
  Tree(pass: pass, nodes: grower.nodes, uid: 0)

proc spaceless(grower: var Grower, tree: Tree): Tree =
  grower.norm(spaceless, tree): spacelessAction

proc simplified(grower: var Grower, tree: Tree): Tree =
  grower.norm(simplified, tree): simplifyAny

proc normed*(grower: var Grower, tree: Tree): Tree =
  result = tree
  result = grower.spaceless(result)
  result = grower.simplified(result)
