import intern
import lex
import norm
import parse
import std/algorithm
import std/tables

type
  Module* = ref object
    # TODO Links from resolved to parsed.
    # TODO Replace parsed with a list of ranges? Would be only 8 bytes each.
    parsed*: Tree
    resolved*: Tree

  TypeKind = enum
    typeFun
    typeInt
    typeString
    typeType
    typeUnknown

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
    # TODO If we get comptime, do we need a value rather than a node???
    node: NodeId
    pub: bool
    text: TextId
    uid: NodeNum

  DefId = int

  Defs = ref object
    ## Enables keeping things sorted by text key without having a separate list
    ## of overloads for each key.
    defs: seq[Def]
    stackDefs: seq[Def]
    table: Table[TextId, int]

  Runner = object
    defs: Defs
    grower: Grower
    pool*: Pool[TextId] # Use only for ad hoc debugging.
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

proc runNode(
  running: var Running,
  parent: Node,
  node: Node,
  nodeId: NodeId,
  inUdef: bool = false,
)

# proc runToken(running: var Running, nodeId: NodeId, token: Token): Value =
#   case token.kind:
#   of integer:
#     Value(kind: valConst, value: nodeId, constType: Type(kind: typeInt))
#   of stringText:
#     Value(kind: valConst, value: nodeId, constType: Type(kind: typeString))
#   else:
#     Value(kind: valNone)

func find(defs: Defs, text: TextId): Def =
  # First look through local stacks.
  # TODO Just add to overload set?
  for stackDefId in countdown(defs.stackDefs.high, 0):
    let stackDef = defs.stackDefs[stackDefId]
    if stackDef.text == text:
      return stackDef
  # Then check top levels.
  let defId = defs.table.getOrDefault(text, 0)
  if defId > 0:
    return defs.defs[defId]

func getFirstId(nodes: seq[Node], node: Node): Token =
  case node.kind:
  of leaf: node.token
  of num: raise ValueError.newException "id invalid"
  of prefix:
    nodes.getFirstId(
      nodes.kidAt(
        node,
        case nodes.calleeKind(node):
        of keyIs: 1
        else: 0
      )
    )
  else: nodes.getFirstId(nodes.kidAt(node))

func buildDef(tree: Tree, nodeId: NodeId, moduleId: int32 = 0): Def =
  let
    node = tree.nodes[nodeId]
    numNode = tree.nodes[node.kids.idx + 1]
    idNode = tree.nodes[node.kids.idx + 2]
    idToken = tree.nodes.getFirstId(idNode)
  var isPub = false
  if idNode.kind notin {leaf, num}:
    block pub:
      for idKidId in idNode.kidIds:
        let idKid = tree.nodes[idKidId]
        if idKid.kind == leaf and idKid.token.text == pubId:
          isPub = true
          break pub
  # TODO Need a tree or module also?
  Def(
    node: nodeId,
    pub: isPub,
    text: idToken.text,
    uid: NodeNum(signed: moduleId, unsigned: numNode.num.unsigned),
  )

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

proc extractTops(running: Running, moduleId: int32 = 0) =
  ## Get just the top-level udefs.
  let tree = running.tree
  for kidId in tree.root.kidIds:
    # Struct members should be generated as top-level functions.
    let kid = tree.nodes[kidId]
    if tree.calleeKind(kid) == udef:
      let def = buildDef(tree, kidId, moduleId = moduleId)
      if moduleId == 0 or def.pub:
        running.defs.defs.add(def)
  # Sort and point to the first for each group.
  running.defs.defs.sort(func (x, y: Def): int = x.text - y.text)
  var lastText: TextId = 0
  for index, def in running.defs.defs.pairs:
    if def.text != lastText:
      running.defs.table[def.text] = index
  echo("defs: ", running.defs.defs)
  echo("table: ", running.defs.table)

proc extractExports(running: var Running, module: Module) =
  # Not all these fields get used for extractTops needs.
  # TODO Reduce needs for extractTops?
  var importing = Runner(
    defs: running.defs,
    grower: running.grower,
    pool: running.grower.pool,
    tree: module.resolved,
    uid: 0,
  )
  (addr importing).extractTops(moduleId = module.resolved.moduleId)

proc runDef(running: var Running, node: Node) =
  let tree = running.tree
  running.buildUdef node, proc(running: var Running): Node =
    # Skip the original opDef.
    for kidId in node.kids.idx + 1 .. node.kidsLast:
      running.runNode(parent = node, node = tree.nodes[kidId], nodeId = kidId)
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
        running.runNode(parent = node, node = kid, nodeId = kidId)
        unknownType
    else:
      running.runNode(parent = node, node = kid, nodeId = kidId)
  running.nest(prefix, begin)

proc runQuote(running: var Running, node: Node) =
  let
    begin = running.here
    tree = running.tree
  for kidId in node.kidIds:
    running.runNode(parent = node, node = tree.nodes[kidId], nodeId = kidId)
  # In the future, these could also become tuples.
  running.add(stringType)
  running.nest(prefixt, begin)

proc runPrefix(
  running: var Running,
  parent: Node,
  node: Node,
  nodeId: NodeId,
  inUdef: bool = false,
) =
  let
    defBegin = running.defs.stackDefs.len
    begin = running.here
    tree = running.tree
    calleeKind = tree.calleeKind(node)
    nowInUdef = calleeKind == udef
  # TODO Generalized call dispatch.
  case calleeKind:
  of keyFor:
    # TODO If parent is call, and we're last and grand is udef, then keep
    # TODO stackDefs for value.
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
  of udef:
    if parent.kind != top:
      let def = buildDef(tree, nodeId)
      # TODO Support stacks of tables in case many at some level?
      running.defs.stackDefs.add(def)
      # text: idToken.text,
      # uid: numNode.num.unsigned,
      echo("saw udef ", running.pool[def.text], " ", def.uid, " ", inUdef)
  else:
    discard
  # Use raw kid ids by default here and original node kind to preserve typed.
  let lastId = node.kids.thru
  for kidId in node.kids.idx .. lastId:
    let kid = tree.nodes[kidId]
    running.runNode(
      parent = node,
      node = kid,
      nodeId = kidId,
      inUdef =
        (nowInUdef and kidId < lastId) or
        (inUdef and tree.calleeKind(parent) == udef),
    )
  running.nest(node.kind, begin)
  if not (inUdef or nowInUdef):
    running.defs.stackDefs.setLen(defBegin)

proc runNode(
  running: var Running,
  parent: Node,
  node: Node,
  nodeId: NodeId,
  inUdef: bool = false,
) =
  case node.kind:
  of leaf:
    if node.token.kind == id:
      let found = running.defs.find(node.token.text)
      echo("find id ", running.pool[node.token.text], " at ", found.uid)
    running.add(node)
  of num:
    running.add(node)
  of prefix, prefixt:
    running.runPrefix(
      parent = parent, node = node, nodeId = nodeId, inUdef = inUdef
    )
  else:
    discard

proc runTop(running: var Running, node: Node) =
  let
    begin = running.here
    tree = running.tree
  for kidId in node.kidIds:
    let kid = tree.nodes[kidId]
    running.runNode(parent = node, node = kid, nodeId = kidId)
  running.nest(top, begin)

proc resolveOnce(grower: var Grower, imports: seq[Module], tree: Tree): Tree =
  # TODO Some buffer type for running like for parsing and norming?
  # TODO Resolve imports.
  # TODO Put top levels into seq then sort by name.
  # TODO Also a table for lookup?
  # TODO Struct members are brought out to their level.
  # TODO Recurse keeping explicit stack of local definitions to wade through.
  var
    runner = Runner(
      defs: Defs(), grower: grower, pool: grower.pool, tree: tree, uid: tree.uid
    )
    running = addr runner
  # Add a bogus at 0, so we can use 0 as broken.
  running.defs.defs.add(
    Def(node: -1, pub: false, text: 0, uid: NodeNum(signed: 0, unsigned: 0))
  )
  for imp in imports:
    running.extractExports(imp)
  running.uid += 1
  # Extract tops.
  running.extractTops
  # Build round.
  grower.nodes.setLen(0)
  grower.working.setLen(0)
  let begin = running.here
  running.runTop running.tree.root
  running.nest(top, begin)
  result = Tree(pass: resolve, nodes: grower.nodes, uid: uint32(running.uid))

proc resolve*(grower: var Grower, imports: seq[Module], tree: Tree): Tree =
  # TODO More robust imports.
  result = tree
  # TODO Pick a good max iterations.
  for i in 1..5:
    let old = result
    result = grower.resolveOnce(
      # Skip imports first time so local tops can compete.
      imports = if i > 1: imports else: @[],
      tree = result,
    )
    if result.nodes == old.nodes:
      echo "Same at ", i
      break
    echo "Changed at ", i
