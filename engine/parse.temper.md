## Parsing

### ParseNode

    // struct
    export class ParseNode(
      public kind: ParseKind,
      public kids: Range/*<ParseNode>*/,
      public token: Token,
    ) {}

### ParseKind

    export let ParseKind = Int32;

type Parse just exists for hosting parse kind values.

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

    // struct
    export class Range/*<Item>*/(
      public start: Int,
      public end: Int,
    ) {}
