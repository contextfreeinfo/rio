## Abstract tree

There's a single abstract tree for each module. Node details are stored in side
tables and are cross-referenced by index. Beyond that, the nodes are designed to
be convenient but versatile. Ideally we have as few node types as needed for
easy tree handling.

### ModuleBuilder

The expectation again is that most of these things will be structs in the
future, so design for that.

Beyond that matter, we can't have both mutable children and recursive types,
because that would allow for cycles. So we either need to be immutable or
non-recursive.

Some passes are handy with mutability, although mutable types are slower in
Temper-built Rust than are imu types.

    export class ModuleBuilder() {
      public nodes: ListBuilder<Node> = new ListBuilder();
      public work: ListBuilder<Node> = new ListBuilder();
      public blocks: ListBuilder<Block> = new ListBuilder();
    }

### Source

    export class Source(
      public path: String,
      public range: Range,
    ) {}

### Node

    export let DefFlags = Int;
    export let DetailId = Int;
    export let NodeId = Int;
    export let NodeKind = Int;
    export let NodeRange = Range;

    export class Node(

The node kind says which side table the index is for.

      public kind: NodeKind,
      public index: DetailId,
      public source: Source,
    ) {

Node kinds go here for now.

      public static none: NodeKind = 0;
      public static args: NodeKind = Node.none + 1;
      public static block: NodeKind = Node.args + 1;
      public static call: NodeKind = Node.block + 1;
      public static nym`case`: NodeKind = Node.call + 1;
      public static fun: NodeKind = Node.case + 1;
      public static get: NodeKind = Node.fun + 1;
      public static ref: NodeKind = Node.get + 1;
      public static nym`return`: NodeKind = Node.ref + 1;
      public static switch: NodeKind = Node.return + 1;
      public static type: NodeKind = Node.switch + 1;
      public static value: NodeKind = Node.type + 1; 
      public static var: NodeKind = Node.value + 1;
    }

### Block

All node detail types have an index back into the main node list.

    export class Block(
      public index: NodeId,
      public kids: NodeRange,
    ) {}

### Call

    export class Call(
      public index: NodeId,
      public callee: NodeId,
      public args: NodeRange,
    ) {}

### Case

    export class Case(
      public index: NodeId,
      public patterns: NodeRange,
      public gate: NodeId,
      public kids: NodeRange,
    ) {}

### Fun

    export class Fun(
      public index: NodeId,
      public name: TextId,
      public flags: DefFlags,
      public params: NodeRange,
      public nym`return`: NodeId,
      public kid: NodeRange,
    ) {}

### Get

    export class Get(
      public index: NodeId,
      public subject: NodeId,
      public member: NodeId,
    ) {}

### Break

Break also handles returns.

    export class Break(
      public index: NodeId,
      public kind: TokenKind,
      public label: NodeId,
      public value: NodeId,
    ) {}
