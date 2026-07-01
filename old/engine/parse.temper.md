## Parsing

### ParseNode

Here `@value` could hint to use backend value type. This also implies `@imu` and
requires that all sealed subtypes also are also `@value`. This could change the
backend representation such as to plain `enum` and `Copy` in Rust. Doing this
might also disallow subinterfaces?

    // union or @value?
    export sealed interface ParseNode {

#### parseKind

For the moment, at least, we use Token instances directly as ParseNode instances
to avoid extra allocations. So name this `parseKind` to keep it different from
the token `kind`.

In the future, presumably ParseNode is a union of Token and ParseParent.

      public get parseKind(): ParseKind;

#### asParent

      public get asParent(): ParseParent;

#### asToken

      public get asToken(): Token;

#### stringifyTree

      public static stringifyTree(
        nodes: Listed<ParseNode>,
        interner: Interner,
      ): String {
        let builder = new StringBuilder();
        let root = nodes[nodes.length - 1];
        stringifyParseTreeWith(builder, 0, root, nodes, interner);
        builder.toString()
      }
    }

    let noneParse = new ParseParent(Parse.none, Range.empty);

#### stringifyParseTreeWith

    let stringifyParseTreeWith(
      builder: StringBuilder,
      indent: Int,
      node: ParseNode,
      nodes: Listed<ParseNode>,
      interner: Interner,
    ): Void {
      appendIndent(builder, indent);
      when (node) {
        is ParseParent -> do {
          builder.append(Parse.names[node.kind]);
          builder.append("\n");
          let indent = indent + 1;
          for (var i = node.kids.start; i < node.kids.end; i += 1) {
            stringifyParseTreeWith(builder, indent, nodes[i], nodes, interner);
          }
        }
        is Token -> do {
          // TODO Stringify token into builder directly.
          builder.append(node.stringify(interner));
          builder.append("\n");
        }
      }
    }

#### appendIndent

    let appendIndent(builder: StringBuilder, indent: Int): Void {
      for (var i = 0; i < indent; i += 1) {
        builder.append("   ");
      }
    }

### ParseParent

A parse tree is constructed of ParseParent nodes, with Token nodes as leaves.

    // struct
    export class ParseParent(
      public kind: ParseKind,
      public kids: Range/*<ParseNode>*/,
    ) extends ParseNode {
      public get parseKind(): ParseKind {
        kind
      }

      public get asParent(): ParseParent {
        this
      }

      public get asToken(): Token {
        noneToken
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

#### ParseKind names

As for TokenKind names, order matters here, and Temper should automate in the
future.

      public static names = [
        "none",
        "args",
        "block",
        "call",
        "case",
        "comment",
        "else",
        "fun",
        "infix",
        "junk",
        "modify",
        "param",
        "params",
        "prefix",
        "return",
        "string",
        "switch",
        "switchEmpty",
        "token",
        "var",
      ];
    }

### Range

We currently can't represent the Item kind because of
[a bug in Rust codegen](https://github.com/temperlang/temper/issues/271).

    // struct
    export class Range/*<Item>*/(
      public start: Int,
      public end: Int,
    ) {
      public static empty: Range = new Range(0, 0);

      public get length(): Int {
        end - start
      }
    }

### Parser

    export class Parser {
      private var tokens: List<Token> = [];
      private var index: Int = 0;
      private context: ListBuilder<TokenKind> = new ListBuilder();
      private nodes: ListBuilder<ParseNode> = new ListBuilder();
      private work: ListBuilder<ParseNode> = new ListBuilder();

#### parse

Returns a list of parse nodes, where the last node is the root of the tree and
index 0 is always bogus.

      public parse(tokens: List<Token>): List<ParseNode> {

Reset for reuse.

        this.tokens = tokens;
        index = 0;
        context.clear();
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

TODO Maybe change out the splice method for something else???

        nodes.addAll(work.splice(start));
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

TODO Also keep context stack and skip vspace if inside `{}`, `[]`, or `()`.

Found a significant token, so done.

            else -> return true;
          }
        }

Ran out of tokens.

        false
      }

#### peek

Provide a bogus value if `!has()` with kind `Token.none`.

      private peek(): Token {
        if (has()) {
          tokens[index]
        } else {
          noneToken
        }
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
          let token = peek();
          when (token.kind) {
            Token.vspace -> pushToken(token);
            else -> statement();
          }
        }
        commit(Parse.block, start);

Finally, push the above root block itself.

        commit(Parse.block, 0);
      }

#### args

      private args(): Void {
        let start = work.length;
        pushToken(peek());
        params: while (has()) {
          let token = peek();
          when (token.kind) {
            Token.comma, Token.vspace -> pushToken(token);
            Token.roundClose -> do {
              pushToken(token);
              break params;
            }
            else -> expr();
          }
        }
        commit(Parse.args, start);
      }

#### add

      private add(): Void {
        let start = work.length;
        call();
        while (true) {
          let token = peek();
          when (token.kind) {
            Token.add, Token.sub -> do {
              pushToken(token);
              call();
              commit(Parse.infix, start);
            }
            else -> return;
          }
        }
      }

#### atom

These constructs are recognized by a prefix token.

      private atom(): Void {
        if (!has()) {
          return;
        }
        let token = peek();
        when (token.kind) {

Currently passing down the token as a small optimization. I recall seeing some
evidence that it helped in a previous implementation in Go.

          Token.case -> nym`case`(token);
          Token.else -> nym`else`(token);
          Token.fun -> fun(token);
          Token.id, Token.int -> pushToken(token);
          Token.plug, Token.pub -> modify(token);
          Token.return -> this.return(token); // nym`return` fails here!!!
          Token.stringOpen -> string(token);
          Token.sub -> prefix(token);
          Token.switch -> switch(token);
          Token.then -> block(); // simpler to redo token in this case
          Token.var -> nym`var`(token);
          Token.vspace -> void;
          else -> do {
            let start = work.length;
            pushToken(token);
            commit(Parse.junk, start);
          }
        }
      }

#### block

      private block(): Void {
        let start = work.length;
        let token = peek();
        if (token.kind == Token.then) {
          pushToken(token);
        }
        let token = peek();
        when (token.kind) {

Multiline blocks until end.

TODO Push `then` context. Pop at `end`.

          Token.vspace -> do {
            block: while (has()) {
              let token = peek();
              when (token.kind) {
                Token.vspace -> pushToken(token);
                Token.end -> do {
                  pushToken(token);
                  break block;
                }
                else -> statement();
              }
            }
            commit(Parse.block, start);
          }

Inline single-expression bodies.

          else -> expr();
        }
      }

#### call

      private call(): Void {
        let start = work.length;
        atom();
        while (true) {
          let token = peek();
          when (token.kind) {
            Token.roundOpen -> do {
              args();
              commit(Parse.call, start);
            }
            else -> return;
          }
        }
      }

#### case

      private nym`case`(token: Token): Void {
        let start = work.length;
        pushToken(token);
        let patternsStart = work.length;
        expr();

TODO Multiple patterns separated by comma.

        commit(Parse.args, patternsStart);
        caseFinish();
        commit(Parse.case, start);
      }

#### caseFinish

      private caseFinish(): Void {
        let start = work.length;
        let token = peek();

TODO Still need to add case guards using `if`.

We expect `then` for inline expressions, but be flexible in parsing.

        if (token.kind == Token.then) {
          pushToken(token);
        }
        let token = peek();
        when (token.kind) {

Case blocks are special because they end not only on `end` but also on either
`case` or `else`.

          Token.vspace -> do {
            block: while (has()) {
              let token = peek();
              when (token.kind) {
                Token.vspace -> pushToken(token);
                Token.case, Token.else, Token.end -> break block;
                else -> statement();
              }
            }
          }

A single inline expression is also allowed instead of a block.

          else -> expr();
        }
        commit(Parse.block, start);
      }

#### compare

      private compare(): Void {
        let start = work.length;
        add();
        while (true) {
          let token = peek();
          when (token.kind) {
            Token.eqEq, Token.ge, Token.gt, Token.le, Token.lt, Token.neq ->
              do {
                pushToken(token);
                add();
                commit(Parse.infix, start);
              }
            else -> return;
          }
        }
      }

#### else

      private nym`else`(token: Token): Void {
        let start = work.length;
        pushToken(token);
        caseFinish();
        commit(Parse.else, start);
      }

#### expr

      private expr(): Void {
        compare();
      }

#### fun

      private fun(token: Token): Void {
        let start = work.length;
        pushToken(token);
        let token = peek();
        if (token.kind == Token.id) {
          pushToken(token);
        }
        if (peek().kind == Token.roundOpen) {
          params();
        }

Note that `block` internally allows for inline single-expression bodies.

        block();
        commit(Parse.fun, start);
      }

#### modify

      private modify(token: Token): Void {
        let start = work.length;
        pushToken(token);

This loop allows for duplicates, which is ok for flexibility.

        mods: while (has()) {
          let token = peek();
          when (token.kind) {
            Token.plug, Token.pub -> pushToken(token);
            else -> break mods;
          }
        }
        expr();
        commit(Parse.modify, start);
      }

#### param

      private param(): Void {
        let start = work.length;
        var hadExpr = false;
        param: while (has()) {
          let token = peek();
          when (token.kind) {
            Token.comma, Token.roundClose -> break param;

TODO Once we manage context, these will get eaten automatically. We can then
get rid of the whole loop here.

            Token.vspace -> pushToken(token);
            else -> if (hadExpr) {
              break param;
            } else {
              expr();
              hadExpr = true;
            }
          }
        }
        commit(Parse.param, start);
      }

#### params

      private params(): Void {
        let start = work.length;
        pushToken(peek());
        params: while (has()) {
          let token = peek();
          when (token.kind) {
            Token.comma, Token.vspace -> pushToken(token);
            Token.roundClose -> do {
              pushToken(token);
              break params;
            }
            else -> param();
          }
        }
        commit(Parse.params, start);
      }

#### prefix

      private prefix(token: Token): Void {
        let start = work.length;
        pushToken(token);
        expr();
        commit(Parse.prefix, start);
      }

#### return

      private nym`return`(token: Token): Void {
        let start = work.length;
        pushToken(token);
        let token = peek();
        if (token.kind != Token.vspace) {
          expr();
        }
        commit(Parse.return, start);
      }

#### statement

For flexibility in parsing for tooling, statement is always expression, but
separating it allows for providing intent. At later stages, we might error on
certain constructs given below statement level.

      private statement(): Void {
        expr();
      }

#### string

      private string(token: Token): Void {
        let start = work.length;
        pushToken(token);
        string: while (has()) {
          let token = peek();
          when (token.kind) {
            Token.stringClose -> do {
              pushToken(token);
              break string;
            }
            Token.vspace -> break string;
          }
          pushToken(token);
        }
        commit(Parse.string, start);
      }

#### switch

      private switch(token: Token): Void {
        let start = work.length;
        pushToken(token);
        var kind = Parse.switch;
        let token = peek();
        when (token.kind) {
          Token.then, Token.vspace -> kind = Parse.switchEmpty;
          else -> expr();
        }
        block();
        commit(kind, start);
      }

#### var

Vars have various semi-optional parts, so we check each as we go. We could
structure more below, but it's simple enough to string along without introducing
more layers.

      private nym`var`(token: Token): Void {
        let start = work.length;
        pushToken(token);

Name is recognized later as the first thing after the `var` if no `eq` first.

        let token = peek();
        if (token.kind == Token.id) {
          pushToken(token);
        }

Type is recognized by a second node after `var`, if present.

        let token = peek();
        when (token.kind) {
          Token.eq, Token.vspace -> void;
          else -> expr(); // type
        }

Init is recognized by being after an `eq` token.

        let token = peek();
        if (token.kind == Token.eq) {
          pushToken(token);

Context doesn't easily work around vspace here because there's no special
terminator. Any non-vspace token means vspace time is over.

          vspace: while (true) {
            let token = peek();
            when (token.kind) {
              Token.vspace -> pushToken(token);
              else -> break vspace;
            }
          }
          expr();
        }
        commit(Parse.var, start);
      }

#### Parser end

    }
