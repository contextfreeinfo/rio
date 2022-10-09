import intern
import std/options
import std/sequtils
import std/tables

type
  TokenKind* {.size: sizeof(uint32).} = enum
    comment,
    eof,
    hspace,
    id,
    integer,
    keyAs,
    keyBe,
    keyEnd,
    keyFor,
    keyIs,
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

  Lexer* = ref object ## Shared state across lexing tasks.
    keys: Table[TextId, TokenKind] ## Can be shared across lexers.
    pool*: Pool[TextId] ## Need to be shared across multiple lexers if any.
    tokens: seq[Token] ## Working buffer to be cleared per run.

  Token* = object
    kind*: TokenKind
    text*: TextId

  Tokens* = ref object
    tokens*: seq[Token]

  LexingMode = enum
    default,
    stringContent

  Lexing = ref object
    ## Independent state, except for the lexer.
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

proc nextHSpace(lexing: var Lexing): TokenKind {.raises: [].} =
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

proc nextVSpace(lexing: var Lexing): TokenKind {.raises: [].} =
  lexing.advanceWhile(func(c: char): bool = c in VSpaceChars)
  vspace

proc nextDefault(lexing: var Lexing): TokenKind {.raises: [].} =
  case lexing.current:
    of 'A'..'Z', 'a'..'z', '_': lexing.nextId()
    of '#': lexing.nextComment()
    of '"':
      lexing.mode = stringContent
      lexing.advanceAs(quoteDouble)
    of HSpaceChars: lexing.nextHSpace()
    of DigitChars: lexing.nextNumber()
    of VSpaceChars: lexing.nextVSpace()
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

proc next(lexing: var Lexing): Token {.raises: [].} =
  let begin = lexing.index
  if lexing.index >= lexing.text.len:
    return Token(kind: TokenKind.eof, text: lexing.lexer.pool.emptyId)
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
  Token(kind: finalKind, text: text)

proc lex(lexing: var Lexing): Tokens =
  let lexer = addr lexing.lexer
  lexer.tokens.setLen(0)
  while true:
    let next = lexing.next
    lexer.tokens.add(next)
    if next.kind == TokenKind.eof:
      break
  Tokens(tokens: lexer.tokens)

proc newLexer*(): Lexer =
  var pool = newPool[TextId]()
  Lexer(
    keys: {
      "as": keyAs,
      "be": keyBe,
      "end": keyEnd,
      "for": keyFor,
      "is": keyIs,
      "of": keyOf,
      "to": keyTo,
    }.map(
      func(pair: (string, TokenKind)): (TextId, TokenKind) =
        (pool.intern(pair[0]), pair[1])
    ).toTable,
    pool: pool,
  )

proc lex*(lexer: var Lexer, text: string): Tokens =
  var lexing = Lexing(index: 0, lexer: lexer, text: text)
  lexing.lex
