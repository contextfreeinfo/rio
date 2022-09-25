import intern
import std/options
import std/sequtils
import std/tables

type
  TokenKind* = enum
    comment,
    eof,
    hspace,
    id,
    integer,
    keyAs,
    keyBe,
    keyEnd,
    keyFor,
    keyOf,
    keyTo,
    opColon,
    opAdd,
    opEq,
    opGe,
    opGt,
    opIs,
    opLe,
    opLt,
    opNe,
    opSub,
    other,
    quoteDouble,
    roundBegin,
    roundEnd,
    stringText,
    vspace

  TextId* = int32

  Lexer* = ref object
    keys: Table[TextId, TokenKind]
    pool*: Pool[TextId]

  Token* = object
    kind*: TokenKind
    text*: TextId

  Lexing = ref object
    begin: int
    index: int
    lexer: Lexer
    text: string

const
  HSpaceChars = {' ', '\t'}

func idContinue(c: char): bool {.raises: [].} =
  case c:
    of '.', ',', ';', ':', '?', ' ', '\t', '\r', '\n', '(', ')', '[', ']', '{',
      '}', '#': false
    else: true

proc advanceWhile(
  lexing: var Lexing, check: proc(c: char): bool {.noSideEffect.}
) {.raises: [].} =
  block done:
    for index in lexing.index + 1 ..< lexing.text.len:
      if not check(lexing.text[index]):
        lexing.index = index
        break done
    lexing.index = lexing.text.len

proc nextHspace(lexing: var Lexing): TokenKind {.raises: [].} =
  lexing.advanceWhile(func(c: char): bool = c in HSpaceChars)
  hspace

proc nextId(lexing: var Lexing): TokenKind {.raises: [].} =
  lexing.advanceWhile(func(c: char): bool = c.idContinue)
  id

proc nextOther(lexing: var Lexing): TokenKind {.raises: [].} =
  lexing.index += 1
  other

proc next(lexing: var Lexing): Option[Token] {.raises: [].} =
  let begin = lexing.index
  if lexing.index >= lexing.text.len:
    return none(Token)
  let
    kind = case lexing.text[lexing.index]:
      of 'A'..'Z', 'a'..'z', '_': lexing.nextId()
      of HSpaceChars: lexing.nextHspace()
      else: lexing.nextOther()
    text = lexing.lexer.pool.intern(lexing.text[begin ..< lexing.index])
    # Figure out if we have a keyword instead of an id.
    finalKind = if kind == TokenKind.id:
      lexing.lexer.keys.getOrDefault(text, kind)
    else:
      kind
  some(Token(kind: finalKind, text: text))

proc lex(lexing: var Lexing): seq[Token] =
  while true:
    let next = lexing.next
    if next.isSome:
      result.add(next.get)
    else:
      break

proc newLexer*(): Lexer =
  var pool = newPool[TextId]()
  Lexer(
    keys: {
      "as": keyAs,
      "be": keyBe,
      "end": keyEnd,
      "for": keyFor,
      "of": keyOf,
      "to": keyTo,
    }.map(
      func(pair: (string, TokenKind)): (TextId, TokenKind) =
        (pool.intern(pair[0]), pair[1])
    ).toTable,
    pool: pool,
  )

proc lex*(lexer: var Lexer, text: string): seq[Token] =
  var lexing = Lexing(begin: 0, index: 0, lexer: lexer, text: text)
  lexing.lex
