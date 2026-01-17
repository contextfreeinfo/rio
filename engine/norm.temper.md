## Norming

I using norming to mean converting a parse tree into an abstract tree.

    export class Normer {

      private builder: ModuleBuilder = new ModuleBuilder();

      private var parsed: List<ParseNode> = [];

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

#### normNode

      private normNode(node: ParseNode): Void {
        when (node.parseKind) {
          Parse.args -> normArgs(node as ParseParent);
          Parse.block -> void;
          Parse.call -> void;
          Parse.case -> void;
          Parse.comment -> void;
          Parse.else -> void;
          Parse.fun -> void;
          Parse.infix -> void;
          Parse.junk -> void;
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
        } orelse panic();
      }

#### normArgs

      private normArgs(node: ParseParent): Void {
        //
      }

    }
