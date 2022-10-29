import parse

type
  Normer = object
    grower: Grower
    tree: Tree

  Norming = ptr Normer

func here(norming: Norming): NodeId = norming.grower.here

# proc nest(norming: var Norming, kind: NodeKind, begin: NodeId) =
#   norming.grower.nest(kind, begin)

proc nestMaybe(norming: var Norming, kind: NodeKind, begin: NodeId) =
  norming.grower.nestMaybe(kind, begin)

proc spacelessAction(norming: var Norming, node: Node) =
  if node.kind == space:
    return
  # Add all nonspace.
  case node.kind:
  of leaf:
    norming.grower.working.add(node)
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
