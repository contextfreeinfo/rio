import lex
import norm
import parse
import std/tables

type
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
    valAny,
    valConst,
    valFun,
    # valInt,
    # valIntText,
    valNone, # Just for modules. TODO Should be struct consts instead???
    # valString,
    valType,

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
    tree: Tree

  Running = ptr Runner

proc extractTops(running: var Running) =
  let
    tree = running.tree
    root = tree.root
  for kidId in root.kids.idx .. root.kids.thru:
    let kid = tree.nodes[kidId]
    # Struct members should be generated as top-level functions by now.
    # TODO For const strings like "name" we have a TextId for the token inside.
    # TODO name case = for person is Person to String be `person.get "name"`
    # TODO name for person is Person to String = `person.get "name"`
    if tree.calleeKind(kid) == opIs:
      let target = tree.kidAt(kid, 1)
      # TODO Destructuring at top level? Only for imports???
      if target.kind == leaf:
        running.defs.defs.add(Def(name: target.token, node: kidId))
  echo "Defs: ", running.defs.defs

proc run(running: var Running, nodeId: NodeId, token: Token): Value =
  case token.kind:
  of integer:
    Value(kind: valConst, value: nodeId, constType: Type(kind: typeInt))
  of stringText:
    Value(kind: valConst, value: nodeId, constType: Type(kind: typeString))
  else:
    Value(kind: valNone)

proc run(running: var Running, node: Node): Value =
  let tree = running.tree
  for kidId in node.kids.idx .. node.kids.thru:
    let kid = tree.nodes[kidId]
    let value =
      case kid.kind:
      of leaf:
        running.run(kidId, kid.token)
      else:
        # TODO Generalized call dispatch.
        case tree.calleeKind(kid):
        of opIs:
          # TODO Gather up definitions.
          # TODO If destructuring, perform destructure down to single?
          # TODO How to represent overloads?
          running.run(kid)
        else:
          running.run(kid)
    discard value
  Value(kind: valNone)

proc resolve*(tree: Tree) =
  # TODO Some buffer type for running like for parsing and norming?
  # TODO Resolve imports.
  # TODO Put top levels into seq then sort by name.
  # TODO Also a table for lookup?
  # TODO Struct members are brought out to their level.
  # TODO Recurse keeping explicit stack of local definitions to wade through.
  var
    resolver = Runner(defs: Defs(), tree: tree)
    running = addr resolver
  running.extractTops
  discard running.run running.tree.root
