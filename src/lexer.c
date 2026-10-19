/* lexer.c - turning source text into tokens, and reporting compile errors. The rules for tokens:
   docs/spec/lexical.md. */
#include "compiler.h"
#include <stddef.h>
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
  if ((p >= e && c->text.read && !c->textDone) || p - start >= rio_TOKEN_MOST) rio_TOKEN_FAIL("number too long");
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

/* ------------------------------------------------------------------ reading a part at a time */

/* whether p is in the window (where a text read a part at a time is) */
static int rio_inWindow(const rio_Compiler *c, const char *p) {
  return c->text.read && p && p >= c->window && p <= c->window + c->windowSize;
}

/* bytes a token needs to have ahead of it: half the window (a name, a number, an operator) */
static uint32_t rio_aheadBytes(const rio_Compiler *c) { return c->windowSize / 2 < 256 ? c->windowSize / 2 : 256; }

/* More of a text read a part at a time, into its window: what's from keep on stays (and the tokens read,
   from where they start: they may still be wanted), moved down to the window's start, and more is read
   after it. How far it moved (each pointer into the window moves down by that), or -1 if no more can
   come (its end, or no room for it) */
static ptrdiff_t rio_refill(rio_Vm *vm, const char *keep) {
  rio_Compiler *c = vm->compiler;
  char *w = c->window;
  ptrdiff_t shift;
  uint32_t have;
  int n;
  if (!c->text.read || c->textDone || c->failed) return -1;
  if (rio_inWindow(c, c->tok.text) && c->tok.text < keep) keep = c->tok.text;
  if (rio_inWindow(c, c->peek.text) && c->peek.text < keep) keep = c->peek.text;
  shift = keep - w;
  have = (uint32_t)(c->srcEnd - keep);
  if (shift) {
    memmove(w, keep, have);
    c->srcAt += (uint32_t)shift;
    c->srcEnd -= shift;
    if (rio_inWindow(c, c->cursor)) c->cursor -= shift;
    if (rio_inWindow(c, c->tok.text)) c->tok.text -= shift;
    if (rio_inWindow(c, c->peek.text)) c->peek.text -= shift;
  }
  if (have >= c->windowSize) { /* (all of it wanted still: no room) */
    rio_failAt(vm, c->line, 0, 0, "source window too small (rio_setSourceWindow)");
    return -1;
  }
  n = c->text.read(c->text.readData, w + have, c->srcAt + have, c->windowSize - have);
  if (n < 0) {
    rio_failAt(vm, c->line, 0, 0, "can't read the source");
    c->textDone = 1;
    return -1;
  }
  if (n == 0) c->textDone = 1;
  c->srcEnd = w + have + n;
  return shift;
}

/* a token's text, out of the window (which moves on), into a copy of its own: one the other token
   isn't using */
static void rio_rescue(rio_Compiler *c, rio_Token *t, const rio_Token *other) {
  char *to = other->text == c->tokenCopy[0] ? c->tokenCopy[1] : c->tokenCopy[0];
  if (!rio_inWindow(c, t->text) || t->kind == rio_Tok_string) return;
  memcpy(to, t->text, (size_t)(t->len < rio_TOKEN_MOST ? t->len : rio_TOKEN_MOST));
  t->text = to;
}

/* more after p, moving p, s, start and e with the text: whether there's any */
static int rio_more(rio_Vm *vm, const char **p, const char **s, const char **start, const char **e, const char *keep) {
  ptrdiff_t d = rio_refill(vm, keep);
  if (d < 0) return 0;
  *p -= d;
  *s -= d;
  *start -= d;
  *e = vm->compiler->srcEnd;
  return *p < *e;
}

/* whether more of the text could come (it's read a part at a time, and its end hasn't been) */
static int rio_moreToCome(const rio_Compiler *c) { return c->text.read && !c->textDone; }

/* the window, read again from at, the cursor at cursorAt (at or after at) */
static void rio_reread(rio_Vm *vm, uint32_t at, uint32_t cursorAt) {
  rio_Compiler *c = vm->compiler;
  int n;
  rio_rescue(c, &c->tok, &c->peek); /* (the tokens read: copied out first, the window's text changing) */
  rio_rescue(c, &c->peek, &c->tok);
  n = c->text.read(c->text.readData, c->window, at, c->windowSize);
  if (n < 0) {
    rio_failAt(vm, c->line, 0, 0, "can't read the source");
    n = 0;
  }
  c->src = c->window;
  c->srcAt = at;
  c->srcEnd = c->window + n;
  c->textDone = 0;
  c->cursor = c->src + (cursorAt - at < (uint32_t)n ? cursorAt - at : (uint32_t)n);
}

const char *rio_lookAhead(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  uint32_t at = c->tok.at;
  if (!c->text.read) return c->src + (at - c->srcAt);
  if (at >= c->srcAt && at <= rio_at(c, c->srcEnd) && (c->textDone || (uint32_t)(rio_at(c, c->srcEnd) - at) >= rio_aheadBytes(c)))
    return c->src + (at - c->srcAt); /* (there, and enough after it) */
  rio_reread(vm, at, rio_at(c, c->cursor)); /* (else read again from there: the cursor's near after it) */
  return c->src;
}

/* the token at the cursor, into t */
void rio_lex(rio_Vm *vm, rio_Token *t) {
  rio_Compiler *c = vm->compiler;
  const char *p = c->cursor, *e = c->srcEnd, *s = p, *start = p;
  int newline = 0, i;
#define rio_MORE() (p < e || rio_more(vm, &p, &s, &start, &e, p))
  if (c->failed) {
    t->kind = rio_Tok_eof;
    t->len = 0;
    t->newline = 0;
    return;
  }
  if (c->text.read) { /* (the tokens read: copied out, so the window can move on) */
    rio_rescue(c, &c->tok, &c->peek);
    if (t != &c->peek) rio_rescue(c, &c->peek, &c->tok);
    t->text = ""; /* (what t was: done with, the token before, rio_next's to keep) */
    t->len = 0;
  }
  for (;;) { /* space and comments */
    while (rio_MORE() && rio_isSpace(*p)) {
      if (*p == '\n') {
        newline = 1;
        c->line++;
        c->lineAt = rio_at(c, p + 1);
      }
      p++;
    }
    if (p < e && *p == '#') { /* comments run to the end of the line */
      while (rio_MORE() && *p != '\n') p++;
      continue;
    }
    if (rio_moreToCome(c) && (uint32_t)(e - p) < rio_aheadBytes(c)) rio_more(vm, &p, &s, &start, &e, p); /* (a token's worth ahead) */
    if (p + 1 < e && p[0] == '/' && (p[1] == '/' || p[1] == '*')) { /* a habit from other languages */
      rio_failAt(vm, c->line, (int)(rio_at(c, p) - c->lineAt) + 1, 2, "comments start with #");
      t->kind = rio_Tok_eof;
      t->len = 0;
      return;
    }
    if (p < e && (*p == '#' || rio_isSpace(*p))) continue; /* (more of them, read now) */
    break;
  }
  t->newline = (uint8_t)newline;
  t->line = c->line;
  t->col = (int)(rio_at(c, p) - c->lineAt) + 1;
  t->at = rio_at(c, p);
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
    if ((p >= e && rio_moreToCome(c)) || p - s >= rio_TOKEN_MOST) rio_TOKEN_FAIL("name too long (255 characters at most)");
    t->kind = rio_Tok_id;
    for (i = 0; i < (int)(sizeof rio_keywords / sizeof rio_keywords[0]); i++)
      if ((int)strlen(rio_keywords[i]) == p - s && !memcmp(rio_keywords[i], s, p - s)) t->kind = rio_Tok_proc + i;
  } else if (rio_isDigit(*p)) {
    rio_lexNumberAt(vm, t, p, start);
    return;
  } else if (*p == '"') { /* the bytes between the quotes, escapes done, straight into the pool (as they're read) */
    uint8_t *out = c->pool + c->poolUsed;
    uint32_t room = c->poolCap - c->poolUsed, n = 0;
    t->text = ""; /* (its bytes: in the pool, so the window needn't keep it) */
    t->len = 0;
    for (p++; rio_MORE() && *p != '"'; p++) {
      int ch = (uint8_t)*p;
      if (ch == '\\') {
        p++;
        if (!rio_MORE()) break;
        if (!rio_isEscape(*p)) {
          rio_failAt(vm, c->line, (int)(rio_at(c, p - 1) - c->lineAt) + 1, 2, rio_msgUnknownEscape);
          t->kind = rio_Tok_eof;
          t->len = 0;
          return;
        }
        ch = rio_escapeChar(*p);
      } else if (ch == '\n') {
        c->line++;
        c->lineAt = rio_at(c, p + 1);
      }
      if (n >= room) {
        rio_failAt(vm, t->line, t->col, 1, "out of compiler memory");
        t->kind = rio_Tok_eof;
        t->len = 0;
        return;
      }
      out[n++] = (uint8_t)ch;
    }
    if (p >= e) {
      c->cursor = e;
      rio_LEX_FAIL(rio_msgUnterminatedString);
    }
    t->kind = rio_Tok_string;
    t->text = (const char *)out;
    t->len = (int)n;
    t->val.u = c->poolUsed;
    c->poolUsed += n;
    t->width = (int)(rio_at(c, p + 1) - t->at);
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
#undef rio_MORE
}

/* move on a token (noting, for a value standing for its field, the last name read: rio_punField) */
void rio_next(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  if (c->tok.kind == rio_Tok_id) {
    c->lastNameLen = c->tok.len < (int)sizeof c->lastName ? c->tok.len : -1;
    if (c->lastNameLen > 0) memcpy(c->lastName, c->tok.text, (size_t)c->lastNameLen);
  } else if (c->tok.kind != '.') c->pathBreaks++;
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
  uint32_t cursorAt = rio_at(c, c->cursor), lineAt = c->lineAt, poolUsed = c->poolUsed;
  int line = c->line;
  rio_Token t;
  rio_lex(vm, &t);
  if (cursorAt >= c->srcAt) c->cursor = c->src + (cursorAt - c->srcAt); /* (where it was: the text may have moved in its window) */
  else rio_reread(vm, cursorAt, cursorAt); /* (or moved on past it: read again from there) */
  c->lineAt = lineAt;
  c->line = line;
  c->poolUsed = poolUsed; /* (a string's bytes: lexed again, when it's read) */
  return t.kind;
}

/* reading the current text from at */
void rio_readFrom(rio_Vm *vm, uint32_t at, uint32_t lineAt, int line) {
  rio_Compiler *c = vm->compiler;
  c->lineAt = lineAt;
  c->line = line;
  if (c->text.read) { /* a part at a time: the window, from there */
    if (!c->window) {
      rio_failAt(vm, 0, 0, 0, "no room for a window to read the source through");
      return;
    }
    if (rio_inWindow(c, c->tok.text)) c->tok.text = ""; /* (what's in the window now is the text before: done with) */
    if (rio_inWindow(c, c->peek.text)) c->peek.text = "";
    c->src = c->cursor = c->srcEnd = c->window;
    c->srcAt = at;
    c->textDone = 0;
    rio_refill(vm, c->window);
    return;
  }
  c->src = c->text.src;
  c->srcEnd = c->src + c->text.len;
  c->srcAt = 0;
  c->cursor = c->src + (at < c->text.len ? at : c->text.len);
}

const char *rio_keep(rio_Vm *vm, const char *s, int n) {
  rio_Compiler *c = vm->compiler;
  char *at;
  if (!c->window || s < c->tokenCopy[0] || (s >= c->keep && s < c->window) || s > c->window + c->windowSize) return s; /* (text that stays where it is: no copy) */
  at = c->keep + c->keepUsed;
  if (n < 0 || n > (int)(rio_KEEP_BYTES - c->keepUsed)) {
    rio_fail(vm, "too many names in one statement");
    return c->keep;
  }
  memcpy(at, s, (size_t)n);
  c->keepUsed += (uint32_t)n;
  return at;
}

/* Lexes text (a -D value, after any minus sign) as a number, into t. Returns whether all of it is one
   number; when it isn't, any error that made is undone, and nothing else changes. */
int rio_lexNumber(rio_Vm *vm, const char *text, rio_Token *t) {
  rio_Compiler *c = vm->compiler;
  const char *src = c->src, *cursor = c->cursor, *srcEnd = c->srcEnd, *failMessage = c->failMessage;
  uint32_t srcAt = c->srcAt, lineAt = c->lineAt;
  const char *errorFile = vm->errorFile;
  int line = c->line, negative = *text == '-', errorKind = vm->errorKind, ok;
  rio_Token tok = c->tok, peek = c->peek;
  rio_SourceFn read = c->text.read;
  char errorText[rio_ERROR_BUF_SIZE];
  if (c->failed) return 0;
  c->text.read = 0; /* (text in memory, whatever the file's read as) */
  memcpy(errorText, vm->errorText, sizeof errorText);
  c->src = c->cursor = text + negative;
  c->srcAt = 0;
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
  c->text.read = read;
  c->src = src;
  c->cursor = cursor;
  c->srcEnd = srcEnd;
  c->srcAt = srcAt;
  c->line = line;
  c->lineAt = lineAt;
  if (ok && negative) {
    if (t->kind == rio_Tok_int) t->val.u = 0u - t->val.u;
    else t->val.f = -t->val.f;
  }
  return ok;
}

#endif
