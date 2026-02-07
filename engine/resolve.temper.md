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

TODO Just keep `tops` in ModuleBuilder the whole time?

      private tops: MapBuilder<TextId, Def> = new MapBuilder();

#### resolve

      public resolve(module: ModuleBuilder): Void {

Clean up before processing, reusing previously allocated space where possible.

        nodes = module.nodes;
        tops.clear();

Extract tops.

        let root = module.root;
        tops: for (var i = root.kids.start; i < root.kids.end; ++i) {
          let kid = nodes[i];
          let def = kid.asDef() ?? continue tops;
          this.tops[def.name] = def;
        }

TODO Recurse.

      }

    }
