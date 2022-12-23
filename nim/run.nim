import intern
import lex
import norm
import parse
import std/tables

type
  Module* = ref object
    # TODO Links from resolved to parsed.
    parsed*: Tree
    resolved*: Tree

  TypeKind = enum
    typeFun,
    typeInt,
    typeString,
    typeType,
    typeUnknown,

  Type = object
    case kind: TypeKind
    of typeFun:
      # TODO Module ref?
      sig: NodeId
    of typeInt, typeString, typeUnknown: discard
    of typeType: discard

  ValueKind = enum
    valAny
    valConst
    valFun
    # valInt
    # valIntText
    valNone # Just for modules. TODO Should be struct consts instead???
    # valString
    valType

  Value = object
    case kind: ValueKind
    of valAny:
      anyType: TypeKind
    of valConst:
      # TODO Module ref?
      value: NodeId
      constType: Type
    of valFun:
      sig: NodeId
      body: NodeId
    # of valInt:
    #   # TODO Some way to represent overflow?
    #   intVal: int64
    # of valIntText:
    #   intText: TextId
    of valNone: discard
    # of valString:
    #   # Any precat needs added to the interns.
    #   stringText: TextId
    of valType:
      typeType: Type

  Def = object
    name: Token
    # TODO If we get comptime, do we need a value rather than a node???
    node: NodeId

  DefId = int

  Defs = ref object
    defs: seq[Def]
    table: Table[TextId, int]

  Runner = object
    defs: Defs
    grower: Grower
    tree: Tree
    uid: Uid

  Running = ptr Runner

const
  stringType = Node(kind: leaf, token: Token(kind: stringText, text: emptyId))
  unknownType = Node(kind: leaf, token: Token(kind: other, text: emptyId))

proc add(running: var Running, node: Node) = running.grower.working.add(node)

func here(running: Running): NodeId = running.grower.here

func latest(running: Running): Node =
  running.grower.working[running.grower.working.high]

proc nest(running: var Running, kind: NodeKind, begin: NodeId) =
  running.grower.nest(kind, begin)

proc extractTops(running: var Running) =
  let
    tree = running.tree
    root = tree.root
  for kidId in root.kidIds:
    let kid = tree.nodes[kidId]
    # Struct members should be generated as top-level functions by now.
    # TODO For const strings like "name" we have a TextId for the token inside.
    # TODO name case = for person is Person to String be `person.get "name"`
    # TODO name for person is Person to String = `person.get "name"`
    if tree.calleeKind(kid) == opDef:
      let target = tree.kidAt(kid, 1)
      # TODO Destructuring at top level? Only for imports???
      if target.kind == leaf:
        running.defs.defs.add(Def(name: target.token, node: kidId))
  echo "Defs: ", running.defs.defs

proc runNode(running: var Running, parent: Node, node: Node)

# proc runToken(running: var Running, nodeId: NodeId, token: Token): Value =
#   case token.kind:
#   of integer:
#     Value(kind: valConst, value: nodeId, constType: Type(kind: typeInt))
#   of stringText:
#     Value(kind: valConst, value: nodeId, constType: Type(kind: typeString))
#   else:
#     Value(kind: valNone)

proc buildUdef(
  running: var Running,
  node: Node,
  action: proc(running: var Running): Node
) =
  let begin = running.here
  running.add Node(kind: leaf, token: Token(kind: udef, text: emptyId))
  running.add Node(kind: num, num: NodeNum(signed: 0, unsigned: running.uid))
  running.uid += 1
  let typ = running.action()
  running.add(typ)
  running.nest(prefixt, begin)

func getFirstId(nodes: seq[Node], node: Node): Token =
  case node.kind:
  of leaf: node.token
  of num: raise ValueError.newException "id invalid"
  else: nodes.getFirstId(nodes.kidAt(node))

proc extractPubs(tree: Tree) =
  ## Get just the published top-levels.
  for kidId in tree.root.kidIds:
    let kid = tree.nodes[kidId]
    if tree.calleeKind(kid) == udef:
      let idNode = tree.nodes[kid.kids.idx + 2]
      for idKidId in idNode.kidIds:
        let idKid = tree.nodes[idKidId]
        if idKid.kind == leaf and idKid.token.text == pubId:
          echo "pub udef: ", tree.nodes.getFirstId(idNode).text

proc runDef(running: var Running, node: Node) =
  let tree = running.tree
  running.buildUdef node, proc(running: var Running): Node =
    # Skip the original opDef.
    for kidId in node.kids.idx + 1 .. node.kidsLast:
      running.runNode(parent = node, node = tree.nodes[kidId])
    unknownType

proc runFor(running: var Running, node: Node) =
  let
    begin = running.here
    tree = running.tree
  # TODO Args, return type, maybe be. Effects?
  for kidId in node.kidIds:
    let kid = tree.nodes[kidId]
    case tree.calleeKind(kid):
    of keyIs, id:
      # TODO Only do this for id if kid.kind == id?
      running.buildUdef kid, proc(running: var Running): Node =
        running.runNode(parent = node, node = kid)
        unknownType
    else:
      running.runNode(parent = node, node = kid)
  running.nest(prefix, begin)

proc runQuote(running: var Running, node: Node) =
  let
    begin = running.here
    tree = running.tree
  for kidId in node.kidIds:
    running.runNode(parent = node, node = tree.nodes[kidId])
  # In the future, these could also become tuples.
  running.add(stringType)
  running.nest(prefixt, begin)

proc runPrefix(running: var Running, parent: Node, node: Node) =
  let
    begin = running.here
    tree = running.tree
  # TODO Generalized call dispatch.
  case tree.calleeKind(node):
  of keyFor:
    running.runFor(node)
    return
  of opDef:
    # TODO Gather up definitions.
    # TODO If destructuring, perform destructure down to single?
    # TODO How to represent overloads?
    running.runDef(node)
    return
  of quoteDouble:
    running.runQuote(node)
    return
  else:
    discard
  # Use raw kid ids by default here and original node kind to preserve typed.
  for kidId in node.kids.idx .. node.kids.thru:
    let kid = tree.nodes[kidId]
    running.runNode(parent = node, node = kid)
  running.nest(node.kind, begin)

proc runNode(running: var Running, parent: Node, node: Node) =
  case node.kind:
  of leaf:
    running.add(node)
  of num:
    running.add(node)
  of prefix, prefixt:
    running.runPrefix(parent = parent, node = node)
  else:
    discard

proc runTop(running: var Running, node: Node) =
  let
    begin = running.here
    tree = running.tree
  for kidId in node.kidIds:
    let kid = tree.nodes[kidId]
    running.runNode(parent = node, node = kid)
    let latest = running.latest
    if latest.kind in {prefix, prefixt}:
      if running.grower.nodes.calleeKind(latest) == udef:
        # TODO Instead of logging, track uid -> nodeid.
        let
          # For udefs, we have the id after "udef" and the uid.
          idNode = running.grower.nodes[latest.kids.idx + 2]
          idToken = running.grower.nodes.getFirstId(idNode)
        stdout.write "udef "
        idToken.print(pool = running.grower.pool)
  running.nest(top, begin)

proc resolveOnce(grower: var Grower, tree: Tree): Tree =
  # TODO Some buffer type for running like for parsing and norming?
  # TODO Resolve imports.
  # TODO Put top levels into seq then sort by name.
  # TODO Also a table for lookup?
  # TODO Struct members are brought out to their level.
  # TODO Recurse keeping explicit stack of local definitions to wade through.
  var
    # TODO Get uid from tree!
    resolver = Runner(defs: Defs(), grower: grower, tree: tree, uid: tree.uid)
    running = addr resolver
  # running.extractTops
  grower.nodes.setLen(0)
  grower.working.setLen(0)
  let begin = running.here
  running.runTop running.tree.root
  running.nest(top, begin)
  Tree(pass: resolve, nodes: grower.nodes, uid: uint32(running.uid))

proc resolve*(grower: var Grower, imports: seq[Module], tree: Tree): Tree =
  result = tree
  # TODO Pick a good max iterations.
  for i in 1..5:
    let old = result
    result = grower.resolveOnce(result)
    if result.nodes == old.nodes:
      echo "Same at ", i
      break
    echo "Changed at ", i
  result.extractPubs
