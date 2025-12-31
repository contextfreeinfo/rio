# Implementation for Rio

Backends need to provide access to source code. We just receive it here.

    export class Engine {
      private lexer: Lexer = new Lexer();

      public process(source: String): Void {
        lexer.lex(source);
      }
    }

We can directly test here, though.

    test("lex") {
      let tokens = new Lexer().lex(hi);
      assert(tokens.length > 0);
    }

    let hi = """
      "pub fun main(sys)
      "   # Prove we can call twice.
      "   greet()
      "   greet()
      "end
      "
      "fun greet()
      "   # Had some parsing bugs, so try string variety.
      "   log("I say, \"Hi!\"")
      "   log("Now bye.")
      "   log("")
      "end
    ;
