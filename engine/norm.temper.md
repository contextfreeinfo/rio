## Norming

I using norming to mean converting a parse tree into an abstract tree.

    export class Normer(
      public interner: Interner,
    ) {

      private builder: ModuleBuilder = new ModuleBuilder();
      // private get workStart(): Int { builder.work.length }

      private var parsed: List<ParseNode> = [];

      private parsedAt(parent: ParseParent, kidIndex: Int): ParseNode {
        let kids = parent.kids;
        if (kidIndex >= kids.start && kidIndex < kids.end) {
          parsed[kidIndex]
        } else {
          noneParse
        }
      }

#### norm

      public norm(parsed: List<ParseNode>): ModuleBuilder {
        this.parsed = parsed;
        builder.reset();
        if (parsed.length == 0) {
          return new ModuleBuilder();
        }
        normNode(parsed[parsed.length - 1]);

TODO Some way to pull out one ModuleBuilder from another. Could pull out an imu
thing, but being able to reassign nodes in place can be handy.

        return new ModuleBuilder();
      }

#### expectToken

Returns the index of the next meaningful parsed node if it's a matching token.
Otherwise returns parent kids end.

      private expectToken(
        parent: ParseParent,
        kind: TokenKind,
        kidIndex: Int = parent.kids.start,
      ): Int {
        let kidIndex = nextParsed(parent, kidIndex);
        when (parsedAt(parent, kidIndex).asToken.kind) {
          kind -> kidIndex;
          else -> parent.kids.end;
        }
      }

#### loopParsed

      private loopParsed(
        parent: ParseParent,
        looper: ParseNodeLooper,
        kidIndex: Int = parent.kids.start,
      ): Int {
        let kids = parent.kids;
        for (
          var i = nextParsed(parent, kidIndex);
          i < kids.end;
          i = nextParsed(parent, i + 1)
        ) {
          if (looper(parsed[i])) {
            return i + 1;
          }
        }
        kids.end
      }

#### nextParsed

Returns the index of the nearest meaningful parse node, starting at start.

      private nextParsed(
        parent: ParseParent,
        start: Int = parent.kids.start,
        keepVSpace: Boolean = false,
      ): Int {
        let kids = parent.kids;
        for (var i = start; i < kids.end; ++i) {
          let kid = parsed[i];
          when (kid.parseKind) {
            Parse.comment -> void;
            Parse.token -> do {
              when (kid.asToken.kind) {
                Token.commentOpen, Token.commentText, Token.hspace -> void;
                Token.vspace -> if (keepVSpace) {
                  return i;
                }
                else -> return i;
              }
            }
            else -> return i;
          }
        }
        kids.end
      }

#### normNodeCommit

      private normNodeCommit(node: ParseNode): NodeId {
        let start = builder.work.length;
        normNode(node);
        if (builder.work.length > start) {
          builder.commitHeadless(start);
          builder.nodes.length - 1
        } else {
          0
        }
      }

#### normNode

      private normNode(node: ParseNode): Void {
        let asParent = node.asParent;
        when (node.parseKind) {
          Parse.args -> normArgs(asParent);
          Parse.block -> normBlock(asParent);
          Parse.call -> normCall(asParent);
          Parse.case -> normCase(asParent);
          Parse.else -> normElse(asParent);
          Parse.fun -> normFun(asParent);
          Parse.infix -> normFun(asParent);
          Parse.modify -> normModify(asParent);
          Parse.param -> normParam(asParent);
          Parse.params -> normParams(asParent);
          Parse.prefix -> normPrefix(asParent);
          Parse.return -> normReturn(asParent);
          Parse.string -> normString(asParent);
          Parse.switch, Parse.switchEmpty -> normSwitch(asParent);
          Parse.token -> normToken(node.asToken);
          Parse.var -> normVar(asParent);

Nothing to do with these.

          Parse.comment, Parse.junk -> void;
        } orelse panic();
      }

#### normArgs

      private normArgs(node: ParseParent): Void {
        var kidIndex = expectToken(node, Token.roundOpen);
        kidIndex = normArgItems(node, kidIndex + 1);
        expectToken(node, Token.roundClose, kidIndex);
      }

#### normArgItems

      private normArgItems(node: ParseParent, kidIndex: Int): Int {
        let start = builder.work.length;
        let kidIndex = loopParsed(node, kidIndex) { kid =>
          var needNorm = false;
          let done = when (kid.parseKind) {
            Parse.token -> do {
              let token = kid as Token orelse panic();
              when (token.kind) {
                Token.comma -> false;
                Token.roundClose -> true;
                else -> do {
                  needNorm = true;
                  false
                }
              }
            }
            else -> do {
              needNorm = true;
              false
            }
          };
          if (needNorm) {
            normNode(kid);
          }
          done
        }
        builder.commitBlock(start);
        kidIndex
      }

#### normBlock

      private normBlock(node: ParseParent): Void {
        let start = builder.work.length;
        loopParsed(node) { kid =>
          normNode(kid);
          false
        }
        builder.commitBlock(start);
      }

#### normCall

      private normCall(node: ParseParent): Void {
        let start = builder.work.length;
        var kidIndex = nextParsed(node);
        normNode(parsedAt(node, kidIndex));
        var args = Range.empty;
        var part = parsedAt(node, (kidIndex = nextParsed(node, kidIndex + 1)));
        if (part.parseKind == Parse.args) {
          normArgs(part.asParent);
          args = builder.popWorkBlock();
          part = parsedAt(node, (kidIndex = nextParsed(node, kidIndex + 1)));
        }

We're about to push the callee as the next committed node.

        builder.commit(start, { args, callee: builder.nodes.length });
      }

#### normCase

      private normCase(node: ParseParent): Void {
        var kidIndex = expectToken(node, Token.case);
        var part = parsedAt(node, (kidIndex = nextParsed(node, kidIndex + 1)));
        var patterns = Range.empty;
        if (part.parseKind == Parse.args) {
          let args = part.asParent;
          normArgItems(args, args.kids.start);
          patterns = builder.popWorkBlock();
          part = parsedAt(node, (kidIndex = nextParsed(node, kidIndex + 1)));
        }
        var kids = Range.empty;
        if (part.parseKind == Parse.block) {
          normBlock(part.asParent);
          kids = builder.popWorkBlock();
          part = parsedAt(node, (kidIndex = nextParsed(node, kidIndex + 1)));
        }
        builder.work.add({ class: Case, patterns, kids });
      }

#### normElse

      private normElse(node: ParseParent): Void {
        var kidIndex = expectToken(node, Token.else);
        var part = parsedAt(node, (kidIndex = nextParsed(node, kidIndex + 1)));
        var kids = Range.empty;
        if (part.parseKind == Parse.block) {
          normBlock(part.asParent);
          kids = builder.popWorkBlock();
          part = parsedAt(node, (kidIndex = nextParsed(node, kidIndex + 1)));
        }
        builder.work.add({ class: Case, always: true, kids });
      }

#### normFun

      private normFun(node: ParseParent): Void {
        var kidIndex = expectToken(node, Token.fun);
        var part = parsedAt(node, (kidIndex = nextParsed(node, kidIndex + 1)));
        var name = 0;
        if (part.asToken.kind == Token.id) {
          name = part.asToken.text;
          part = parsedAt(node, (kidIndex = nextParsed(node, kidIndex + 1)));
        }
        var params = Range.empty;
        if (part.parseKind == Parse.params) {
          normParams(part.asParent);

TODO Get back to this when params pushes a block.

          // params = builder.popWorkBlock();
          part = parsedAt(node, (kidIndex = nextParsed(node, kidIndex + 1)));
        }
        var kids = Range.empty;
        if (part.parseKind == Parse.block) {
          normBlock(part.asParent);
          kids = builder.popWorkBlock();
          part = parsedAt(node, (kidIndex = nextParsed(node, kidIndex + 1)));
        }
        builder.work.add({ class: Fun, name, params, kids });
      }

#### normInfix

We turn infix expressions into method calls, such as changing `a + b` into
`a.add(b)`. And this is an example of the abstract tree not being so directly
tied to core syntax.

      private normInfix(node: ParseParent): Void {
        let start = builder.work.length;

Make a get node for the method call, treating the first operand as the subject.

        var kidIndex = nextParsed(node);
        let subject = normNodeCommit(parsedAt(node, kidIndex));

Translate the operator to a method name.

        var member = 0;
        let op = parsedAt(node, (kidIndex = nextParsed(node, kidIndex + 1)));
        let name = when (op.asToken.kind) {
          Token.add -> "add";
          Token.eqEq -> "eq";
          Token.gt -> "gt";
          Token.lt -> "lt";
          Token.sub -> "sub";
          else -> "";
        };
        if (name != "") {
          let name = interner[name];
          builder.work.add({ class: Ref, name });
          builder.commitHeadless(start);
          member = builder.nodes.length - 1;
        }

Commit the get.

        builder.commit(start, { class: Get, subject, member });
        builder.commitHeadless(start);
        let callee = builder.nodes.length - 1;

Make the other operand a method arg.

        let other = parsedAt(node, (kidIndex = nextParsed(node, kidIndex + 1)));
        normNode(other);
        builder.commitBlock(start);
        let args = builder.popWorkBlock();

Finish.

        builder.commit(start, { class: Call, callee, args });
      }

#### normModify

      private normModify(node: ParseParent): Void {

Gather up flags.

        var flags = 0;
        var kidIndex = nextParsed(node);
        var part = parsedAt(node, kidIndex);
        modify: while (true) {
          when (part.asToken.kind) {
            Token.plug -> flags |= DefFlag.plug;
            Token.pub -> flags |= DefFlag.pub;
            else -> break modify;
          }
          part = parsedAt(node, (kidIndex = nextParsed(node, kidIndex + 1)));
        }

Apply them to any nested def.

        normNode(part);
        part = parsedAt(node, (kidIndex = nextParsed(node, kidIndex + 1)));
        let workTop = builder.work[builder.work.length - 1];

We really badly need `{ ...workTop, flags }` splatting here.

        when (workTop) {
          is Fun -> builder.work[builder.work.length - 1] = {
            class: Fun,
            flags: flags | workTop.flags,
            source: workTop.source,
            name: workTop.name,
            params: workTop.params,
            nym`return`: workTop.return,
            kids: workTop.kids,
          };
          is Var -> builder.work[builder.work.length - 1] = {
            class: Var,
            flags: flags | workTop.flags,
            name: workTop.name,
            type: workTop.type,
            value: workTop.value,
          };
        }
      }

#### normParam

      private normParam(node: ParseParent): Void {
      }

#### normParams

      private normParams(node: ParseParent): Void {
      }

#### normPrefix

      private normPrefix(node: ParseParent): Void {
      }

#### normReturn

      private normReturn(node: ParseParent): Void {
      }

#### normString

      private normString(node: ParseParent): Void {
      }

#### normSwitch

      private normSwitch(node: ParseParent): Void {
      }

#### normToken

      private normToken(token: Token): Void {
      }

#### normTokenInt

      private normTokenInt(node: ParseParent): Void {
      }

#### normVar

      private normVar(node: ParseParent): Void {
      }

#### normVarFinish

      private normVarFinish(node: ParseParent): Void {
      }

    }

#### ParseNodeLooper

Return true to finish looping early.

    @fun interface ParseNodeLooper(node: ParseNode): Boolean;
