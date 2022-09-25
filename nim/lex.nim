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
    keyElse,
    keyEnd,
    keyFor,
    keyOf,
    keyTo,
    opColon,
    opAdd,
    opDot,
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
    stringEscape,
    stringText,
    vspace

  TextId* = int32

  # Shared state across lexing tasks.
  Lexer* = ref object
    keys: Table[TextId, TokenKind]
    pool*: Pool[TextId]

  Token* = object
    kind*: TokenKind
    text*: TextId

  LexingMode = enum
    default,
    stringContent

  # Independent state, except for the lexer.
  Lexing = ref object
    begin: int
    index: int
    lexer: Lexer
    mode: LexingMode
    text: string

const
  DigitChars = {'0'..'9'}
  HSpaceChars = {' ', '\t'}
  StringSpecialChars = {'"', '\\'}
  VSpaceChars = {'\r', '\n'}

func idContinue(c: char): bool {.raises: [].} =
  case c:
    of '.', ',', ';', ':', '?', ' ', '\t', '\r', '\n', '(', ')', '[', ']', '{',
      '}', '#': false
    else: true

proc advance(lexing: var Lexing): char {.raises: [].} =
  result = lexing.text[lexing.index]
  lexing.index += 1

proc advanceAs(lexing: var Lexing, kind: TokenKind): TokenKind {.raises: [].} =
  lexing.index += 1
  kind

proc advanceWhile(
  lexing: var Lexing, check: proc(c: char): bool {.noSideEffect.}
) {.raises: [].} =
  block done:
    for index in lexing.index + 1 ..< lexing.text.len:
      if not check(lexing.text[index]):
        lexing.index = index
        break done
    lexing.index = lexing.text.len

func current(lexing: Lexing): char {.raises: [].} = lexing.text[lexing.index]

proc nextComment(lexing: var Lexing): TokenKind {.raises: [].} =
  lexing.advanceWhile(func(c: char): bool = not (c in VSpaceChars))
  comment

proc nextHspace(lexing: var Lexing): TokenKind {.raises: [].} =
  lexing.advanceWhile(func(c: char): bool = c in HSpaceChars)
  hspace

proc nextId(lexing: var Lexing): TokenKind {.raises: [].} =
  lexing.advanceWhile(func(c: char): bool = c.idContinue)
  id

proc nextNumber(lexing: var Lexing): TokenKind {.raises: [].} =
  lexing.advanceWhile(func(c: char): bool = c in DigitChars)
  integer

proc nextOther(lexing: var Lexing): TokenKind {.raises: [].} =
  case lexing.advance:
    of '(': roundBegin
    of ')': roundEnd
    of '.': opDot
    of '+': opAdd
    of '-': opSub
    of '=':
      case lexing.current:
        of '=': lexing.advanceAs(opEq)
        else: opIs
    of '>':
      case lexing.current:
        of '=': lexing.advanceAs(opGe)
        else: opGt
    of '<':
      case lexing.current:
        of '=': lexing.advanceAs(opLe)
        else: opLt
    of '!':
      case lexing.current:
        of '=': lexing.advanceAs(opNe)
        else: other
    else: other

proc nextVspace(lexing: var Lexing): TokenKind {.raises: [].} =
  lexing.advanceWhile(func(c: char): bool = c in VSpaceChars)
  vspace

proc nextDefault(lexing: var Lexing): TokenKind {.raises: [].} =
  case lexing.current:
    of 'A'..'Z', 'a'..'z', '_': lexing.nextId()
    of '#': lexing.nextComment()
    of '"':
      lexing.mode = stringContent
      lexing.advanceAs(quoteDouble)
    of HSpaceChars: lexing.nextHspace()
    of DigitChars: lexing.nextNumber()
    of VSpaceChars: lexing.nextVspace()
    else: lexing.nextOther()

proc nextStringContent(lexing: var Lexing): TokenKind {.raises: [].} =
  case lexing.advance:
    of '"':
      lexing.mode = default
      quoteDouble
    of '\\': lexing.advanceAs(stringEscape)
    else:
      lexing.advanceWhile(func(c: char): bool = not (c in StringSpecialChars))
      stringText

proc next(lexing: var Lexing): Option[Token] {.raises: [].} =
  let begin = lexing.index
  if lexing.index >= lexing.text.len:
    return none(Token)
  let
    kind = case lexing.mode:
      of default: lexing.nextDefault()
      of stringContent: lexing.nextStringContent()
    text = lexing.lexer.pool.intern(lexing.text[begin ..< lexing.index])
    # Figure out if we have a keyword instead of an id.
    finalKind = if kind == id:
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
      "else": keyElse,
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
