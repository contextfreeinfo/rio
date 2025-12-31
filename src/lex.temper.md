## Lexer

    export let lex(source: String): List<Token> {
      [
        new Token(tokenNone, 0),
      ]
    }

    export class Token(
      public kind: TokenKind,
      public text: TextId,
    ) {}

    let TokenKind = Int;
    let TextId = Int;

    export let tokenNone: TokenKind = 0;
    export let tokenAdd: TokenKind = tokenNone + 1;
    export let tokenAs: TokenKind = tokenAdd + 1;
    export let tokenBreak: TokenKind = tokenAs + 1;
    export let tokenCase: TokenKind = tokenBreak + 1;
    export let tokenChange: TokenKind = tokenCase + 1;
    export let tokenClass: TokenKind = tokenChange + 1;
    export let tokenComma: TokenKind = tokenClass + 1;
    export let tokenCommentOpen: TokenKind = tokenComma + 1;
    export let tokenCommentText: TokenKind = tokenCommentOpen + 1;
    export let tokenConst: TokenKind = tokenCommentText + 1;
    export let tokenContinue: TokenKind = tokenConst + 1;
    export let tokenElse: TokenKind = tokenContinue + 1;
    export let tokenEnd: TokenKind = tokenElse + 1;
    export let tokenEq: TokenKind = tokenEnd + 1;
    export let tokenEqEq: TokenKind = tokenEq + 1;
    export let tokenEnum: TokenKind = tokenEqEq + 1;
    export let tokenFor: TokenKind = tokenEnum + 1;
    export let tokenFrom: TokenKind = tokenFor + 1;
    export let tokenFun: TokenKind = tokenFrom + 1;
    export let tokenGe: TokenKind = tokenFun + 1;
    export let tokenGt: TokenKind = tokenGe + 1;
    export let tokenHSpace: TokenKind = tokenGt + 1;
    export let tokenId: TokenKind = tokenHSpace + 1;
    export let tokenIf: TokenKind = tokenId + 1;
    export let tokenInt: TokenKind = tokenIf + 1;
    export let tokenIs: TokenKind = tokenInt + 1;
    export let tokenImport: TokenKind = tokenIs + 1;
    export let tokenLe: TokenKind = tokenImport + 1;
    export let tokenLt: TokenKind = tokenLe + 1;
    export let tokenJunk: TokenKind = tokenLt + 1;
    export let tokenNot: TokenKind = tokenJunk + 1;
    export let tokenNeq: TokenKind = tokenNot + 1;
    export let tokenPlug: TokenKind = tokenNeq + 1;
    export let tokenPub: TokenKind = tokenPlug + 1;
    export let tokenReturn: TokenKind = tokenPub + 1;
    export let tokenRoundClose: TokenKind = tokenReturn + 1;
    export let tokenRoundOpen: TokenKind = tokenRoundClose + 1;
    export let tokenStringEscape: TokenKind = tokenRoundOpen + 1;
    export let tokenStringText: TokenKind = tokenStringEscape + 1;
    export let tokenStringClose: TokenKind = tokenStringText + 1;
    export let tokenStringOpen: TokenKind = tokenStringClose + 1;
    export let tokenStruct: TokenKind = tokenStringOpen + 1;
    export let tokenSub: TokenKind = tokenStruct + 1;
    export let tokenSwitch: TokenKind = tokenSub + 1;
    export let tokenThen: TokenKind = tokenSwitch + 1;
    export let tokenVSpace: TokenKind = tokenThen + 1;
    export let tokenUnion: TokenKind = tokenVSpace + 1;
    export let tokenUse: TokenKind = tokenUnion + 1;
    export let tokenVar: TokenKind = tokenUse + 1;
    export let tokenVartype: TokenKind = tokenVar + 1;

    class Lexer {
      public var index: StringIndex = String.begin;
      public var source: String = "";
      public var peeked: Int = 0;
      public var tokens: ListBuilder<Token> = new ListBuilder();
    }
