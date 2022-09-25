type
  NodeKind* = enum
    comment,
    define
  Node = object
    kind: NodeKind
