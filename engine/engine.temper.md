# Engine

The core functionality of Rio parsing, interpreting, and translating goes in
this library, but it doesn't connect much to actual capabilities.

Backends need to provide access to source code. We just receive it here.

    export class Engine {
      private lexer: Lexer = new Lexer();

      public process(source: String): Void {
        lexer.lex(source);
      }
    }

## Testing

### Test cases

We can directly test here, even though we can't read files at runtime.

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
      assert(normed.nodes.length == 22);
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
