/* lexer.c - turning source text into tokens, and reporting compile errors. The rules for tokens:
   docs/spec/lexical.md. */
#include "compiler.h"
#ifndef rio_NO_COMPILER

const char rio_msgMissingEnd[] = "missing 'end'";
const char rio_msgUnclosedBracket[] = "unclosed bracket";
const char rio_msgExpectedExpr[] = "expected expression";
const char rio_msgUnterminatedString[] = "unterminated string";

/* ------------------------------------------------------------------ errors */

/* after an error, every token is the end of the source */
static void rio_stopReading(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  c->failed = 1;
  c->tok.kind = c->peek.kind = rio_Tok_eof;
  c->tok.len = c->peek.len = 0;
}

/* names the file being read, when it isn't the main source */
static void rio_setErrorFile(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  if (c->curFile >= 2) vm->errorFile = c->names + c->modules[c->curFile].file;
}

/* an error with a position of its own (line 0: none) */
void rio_failAt(rio_Vm *vm, int line, int col, int width, const char *message) {
  rio_Compiler *c = vm->compiler;
  if (c->failed) return;
  c->failMessage = message;
  rio_setError(vm, rio_ErrorKind_compile, line, col, width, message);
  rio_setErrorFile(vm);
  rio_stopReading(vm);
}

/* an error at the current token, or at the end of the expression just finished when the current token
   is already on the next line */
void rio_fail(rio_Vm *vm, const char *message) {
  rio_Compiler *c = vm->compiler;
  rio_Token *t = &c->tok;
  int atPrev = t->newline || t->kind == rio_Tok_eof, n, i;
  if (c->failed) return; /* only the first error counts */
  c->failMessage = message;
  if (atPrev) n = rio_setError(vm, rio_ErrorKind_compile, c->prevLine, c->prevCol, c->prevWidth, message);
  else n = rio_setError(vm, rio_ErrorKind_compile, t->line, t->col, t->width, message);
  if (!atPrev && t->len > 0) { /* quote the token */
    char *out = vm->errorText;
    n = rio_appendText(out, n, " near '");
    for (i = 0; i < t->len && i < 24 && n < rio_ERROR_BUF_SIZE - 2; i++) out[n++] = t->text[i];
    n = rio_appendText(out, n, "'");
    out[n] = 0;
  }
  rio_setErrorFile(vm);
  rio_stopReading(vm);
}

/* an error about the current token itself, even when it starts a new line */
void rio_failAtToken(rio_Vm *vm, const char *message) {
  vm->compiler->tok.newline = 0;
  rio_fail(vm, message);
}

/* ------------------------------------------------------------------ tokens */

/* in the order of rio_Tok_proc on */
static const char *const rio_keywords[] = {
  "proc", "struct", "if", "else", "for", "in", "end", "return", "break", "continue", "import", "include",
  "enum", "union", "switch", "case", "is", "nil", "and", "or", "not",
};

/* two-character operators: the pairs, and their tokens (negative: c= assignment for operator c) */
static const char rio_operatorPairs[] = "::" ":=" "==" "!=" "<=" ">=" "&&" "||" "<<" ">>" "->" "+=" "-=" "*=" "/=" "%=" "&=" "|=" "^=";
static const short rio_operatorTokens[] = {
  rio_Tok_colonColon, rio_Tok_colonEq, rio_Tok_eqEq, rio_Tok_notEq, rio_Tok_lessEq, rio_Tok_greaterEq,
  rio_Tok_and, rio_Tok_or, rio_Tok_shiftLeft, rio_Tok_shiftRight, rio_Tok_arrow,
  -'+', -'-', -'*', -'/', -'%', -'&', -'|', -'^',
};

static int rio_digitValue(int c, int base) { /* -1: not a digit of this base */
  int v = rio_isDigit(c) ? c - '0' : (c | 32) >= 'a' && (c | 32) <= 'f' ? (c | 32) - 'a' + 10 : -1;
  return v < base ? v : -1;
}

/* the escapes (docs/spec/lexical.md): \n \t \r \0, and \\ \" \' for the character itself */
static const char rio_msgUnknownEscape[] = "unknown escape (these work: \\n \\t \\r \\0 \\\\ \\\" \\')";
static int rio_isEscape(int c) { return c && strchr("ntr0\\\"'", c) != 0; }
int rio_escapeChar(int c) { return c == 'n' ? '\n' : c == 't' ? '\t' : c == 'r' ? '\r' : c == '0' ? 0 : c; }

/* an error in the token being lexed: there, and the token reads as the end */
#define rio_LEX_FAIL(message)       \
  do {                              \
    rio_fail(vm, message);          \
    t->kind = rio_Tok_eof;          \
    t->len = 0;                     \
    return;                         \
  } while (0)
#define rio_TOKEN_FAIL(message)                                         \
  do {                                                                  \
    rio_failAt(vm, t->line, t->col, (int)(p - start), message);         \
    t->kind = rio_Tok_eof;                                              \
    t->len = 0;                                                         \
    return;                                                             \
  } while (0)

/* 123, 1_000, 0xFF, 0b1010, 1.5, 2e-3: an _ may go between any two digits. p is at the first digit. */
static void rio_lexNumberAt(rio_Vm *vm, rio_Token *t, const char *p, const char *start) {
  rio_Compiler *c = vm->compiler;
  const char *e = c->srcEnd;
  uint32_t v = 0;
  double d = 0, scale = 1;
  int isFloat = 0, base = 10, digits = 0, tooBig = 0, k;
  if (*p == '0' && p + 1 < e && ((p[1] | 32) == 'x' || (p[1] | 32) == 'b')) {
    base = (p[1] | 32) == 'x' ? 16 : 2;
    p += 2;
  }
  while (p < e && (k = rio_digitValue(*p, base)) >= 0) {
    if ((uint64_t)v * (uint32_t)base + (uint32_t)k > 0xFFFFFFFFu) tooBig = 1;
    v = v * (uint32_t)base + (uint32_t)k;
    d = d * 10 + k;
    digits++;
    p++;
    if (p + 1 < e && *p == '_' && rio_digitValue(p[1], base) >= 0) p++;
  }
  if (!digits) rio_TOKEN_FAIL(base == 16 ? "hex digits expected after 0x" : "binary digits expected after 0b");
  if (base == 10) {
    if (p + 1 < e && *p == '.' && rio_isDigit(p[1])) {
      for (isFloat = 1, p++; p < e && rio_isDigit(*p); p++) {
        d += (*p - '0') * (scale /= 10);
        if (p + 2 < e && p[1] == '_' && rio_isDigit(p[2])) p++;
      }
    }
    if (p < e && (*p == 'e' || *p == 'E')) {
      int negative = 0, x = 0;
      p++;
      if (p < e && (*p == '-' || *p == '+')) negative = *p++ == '-';
      if (p >= e || !rio_isDigit(*p)) rio_TOKEN_FAIL("exponent digits expected");
      while (p < e && rio_isDigit(*p)) { /* past 1000 it's inf or 0 anyway */
        if (x < 1000) x = x * 10 + (*p - '0');
        p++;
        if (p + 1 < e && *p == '_' && rio_isDigit(p[1])) p++;
      }
      while (x--) d = negative ? d / 10 : d * 10;
      isFloat = 1;
    }
  }
  if (p < e && (rio_isAlpha(*p) || rio_isDigit(*p))) { /* 1__0, 1_, 0b102, 0xFG, 12px */
    const char *bad = p;
    int underscore = 0;
    while (p < e && (rio_isAlpha(*p) || rio_isDigit(*p))) underscore |= *p++ == '_';
    rio_TOKEN_FAIL(underscore ? "_ goes between digits"
                   : base == 2 && rio_isDigit(*bad) ? "binary digits are 0 and 1"
                                                     : "bad number");
  }
  if (tooBig && !isFloat) rio_TOKEN_FAIL("number too big for Int (32 bits)");
  /* 2147483648 fits only after a minus sign (in a -D value, or right after a '-' token) */
  if (base == 10 && !isFloat && v > 0x7FFFFFFFu &&
      !(v == 0x80000000u && (c->lexingNegative || (t == &c->peek && c->tok.kind == '-'))))
    rio_TOKEN_FAIL("number too big for Int (the most is 2147483647; hex and binary can give any 32 bits)");
  t->kind = isFloat ? rio_Tok_float : rio_Tok_int;
  if (isFloat) t->val.f = (float)d;
  else t->val.u = v;
  t->len = t->width = (int)(p - start);
  c->cursor = p;
}

/* the token at the cursor, into t */
void rio_lex(rio_Vm *vm, rio_Token *t) {
  rio_Compiler *c = vm->compiler;
  const char *p = c->cursor, *e = c->srcEnd, *s, *start;
  int newline = 0, i;
  if (c->failed) {
    t->kind = rio_Tok_eof;
    t->len = 0;
    t->newline = 0;
    return;
  }
  for (;;) { /* space and comments */
    while (p < e && rio_isSpace(*p)) {
      if (*p == '\n') {
        newline = 1;
        c->line++;
        c->lineStart = p + 1;
      }
      p++;
    }
    if (p < e && *p == '#') { /* comments run to the end of the line */
      while (p < e && *p != '\n') p++;
      continue;
    }
    if (p + 1 < e && p[0] == '/' && (p[1] == '/' || p[1] == '*')) { /* a habit from other languages */
      rio_failAt(vm, c->line, (int)(p - c->lineStart) + 1, 2, "comments start with #");
      t->kind = rio_Tok_eof;
      t->len = 0;
      return;
    }
    break;
  }
  t->newline = (uint8_t)newline;
  t->line = c->line;
  t->col = (int)(p - c->lineStart) + 1;
  t->text = s = start = p;
  t->op = 0;
  t->val.i = 0;
  if (p >= e) {
    t->kind = rio_Tok_eof;
    t->len = t->width = 0;
    c->cursor = e;
    return;
  }
  if (rio_isAlpha(*p)) { /* a name or keyword */
    while (p < e && (rio_isAlpha(*p) || rio_isDigit(*p))) p++;
    t->kind = rio_Tok_id;
    for (i = 0; i < (int)(sizeof rio_keywords / sizeof rio_keywords[0]); i++)
      if ((int)strlen(rio_keywords[i]) == p - s && !memcmp(rio_keywords[i], s, p - s)) t->kind = rio_Tok_proc + i;
  } else if (rio_isDigit(*p)) {
    rio_lexNumberAt(vm, t, p, start);
    return;
  } else if (*p == '"') { /* the text is between the quotes, escapes and all */
    for (s = ++p; p < e && *p != '"'; p++) {
      if (*p == '\\' && ++p < e && !rio_isEscape(*p)) {
        rio_failAt(vm, c->line, (int)(p - 1 - c->lineStart) + 1, 2, rio_msgUnknownEscape);
        t->kind = rio_Tok_eof;
        t->len = 0;
        return;
      }
      if (p < e && *p == '\n') {
        c->line++;
        c->lineStart = p + 1;
      }
    }
    if (p >= e) {
      c->cursor = e;
      rio_LEX_FAIL(rio_msgUnterminatedString);
    }
    t->kind = rio_Tok_string;
    t->text = s;
    t->len = (int)(p - s);
    t->width = (int)(p + 1 - start);
    c->cursor = p + 1;
    return;
  } else if (*p == '\'') { /* a character: its code, as an Int */
    int ch = p + 1 < e ? p[1] : 0;
    p += 2;
    if (ch == '\\' && p < e) {
      if (!rio_isEscape(*p)) rio_TOKEN_FAIL(rio_msgUnknownEscape);
      ch = rio_escapeChar(*p++);
    }
    if (p >= e || *p != '\'') rio_LEX_FAIL("bad char literal");
    p++;
    t->kind = rio_Tok_int;
    t->val.i = ch;
  } else if (p + 2 < e && p[0] == '.' && p[1] == '.' && (p[2] == '<' || p[2] == '=')) {
    t->kind = p[2] == '<' ? rio_Tok_rangeExcl : rio_Tok_rangeIncl;
    p += 3;
  } else if (p + 2 < e && (p[0] == '<' || p[0] == '>') && p[1] == p[0] && p[2] == '=') { /* <<= >>= */
    t->kind = rio_Tok_opAssign;
    t->op = p[0] == '<' ? rio_Tok_shiftLeft : rio_Tok_shiftRight;
    p += 3;
  } else { /* one character, or a pair */
    t->kind = *p++;
    for (i = 0; p < e && i < (int)(sizeof rio_operatorTokens / sizeof rio_operatorTokens[0]); i++) {
      if (rio_operatorPairs[i * 2] == p[-1] && rio_operatorPairs[i * 2 + 1] == *p) {
        p++;
        if (rio_operatorTokens[i] < 0) {
          t->kind = rio_Tok_opAssign;
          t->op = -rio_operatorTokens[i];
        } else t->kind = rio_operatorTokens[i];
        break;
      }
    }
    if (t->kind == rio_Tok_and || t->kind == rio_Tok_or)
      rio_TOKEN_FAIL(t->kind == rio_Tok_and ? "write and instead of &&" : "write or instead of ||");
    if (t->kind == '!') rio_TOKEN_FAIL("write not instead of !");
  }
  t->len = t->width = (int)(p - s);
  c->cursor = p;
}

/* move on a token */
void rio_next(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  c->prevLine = c->tok.line;
  c->prevCol = c->tok.col;
  c->prevWidth = c->tok.width;
  c->tok = c->peek;
  rio_lex(vm, &c->peek);
}

void rio_expect(rio_Vm *vm, int kind, const char *message) {
  if (vm->compiler->tok.kind != kind) rio_FAIL(, message);
  rio_next(vm);
}

/* the token after the lookahead, without consuming anything */
int rio_peekAfterNext(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  const char *cursor = c->cursor, *lineStart = c->lineStart;
  int line = c->line;
  rio_Token t;
  rio_lex(vm, &t);
  c->cursor = cursor;
  c->lineStart = lineStart;
  c->line = line;
  return t.kind;
}

/* Lexes text (a -D value, after any minus sign) as a number, into t. Returns whether all of it is one
   number; when it isn't, any error that made is undone, and nothing else changes. */
int rio_lexNumber(rio_Vm *vm, const char *text, rio_Token *t) {
  rio_Compiler *c = vm->compiler;
  const char *cursor = c->cursor, *srcEnd = c->srcEnd, *lineStart = c->lineStart, *failMessage = c->failMessage;
  const char *errorFile = vm->errorFile;
  int line = c->line, negative = *text == '-', errorKind = vm->errorKind, ok;
  rio_Token tok = c->tok, peek = c->peek;
  char errorText[rio_ERROR_BUF_SIZE];
  if (c->failed) return 0;
  memcpy(errorText, vm->errorText, sizeof errorText);
  c->cursor = text + negative;
  c->srcEnd = text + strlen(text);
  c->lexingNegative = negative;
  rio_lex(vm, t);
  c->lexingNegative = 0;
  ok = !c->failed && (t->kind == rio_Tok_int || t->kind == rio_Tok_float) && c->cursor == c->srcEnd;
  if (c->failed) { /* not a number after all (12px, 1_): undo the error */
    c->failed = 0;
    c->tok = tok;
    c->peek = peek;
    vm->errorKind = errorKind;
    memcpy(vm->errorText, errorText, sizeof errorText);
    c->failMessage = failMessage;
    vm->errorFile = errorFile;
  }
  c->cursor = cursor;
  c->srcEnd = srcEnd;
  c->line = line;
  c->lineStart = lineStart;
  if (ok && negative) {
    if (t->kind == rio_Tok_int) t->val.u = 0u - t->val.u;
    else t->val.f = -t->val.f;
  }
  return ok;
}

#endif
