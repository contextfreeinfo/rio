# Abstract tree

There's a single abstract tree for each module. Node details are stored in side
tables and are cross-referenced by index. Beyond that, the nodes are designed to
be convenient but versatile. Ideally we have as few node types as needed for
easy tree handling.

And I guess technically people could call this an abstract *syntax* tree, but
meh.

## ModuleBuilder

We can't have both mutable children and recursive types, because that would
allow for cycles. So we either need to be immutable or non-recursive. We avoid
recursive by using indices into the node list builder.

Individual node types themselves are imu, however, so fully imu lists also can
be made.

    export class ModuleBuilder(
      public nodes: ListBuilder<Node> = [emptyNode].toListBuilder(),
    ) { // TODO extends Modular

### tops

      public tops: MapBuilder<TextId, NodeId> = new MapBuilder();

### root

      public get root(): Block {
        nodes[nodes.length - 1] as Block orelse panic()
      }

### reset

Call reset before each reuse of a ModuleBuilder.

      public reset(): Void {
        resetOne(nodes);
        resetOne(work);
      }

### commit

      public commit(start: Int, parent: Node): Void {
        commitHeadless(start);
        work.add(parent);
      }

### commitBlock

      public commitBlock(start: Int): Void {
        commit(start, {
          class: Block,
          kids: {
            start: nodes.length,
            end: nodes.length + work.length - start,
          },
        });
      }

### commitHeadless

TODO Is looping faster than intermediate allocation here? Same issue as for
parse tree building.

      public commitHeadless(start: Int): Void {
        nodes.addAll(work.splice(start));
      }

### extract

Provides a copy of the builder for use in later passes.

      public extract(): ModuleBuilder {
        new ModuleBuilder(nodes.toListBuilder())
      }

### popWorkBlock

      public popWorkBlock(): Range {
        let last = work.removeLast() as Block orelse panic();
        last.kids
      }

### Data

Init everything to a bogus member at 0 so that can mean a nullish value.

Nodes reference side tables by kind and index.

      public work: ListBuilder<Node> = [emptyNode].toListBuilder();

    }

## resetOne

Resets a list down to the first member, avoiding splice because that allocates.

    let resetOne<T>(items: ListBuilder<T>): Void {
      let first = items[0];
      items.clear();
      items.add(first);
    }

## Source

    export class Source(
      public path: TextId,
      public range: Range,
    ) {
      public static none: Source = { path: 0, range: Range.empty };
    }

## Definition flags

    export let DefFlags = Int;

    export class DefFlag {
      public static capture = 1;
      public static global = DefFlag.capture * 2;
      public static plug = DefFlag.global * 2;
      public static pub = DefFlag.plug * 2;
    }

## Definition

    export sealed interface Def {
      public name: TextId;
      public flags: DefFlags;
    }

## Node

    export let NodeId = Int;
    export let NodeRange = Range;

The original plan was to make this a value-friendly type and store side tables
of value-friendly types, but I wimped out and simplified down to sealed
interface.

TODO Could we still make this a value type as an enum?

    export sealed interface Node {
      public source: Source;

### stringifyTree

      public static stringifyTree(
        nodes: Listed<Node>,
        interner: Interner,
      ): String {
        if (nodes.isEmpty) {
          return "";
        }
        let buffer = new StringBuilder();
        let stringer = new TreeStringer(
          nodes,
          interner,
          fn (string) { buffer.append(string) },
        );
        stringer.stringify(nodes.length - 1);
        buffer.toString()
      }

    }


TODO Making emptyNode a static member of the Node interface silently blocked
backend translations.

    let emptyNode: Node = new Block() as Node;

## nodeAsDef

TODO Make this a member method of Node, but Rust ends up with a problem.

    @extension("asDef")
    export let nodeAsDef(node: Node): Def? {
      when (node) {
        is Fun -> node;
        is Var -> node;
        else -> null;
      }
    }

## TreeStringer

TODO Work around failing StringBuilder property type.

    export @fun interface Appender(string: String): Void;

    export class TreeStringer(
      public nodes: Listed<Node>,
      public interner: Interner,
      public appender: Appender,
      // public builder: StringBuilder = new StringBuilder(),
      public var indentLevel: Int = 0,
      public indentText: String = "  ",
    ) {

### append

      public append(string: String): Void {
        appender(string);
      }

### endLine

      public endLine(): Void {
        // builder.append("\n");
        append("\n");
      }

### indent

      public indent(): Void {
        for (var i = 0; i < indentLevel; ++i) {
          // builder.append(indentText);
          append(indentText);
        }
      }

### stringify

      public stringify(index: NodeId): Void {
        let node = nodes[index];

TODO Until we improve codegen, put these in order of most to least likely. Well,
maybe get stats first then sort them.

        when (node) {
          is Block -> stringifyBlock(node);
          is Break -> stringifyBreak(node);
          is Call -> stringifyCall(node);
          is Fun -> stringifyFun(node, index);
          is StringValue -> stringifyStringValue(node);
          is Ref -> stringifyRef(node);
          else -> append("TODO Some Node");
        }
      }

### stringifyBlock

      public stringifyBlock(block: Block): Void {
        stringifyStatements(block.kids);
      }

### stringifyBreak

      public stringifyBreak(nym`break`: Break): Void {
        indent();
        when (nym`break`.kind) {
          Token.break -> append("break");
          Token.return -> append("return");
          else -> panic();
        }

TODO Break labels.

        if (nym`break`.value != 0) {
          append(" ");
          stringify(nym`break`.value);
        }
        endLine();
      }

### stringifyCall

      public stringifyCall(call: Call): Void {
        stringify(call.callee);
        append("(");
        let args = call.args;
        for (var i = args.start; i < args.end; ++i) {
          if (i > args.start) {
            append(", ");
          }
          stringify(i);
        }
        append(")");
      }

### stringifyFun

      public stringifyFun(fun: Fun, id: NodeId): Void {
        indent();
        append("fun");
        if (fun.name != 0) {
          append(" ");
          append(interner.string(fun.name) ?? panic());
          append("@");
          append(id.toString());
        }
        append("(");
        let params = fun.params;
        for (var i = params.start; i < params.end; ++i) {
          if (i > params.start) {
            append(", ");
          }
          let param = nodes[i] as Var orelse panic();
          // stringify(i);
        }
        append(")");
        endLine();
        ++indentLevel;
        stringifyStatements(fun.kids);
        --indentLevel;
        indent();
        append("end");
        endLine();
      }

### stringifyRef

      public stringifyRef(ref: Ref): Void {
        append(interner.string(ref.name) ?? panic());
        if (ref.target != 0) {
          append("@");
          if (ref.module != 0) {
            append(ref.module.toString());
            append("/");
          }
          append(ref.target.toString());
        }
      }

### stringifyStatements

      public stringifyStatements(kids: NodeRange): Void {
        for (var i = kids.start; i < kids.end; ++i) {
          indent();
          stringify(i);
          endLine();
        }
      }

### stringifyStringValue

      public stringifyStringValue(value: StringValue): Void {
        let text = interner.string(value.value) ?? panic();
        append("\"");
        var lastStart = String.begin;
        chars: for (var i = String.begin; i < text.end; i = text.next(i)) {
          let c = text[i];

TODO What else should be escaped?

          let escape = when (c) {
            char'"' -> '"';
            char "\\" -> "\\";
            char"\n" -> "n";
            char"\r" -> "r";
            char"\t" -> "t";
            else -> if (c < char" ") {
              let pad = if (c < 0x10) { "0" } else { "" };
              "\\x${pad}${c.toString(0x10)}"
            } else {
              continue chars;
            }
          }
          if (lastStart < i) {
            append(text.slice(lastStart, i));
          }
          lastStart = text.next(i);
          append("\\");
          append(escape);
        }
        if (lastStart < text.end) {
          append(text.slice(lastStart, text.end));
        }
        append("\"");
      }
    }

## Block

All node detail types have an index back into the main node list.

    export class Block(
      public source: Source = Source.none,
      public kids: NodeRange = Range.empty,
    ) extends Node {}

## Break

Break also handles returns.

    export class Break(
      public source: Source = Source.none,
      public kind: TokenKind = Token.none,
      public label: NodeId = 0,
      public value: NodeId = 0,
    ) extends Node {}

## Call

    export class Call(
      public source: Source = Source.none,
      public callee: NodeId = 0,
      public args: NodeRange = Range.empty,
    ) extends Node {}

## Case

    export class Case(
      public source: Source = Source.none,
      public always: Boolean = false,
      public patterns: NodeRange = Range.empty,
      public gate: NodeId = 0,
      public kids: NodeRange = Range.empty,
    ) extends Node {}

## Fun

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
    ) extends Node & Def {}

## Get

For any dot access, actually, including as an assignment target.

    export class Get(
      public source: Source = Source.none,
      public subject: NodeId = 0,
      public member: NodeId = 0,
    ) extends Node {}

## IntValue

    export class IntValue(
      public source: Source = Source.none,
      public value: Int = 0,
    ) extends Node {}

## Ref

    export class Ref(
      public source: Source = Source.none,
      public name: TextId = 0,
      public module: TextId = 0,
      public target: NodeId = 0,
    ) extends Node {}

## StringValue

    export class StringValue(
      public source: Source = Source.none,
      public value: TextId = 0,
    ) extends Node {}

## Switch

    export class Switch(
      public source: Source = Source.none,
      public subject: NodeId = 0,
      public kids: Range = Range.empty,
    ) extends Node {}

## Var

    export class Var(
      public source: Source = Source.none,
      public name: TextId = 0,
      public flags: DefFlags = 0,
      public type: NodeId = 0,
      public value: NodeId = 0,
    ) extends Node & Def {}
