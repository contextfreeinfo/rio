/* fmt.c - rio_format: Rio code laid out the one way (rio fmt). Only spaces change: those between a line's
   tokens, and its indent (2 for each block it's in). Never a token, a line break, or a comment's words,
   so a program means the same after (rio_format checks that, token by token, before it gives the code
   back). Apart from the compiler: it needs no vm, no memory but the caller's, and works on code that
   doesn't compile yet (an editor's, as it's saved).

   Indents come from a line's own words: if, for and switch open a block (without do); so does a proc
   (name :: proc, static proc or callback proc), a struct, enum or union, an if or switch giving a value
   (x := if ...), and a line ending in do; end closes one, and else and case sit back a step. A line
   going on (inside brackets, a string's next line, or after an operator or a comma at the end of the one
   before) keeps its place relative to its statement's start, wherever that moves, but at least 2 in
   from it (unless it closes a bracket). Spaces between tokens follow rio_fmtSpace: - and & are binary
   after an operand, else unary; * before ::, := and the like marks an export. Lined up stays lined up:
   lines whose ::, := or = sat in one column (with a run of spaces before one) keep it, wider if a line
   needs; a comment after code keeps its column, and one under it stays under it. A line whose tokens
   would run together, spaced (- >= as ->=), stays as written. */
#include <string.h>
#include "rio.h"

enum { rio_Fmt_comment = 1, rio_Fmt_string, rio_Fmt_char, rio_Fmt_number, rio_Fmt_name, rio_Fmt_op, rio_Fmt_other };

typedef struct {
  const char *s;
  int len, kind, gap; /* gap: the spaces and tabs before it */
  int open;           /* a string that goes on to the next line (no closing quote on this one) */
} rio_FmtTok;

static const char *const rio_fmtOps[] = {">>>=", "..<", "..=", ">>>", "<<=", ">>=", "::", ":=", "==", "!=", "<=", ">=",
  "&&", "||", "<<", ">>", "->", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "//", "/*", 0}; /* (as rio_lex reads
                                                                                    them; the last two it refuses) */
static const char *const rio_fmtBinary[] = {"+", "-", "*", "/", "%", "&", "|", "^", "<<", ">>", ">>>", "==", "!=", "<",
  ">", "<=", ">=", "=", ":=", "::", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", ">>>=", "->", "and",
  "or", 0};
static const char *const rio_fmtSpaced[] = {"return", "do", "in", "not", "and", "or", "if", "for", "switch", "case",
  "else", "by", "is", "break", "continue", "import", 0}; /* (words, not operands: a space after; end is an operand too, xs[end]) */

static int rio_fmtAlpha(int c) { return ((c | 32) >= 'a' && (c | 32) <= 'z') || c == '_'; }
static int rio_fmtDigit(int c) { return c >= '0' && c <= '9'; }
static int rio_fmtIs(const rio_FmtTok *t, const char *s) { return t->len == (int)strlen(s) && !memcmp(t->s, s, (size_t)t->len); }
static int rio_fmtIn(const rio_FmtTok *t, const char *const *set) {
  for (; *set; set++)
    if (rio_fmtIs(t, *set)) return 1;
  return 0;
}
static int rio_fmtWord(const rio_FmtTok *t) { return t->kind == rio_Fmt_name || t->kind == rio_Fmt_op; }

/* the token at p, before e (a line's end: its spaces left off), its spaces first; returns past it, or
   with t->kind 0 at e */
static const char *rio_fmtToken(const char *p, const char *e, rio_FmtTok *t) {
  const char *s;
  int i;
  t->gap = t->open = 0;
  while (p < e && (*p == ' ' || *p == '\t')) p++, t->gap++;
  t->s = s = p;
  t->kind = 0;
  if (p >= e) {
    t->len = 0;
    return p;
  }
  if (*p == '#') {
    t->kind = rio_Fmt_comment;
    p = e;
  } else if (*p == '"') { /* (to its closing quote, or the line's end) */
    t->kind = rio_Fmt_string;
    for (p++; p < e && *p != '"'; p++)
      if (*p == '\\' && p + 1 < e) p++;
    if (p < e) p++;
    else t->open = 1;
  } else if (*p == '\'') { /* 'a', '\n', '\u{1F600}': else just the quote */
    const char *q = p + 1;
    if (q < e && *q == '\\') q += 2;
    else q++;
    while (q < e && *q != '\'') q++;
    t->kind = q < e && q - p >= 2 ? rio_Fmt_char : rio_Fmt_other;
    p = t->kind == rio_Fmt_char ? q + 1 : p + 1;
  } else if (rio_fmtDigit(*p)) { /* (bad ones too, 12px, as one: rio_lex would stop at them) */
    int decimal = !(*p == '0' && p + 1 < e && ((p[1] | 32) == 'x' || (p[1] | 32) == 'b')), dot = 0;
    t->kind = rio_Fmt_number;
    for (p++; p < e; p++) {
      if (rio_fmtAlpha(*p) || rio_fmtDigit(*p)) continue;
      if (decimal && !dot && *p == '.' && p + 1 < e && rio_fmtDigit(p[1])) {
        dot = 1;
        continue;
      }
      if (decimal && (*p == '+' || *p == '-') && (p[-1] | 32) == 'e' && p + 1 < e && rio_fmtDigit(p[1])) continue;
      break;
    }
  } else if (rio_fmtAlpha(*p)) {
    t->kind = rio_Fmt_name;
    while (p < e && (rio_fmtAlpha(*p) || rio_fmtDigit(*p))) p++;
  } else if ((uint8_t)*p >= 0x80) { /* (a character outside a string or comment, UTF-8: kept whole) */
    t->kind = rio_Fmt_other;
    for (p++; p < e && ((uint8_t)*p & 0xC0) == 0x80; p++) {}
  } else {
    t->kind = *p > ' ' ? rio_Fmt_op : rio_Fmt_other;
    for (i = 0; t->kind == rio_Fmt_op && rio_fmtOps[i]; i++) {
      size_t n = strlen(rio_fmtOps[i]);
      if ((size_t)(e - p) >= n && !memcmp(p, rio_fmtOps[i], n)) {
        p += n - 1;
        break;
      }
    }
    p++;
  }
  t->len = (int)(p - s);
  return p;
}

/* an operand ends at t: a - or & after it is binary, else unary (-x, &x, &T) */
static int rio_fmtOperand(const rio_FmtTok *t) {
  return (t->kind == rio_Fmt_name && !rio_fmtIn(t, rio_fmtSpaced)) || t->kind == rio_Fmt_number || t->kind == rio_Fmt_string ||
         t->kind == rio_Fmt_char || rio_fmtIs(t, ")") || rio_fmtIs(t, "]") || rio_fmtIs(t, "}");
}

/* t, a *, marks an export (name* :: ..., import .x*), not a product: next is what follows it (kind 0
   at the line's end, where a * with a gap before it is a product going on to the next line) */
static int rio_fmtExport(const rio_FmtTok *t, const rio_FmtTok *next) {
  return rio_fmtIs(t, "*") && (next->kind == 0 || next->kind == rio_Fmt_comment ? !t->gap :
    rio_fmtIs(next, "::") || rio_fmtIs(next, ":=") || rio_fmtIs(next, "=") || rio_fmtIs(next, ":") ||
    rio_fmtIs(next, ",") || rio_fmtIs(next, ")") || rio_fmtIs(next, "}"));
}

/* the spaces between tokens a and b (next: the one after b): 0 or 1, or -1 for as written (a gap or
   none). unary: a is a unary operator (-x, &x, ~x) */
static int rio_fmtSpace(const rio_FmtTok *a, const rio_FmtTok *b, const rio_FmtTok *next, int unary) {
  if (rio_fmtIs(b, "*") && (next->kind == 0 || next->kind == rio_Fmt_comment)) return b->gap > 0; /* (what it is depends on the gap) */
  if (rio_fmtIs(b, ",") || rio_fmtIs(b, ")") || rio_fmtIs(b, "]") || rio_fmtIs(b, "}") || rio_fmtIs(a, "(") ||
      rio_fmtIs(a, "[") || rio_fmtIs(a, "{") || rio_fmtIs(a, ".") || rio_fmtIs(a, "..<") || rio_fmtIs(a, "..=") ||
      rio_fmtIs(b, "..<") || rio_fmtIs(b, "..=") || rio_fmtIs(b, ":") || rio_fmtIs(b, ";") || unary || rio_fmtIs(a, "?"))
    return 0;
  if (rio_fmtIs(b, ".")) /* x.y, f().y; 1 .5 as written (1.5 is a Float); an implied .north after a space */
    return a->kind == rio_Fmt_number && next->kind == rio_Fmt_number ? -1 : !rio_fmtOperand(a);
  if (rio_fmtIs(a, ":") || rio_fmtIs(a, ",")) return 1;
  if (rio_fmtOperand(a) && (rio_fmtIs(b, "(") || rio_fmtIs(b, "[") || rio_fmtIs(b, "{") || rio_fmtExport(b, next) ||
      (rio_fmtIs(b, "?") && a->kind == rio_Fmt_name)))
    return 0; /* a call, an index, T{...}, [N]T, name*, name?: T */
  if (rio_fmtIs(a, "]") && (b->kind == rio_Fmt_name || rio_fmtIs(b, "?"))) return rio_fmtIn(b, rio_fmtSpaced); /* [N]T, []?T */
  return 1;
}
/* a line's effect on the blocks it's in: its own step back (-1 for end, else and case), and the
   change after it */
static void rio_fmtBlocks(const char *p, const char *e, int *back, int *after) {
  rio_FmtTok t, first, second, prev = {0}, last = {0};
  int n = 0, hasDo = 0, opens = 0, label = 0, sawIf = 0, sawSwitch = 0, afterColons = -1;
  *back = *after = 0;
  for (;;) {
    p = rio_fmtToken(p, e, &t);
    if (!t.kind) break;
    if (!rio_fmtWord(&t)) continue;
    if (n == 0) first = t;
    if (n == 1) second = t;
    if (n == 2 && rio_fmtIs(&second, ":") && (rio_fmtIs(&t, "if") || rio_fmtIs(&t, "for") || rio_fmtIs(&t, "switch")))
      label = 1; /* outer: for ... */
    if (rio_fmtIs(&t, "do")) hasDo = 1;
    if (afterColons == 0 && (rio_fmtIs(&t, "struct") || rio_fmtIs(&t, "enum") || rio_fmtIs(&t, "union"))) opens = 1;
    if (afterColons >= 0) { /* name :: proc, :: static proc, :: callback proc */
      if (rio_fmtIs(&t, "proc")) opens = 1;
      afterColons = rio_fmtIs(&t, "static") || rio_fmtIs(&t, "callback") ? afterColons + 1 : -2;
    }
    if (afterColons == -1 && rio_fmtIs(&t, "::")) afterColons = 0;
    if (n >= 1 && ((rio_fmtIs(&t, "if") && !sawIf++) || (rio_fmtIs(&t, "switch") && !sawSwitch++)) &&
        (rio_fmtIs(&prev, ":=") || rio_fmtIs(&prev, "=") || rio_fmtIs(&prev, "return")))
      opens = 1; /* x := if ..., return switch ... */
    prev = last = t;
    n++;
  }
  if (!n) return;
  if (rio_fmtIs(&first, "end")) {
    *back = *after = -1;
    return;
  }
  if (rio_fmtIs(&first, "else") || rio_fmtIs(&first, "case")) {
    *back = -1;
    return;
  }
  if (label || rio_fmtIs(&first, "if") || rio_fmtIs(&first, "for") || rio_fmtIs(&first, "switch")) opens = 1;
  if (hasDo) opens = 0; /* (one line: if a do b) */
  if (rio_fmtIs(&last, "do")) opens = 1;
  *after = opens;
}

typedef struct {
  char *out;
  uint32_t len, cap;
  int col; /* the line's column so far (UTF-8: a character each) */
} rio_FmtOut;

static void rio_fmtPut(rio_FmtOut *o, const char *s, int n) {
  int i;
  for (i = 0; i < n; i++) {
    if (o->len < o->cap) o->out[o->len] = s[i];
    o->len++;
    o->col += ((uint8_t)s[i] & 0xC0) != 0x80;
  }
}
static void rio_fmtSpaces(rio_FmtOut *o, int n) {
  for (; n > 0; n--) rio_fmtPut(o, " ", 1);
}

/* the columns from s to e (UTF-8: a character each) */
static int rio_fmtColumns(const char *s, const char *e) {
  int n = 0;
  for (; s < e; s++) n += ((uint8_t)*s & 0xC0) != 0x80;
  return n;
}

/* a line's end: e (its \n or the end), and how far its text runs, its spaces (and a \r) left off */
static const char *rio_fmtLineEnd(const char *p, const char *end, const char **e) {
  const char *q = memchr(p, '\n', (size_t)(end - p));
  *e = q ? q : end;
  q = *e;
  while (q > p && (q[-1] == ' ' || q[-1] == '\t' || q[-1] == '\r')) q--;
  return q;
}

static int rio_fmtIndent(const char *p, const char *e) {
  int n = 0;
  while (p < e && *p == ' ') p++, n++;
  return n;
}

/* t is the ::, := or = that lines up with the lines around it (the line's first, outside brackets) */
static int rio_fmtAligns(const rio_FmtTok *t, int brackets) {
  return !brackets && (rio_fmtIs(t, "::") || rio_fmtIs(t, ":=") || rio_fmtIs(t, "="));
}

/* what lines a line up with the ones around it: the column of its first ::, := or = (from the indent:
   col), and which; -1: none */
static int rio_fmtAlignKey(const char *p, const char *e, int *col) {
  const char *s;
  int brackets = 0, indent = rio_fmtIndent(p, e);
  rio_FmtTok t;
  for (s = p += indent;;) {
    p = rio_fmtToken(p, e, &t);
    if (!t.kind || t.kind == rio_Fmt_comment) return -1;
    if (rio_fmtAligns(&t, brackets)) {
      *col = rio_fmtColumns(s, t.s);
      return (*col & 0xFFFFFF) << 2 | (t.len == 1 ? 1 : t.s[1] == ':' ? 2 : 3);
    }
    if (t.kind == rio_Fmt_op && t.len == 1 && strchr("([{", *t.s)) brackets++;
    if (t.kind == rio_Fmt_op && t.len == 1 && strchr(")]}", *t.s)) brackets--;
  }
}

/* a line's code (line: its start, oldIndent spaces before its first token; e: its end), spaced, into
   o, at the column it's at. alignTo: where the line's first ::, := or = goes, from the code's start (-1:
   just spaced); at, if not 0, gets where it went from the code's start (-1: it can't line up, tight
   against the token before it, or first), and it returns 1 if it had a run of spaces before it. A
   comment at the end keeps its column (from the indent) if it had more than a space before it, so
   comments lined up stay lined up: comment gets its column, before and after */
static int rio_fmtCode(rio_FmtOut *o, const char *line, const char *e, int oldIndent, int alignTo, int *at,
                       int comment[2]) {
  rio_FmtTok t, next, prev = {0};
  int start = o->col, unary = 0, brackets = 0, first = 1, aligned = 0, run = 0;
  const char *q = rio_fmtToken(line, e, &t);
  for (; t.kind; first = 0) {
    const char *after = rio_fmtToken(q, e, &next);
    int space = 0;
    if (!first && t.kind == rio_Fmt_comment) {
      space = t.gap > 1 ? rio_fmtColumns(line, t.s) - oldIndent + start - o->col : t.gap;
      space = t.gap > 1 && space < 1 ? 1 : space;
    } else if (!first) {
      space = rio_fmtSpace(&prev, &t, &next, unary);
      space = space < 0 ? t.gap > 0 : space;
    }
    if (!aligned && t.kind != rio_Fmt_comment && rio_fmtAligns(&t, brackets)) { /* (first: with nothing to line up) */
      aligned = 1;
      if (alignTo >= 0 && space > 0) space = start + alignTo - o->col < 1 ? 1 : start + alignTo - o->col;
      if (at) *at = first || !space ? -1 : o->col - start + space;
      run = t.gap > 1;
    }
    rio_fmtSpaces(o, space);
    if (!first && t.kind == rio_Fmt_comment) {
      comment[0] = rio_fmtColumns(line, t.s);
      comment[1] = o->col;
    }
    rio_fmtPut(o, t.s, t.len);
    if (t.kind == rio_Fmt_comment) break;
    unary = (rio_fmtIs(&t, "-") || rio_fmtIs(&t, "&") || rio_fmtIs(&t, "~") || rio_fmtIs(&t, "!")) && (first || !rio_fmtOperand(&prev));
    if (t.kind == rio_Fmt_op && t.len == 1 && strchr("([{", *t.s)) brackets++;
    if (t.kind == rio_Fmt_op && t.len == 1 && strchr(")]}", *t.s)) brackets--;
    prev = t;
    t = next;
    q = after;
  }
  return run;
}

/* lines a and b (to ae and be) have the same tokens */
static int rio_fmtSameLine(const char *a, const char *ae, const char *b, const char *be) {
  rio_FmtTok s, t;
  do {
    a = rio_fmtToken(a, ae, &s);
    b = rio_fmtToken(b, be, &t);
    if (s.kind != t.kind || s.len != t.len || memcmp(s.s, t.s, (size_t)s.len)) return 0;
  } while (s.kind);
  return 1;
}

/* where a line's first ::, := or = goes, from the code's start, laid out on its own (-1: it can't line
   up: as rio_fmtCode's at, or the line too long to try, or kept as written); run: it had a run of
   spaces before it */
static int rio_fmtMeasure(const char *r, const char *re, int *run) {
  char buf[256];
  rio_FmtOut m = {buf, 0, sizeof buf, 0};
  int at = -1, comment[2];
  *run = rio_fmtCode(&m, r, re, rio_fmtIndent(r, re), -1, &at, comment);
  return m.len <= m.cap && rio_fmtSameLine(r, re, buf, buf + m.len) ? at : -1;
}

/* the code's tokens, line by line, are the same: rio_format changed only spaces */
static int rio_fmtSame(const char *a, const char *aEnd, const char *b, const char *bEnd) {
  for (;;) {
    const char *ae, *be, *aText = rio_fmtLineEnd(a, aEnd, &ae), *bText = rio_fmtLineEnd(b, bEnd, &be);
    if (!rio_fmtSameLine(a, aText, b, bText)) return 0;
    if ((ae == aEnd) != (be == bEnd)) return 0;
    if (ae == aEnd) return 1;
    a = ae + 1;
    b = be + 1;
  }
}

int32_t rio_format(const char *src, uint32_t len, char *out, uint32_t cap) {
  static const rio_FmtTok none = {0};
  const char *p = src, *end = src + len;
  rio_FmtOut o = {out, 0, cap, 0};
  int depth = 0, brackets = 0, stmtOld = 0, stmtNew = 0, goesOn = 0, inString = 0, comment[2] = {-1, -1};
  int blockKey = -1, blockTo = -1;
  const char *blockEnd = src; /* (lines lined up: to here, each its ::, := or = at blockTo, from its code's start) */
  for (;;) {
    const char *eol, *e = rio_fmtLineEnd(p, end, &eol), *q;
    rio_FmtTok t, prev = {0};
    int oldIndent = rio_fmtIndent(p, e), indent = 0, key, col, alignTo = -1;
    uint32_t codeAt;
    rio_fmtToken(p, e, &t);
    o.col = 0;
    if (t.kind == rio_Fmt_comment && oldIndent == comment[0] && oldIndent > 0) {
      indent = comment[1]; /* (lined up under the comment ending the line before: it stays so) */
    } else if (brackets > 0 || goesOn || (inString && t.kind == rio_Fmt_string)) { /* (a string going on, too) */
      indent = oldIndent - stmtOld + stmtNew; /* (not left of its statement but to close a bracket) */
      if (indent <= stmtNew && !rio_fmtIs(&t, ")") && !rio_fmtIs(&t, "]") && !rio_fmtIs(&t, "}")) indent = stmtNew + 2;
      indent = indent < 0 ? 0 : indent;
      comment[0] = -1;
    } else {
      int back, after;
      rio_fmtBlocks(p, e, &back, &after);
      indent = 2 * (depth + back);
      stmtOld = oldIndent;
      stmtNew = indent = indent < 0 ? 0 : indent;
      depth = depth + after < 0 ? 0 : depth + after;
      comment[0] = -1;
    }
    if ((key = rio_fmtAlignKey(p, e, &col)) >= 0 && key == blockKey && p < blockEnd) alignTo = blockTo;
    else if (key >= 0) { /* lines lined up: from here on, each its ::, := or = in the same column (from its indent) */
      const char *r = p, *re, *reol;
      int n = 0, widest = 0, runs = 0, k, run, at;
      for (;;) {
        re = rio_fmtLineEnd(r, end, &reol);
        if (rio_fmtAlignKey(r, re, &k) != key || (at = rio_fmtMeasure(r, re, &run)) < 0) break;
        n++;
        runs |= run;
        widest = at > widest ? at : widest;
        if (reol >= end) {
          r = end;
          break;
        }
        r = reol + 1;
      }
      blockKey = key; /* (all where they were, unless one's too wide for it) */
      blockEnd = n ? r : p;
      blockTo = alignTo = n < 2 || !runs ? -1 : widest > col ? widest : col;
    }
    if (t.kind) rio_fmtSpaces(&o, indent);
    codeAt = o.len;
    rio_fmtCode(&o, p, e, oldIndent, alignTo, 0, comment);
    if (o.len <= o.cap && !rio_fmtSameLine(p, e, out + codeAt, out + o.len)) { /* (tokens ran together: the line as written) */
      o.len = codeAt;
      o.col = indent;
      rio_fmtPut(&o, t.s, (int)(e - t.s));
      comment[0] = -1;
    }
    for (q = p;;) { /* the brackets left open, and how it ends: going on, or in a string */
      q = rio_fmtToken(q, e, &t);
      if (!t.kind || t.kind == rio_Fmt_comment) break;
      if (q - t.len == p + t.gap) t.gap = 0; /* (the first token's gap is the indent) */
      if (t.kind == rio_Fmt_op && t.len == 1 && strchr("([{", *t.s)) brackets++;
      if (t.kind == rio_Fmt_op && t.len == 1 && strchr(")]}", *t.s) && brackets > 0) brackets--;
      prev = t;
    }
    goesOn = prev.kind && !rio_fmtExport(&prev, &none) && (rio_fmtIn(&prev, rio_fmtBinary) || rio_fmtIs(&prev, ",") || rio_fmtIs(&prev, "not"));
    inString = prev.kind == rio_Fmt_string && prev.open;
    if (eol < end && eol > p && eol[-1] == '\r') rio_fmtPut(&o, "\r\n", 2);
    else if (eol < end) rio_fmtPut(&o, "\n", 1);
    if (eol >= end) break;
    p = eol + 1;
  }
  if (o.len >= o.cap) return -1;
  out[o.len] = 0;
  return rio_fmtSame(src, end, out, out + o.len) ? (int32_t)o.len : -2;
}
