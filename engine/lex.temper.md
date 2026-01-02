## Lexing

### Token

We plan to switch to struct Token in the future for efficiency. And even if we
support unions in structs, we still might want a separate TokenKind, because we
want the same layout for each token kind here.

    export class Token(
      public kind: TokenKind,
      public text: TextId,
    ) {
      public toString(): String {
        "(${Token.names[kind]}: ${text})"
      }

      public stringify(interner: Interner): String {
        "(${Token.names[kind]}: \"${interner.string(text)}\")"
      }

#### TokenKind values

Store these inside `Token` for now for convenience, until we have enums in
Temper.

      public static none: TokenKind = 0;
      public static add: TokenKind = Token.none + 1;
      public static nym`as`: TokenKind = Token.add + 1;
      public static nym`break`: TokenKind = Token.as + 1;
      public static nym`case`: TokenKind = Token.break + 1;
      public static change: TokenKind = Token.case + 1;
      public static nym`class`: TokenKind = Token.change + 1;
      public static comma: TokenKind = Token.class + 1;
      public static commentOpen: TokenKind = Token.comma + 1;
      public static commentText: TokenKind = Token.commentOpen + 1;
      public static nym`const`: TokenKind = Token.commentText + 1;
      public static nym`continue`: TokenKind = Token.const + 1;
      public static nym`else`: TokenKind = Token.continue + 1;
      public static end: TokenKind = Token.else + 1;
      public static eq: TokenKind = Token.end + 1;
      public static eqEq: TokenKind = Token.eq + 1;
      public static nym`enum`: TokenKind = Token.eqEq + 1;
      public static nym`for`: TokenKind = Token.enum + 1;
      public static from: TokenKind = Token.for + 1;
      public static fun: TokenKind = Token.from + 1;
      public static ge: TokenKind = Token.fun + 1;
      public static gt: TokenKind = Token.ge + 1;
      public static hSpace: TokenKind = Token.gt + 1;
      public static id: TokenKind = Token.hSpace + 1;
      public static nym`if`: TokenKind = Token.id + 1;
      public static int: TokenKind = Token.if + 1;
      public static nym`is`: TokenKind = Token.int + 1;
      public static nym`import`: TokenKind = Token.is + 1;
      public static le: TokenKind = Token.import + 1;
      public static lt: TokenKind = Token.le + 1;
      public static junk: TokenKind = Token.lt + 1;
      public static not: TokenKind = Token.junk + 1;
      public static neq: TokenKind = Token.not + 1;
      public static plug: TokenKind = Token.neq + 1;
      public static pub: TokenKind = Token.plug + 1;
      public static nym`return`: TokenKind = Token.pub + 1;
      public static roundClose: TokenKind = Token.return + 1;
      public static roundOpen: TokenKind = Token.roundClose + 1;
      public static stringEscape: TokenKind = Token.roundOpen + 1;
      public static stringText: TokenKind = Token.stringEscape + 1;
      public static stringClose: TokenKind = Token.stringText + 1;
      public static stringOpen: TokenKind = Token.stringClose + 1;
      public static struct: TokenKind = Token.stringOpen + 1;
      public static sub: TokenKind = Token.struct + 1;
      public static switch: TokenKind = Token.sub + 1;
      public static then: TokenKind = Token.switch + 1;
      public static vSpace: TokenKind = Token.then + 1;
      public static union: TokenKind = Token.vSpace + 1;
      public static use: TokenKind = Token.union + 1;
      public static nym`var`: TokenKind = Token.use + 1;
      public static vartype: TokenKind = Token.var + 1;

#### TokenKind names

We currently rely on maintaining the same order below as above. Ideally, Temper
enums provide these automatically.

      public static names = [
        "none",
        "add",
        "as",
        "break",
        "case",
        "change",
        "class",
        "comma",
        "commentOpen",
        "commentText",
        "const",
        "continue",
        "else",
        "end",
        "eq",
        "eqEq",
        "enum",
        "for",
        "from",
        "fun",
        "ge",
        "gt",
        "hSpace",
        "id",
        "if",
        "int",
        "is",
        "import",
        "le",
        "lt",
        "junk",
        "not",
        "neq",
        "plug",
        "pub",
        "return",
        "roundClose",
        "roundOpen",
        "stringEscape",
        "stringText",
        "stringClose",
        "stringOpen",
        "struct",
        "sub",
        "switch",
        "then",
        "vSpace",
        "union",
        "use",
        "var",
        "vartype",
      ];
    }

### TextId

We intern strings so we can store them as Int32 values, largely so we can use
structs in the future instead of classes.

    let TextId = Int32;


### TokenKind

It would be nice to have simple enum instead of Int, so long as enums can be
stored in structs in the future.

    let TokenKind = Int32;

### Lexer

Ideally, we reuse the lexer and just clear out the tokens while retaining
capacity, to reduce allocations.

    export class Lexer(
      public interner: Interner = new Interner(),
    ) {
      public var source: String = "";
      public var index: StringIndex = String.begin;
      public var peeked: Int = 0;
      public var hasPeeked: Boolean = false;

Retaining capacity matters less when using class Token instead of struct, but
ideally we can improve on this later.

      public tokens: ListBuilder<Token> = new ListBuilder();

#### lex

You can call lex multiple times on the same lexer.

      public lex(source: String): List<Token> {

For reuse, first reset the lexer.

        this.source = source;
        index = String.begin;
        peeked = 0;
        hasPeeked = false;
        tokens.clear();

Now do lexing.

        doLex();
        tokens.toList()
      }

#### has

      private has(): Boolean {
        index < source.end
      }

#### next

      private next(): Void {
        hasPeeked = false;
        if (!has()) {
          return;
        }
        index = source.next(index);
      }

#### peek

      private peek(): Int {
        if (hasPeeked) {
          return peeked;
        }
        if (!has()) {
          return 0;
        }

This decoding might require calculation from multiple bytes, such as utf8.
TODO Do we have a way to advance and decode at the same time?

        peeked = source[index];
        hasPeeked = true;
        return peeked;
      }

#### push

      private push(kind: TokenKind, start: StringIndex): Void {
        if (start < index) {
          let text = source.slice(start, index);

Check here for keywords to avoid making extra slices elsewhere. Maybe this is
worth it?

          let kind = when (kind) {
            Token.id -> do {
              let kind = keys.getOr(text, -1);
              when (kind) {
                -1 -> Token.id;
                else -> kind;
              }
            }
            else -> kind
          };
          let textId = interner[text];
          tokens.add(new Token(kind, textId));
        }
      }

#### doLex

      private doLex(): Void {
        token: while (has()) {
          let c = peek();
          if (isLetter(c) || c == char"$" || c == char"_") {
            id();
          } else if (isDigit(c)) {
            number();
          } else if (c == char" " || c == char"\t") {
            hspace();
          } else {

Above deals in char ranges. Below branches on the individual char and always
immediate consumes the first char before taking next steps.

            let start = index;
            next();
            let kind = when (c) {
              char"#" -> do {
                push(Token.commentOpen, start);
                comment();
                continue token;
              }
              char'"' -> do {
                push(Token.stringOpen, start);
                string();
                continue token;
              }
              char"=" -> when (peek()) {
                char"=" -> do {
                  next();
                  Token.eqEq
                }
                else -> Token.eq
              }
              char"+" -> do {
                // TODO Compound operators.
                Token.add
              }
              char"-" -> do {
                // TODO Compound operators.
                // TODO Incorporated negation on numbers.
                Token.sub
              }
              char"<" -> when (peek()) {
                char"=" -> do {
                  next();
                  Token.le
                }
                else -> Token.lt
              }
              char">" -> when (peek()) {
                char"=" -> do {
                  next();
                  Token.ge
                }
                else -> Token.gt
              }
              char"," -> Token.comma;
              char"(" -> Token.roundOpen;
              char")" -> Token.roundClose;
              char"\r" -> do {
                if (peek() == char"\n") {
                  next();
                }
                Token.vSpace
              }
              char"\n" -> Token.vSpace;
              else -> Token.junk;
            };
            push(kind, start);
          }
        }
      }

#### comment

      private comment(): Void {
        let start = index;
        comment: while (has()) {
          let c = peek();
          if (c == char"\n") {
            break comment;
          }
          next();
        }
        push(Token.commentText, start);
      }

#### hspace

      private hspace(): Void {
        let start = index;
        next();
        hspace: while (has()) {
          let c = peek();
          if (!(c == char" " || c == char"\t")) {
            break hspace;
          }
          next();
        }
        push(Token.hSpace, start);
      }

#### id

      private id(): Void {
        let start = index;
        next();
        id: while (has()) {
          let c = peek();
          if (!(isDigit(c) || isLetter(c) || c == char"_")) {
            break id;
          }
          next();
        }
        push(Token.id, start);
      }

#### number

      private number(): Void {
        let start = index;
        // TODO Include negative in int literal?
        next();
        int: while (has()) {
          let c = peek();
          if (!isDigit(c)) {
            break int;
          }
          next();
        }
        // TODO Check if it's a float literal at this point.
        push(Token.int, start);
      }

#### string

Here we loop a mini state machine of sorts, pushing tokens as we go. Each escape
or the text between them is a different token.

      private string(): Void {
        var start = index;
        var kind = Token.stringText;
        string: while (has()) {
          let c = peek();

And here's our current state variety for escapes. If no next, we just get a
dangling escape.

TODO For interpolation, call back into doLex?

          if (kind == Token.stringEscape) {
            if (!(c == char"\r" || c == char"\n")) {
              next();
            }
            push(kind, start);
            kind = Token.stringText;
            start = index;
            continue string;
          }

The main case is continuing string text here.

          when (c) {

Explicit string termination.

            char'"' -> do {
              push(kind, start);
              start = index;
              next();
              kind = Token.stringClose;
              break string;
            }

Start escape.

            char "\\" -> do {
              push(kind, start);
              kind = Token.stringEscape;
              start = index;
            }

Multiline strings are handled at the parsing level, not in the lexer.

            char"\r", char"\n" -> break string;
          }
          next();
        }

Most commonly stringClose, but could be a dangling end.

        push(kind, start);
      }

#### Lexer class end

    }

### Keywords

    let keys = new Map([
      new Pair("as", Token.as),
      new Pair("break", Token.break),
      new Pair("case", Token.case),
      new Pair("class", Token.class),
      new Pair("change", Token.change),
      new Pair("const", Token.const),
      new Pair("continue", Token.continue),
      new Pair("else", Token.else),
      new Pair("end", Token.end),
      new Pair("if", Token.if),
      new Pair("is", Token.is),
      new Pair("import", Token.import),
      new Pair("enum", Token.enum),
      new Pair("for", Token.for),
      new Pair("from", Token.from),
      new Pair("fun", Token.fun),
      new Pair("plug", Token.plug),
      new Pair("pub", Token.pub),
      new Pair("return", Token.return),
      new Pair("struct", Token.struct),
      new Pair("switch", Token.switch),
      new Pair("then", Token.then),
      new Pair("union", Token.union),
      new Pair("use", Token.use),
      new Pair("var", Token.var),
      new Pair("vartype", Token.vartype),
    ]);
