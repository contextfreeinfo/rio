## Abstract tree

There's a single abstract tree for each module. Node details are stored in side
tables and are cross-referenced by index. Beyond that, the nodes are designed to
be convenient but versatile. Ideally we have as few node types as needed for
easy tree handling.

### Modular

    export interface Modular {

#### stringify

TODO

#### Data

      public nodes: Listed<Node>;
      public blocks: Listed<Block>;
      public breaks: Listed<Break>;
      public calls: Listed<Call>;
      public cases: Listed<Case>;
      // TODO classes
      // TODO froms
      public funs: Listed<Fun>;
      public gets: Listed<Get>;
      // TODO ifs
      // TODO loops
      public switches: Listed<Switch>;
      public vars: Listed<Var>;
    }

### ModuleBuilder

The expectation again is that most of these things will be structs in the
future, so design for that.

Beyond that matter, we can't have both mutable children and recursive types,
because that would allow for cycles. So we either need to be immutable or
non-recursive.

Some passes are handy with mutability, although mutable types are slower in
Temper-built Rust than are imu types.

    export class ModuleBuilder { // TODO extends Modular

#### modular

TODO Temporary workaround for covariant return issues?
TODO More efficient in Rust, anyway, to avoid repeated cast wrapping?

And note that we can't cache this because that would create reference cycles.
Make it an explicit method instead of a getter, so we understand there's cost.

      public modular(): Modular {
        new ModuleBuilderModular(this)
      }

#### reset

Call reset before each reuse of a ModuleBuilder.

      public reset(): Void {

TODO Some remove range option that doesn't allocate?

TODO Or use the results from splice here to make a Module instance?

        nodes.splice(1);
        work.splice(1);
        blocks.splice(1);
        breaks.splice(1);
        calls.splice(1);
        cases.splice(1);
        // classes.splice(1);
        // froms.splice(1);
        funs.splice(1);
        gets.splice(1);
        // ifs.splice(1);
        // loops.splice(1);
        switches.splice(1);
        vars.splice(1);
      }

#### Data

Init everything to a bogus member at 0 so that can mean a nullish value.

Nodes reference side tables by kind and index.

      public nodes: ListBuilder<Node> = [new Node()].toListBuilder();
      public work: ListBuilder<Node> = [new Node()].toListBuilder();

Separate side tables should help if we support value types and/or structs, so
design for that already.

      public blocks: ListBuilder<Block> = [new Block()].toListBuilder();
      public breaks: ListBuilder<Break> = [new Break()].toListBuilder();
      public calls: ListBuilder<Call> = [new Call()].toListBuilder();
      public cases: ListBuilder<Case> = [new Case()].toListBuilder();
      // TODO classes
      // TODO froms
      public funs: ListBuilder<Fun> = [new Fun()].toListBuilder();
      public gets: ListBuilder<Get> = [new Get()].toListBuilder();
      // TODO ifs
      // TODO loops
      public switches: ListBuilder<Switch> = [new Switch()].toListBuilder();
      public vars: ListBuilder<Var> = [new Var()].toListBuilder();
    }

### Source

    export class Source(
      public path: TextId,
      public range: Range,
    ) {
      public static none: Source = { path: 0, range: Range.empty };
    }

### Node

    export let DefFlags = Int;
    export let DetailId = Int;
    export let NodeId = Int;
    export let NodeKind = Int;
    export let NodeRange = Range;

    export class Node(

The node kind says which side table the index is for.

      public kind: NodeKind = Node.none,
      public index: DetailId = 0,
      public source: Source = Source.none,
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

    class ModuleBuilderModular(
      private builder: ModuleBuilder,
    ) extends Modular {
      public get nodes: Listed<Node> { builder.nodes }
      public get blocks: Listed<Block> { builder.blocks };
      public get breaks: Listed<Break> { builder.breaks };
      public get calls: Listed<Call> { builder.calls };
      public get cases: Listed<Case> { builder.cases };
      // TODO classes
      // TODO froms
      public get funs: Listed<Fun> { builder.funs };
      public get gets: Listed<Get> { builder.gets };
      // TODO ifs
      // TODO loops
      public get switches: Listed<Switch> { builder.switches };
      public get vars: Listed<Var> { builder.vars };
    }

### Block

All node detail types have an index back into the main node list.

    export class Block(
      public index: NodeId = 0,
      public kids: NodeRange = Range.empty,
    ) {}

### Break

Break also handles returns.

    export class Break(
      public index: NodeId = 0,
      public kind: TokenKind = Token.none,
      public label: NodeId = 0,
      public value: NodeId = 0,
    ) {}

### Call

    export class Call(
      public index: NodeId = 0,
      public callee: NodeId = 0,
      public args: NodeRange = Range.empty,
    ) {}

### Case

    export class Case(
      public index: NodeId = 0,
      public patterns: NodeRange = Range.empty,
      public gate: NodeId = 0,
      public kids: NodeRange = Range.empty,
    ) {}

### Fun

    export class Fun(
      public index: NodeId = 0,
      public name: TextId = 0,
      public flags: DefFlags = 0,
      public params: NodeRange = Range.empty,
      public nym`return`: NodeId = 0,
      public kid: NodeRange = Range.empty,
    ) {}

### Get

For any dot access, actually, including as an assignment target.

    export class Get(
      public index: NodeId = 0,
      public subject: NodeId = 0,
      public member: NodeId = 0,
    ) {}

### Switch

    export class Switch(
      public subject: NodeId = 0,
      public kids: Range = Range.empty,
    ) {}

### Var

    export class Var(
      public index: NodeId = 0,
      public name: TextId = 0,
      public flags: DefFlags = 0,
      public type: NodeId = 0,
      public value: NodeId = 0,
    ) {}
