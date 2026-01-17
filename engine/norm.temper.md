## Norming

I using norming to mean converting a parse tree into an abstract tree.

    export class Normer {

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
        when (parsedAt(parent, kidIndex).token.kind) {
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
              when (kid.token.kind) {
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

#### normNode

      private normNode(node: ParseNode): Void {
        when (node.parseKind) {
          Parse.args -> normArgs(node as ParseParent);
          Parse.block -> normBlock(node as ParseParent);
          Parse.call -> normCall(node as ParseParent);
          Parse.case -> normCase(node as ParseParent);
          Parse.else -> normElse(node as ParseParent);
          Parse.fun -> void;
          Parse.infix -> void;
          Parse.modify -> void;
          Parse.param -> void;
          Parse.params -> void;
          Parse.prefix -> void;
          Parse.return -> void;
          Parse.string -> void;
          Parse.switch -> void;
          Parse.switchEmpty -> void;
          Parse.token -> void;
          Parse.var -> void;

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
          normArgs(part as ParseParent orelse panic());
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
          let args = part as ParseParent orelse panic();
          normArgItems(args, args.kids.start);
          patterns = builder.popWorkBlock();
          part = parsedAt(node, (kidIndex = nextParsed(node, kidIndex + 1)));
        }
        var kids = Range.empty;
        if (part.parseKind == Parse.block) {
          normBlock(part as ParseParent orelse panic());
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
          normBlock(part as ParseParent orelse panic());
          kids = builder.popWorkBlock();
          part = parsedAt(node, (kidIndex = nextParsed(node, kidIndex + 1)));
        }
        builder.work.add({ class: Case, always: true, kids });
      }

    }

#### ParseNodeLooper

Return true to finish looping early.

    @fun interface ParseNodeLooper(node: ParseNode): Boolean;
