# Engine

The core functionality of Rio parsing, interpreting, and translating goes in
this library, but it doesn't connect much to actual capabilities.

Backends need to provide access to source code. We just receive it here.

    export class Engine {
      private var core: Core = new Core();
      private var coreModule: ModuleBuilder = new ModuleBuilder();
      private lexer: Lexer = new Lexer();
      private parser: Parser = new Parser();
      private normer: Normer = new Normer(lexer.interner);
      private resolver: Resolver = new Resolver();
      public modules: MapBuilder<TextId, ModuleBuilder> = new MapBuilder();

      public process(name: String, source: String): ModuleBuilder {
        initCoreIfNeeded(name);
        let tokens = lexer.lex(source);
        let parseNodes = parser.parse(tokens);
        let normed = normer.norm(parseNodes);
        resolver.resolve(normed, modules);
        // Store the result and also return for convenience.
        modules[textId(name)] = normed;
        normed
      }

### initCore

      private initCoreIfNeeded(name: String): Void {
        if (modules.length == 0 && name != "core") {
          coreModule = process("core", coreSource);
          let coreTops = coreModule.tops;
          do {
            core = {
              class: Core,
              error: coreTops[textId("error")],
              false: coreTops[textId("false")],
              log: coreTops[textId("log")],
              no: coreTops[textId("no")],
              null: coreTops[textId("null")],
              true: coreTops[textId("true")],
              void: coreTops[textId("void")],
              yes: coreTops[textId("yes")],
            };
          } orelse panic();
        }
      }

### interner

Provide the interner publicly but also helpers, all for convenience.

      public get interner(): Interner {
        lexer.interner
      }

      public string(text: TextId): String? {
        lexer.interner.string(text)
      }

      public textId(string: String): TextId {
        lexer.interner[string]
      }
    }

## Testing

We can directly test here, even though we can't read files at runtime.

### Use an engine

Run through an engine for aggregate behavior.

    test("engine") {
      let engine = new Engine();
      let module = engine.process("hi", hi);
      assert(engine.modules.length == 2);
      assert(module.nodes.length == expectedHiLength);
      let core = engine.modules[engine.textId("core")];
      assert(core.nodes.length == 85);
      assert(core.tops.length == 8);
      assert(
        core.tops.keys().join(" ") { x => engine.string(x) ?? "?" } ==
          "error false log no null true void yes",
      );
    }

    let expectedHiLength = 22;

### Manual steps

Check results as we go.

    test("steps") {
      let interner = new Interner();
      // Lex.
      let tokens = new Lexer(interner).lex(hi);
      assert(tokens.length == 87);
      // Parse.
      let parseNodes = new Parser().parse(tokens);
      assert(parseNodes.length == 118);
      // Norm.
      let normed = new Normer(interner).norm(parseNodes);
      assert(normed.nodes.length == expectedHiLength);
      assert(normed.nodes[normed.nodes.length - 1] is Block);
      // Resolve.
      new Resolver().resolve(normed);
    }

### Test data

For now, export test data for easier testing in backends.

TODO Stop exporting test data. Maybe make a separate test data library?

    export let hi = """
      // "const cli = import("whatever.com/there/cli")
      // "import "whatever.com/there/cli"
      // "import cli
      // "
      // "pub fun main(sys cli.Sys)
      "pub fun main(sys)
      "   # Prove we can call twice.
      "   greet()
      "   greet()
      "end
      "
      "fun greet()
      "   # Had some parsing bugs, so try string variety.
      "   log("I say, \\"Hi!\\"")
      "   log("Now bye.")
      "   log("
      "     # Also try danglers just above ... and below when we're ready.
      "     # "\\
      "   )
      "   log(
      "     # For now, just make try dangling escape as a separate string.
      "     "\\
      "   )
      "end
    ;
