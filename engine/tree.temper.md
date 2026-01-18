## Abstract tree

There's a single abstract tree for each module. Node details are stored in side
tables and are cross-referenced by index. Beyond that, the nodes are designed to
be convenient but versatile. Ideally we have as few node types as needed for
easy tree handling.

And I guess technically people could call this an abstract *syntax* tree, but
meh.

### ModuleBuilder

We can't have both mutable children and recursive types, because that would
allow for cycles. So we either need to be immutable or non-recursive. We avoid
recursive by using indices into the node list builder.

Individual node types themselves are imu, however, so fully imu lists also can
be made.

    export class ModuleBuilder { // TODO extends Modular

#### reset

Call reset before each reuse of a ModuleBuilder.

      public reset(): Void {
        resetOne(nodes);
        resetOne(work);
      }

#### commit

      public commit(start: Int, parent: Node): Void {
        commitHeadless(start);
        work.add(parent);
      }

#### commitBlock

      public commitBlock(start: Int): Void {
        commit(start, {
          class: Block,
          kids: {
            start: nodes.length,
            end: nodes.length + work.length - start,
          },
        });
      }

#### commitHeadless

TODO Is looping faster than intermediate allocation here? Same issue as for
parse tree building.

      public commitHeadless(start: Int): Void {
        nodes.addAll(work.splice(start));
      }

#### popWorkBlock

      public popWorkBlock(): Range {
        let last = work.removeLast() as Block orelse panic();
        last.kids
      }

#### Data

Init everything to a bogus member at 0 so that can mean a nullish value.

Nodes reference side tables by kind and index.

      public nodes: ListBuilder<Node> = [emptyNode].toListBuilder();
      public work: ListBuilder<Node> = [emptyNode].toListBuilder();

    }

### resetOne

Resets a list down to the first member, avoiding splice because that allocates.

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
    export let NodeId = Int;
    export let NodeRange = Range;

The original plan was to make this a value-friendly type and store side tables
of value-friendly types, but I wimped out and simplified down to sealed
interface.

TODO Could we still make this a value type as an enum?

    export sealed interface Node {
      public source: Source;

#### stringifyTree

TODO Stringify tree.

      public static stringifyTree(
        nodes: Listed<Node>,
        interner: Interner,
      ): String {
        ""
      }

    }

TODO Making emptyNode a static member of the Node interface silently blocked
backend translations.

    let emptyNode: Node = new Block() as Node;

### Block

All node detail types have an index back into the main node list.

    export class Block(
      public source: Source = Source.none,
      public kids: NodeRange = Range.empty,
    ) extends Node {}

### Break

Break also handles returns.

    export class Break(
      public source: Source = Source.none,
      public kind: TokenKind = Token.none,
      public label: NodeId = 0,
      public value: NodeId = 0,
    ) extends Node {}

### Call

    export class Call(
      public source: Source = Source.none,
      public callee: NodeId = 0,
      public args: NodeRange = Range.empty,
    ) extends Node {}

### Case

    export class Case(
      public source: Source = Source.none,
      public always: Boolean = false,
      public patterns: NodeRange = Range.empty,
      public gate: NodeId = 0,
      public kids: NodeRange = Range.empty,
    ) extends Node {}

### Fun

TODO If we want to enum Node, we might want to break these bigger nodes into a
collection of smaller nodes, so we don't get any much bigger than others. I
think this is currently 10 * 4 == 40 bytes if all were value types. And Block is
only 5 * 4 == 20 bytes. Adjustments below get Fun down to either 4 or 6 items
for 16 or 24 bytes, so we could look into such later.

    export class Fun(

TODO Like maybe have a Def node and even delegate Source to it?

      public source: Source = Source.none,
      public name: TextId = 0,
      public flags: DefFlags = 0,

TODO And even a Signature node to keep params and return together?

      public params: NodeRange = Range.empty,
      public nym`return`: NodeId = 0,
      public kids: NodeRange = Range.empty,
    ) extends Node {}

### Get

For any dot access, actually, including as an assignment target.

    export class Get(
      public source: Source = Source.none,
      public subject: NodeId = 0,
      public member: NodeId = 0,
    ) extends Node {}

### Ref

    export class Ref(
      public source: Source = Source.none,
      public name: TextId = 0,

TODO For imports, should module be here, or should we have a different node for
defining such imports?

      public target: NodeId = 0,
    ) extends Node {}

### Switch

    export class Switch(
      public source: Source = Source.none,
      public subject: NodeId = 0,
      public kids: Range = Range.empty,
    ) extends Node {}

### Var

    export class Var(
      public source: Source = Source.none,
      public name: TextId = 0,
      public flags: DefFlags = 0,
      public type: NodeId = 0,
      public value: NodeId = 0,
    ) extends Node {}
