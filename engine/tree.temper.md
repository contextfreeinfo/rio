## Abstract tree

There's a single abstract tree for each module. Node details are stored in side
tables and are cross-referenced by index. Beyond that, the nodes are designed to
be convenient but versatile. Ideally we have as few node types as needed for
easy tree handling.

And I guess technically people could call this an abstract *syntax* tree, but
meh.

### Modular

TODO Just make a separate Tree or Module thing and require pumping into there
instead of worrying about this interface.

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
        resetOne(nodes);
        resetOne(work);
        resetOne(blocks);
        resetOne(breaks);
        resetOne(calls);
        resetOne(cases);
        // resetOne(classes);
        // resetOne(froms);
        resetOne(funs);
        resetOne(gets);
        // resetOne(ifs);
        // resetOne(loops);
        resetOne(switches);
        resetOne(vars);
      }

#### commit

      public commit(parent: Node, start: Int): Void {
        commitHeadless(start);
        work.add(parent);
      }

#### commitBlock

      public commitBlock(start: Int): Void {
        let oldLength = nodes.length;
        commit(

TODO Track the source as we go automatically in some fashion?

          { kind: Node.block, index: blocks.length, source: Source.none },
          start,
        )
        blocks.add({
          class: Block,
          // TODO No way to get index here. We'd have to get it later.
          kids: { start: oldLength, end: nodes.length },
        });
      }

#### commitHeadless

      public commitHeadless(start: Int): Void {
        // TODO Loop through making new versions of each side table entry with the NodeId of each?
        nodes.addAll(work.splice(start));
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

    let resetOne<T>(items: ListBuilder<T>): Void {
      let first = items[0];
      items.clear();
      items.add(first);
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
      @noProperty builder: ModuleBuilder,
    ) extends Modular {

TODO Better to precache all these referenced?

      public nodes: Listed<Node> = builder.nodes;
      public blocks: Listed<Block> = builder.blocks;
      public breaks: Listed<Break> = builder.breaks;
      public calls: Listed<Call> = builder.calls;
      public cases: Listed<Case> = builder.cases;
      // TODO classes
      // TODO froms
      public funs: Listed<Fun> = builder.funs;
      public gets: Listed<Get> = builder.gets;
      // TODO ifs
      // TODO loops
      public switches: Listed<Switch> = builder.switches;
      public vars: Listed<Var> = builder.vars;
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
