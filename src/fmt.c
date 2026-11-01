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
  int brk;            /* line breaks before it (rio_fmtItNext's: 0 within a line) */
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
  t->gap = t->open = t->brk = 0;
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
    rio_fmtIs(next, ",") || rio_fmtIs(next, ")") || rio_fmtIs(next, "]") || rio_fmtIs(next, "}"));
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
static const char *rio_fmtLineEnd(const char *p, const char *end, const char **e);

/* tokens over lines, p to end (rio_fmtItNext) */
typedef struct {
  const char *p, *text, *eol, *end;
  int brk; /* the line breaks before the next token */
} rio_FmtIt;

static void rio_fmtItAt(rio_FmtIt *it, const char *p, const char *end) {
  it->p = p;
  it->end = end;
  it->brk = 0;
  it->text = rio_fmtLineEnd(p, end, &it->eol);
}

/* the next token, with the line breaks before it in t->brk; kind 0 at the end */
static void rio_fmtItNext(rio_FmtIt *it, rio_FmtTok *t) {
  for (;;) {
    it->p = rio_fmtToken(it->p, it->text, t);
    t->brk = it->brk;
    if (t->kind || it->eol >= it->end) break;
    it->p = it->eol + 1;
    it->text = rio_fmtLineEnd(it->p, it->end, &it->eol);
    it->brk++;
  }
  if (t->kind) it->brk = 0;
}

/* a statement's effect (p to end, its lines) on the blocks it's in: its own step back (-1 for end, else
   and case), and the change after it */
static void rio_fmtBlocks(const char *p, const char *end, int *back, int *after) {
  rio_FmtIt it;
  rio_FmtTok t, first, second, prev = {0}, last = {0};
  int n = 0, hasDo = 0, opens = 0, label = 0, sawIf = 0, sawSwitch = 0, afterColons = -1;
  *back = *after = 0;
  rio_fmtItAt(&it, p, end);
  for (;;) {
    rio_fmtItNext(&it, &t);
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
static const char *rio_fmtStatement(const char *p, const char *end, int *brackets, int *goesOn, int *inString);
static int rio_fmtListy(const char *p, const char *end);
static int rio_fmtMeasure(const char *r, const char *re, const char *end, int *run) {
  char buf[256];
  rio_FmtOut m = {buf, 0, sizeof buf, 0};
  int at = -1, comment[2], b, g, s;
  const char *last = rio_fmtStatement(r, end, &b, &g, &s);
  if (!last || rio_fmtListy(r, last)) return -1; /* (its lists laid out over lines) */
  *run = rio_fmtCode(&m, r, re, rio_fmtIndent(r, re), -1, &at, comment);
  return m.len <= m.cap && rio_fmtSameLine(r, re, buf, buf + m.len) ? at : -1;
}

/* ------------------------------------------------------------------ lists over lines */

static int rio_fmtOpens(const rio_FmtTok *t) { return t->kind == rio_Fmt_op && t->len == 1 && strchr("([{", *t->s); }
static int rio_fmtCloses(const rio_FmtTok *t) { return t->kind == rio_Fmt_op && t->len == 1 && strchr(")]}", *t->s); }

/* a line break between a and b, inside brackets, that Rio reads as a space, so rio fmt may take it out
   or put one in: after a comma, an opening bracket or an operator, or before a closing bracket (never
   after a comment, in a string going on to the next line, or after a * that may be an export's) */
static int rio_fmtWraps(const rio_FmtTok *a, const rio_FmtTok *b) {
  return a->kind != rio_Fmt_comment && !a->open && !rio_fmtIs(a, "*") &&
         (rio_fmtIs(a, ",") || rio_fmtOpens(a) || rio_fmtCloses(b) || rio_fmtIn(a, rio_fmtBinary));
}

/* how a list in brackets, with commas, is laid out (none: as written, keeping its line breaks):
     f(a, b, c)       line: all on one line (its breaks taken out), with no trailing comma
     f(a, b, [        hug: the same, but its last item a list over lines
       ...
     ])
     f(               indented: a break after the opener, and no trailing comma: the items on one
       a, b, c        line of their own
     )
     f(               each: a trailing comma, the first item on the opener's line: an item a line
       a,
       b,
     )
     f(               rows: a trailing comma and a break after the opener: the lines as written
       a, b,
       c,
     )
   A list that can't be put on one line (a comment in it, a string going on over lines, a break where
   Rio might not read it as a space, a list over lines in it but last) goes each or rows, with a
   trailing comma added. Each and rows keep the breaks inside an item; rows keep blank lines too. */
enum { rio_Lay_written, rio_Lay_line, rio_Lay_hug, rio_Lay_indented, rio_Lay_each, rio_Lay_rows };

typedef struct {
  int form, comma;  /* comma: one is added after the token at last */
  const char *last; /* the list's last token (not a comment) */
  rio_FmtTok close;
} rio_FmtList;

/* how a list (it just past its opener, open) is laid out, into l; returns 1 if it's over lines, or -1
   for lists in lists too deep to follow (32). listy gets whether it's laid out other than as written:
   commas, and lines or a trailing comma. it ends past the closer, or at the end for a list that
   doesn't close (as written) */
static int rio_fmtScan(rio_FmtIt *it, const rio_FmtTok *open, rio_FmtList *l, int depth, int *listy) {
  rio_FmtTok t, prev = *open, code = *open;
  int commas = 0, first = -1, spans = 0, joins = 1, multis = 0, lastMulti = 0;
  l->form = rio_Lay_written;
  l->comma = 0;
  if (depth >= 32) return -1;
  for (;;) {
    rio_fmtItNext(it, &t);
    if (!t.kind) return 1;
    if (first < 0) first = t.brk > 0 || t.kind == rio_Fmt_comment;
    if (t.brk) {
      spans = 1;
      joins &= rio_fmtWraps(&prev, &t);
    }
    prev = t;
    if (t.kind == rio_Fmt_comment) {
      joins = 0;
      continue;
    }
    if (rio_fmtCloses(&t)) break;
    lastMulti = 0;
    if (rio_fmtOpens(&t)) {
      rio_FmtList inner;
      int m = rio_fmtScan(it, &t, &inner, depth + 1, listy);
      if (m < 0) return -1;
      multis += m;
      lastMulti = m;
      prev = code = inner.close;
      continue;
    }
    commas |= rio_fmtIs(&t, ",");
    joins &= !t.open;
    code = t;
  }
  l->close = t;
  l->last = code.s;
  if (!commas) return spans || multis;
  *listy |= spans || rio_fmtIs(&code, ",");
  if (rio_fmtIs(&code, ",")) l->form = first ? rio_Lay_rows : rio_Lay_each;
  else if (!joins || (multis && (multis > 1 || !lastMulti || first))) {
    l->form = first ? rio_Lay_rows : rio_Lay_each;
    l->comma = !code.open; /* (not after a string going on: it would be in it) */
  } else l->form = multis ? rio_Lay_hug : first ? rio_Lay_indented : rio_Lay_line;
  return l->form != rio_Lay_line;
}

/* whether a statement's code (p to end) is laid out other than line by line: a list with commas over
   lines, or with a trailing comma (and none too deep to follow) */
static int rio_fmtListy(const char *p, const char *end) {
  rio_FmtIt it;
  rio_FmtTok t;
  rio_FmtList l;
  int listy = 0;
  rio_fmtItAt(&it, p, end);
  for (;;) {
    rio_fmtItNext(&it, &t);
    if (!t.kind) return listy;
    if (rio_fmtOpens(&t) && rio_fmtScan(&it, &t, &l, 0, &listy) < 0) return 0;
  }
}

typedef struct {
  rio_FmtOut *o;
  rio_FmtTok prev;
  int unary, lineIndent, srcIndent, atStart, force, crlf, added; /* srcIndent: the indent of the line read from;
                                                                    force: a break must come next; added: a
                                                                    comma, just put in */
} rio_FmtLay;

/* t, after brk (-1: none; else a line break, blanks blank lines, and brk's indent), or else a space */
static void rio_fmtLayPut(rio_FmtLay *L, const rio_FmtTok *t, const rio_FmtTok *next, int brk, int blanks) {
  if (brk >= 0) {
    for (blanks = blanks < 0 ? 0 : blanks; blanks >= 0; blanks--) rio_fmtPut(L->o, L->crlf ? "\r\n" : "\n", L->crlf ? 2 : 1);
    L->o->col = 0;
    rio_fmtSpaces(L->o, brk);
    L->lineIndent = brk;
  } else if (!L->atStart) {
    static const rio_FmtTok none = {0};
    int space = t->kind != rio_Fmt_comment ? rio_fmtSpace(&L->prev, t, next->brk ? &none : next, L->unary) :
                t->gap - (L->added && t->gap > 1); /* (a comment after an added comma: where it was) */
    rio_fmtSpaces(L->o, space < 0 ? t->gap > 0 : space);
  }
  rio_fmtPut(L->o, t->s, t->len);
  if (t->brk) L->srcIndent = t->gap;
  L->unary = (rio_fmtIs(t, "-") || rio_fmtIs(t, "&") || rio_fmtIs(t, "~") || rio_fmtIs(t, "!")) &&
             (brk >= 0 || L->atStart || !rio_fmtOperand(&L->prev)); /* (first on a line: as rio_fmtCode has it) */
  L->prev = *t;
  L->atStart = 0;
  L->force = t->kind == rio_Fmt_comment || t->open;
  L->added = 0;
}

/* a list's tokens, its opener (open) just put, through its closer, laid out as rio_fmtScan says; with
   no opener, a statement's, as written, to its end. Lines kept as written go where they were from base
   and old (the new and old indents of the statement's start, or of the line of the list over lines
   they're in), as line by line (rio_format), but not left of base (unless closing a bracket) */
static void rio_fmtLayList(rio_FmtLay *L, rio_FmtIt *it, const rio_FmtTok *open, int depth, int base, int old) {
  static const rio_FmtTok comma = {",", 1, rio_Fmt_op, 0, 0, 0};
  rio_FmtList l;
  rio_FmtIt look = *it;
  rio_FmtTok t, next;
  int items = L->lineIndent + 2, closer = L->lineIndent, first = 1, boundary = 1, listy = 0;
  l.form = rio_Lay_written;
  l.comma = 0;
  l.last = 0;
  if (open) rio_fmtScan(&look, open, &l, depth, &listy);
  if (l.form != rio_Lay_written) { /* (its own lines: lists in it go from it) */
    base = L->lineIndent;
    old = L->srcIndent;
  }
  for (;; first = 0) {
    int brk = -1, blanks = 0, written; /* (written: where it was, from its statement's or list's start) */
    rio_fmtItNext(it, &t);
    if (!t.kind) return;
    look = *it;
    rio_fmtItNext(&look, &next);
    written = t.gap - old + base;
    if (open && rio_fmtCloses(&t)) {
      if (l.form == rio_Lay_written) brk = t.brk ? (written < 0 ? 0 : written) : -1;
      else if (l.form >= rio_Lay_indented || L->force) brk = closer;
      rio_fmtLayPut(L, &t, &next, brk, 0);
      return;
    }
    written = written <= base ? base + 2 : written;
    switch (l.form) {
    case rio_Lay_written:
      brk = t.brk ? written : L->force ? written : -1;
      blanks = t.brk - 1;
      break;
    case rio_Lay_indented:
      brk = first ? items : -1;
      break;
    case rio_Lay_each:
      if (L->prev.open && t.kind == rio_Fmt_string) brk = L->lineIndent; /* (a string's next line: under its first) */
      else if (first || (boundary && t.kind != rio_Fmt_comment) || (t.brk && t.kind == rio_Fmt_comment)) brk = items;
      else if (t.brk || L->force) brk = items + 2;
      break;
    case rio_Lay_rows:
      if (L->prev.open && t.kind == rio_Fmt_string) brk = L->lineIndent;
      else if (first || (t.brk && (boundary || t.kind == rio_Fmt_comment))) brk = items;
      else if (t.brk || L->force) brk = boundary ? items : items + 2;
      blanks = first ? 0 : t.brk - 1;
      break;
    }
    rio_fmtLayPut(L, &t, l.comma && t.s == l.last ? &comma : &next, brk, blanks); /* (spaced as it will be followed) */
    if (rio_fmtOpens(&t)) rio_fmtLayList(L, it, &t, depth + 1, base, old);
    if (l.comma && L->prev.s == l.last) {
      rio_fmtLayPut(L, &comma, &next, -1, 0);
      L->added = 1;
    }
    if (t.kind != rio_Fmt_comment) boundary = rio_fmtIs(&t, ",") && !rio_fmtOpens(&t);
  }
}

/* the code's tokens are the same, but for commas added before closing brackets, and line breaks only
   put in or taken out inside brackets where Rio reads them as spaces (rio_fmtWraps) */
static int rio_fmtSameCode(const char *a, const char *aEnd, const char *b, const char *bEnd) {
  rio_FmtIt ia, ib;
  rio_FmtTok s, t, before = {0}, code = {0};
  int depth = 0;
  rio_fmtItAt(&ia, a, aEnd);
  rio_fmtItAt(&ib, b, bEnd);
  for (;;) {
    rio_fmtItNext(&ia, &s);
    rio_fmtItNext(&ib, &t);
    if (rio_fmtIs(&t, ",") && !rio_fmtIs(&s, ",")) { /* (added: right after the list's last token) */
      rio_FmtIt la = ia;
      rio_FmtTok u = s;
      while (u.kind == rio_Fmt_comment) rio_fmtItNext(&la, &u);
      if (!depth || t.brk || !rio_fmtCloses(&u) || rio_fmtIs(&code, ",") || rio_fmtOpens(&code)) return 0;
      rio_fmtItNext(&ib, &t);
    }
    if (s.kind != t.kind || s.len != t.len || memcmp(s.s, t.s, (size_t)s.len)) return 0;
    if (!s.kind) return 1;
    if ((s.brk > 0) != (t.brk > 0) && (!depth || !rio_fmtWraps(&before, &s))) return 0;
    if (rio_fmtOpens(&s)) depth++;
    if (rio_fmtCloses(&s) && depth > 0) depth--;
    before = s;
    if (s.kind != rio_Fmt_comment) code = s;
  }
}

/* the line, p to e, leaves brackets open (counted on from *brackets), or ends going on: after an
   operator or comma (not an export's *), or in a string going on to the next line */
static void rio_fmtLineState(const char *p, const char *e, int *brackets, int *goesOn, int *inString) {
  static const rio_FmtTok none = {0};
  rio_FmtTok t, prev = {0};
  const char *q;
  for (q = p;;) {
    q = rio_fmtToken(q, e, &t);
    if (!t.kind || t.kind == rio_Fmt_comment) break;
    if (q - t.len == p + t.gap) t.gap = 0; /* (the first token's gap is the indent) */
    if (rio_fmtOpens(&t)) ++*brackets;
    if (rio_fmtCloses(&t) && *brackets > 0) --*brackets;
    prev = t;
  }
  *goesOn = prev.kind && !rio_fmtExport(&prev, &none) && (rio_fmtIn(&prev, rio_fmtBinary) || rio_fmtIs(&prev, ",") || rio_fmtIs(&prev, "not"));
  *inString = prev.kind == rio_Fmt_string && prev.open;
}

/* a comment line marking code rio fmt leaves alone: 1 for # nofmt, -1 for # /nofmt (after a space,
   anything: a note), else 0 */
static int rio_fmtNofmt(const rio_FmtTok *t) {
  int n = t->kind != rio_Fmt_comment ? 0 : t->len >= 8 && !memcmp(t->s, "# /nofmt", 8) ? 8 : t->len >= 7 && !memcmp(t->s, "# nofmt", 7) ? 7 : 0;
  if (!n || (t->len > n && t->s[n] != ' ' && t->s[n] != '\t')) return 0;
  return n == 7 ? 1 : -1;
}

/* where a statement starting at p ends (its last line's end), its lines going on (in brackets, after
   an operator, or a string's), with the state after it; 0 if it has a # nofmt line in it */
static const char *rio_fmtStatement(const char *p, const char *end, int *brackets, int *goesOn, int *inString) {
  const char *eol, *e = rio_fmtLineEnd(p, end, &eol);
  *brackets = 0;
  rio_fmtLineState(p, e, brackets, goesOn, inString);
  while (eol < end && (*brackets > 0 || *goesOn || *inString)) {
    const char *next = eol + 1, *nextEol;
    rio_FmtTok t;
    e = rio_fmtLineEnd(next, end, &nextEol);
    rio_fmtToken(next, e, &t);
    if (rio_fmtNofmt(&t)) return 0;
    if (!*brackets && ((!*goesOn && t.kind != rio_Fmt_string) || !t.kind || t.kind == rio_Fmt_comment))
      break; /* (a string not going on after all; or a blank line, or a comment's, after an operator) */
    rio_fmtLineState(next, e, brackets, goesOn, inString);
    eol = nextEol;
  }
  return eol;
}

int32_t rio_format(const char *src, uint32_t len, char *out, uint32_t cap) {
  const char *p = src, *end = src + len;
  rio_FmtOut o = {out, 0, cap, 0};
  int depth = 0, brackets = 0, stmtOld = 0, stmtNew = 0, goesOn = 0, inString = 0, comment[2] = {-1, -1};
  int blockKey = -1, blockTo = -1, nofmt = 0, crlf;
  const char *nl = memchr(src, '\n', len);
  const char *blockEnd = src; /* (lines lined up: to here, each its ::, := or = at blockTo, from its code's start) */
  crlf = nl && nl > src && nl[-1] == '\r'; /* (every line break as the file's first: \r\n or \n) */
  for (;;) {
    const char *eol, *e = rio_fmtLineEnd(p, end, &eol);
    rio_FmtTok t;
    int oldIndent = rio_fmtIndent(p, e), indent = 0, key, col, alignTo = -1, marker, laid = 0;
    uint32_t lineAt = o.len, codeAt;
    rio_fmtToken(p, e, &t);
    marker = rio_fmtNofmt(&t);
    o.col = 0;
    if (t.kind == rio_Fmt_comment && oldIndent == comment[0] && oldIndent > 0 && !marker) {
      indent = comment[1]; /* (lined up under the comment ending the line before: it stays so) */
    } else if (brackets > 0 || goesOn || (inString && t.kind == rio_Fmt_string)) { /* (a string going on, too) */
      indent = oldIndent - stmtOld + stmtNew; /* (not left of its statement but to close a bracket) */
      if (indent <= stmtNew && !rio_fmtCloses(&t)) indent = stmtNew + 2;
      indent = indent < 0 ? 0 : indent;
      comment[0] = -1;
    } else {
      int back, after, b, g, s;
      const char *last = rio_fmtStatement(p, end, &b, &g, &s);
      rio_fmtBlocks(p, last ? last : e, &back, &after);
      indent = 2 * (depth + back);
      stmtOld = oldIndent;
      stmtNew = indent = indent < 0 ? 0 : indent;
      depth = depth + after < 0 ? 0 : depth + after;
      comment[0] = -1;
      if (!nofmt && t.kind && last && rio_fmtListy(p, last)) { /* lists over lines */
        rio_FmtLay L = {&o, {0}, 0, 0, 0, 1, 0, 0};
        rio_FmtIt it;
        L.lineIndent = indent;
        L.srcIndent = oldIndent;
        L.crlf = crlf;
        rio_fmtSpaces(&o, indent);
        rio_fmtItAt(&it, p, last);
        rio_fmtLayList(&L, &it, 0, 0, indent, oldIndent);
        if (o.len <= o.cap && rio_fmtSameCode(p, last, out + lineAt, out + o.len)) {
          laid = 1;
          eol = last;
          brackets = b;
          goesOn = g;
          inString = s;
          blockKey = -1;
        } else {
          o.len = lineAt; /* (else line by line) */
          o.col = 0;
        }
      }
    }
    if (!laid) {
      if ((key = rio_fmtAlignKey(p, e, &col)) >= 0 && key == blockKey && p < blockEnd) alignTo = blockTo;
      else if (key >= 0) { /* lines lined up: from here on, each its ::, := or = in the same column (from its indent) */
        const char *r = p, *re, *reol;
        int n = 0, widest = 0, runs = 0, k, run, at;
        for (;;) {
          re = rio_fmtLineEnd(r, end, &reol);
          if (rio_fmtAlignKey(r, re, &k) != key || (at = rio_fmtMeasure(r, re, end, &run)) < 0) break;
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
      if (nofmt && marker >= 0) { /* (left alone: as it was, but for spaces at its end, which never count) */
        o.len = lineAt;
        rio_fmtPut(&o, p, (int)(e - p));
        comment[0] = -1;
      }
      rio_fmtLineState(p, e, &brackets, &goesOn, &inString);
    }
    nofmt = marker ? marker > 0 : nofmt;
    if (eol < end) rio_fmtPut(&o, crlf ? "\r\n" : "\n", crlf ? 2 : 1);
    if (eol >= end) break;
    p = eol + 1;
  }
  if (o.len >= o.cap) return -1;
  out[o.len] = 0;
  return rio_fmtSameCode(src, end, out, out + o.len) ? (int32_t)o.len : -2;
}

/* ------------------------------------------------------------------ colors */

/* Rio's words (as the lexer has them, with true and false) */
static const char *const rio_hlWords[] = {"proc", "struct", "if", "else", "for", "in", "end", "return", "break", "continue",
  "import", "include", "enum", "union", "switch", "case", "is", "nil", "and", "or", "not", "do", "true", "false", 0};

/* each token's kind, a line at a time (rio_fmtToken's tokens: a string going on to the next line too) */
int rio_highlight(const char *src, uint32_t len, rio_HighlightFn fn, void *userData) {
  const char *p = src, *end = src + len;
  int count = 0;
  for (;;) {
    const char *eol, *e = rio_fmtLineEnd(p, end, &eol), *q = p;
    rio_FmtTok t, next;
    q = rio_fmtToken(q, e, &t);
    while (t.kind) {
      const char *after = rio_fmtToken(q, e, &next);
      int kind = rio_Hl_code;
      switch (t.kind) {
      case rio_Fmt_comment: kind = rio_Hl_comment; break;
      case rio_Fmt_string: case rio_Fmt_char: kind = rio_Hl_string; break;
      case rio_Fmt_number: kind = rio_Hl_number; break;
      case rio_Fmt_op: kind = rio_Hl_punct; break;
      case rio_Fmt_name:
        kind = rio_fmtIn(&t, rio_hlWords) ? rio_Hl_keyword : *t.s >= 'A' && *t.s <= 'Z' ? rio_Hl_type
               : rio_fmtIs(&next, "(") && !next.gap ? rio_Hl_call : rio_Hl_code;
        break;
      }
      fn(userData, (uint32_t)(t.s - src), (uint32_t)t.len, kind);
      count++;
      t = next;
      q = after;
    }
    if (eol >= end) return count;
    p = eol + 1;
  }
}