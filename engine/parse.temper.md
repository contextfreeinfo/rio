## Parsing

### ParseNode

    // union
    export sealed interface ParseNode {
      public get parseKind(): ParseKind;
    }

    // struct
    export class ParseParent(
      public kind: ParseKind,
      public kids: Range/*<ParseNode>*/,
    ) extends ParseNode {
      public get parseKind(): ParseKind {
        kind
      }
    }

### ParseKind

    export let ParseKind = Int32;

Type Parse just exists for hosting parse kind values.

    export class Parse {
      public static none: ParseKind = 0;
      public static args: ParseKind = Parse.none + 1;
      public static block: ParseKind = Parse.args + 1;
      public static call: ParseKind = Parse.block + 1;
      public static nym`case`: ParseKind = Parse.call + 1;
      public static comment: ParseKind = Parse.case + 1;
      public static nym`else`: ParseKind = Parse.comment + 1;
      public static fun: ParseKind = Parse.else + 1;
      public static infix: ParseKind = Parse.fun + 1;
      public static junk: ParseKind = Parse.infix + 1;
      public static modify: ParseKind = Parse.junk + 1;
      public static param: ParseKind = Parse.modify + 1;
      public static params: ParseKind = Parse.param + 1;
      public static prefix: ParseKind = Parse.params + 1;
      public static nym`return`: ParseKind = Parse.prefix + 1;
      public static string: ParseKind = Parse.return + 1;
      public static switch: ParseKind = Parse.string + 1;
      public static switchEmpty: ParseKind = Parse.switch + 1;
      public static token: ParseKind = Parse.switchEmpty + 1;
      public static nym`var`: ParseKind = Parse.token + 1;
    }

### Range

We currently can't represent the Item kind because of
[a bug in Rust codegen](https://github.com/temperlang/temper/issues/271).

    // struct
    export class Range/*<Item>*/(
      public start: Int,
      public end: Int,
    ) {}

### Parser

    export class Parser {
      private var tokens: List<Token> = [];
      private var index: Int = 0;
      private nodes: ListBuilder<ParseNode> = new ListBuilder();
      private work: ListBuilder<ParseNode> = new ListBuilder();

#### parse

Returns a list of parse nodes, where the last node is the root of the tree and
index 0 is always bogus.

      public parse(tokens: List<Token>): List<ParseNode> {

Reset for reuse.

        this.tokens = tokens;
        index = 0;
        work.clear();
        nodes.clear();

Always add a bogus node at the start so index 0 always means bad.

        nodes.add(new ParseParent(Parse.none, new Range(0, 0)));

Now parse.

        doParse();
        nodes.toList()
      }

#### commit

Moves a working range into actual tree nodes. This strategy allows buffer reuse
rather than allocating lots of individual lists for nodes.

      private commit(kind: ParseKind, start: Int): Void {
        let oldLength = nodes.length;

TODO Does slice allocation here slow things down? Would looping be slower?

        nodes.addAll(work.slice(start, work.length));
        let parent = new ParseParent(kind, new Range(oldLength, nodes.length));
        work.add(parent)
      }

#### has

Here we don't just check if empty, but we also eat comments and horizontal
space, which don't affect parsing.

      private has(): Boolean {
        has: while (index < tokens.length) {
          let token = tokens[index];
          when (token.kind) {

We do nest comment text under the comment open, though.

            Token.commentOpen -> do {
              let start = work.length;
              pushToken(token);
              let next = tokens[index];
              if (next.kind == Token.commentText) {
                pushToken(next);
                commit(Parse.comment, start);
              }
              continue has;
            }

Horizontal space is just pushed as filler.

            Token.hspace -> pushToken(token);

Found a significant token, so done.

            else -> return true;
          }
        }

Ran out of tokens.

        false
      }

#### peek

Panics if !has, so check that first.

      private peek(): Token {
        tokens[index]
      }

#### pushToken

      private pushToken(token: Token): Void {
        work.add(token);
        index += 1;
      }

#### doParse

This handles the top block, which has slightly different rules than nested
blocks because it doesn't end until tokens run out.

      private doParse(): Void {
        let start = work.length;
        while (has()) {

TODO Remove this eating when we implement.

          pushToken(peek())
        }
        commit(Parse.block, start);
      }

```go
func (p *parser) parseBlockTop() {
	start := len(p.work)
	for p.has() {
		switch t := p.peek(); t.Kind {
		case TokenVSpace:
			p.pushToken(t)
		default:
			p.parseStatement()
		}
	}
	p.commit(ParseBlock, start)
}
```

#### Parser end

    }
