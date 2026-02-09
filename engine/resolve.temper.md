## Resolving

Resolves references to their referents.

TODO Distinguish between resets on module shuffle vs just filling in more blanks?

### Resolver

    export class Resolver(

An interner is needed for error messaging, including fuzzy match reporting, but
it's not needed for core resolving, because everything is in ints already.

      // public interner: Interner,
    ) {

      private var nodes: ListBuilder<Node> = new ListBuilder();

And keep these separate instead of `Pair<String, Node>` in the hopes that's
less allocation happening.

      private scopeNames: ListBuilder<TextId> = new ListBuilder();
      private scopeNodes: ListBuilder<NodeId> = new ListBuilder();

TODO Just keep `tops` in ModuleBuilder the whole time?

      private tops: MapBuilder<TextId, NodeId> = new MapBuilder();

#### resolve

      public resolve(module: ModuleBuilder): Void {

Clean up before processing, reusing previously allocated space where possible.

        nodes = module.nodes;
        scopeNames.clear();
        scopeNodes.clear();
        tops.clear();

Extract tops.

        let root = module.root;
        for (var id = root.kids.start; id < root.kids.end; ++id) {
          let kid = nodes[id];
          when (kid) {
            is Fun -> tops[kid.name] = id;
            is Var -> tops[kid.name] = id;
          }
        }

Recurse, using a custom loop for the top that's separate from standard block
handling.

        for (var id = root.kids.start; id < root.kids.end; ++id) {
          let kid = nodes[id];
          when (kid) {
            is Fun -> resolveFunContent(kid);
            is Var -> resolveVarContent(kid);

Statements other than defs at top level aren't currently supported, but resolve
for now, anyway.

            else -> resolveNode(id);
          }
        }
      }

#### pushScope

      private pushScope(name: TextId, node: NodeId): Void {
        scopeNames.add(name);
        scopeNodes.add(node);
      }

#### popScope

TODO Add some `trimTo` method in Temper.

      private popScopeTo(length: Int): Void {
        while (scopeNames.length > length) {
          scopeNames.removeLast();
          scopeNodes.removeLast();
        }
      }

#### resolveNode

      private resolveNode(id: NodeId): Void {
        let node = nodes[id];
        when (node) {
          is Block -> resolveBlock(node);
          is Break -> resolveBreak(node);
          is Call -> resolveCall(node);
          is Case -> resolveCase(node);
          is Fun -> resolveFun(node, id);
          is Get -> resolveGet(node);
          is Ref -> resolveRef(node, id);
          is Switch -> resolveSwitch(node);
          is Var -> resolveVar(node, id);
        }
      }

#### resolveBlock

      private resolveBlock(block: Block): Void {
        let level = scopeNames.length;
        for (var id = block.kids.start; id < block.kids.end; ++id) {
          resolveNode(id);
        }
        popScopeTo(level);
      }

#### resolveBreak

      private resolveBreak(node: Break): Void {
        resolveNode(node.value);
      }

#### resolveCall

      private resolveCall(call: Call): Void {
        resolveNode(call.callee);
        for (var id = call.args.start; id < call.args.end; ++id) {
          resolveNode(id);
        }
      }

#### resolveCase

      private resolveCase(case: Case): Void {
        let level = scopeNames.length;
        for (var id = case.patterns.start; id < case.patterns.end; ++id) {
          resolveNode(id);
        }
        resolveNode(case.gate);
        for (var id = case.kids.start; id < case.kids.end; ++id) {
          resolveNode(id);
        }
        popScopeTo(level);
      }

#### resolveFun

      private resolveFun(fun: Fun, id: NodeId): Void {
        resolveFunContent(fun);
      }

      private resolveFunContent(fun: Fun): Void {

Start a scope around both vars and block kids.

        let level = scopeNames.length;

All proper params are vars. But to be flexible in handling error, be general
about it.

We'll pop these from scope later along with the block kids. They don't need a
separate scope level.

        for (var id = fun.params.start; id < fun.params.end; ++id) {
          resolveNode(id);
        }

Block kids are any kind of node, though.

        for (var id = fun.kids.start; id < fun.kids.end; ++id) {
          resolveNode(id);
        }

Reset the scope when done.

        popScopeTo(level);
      }

#### resolveGet

      private resolveGet(get: Get): Void {
        resolveNode(get.subject);
      }

#### resolveRef

      private resolveRef(ref: Ref, id: NodeId): Void {

On any moving around of tree nodes beyond simple swaps, referent ids are
invalid, but presume we'll reset to zero explicitly before fresh resolve if we
want that.

        if (ref.target != 0) {
          return;
        }

To resolve, first look bottom up (last to first) through the scope.

        let name = ref.name;
        for (var i = scopeNames.length - 1; i >= 0; --i) {
          if (name == scopeNames[i]) {
            let target = scopeNodes[i];
            nodes[id] = { class: Ref, source: ref.source, name, target };
            return;
          }
        }

Then if that fails, check the top-level defs here.

        let target = tops.getOr(name, 0);
        if (target != 0) {
          nodes[id] = { class: Ref, source: ref.source, name, target };
          return;
        }

TODO And if that fails, check builtins.

      }

#### resolveSwitch

      private resolveSwitch(switch: Switch): Void {
        resolveNode(switch.subject);
        for (var id = switch.kids.start; id < switch.kids.end; ++id) {
          resolveNode(id);
        }
      }

#### resolveVar

      private resolveVar(node: Var, id: NodeId): Void {
        pushScope(node.name, id);
        resolveVarContent(node);
      }

      private resolveVarContent(node: Var): Void {
        resolveNode(node.type);
        resolveNode(node.value);
      }

    }
