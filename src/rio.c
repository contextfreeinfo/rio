/* rio.c - compiler + vm. Single pass, no recursion, no allocation: everything lives in Rio and the
   caller's buffers. All locals are statically allocated (no recursion => one frame per proc), so
   bytecode operands are absolute word slots into the memory buffer. */
#include "rio.h"
#include <math.h>
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
#ifndef RIO_NO_CGOTO
#define RIO_CGOTO 1
#endif
#define NORET __attribute__((noreturn))
#elif defined(_MSC_VER)
#define NORET __declspec(noreturn)
#else
#define NORET
#endif

#define OPS RIO_OPS
#define OPENUM(o) OP_##o,
enum { OPS(OPENUM) OP_DATA = 255 };
#define OP_LASTDST OP_FMAX /* ops up to here write one scalar to slot a */
/* small values: up to this many words, an array or struct lives in slots and is copied without being
   asked (x := v, by-value params and returns, array arithmetic temporaries); bigger ones need &T, []T or
   a written type */
#define SMALLW 16

enum { TK_EOF = 256, TK_ID, TK_INT, TK_FLT, TK_STR, TK_PROC, TK_STRUCT, TK_IF, TK_ELSE, TK_FOR, TK_IN,
  TK_END, TK_RETURN, TK_BREAK, TK_CONTINUE, TK_IMPORT, TK_INCLUDE, TK_DCOLON, TK_DECL, TK_EQ, TK_NE, TK_LE, TK_GE, TK_AND,
  TK_OR, TK_SHL, TK_SHR, TK_ARROW, TK_RLT, TK_RLE, TK_OPEQ };
enum { TY_VOID, TY_I32, TY_F32, TY_BYTE, TY_STR, TY_BLOB, TY_BOOL };
enum { K_VOID, K_I32, K_F32, K_BYTE, K_SLICE, K_ARR, K_STRUCT, K_BOOL, K_LIST, K_BUILD };
enum { S_VAR = RIO_S_VAR, S_CONST, S_TYPE, S_FN, S_FFI, S_BI, S_METH, S_SELF, S_MOD, S_ROREF }; /* S_ROREF: a read-only S_SELF */
enum { EK_CONST, EK_ST, EK_MEM, EK_VOID, EK_FN, EK_FFI, EK_BI, EK_TY, EK_LEN, EK_MOD };
enum { OK_BIN, OK_UN, OK_AND, OK_OR, OK_PAREN, OK_CALL, OK_IDX, OK_LIT };
enum { B_PROC, B_IF, B_ELSE, B_LOOP, B_FOR, B_EACH };
enum { BI_LOG, BI_LEN, BI_MIN, BI_MAX, BI_ABS, BI_SQRT, BI_ROUND = BI_SQRT + 11, BI_ATAN2, BI_FMOD = BI_ATAN2 + 2,
  BI_PUSH, BI_POP, BI_CLEAR, BI_CAP, BI_REMOVE, BI_SWAPREMOVE, BI_PUSHALL, BI_FORMAT,
  BI_TOINT, BI_TOFLOAT, BI_TOBOOL, BI_ASSTRING };
#define NONE 0xFFFF
static const char M_END[] = "missing 'end'", M_BRACKET[] = "unclosed bracket", M_EXPR[] = "expected expression",
  M_STRING[] = "unterminated string";
#define TY(t) (&vm->c->type[t])
#define TK (vm->c->tk)
typedef RioEx Ex;
typedef RioOp Op;

static const char *kw[] = {"proc", "struct", "if", "else", "for", "in", "end", "return", "break", "continue", "import", "include"};
static const char *bi[] = {"log", "len", "min", "max", "abs", "sqrt", "sin", "cos", "tan", "asin", "acos",
  "atan", "exp", "ln", "floor", "ceil", "round", "atan2", "pow", "fmod",
  "push", "pop", "clear", "cap", "remove", "swapRemove", "pushAll", "format",
  "toInt", "toFloat", "toBool", "asString"};
static float f_sqrt(float x) { return sqrtf(x); }
static float f_sin(float x) { return sinf(x); }
static float f_cos(float x) { return cosf(x); }
static float f_tan(float x) { return tanf(x); }
static float f_asin(float x) { return asinf(x); }
static float f_acos(float x) { return acosf(x); }
static float f_atan(float x) { return atanf(x); }
static float f_exp(float x) { return expf(x); }
static float f_ln(float x) { return logf(x); }
static float f_floor(float x) { return floorf(x); }
static float f_ceil(float x) { return ceilf(x); }
static float f_round(float x) { return x < 0 ? -floorf(-x + 0.5f) : floorf(x + 0.5f); }
static float f_abs(float x) { return fabsf(x); }
static float f_atan2(float y, float x) { return atan2f(y, x); }
static float f_pow(float x, float y) { return powf(x, y); }
static float f_fmod(float x, float y) { return fmodf(x, y); }
static float (*const mf1[])(float) = {f_sqrt, f_sin, f_cos, f_tan, f_asin, f_acos, f_atan, f_exp, f_ln,
  f_floor, f_ceil, f_round, f_abs};
static float (*const mf2[])(float, float) = {f_atan2, f_pow, f_fmod};

/* ---------------------------------------------------------------- text helpers */
static int fmti(char *b, int32_t v) {
  char t[12]; int n = 0, i = 0; uint32_t u = v < 0 ? 0u - (uint32_t)v : (uint32_t)v;
  do t[n++] = (char)('0' + u % 10); while (u /= 10);
  if (v < 0) b[i++] = '-';
  while (n) b[i++] = t[--n];
  return i;
}
static int fmtf(char *b, float f) {
  int i = 0, e = 0, nd = 1, dec, k; double d = f; uint32_t ip, fp, sc = 1, t;
  if (f != f) { memcpy(b, "nan", 3); return 3; }
  if (d < 0) { b[i++] = '-'; d = -d; }
  if (d > 3.5e38) { memcpy(b + i, "inf", 3); return i + 3; }
  if (d != 0 && (d >= 1e9 || d < 1e-4)) { while (d >= 10) { d /= 10; e++; } while (d < 1) { d *= 10; e--; } }
  ip = (uint32_t)d;
  for (t = ip; t >= 10; t /= 10) nd++;
  dec = 7 - nd; if (dec < 1) dec = 1; if (dec > 6) dec = 6;
  for (k = 0; k < dec; k++) sc *= 10;
  fp = (uint32_t)((d - ip) * sc + 0.5);
  if (fp >= sc) { ip++; fp -= sc; }
  i += fmti(b + i, (int32_t)ip); b[i++] = '.';
  for (k = dec - 1; k >= 0; k--) { b[i + k] = (char)('0' + fp % 10); fp /= 10; }
  while (dec > 1 && b[i + dec - 1] == '0') dec--;
  i += dec;
  if (e) { b[i++] = 'e'; i += fmti(b + i, e); }
  return i;
}
static int cat(char *b, int n, const char *s) { while (*s && n < RIO_ERRBUF - 1) b[n++] = *s++; return n; }

/* "line:col: message" (position parts omitted when unknown); remembers the parts for rio_error_info */
static int seterr(Rio *vm, int kind, int line, int col, int len, const char *m) {
  char *b = vm->err; int n = 0;
  if (line > 0) { n += fmti(b, line); if (col > 0) { b[n++] = ':'; n += fmti(b + n, col); } n = cat(b, n, ": "); }
  vm->ekind = kind; vm->eline = line; vm->ecol = col; vm->elen = len; vm->emsg = n; vm->efile = 0;
  if (kind == RIO_ERUNTIME) n = cat(b, n, "runtime error: ");
  n = cat(b, n, m); b[n] = 0;
  return n;
}
/* compile error at the current token, or at the end of the expression just finished when the
   current token is already on the next line */
NORET static void fail(Rio *vm, const char *m) {
  RioC *c = vm->c; int prev = TK.nl || TK.t == TK_EOF, i;
  c->failmsg = m;
  int n = seterr(vm, RIO_ECOMPILE, prev ? c->pline : TK.line, prev ? c->pcol : TK.col, prev ? c->pw : TK.w, m);
  if (!prev && TK.n > 0) {
    char *b = vm->err;
    n = cat(b, n, " near '");
    for (i = 0; i < TK.n && i < 24 && n < RIO_ERRBUF - 2; i++) b[n++] = TK.s[i];
    n = cat(b, n, "'"); b[n] = 0;
  }
  if (c->curf >= 2) vm->efile = c->names + c->mods[c->curf].file;
  longjmp(c->jb, 1);
}
/* an error about the current token itself, even when it starts a new line */
NORET static void failtok(Rio *vm, const char *m) { TK.nl = 0; fail(vm, m); }

/* ---------------------------------------------------------------- lexer */
static int isal(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
static int isdg(int c) { return c >= '0' && c <= '9'; }
static int esc(int c) { return c == 'n' ? '\n' : c == 't' ? '\t' : c == 'r' ? '\r' : c == '0' ? 0 : c; }
static const char ops2[] = "::" ":=" "==" "!=" "<=" ">=" "&&" "||" "<<" ">>" "->" "+=" "-=" "*=" "/=" "%=" "&=" "|=" "^=";
static const short tok2[] = {TK_DCOLON, TK_DECL, TK_EQ, TK_NE, TK_LE, TK_GE, TK_AND, TK_OR, TK_SHL, TK_SHR,
  TK_ARROW, -'+', -'-', -'*', -'/', -'%', -'&', -'|', -'^'};

static void lex(Rio *vm, RioTok *t) {
  const char *p = vm->c->sp, *e = vm->c->se, *s, *st;
  int nl = 0, i;
  for (;;) {
    while (p < e && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) { if (*p == '\n') { nl = 1; vm->c->line++; vm->c->ls = p + 1; } p++; }
    if (p + 1 < e && p[0] == '/' && p[1] == '/') { while (p < e && *p != '\n') p++; continue; }
    if (p + 1 < e && p[0] == '/' && p[1] == '*') {
      for (p += 2; p + 1 < e && !(p[0] == '*' && p[1] == '/'); p++) if (*p == '\n') { nl = 1; vm->c->line++; vm->c->ls = p + 1; }
      p = p + 1 < e ? p + 2 : e;
      continue;
    }
    break;
  }
  t->nl = (uint8_t)nl; t->line = vm->c->line; t->col = (int)(p - vm->c->ls) + 1; t->s = s = st = p; t->op = 0; t->v.i = 0;
  if (p >= e) { t->t = TK_EOF; t->n = t->w = 0; vm->c->sp = e; return; }
  if (isal(*p)) {
    while (p < e && (isal(*p) || isdg(*p))) p++;
    t->t = TK_ID;
    for (i = 0; i < 12; i++) if ((int)strlen(kw[i]) == p - s && !memcmp(kw[i], s, p - s)) t->t = TK_PROC + i;
  } else if (isdg(*p)) {
    uint32_t v = 0; double d = 0, sc = 1; int isf = 0;
    if (*p == '0' && p + 1 < e && (p[1] == 'x' || p[1] == 'X')) {
      for (p += 2; p < e; p++) {
        int c = *p, h = isdg(c) ? c - '0' : (c | 32) >= 'a' && (c | 32) <= 'f' ? (c | 32) - 'a' + 10 : -1;
        if (h < 0) break;
        v = v * 16 + (uint32_t)h;
      }
    } else {
      for (; p < e && isdg(*p); p++) { v = v * 10 + (uint32_t)(*p - '0'); d = d * 10 + (*p - '0'); }
      if (p + 1 < e && *p == '.' && isdg(p[1])) for (isf = 1, p++; p < e && isdg(*p); p++) d += (*p - '0') * (sc /= 10);
      if (p < e && (*p == 'e' || *p == 'E')) {
        int neg = 0, x = 0;
        p++; if (p < e && (*p == '-' || *p == '+')) neg = *p++ == '-';
        while (p < e && isdg(*p)) x = x * 10 + (*p++ - '0');
        while (x--) d = neg ? d / 10 : d * 10;
        isf = 1;
      }
    }
    t->t = isf ? TK_FLT : TK_INT;
    if (isf) t->v.f = (float)d; else t->v.u = v;
  } else if (*p == '"') {
    for (s = ++p; p < e && *p != '"'; p++) { if (*p == '\\') p++; if (p < e && *p == '\n') { vm->c->line++; vm->c->ls = p + 1; } }
    if (p >= e) { vm->c->sp = e; fail(vm, M_STRING); }
    t->t = TK_STR; t->s = s; t->n = (int)(p - s); t->w = (int)(p + 1 - st); vm->c->sp = p + 1;
    return;
  } else if (*p == '\'') {
    int c = p + 1 < e ? p[1] : 0;
    p += 2;
    if (c == '\\' && p < e) c = esc(*p++);
    if (p >= e || *p != '\'') fail(vm, "bad char literal");
    p++; t->t = TK_INT; t->v.i = c;
  } else if (p + 2 < e && p[0] == '.' && p[1] == '.' && (p[2] == '<' || p[2] == '=')) {
    t->t = p[2] == '<' ? TK_RLT : TK_RLE; p += 3;
  } else if (p + 2 < e && (p[0] == '<' || p[0] == '>') && p[1] == p[0] && p[2] == '=') {
    t->t = TK_OPEQ; t->op = p[0] == '<' ? TK_SHL : TK_SHR; p += 3;
  } else {
    t->t = *p++;
    for (i = 0; p < e && i < (int)sizeof tok2 / (int)sizeof tok2[0]; i++)
      if (ops2[i * 2] == p[-1] && ops2[i * 2 + 1] == *p) {
        p++;
        if (tok2[i] < 0) { t->t = TK_OPEQ; t->op = -tok2[i]; } else t->t = tok2[i];
        break;
      }
  }
  t->n = t->w = (int)(p - s); vm->c->sp = p;
}
static void next(Rio *vm) {
  RioC *c = vm->c;
  c->pline = c->tk.line; c->pcol = c->tk.col; c->pw = c->tk.w; c->tk = c->nx; lex(vm, &c->nx);
}
static void expect(Rio *vm, int t, const char *m) { if (TK.t != t) fail(vm, m); next(vm); }

/* ---------------------------------------------------------------- tables */
static int addname(Rio *vm, const char *s, int n) {
  if (vm->c->nnames + n > (int)vm->c->lim.names) fail(vm, "out of name space");
  memcpy(vm->c->names + vm->c->nnames, s, (size_t)n); vm->c->nnames += n;
  return vm->c->nnames - n;
}
static void addsym(Rio *vm, const char *s, int n, int k, int t, int32_t v) {
  RioSym *y;
  if (vm->c->nsym >= (int)vm->c->lim.syms) fail(vm, "too many symbols");
  y = &vm->c->sym[vm->c->nsym++];
  y->name = (uint16_t)addname(vm, s, n); y->len = (uint16_t)n; y->k = (uint8_t)k; y->t = (uint16_t)t; y->v = v;
  y->mod = (uint16_t)vm->c->curmod; y->ex = 0;
  if (vm->c->exporting && !vm->c->nblk && vm->c->curfn < 0) { y->ex = 1; vm->c->exporting = 0; } /* name* :: ... */
}
static int lookup(Rio *vm, const char *s, int n) {
  int i;
  for (i = vm->c->nsym - 1; i >= 0; i--)
    if (vm->c->sym[i].k != S_METH && (vm->c->sym[i].mod == vm->c->curmod || !vm->c->sym[i].mod) &&
        vm->c->sym[i].len == n && !memcmp(vm->c->names + vm->c->sym[i].name, s, (size_t)n)) return i;
  return -1;
}
/* an exported name of module m */
static int modsym(Rio *vm, int m, const char *s, int n) {
  int i;
  for (i = vm->c->nsym - 1; i >= 0; i--)
    if (vm->c->sym[i].mod == m && vm->c->sym[i].ex && vm->c->sym[i].k != S_METH &&
        vm->c->sym[i].len == n && !memcmp(vm->c->names + vm->c->sym[i].name, s, (size_t)n)) return i;
  return -1;
}
static int lookup_meth(Rio *vm, int t, const char *s, int n) {
  int i;
  for (i = vm->c->nsym - 1; i >= 0; i--)
    if (vm->c->sym[i].k == S_METH && vm->c->sym[i].t == t && (vm->c->sym[i].mod == vm->c->curmod || vm->c->sym[i].ex) && vm->c->sym[i].len == n && !memcmp(vm->c->names + vm->c->sym[i].name, s, (size_t)n)) return i;
  return -1;
}
static int newtype(Rio *vm, int k, int elem, uint32_t n, uint32_t size) {
  RioType *y;
  if (vm->c->ntype >= (int)vm->c->lim.types) fail(vm, "too many types");
  y = &vm->c->type[vm->c->ntype];
  y->k = (uint8_t)k; y->elem = (uint16_t)elem; y->n = n; y->size = size; y->f0 = y->nf = 0;
  y->ref = (uint8_t)(k == K_SLICE || k == K_BUILD || ((k == K_ARR || k == K_LIST) && vm->c->type[elem].ref));
  return vm->c->ntype++;
}
static int slice_of(Rio *vm, int e) {
  int i;
  if (e == TY_BYTE) return TY_BLOB;
  for (i = TY_BLOB + 1; i < vm->c->ntype; i++) if (vm->c->type[i].k == K_SLICE && vm->c->type[i].elem == e) return i;
  return newtype(vm, K_SLICE, e, 0, 8);
}
static int array_of(Rio *vm, int e, uint32_t n) {
  int i; uint32_t es = TY(e)->size;
  for (i = TY_BLOB + 1; i < vm->c->ntype; i++) if (vm->c->type[i].k == K_ARR && vm->c->type[i].elem == e && vm->c->type[i].n == n) return i;
  if (es && n > 0x7FFFFFF0u / es) fail(vm, "array too large");
  return newtype(vm, K_ARR, e, n, (es * n + 3) & ~3u);
}
/* [..N]T: a length word followed by N elements. [..]T: a builder, a view of a length word and the memory
   it counts: (address of the length word, address of the data, capacity) */
static int list_of(Rio *vm, int e, uint32_t n) {
  int i; uint32_t es = TY(e)->size;
  for (i = TY_BLOB + 1; i < vm->c->ntype; i++) if (vm->c->type[i].k == K_LIST && vm->c->type[i].elem == e && vm->c->type[i].n == n) return i;
  if (es && n > 0x7FFFFFF0u / es) fail(vm, "list too large");
  return newtype(vm, K_LIST, e, n, (4 + es * n + 3) & ~3u);
}
static int build_of(Rio *vm, int e) {
  int i;
  for (i = TY_BLOB + 1; i < vm->c->ntype; i++) if (vm->c->type[i].k == K_BUILD && vm->c->type[i].elem == e) return i;
  return newtype(vm, K_BUILD, e, 0, 12);
}
static int vt(int t) { return t == TY_BYTE ? TY_I32 : t; }
static int words(Rio *vm, int t) { return (int)((TY(t)->size + 3) / 4); }

/* ---------------------------------------------------------------- code + storage */
/* pc->line entries (u16 pc, u16 line) grow down from the top of the string pool */
static void markline(Rio *vm) {
  RioC *c = vm->c; uint32_t line = c->lineovr ? c->lineovr : (uint32_t)c->pline; uint16_t e[2];
  if (line == c->lastline && vm->pc - c->lastlinepc < 4096) return; /* (steps stay under 4096: see packlines) */
  if (c->poolcap - c->pool < 4) fail(vm, "out of compiler memory");
  c->poolcap -= 4; e[0] = (uint16_t)vm->pc; e[1] = (uint16_t)(line > 0xFFFF ? 0xFFFF : line);
  memcpy(c->strs + c->poolcap, e, 4); c->nline++; c->lastline = line; c->lastlinepc = vm->pc;
}
static uint32_t cline(Rio *vm, uint32_t pc) { /* line of pc while still compiling */
  RioC *c = vm->c; uint32_t o, line = 0; uint16_t e[2];
  for (o = c->linetop; o > c->poolcap; o -= 4) { memcpy(e, c->strs + o - 4, 4); if (e[0] > pc) break; line = e[1]; }
  return line;
}
static int emit(Rio *vm, int op, int a, int b, int c) {
  RioIns *i;
  if (vm->pc >= vm->codecap) fail(vm, "code too large");
  markline(vm);
  i = &vm->code[vm->pc];
  i->op = (uint8_t)op; i->x = 0; i->a = (uint16_t)a; i->b = (uint16_t)b; i->c = (uint16_t)c;
  return (int)vm->pc++;
}
static void emitx(Rio *vm, int op, int x, int a) { emit(vm, op, a, 0, 0); vm->code[vm->pc - 1].x = (uint8_t)x; }
static void emitw(Rio *vm, int a, uint32_t w) { emit(vm, OP_DATA, a, (int)(w & 0xFFFF), (int)(w >> 16)); }
static int here(Rio *vm) { return (int)(vm->c->lastlabel = vm->pc); }
static int jappend(Rio *vm, int list, int j) { if (j == NONE) return list; vm->code[j].c = (uint16_t)list; return j; }
static void patch(Rio *vm, int list, int to) {
  while (list != NONE) { int n = vm->code[list].c; vm->code[list].c = (uint16_t)to; list = n; }
}
static int alloc(Rio *vm, int n) {
  int r = (int)vm->c->fr;
  vm->c->fr += (uint32_t)n;
  if (vm->c->fr > vm->c->hwm) {
    vm->c->hwm = vm->c->fr;
    if (vm->c->hwm > vm->c->lim.slots || vm->c->hwm * 4 > vm->hi) fail(vm, "out of slots");
  }
  return r;
}
static uint32_t halloc(Rio *vm, uint32_t n) {
  n = (n + 3) & ~3u;
  if (vm->hi < n || vm->hi - n < vm->c->hwm * 4) fail(vm, "out of memory");
  vm->c->big += n; if (vm->c->curfn >= 0) vm->c->func[vm->c->curfn].big += n; /* for the memory report */
  return vm->hi -= n;
}
#define KFIX(i) (vm->c->kfix[(i) >> 3] & (1u << ((i) & 7)))
#define KADR(i) (vm->c->kadr[(i) >> 3] & (1u << ((i) & 7)))
/* n consecutive constants; bit j of adr: word j is a slot address (those never match plain numbers) */
static int kfind(Rio *vm, const uint32_t *v, int n, int adr) {
  RioVal *R = (RioVal *)vm->mem; uint32_t i; int j;
  for (i = 1; i + (uint32_t)n <= vm->nk; i++) {
    for (j = 0; j < n; j++) if (R[i + j].u != v[j] || KFIX(i + j) || !KADR(i + j) != !(adr >> j & 1)) break;
    if (j == n) return (int)i;
  }
  if (vm->nk + (uint32_t)n > vm->kcap) fail(vm, "too many constants");
  for (j = 0; j < n; j++, vm->nk++) {
    R[vm->nk].u = v[j];
    if (adr >> j & 1) vm->c->kadr[vm->nk >> 3] |= (uint8_t)(1u << (vm->nk & 7));
  }
  return (int)vm->nk - n;
}
static int kslot(Rio *vm, uint32_t v) { return kfind(vm, &v, 1, 0); }
/* constant holding a static address; marks those slots as address-taken (they can't become C locals in AOT) */
static uint32_t expose(Rio *vm, uint32_t a, uint32_t n) {
  uint32_t w;
  for (w = a / 4; w < vm->c->lim.slots && w * 4 < a + n; w++) vm->c->exposed[w >> 3] |= (uint8_t)(1u << (w & 7));
  return a;
}
static int kaddr(Rio *vm, uint32_t a, uint32_t n) { uint32_t v = expose(vm, a, n); return kfind(vm, &v, 1, 1); }
/* is the last instruction (+data word) an IDX writing temp d? */
static RioIns *lastidx(Rio *vm, int d) {
  RioIns *x = vm->pc >= 2 ? &vm->code[vm->pc - 2] : 0;
  return x && d >= (int)vm->c->nact && vm->c->lastlabel != vm->pc && x->op == OP_IDX && x->a == d ? x : 0;
}
/* if the last instruction wrote temp `from`, make it write `to` instead */
static int retarget(Rio *vm, int from, int to) {
  RioIns *i;
  if (!vm->pc || vm->c->lastlabel == vm->pc || from < (int)vm->c->nact) return 0;
  i = &vm->code[vm->pc - 1];
  if (i->op == OP_DATA && vm->pc >= 2 && (i[-1].op == OP_LDX || i[-1].op == OP_LDXB)) i--;
  if ((i->op > OP_LASTDST && i->op != OP_LDX && i->op != OP_LDXB) || i->a != from) return 0;
  i->a = (uint16_t)to;
  return 1;
}

/* ---------------------------------------------------------------- expression values */
static Ex mkex(int k, int t, int32_t a) { Ex e; e.k = (uint8_t)k; e.ro = 0; e.lv = 0; e.t = (uint16_t)t; e.t0 = 0; e.a = a; e.off = 0; return e; }
/* Views that outlive their memory's owner. A proc's own memory keeps its type for good, so a view of it
   is always memory-safe, but the next call rewrites it: returning one, or storing one where it outlives
   the call, is almost always a bug. This catches the direct cases; views that come in as parameters
   aren't tracked. */
static int localmem(Rio *vm, const Ex *e) { /* e is storage of the proc being compiled */
  RioCFunc *f; uint32_t a;
  if (vm->c->curfn < 0 || e->k != EK_ST) return 0;
  f = &vm->c->func[vm->c->curfn]; a = (uint32_t)e->a;
  return (a / 4 >= f->fs && a < vm->hi) || (a >= vm->hi && a < f->hi0);
}
static int localview(Rio *vm, const Ex *e) { int k = TY(e->t)->k; return e->lv || ((k == K_ARR || k == K_LIST) && localmem(vm, e)); }
static void setlv(Rio *vm, uint32_t a, int n, int on) { /* mark the local view slots at address a */
  uint32_t s = a / 4;
  for (; n-- > 0; s++) {
    if (on) vm->c->lvs[s >> 3] |= (uint8_t)(1u << (s & 7)); else vm->c->lvs[s >> 3] &= (uint8_t)~(1u << (s & 7));
  }
}
/* a view stored into d: into a local of this proc, that local now holds (or no longer holds) a view of
   the proc's own memory; anywhere else, it mustn't be one */
static void viewstore(Rio *vm, const Ex *d, const Ex *s) {
  int lv = localview(vm, s);
  if (vm->c->curfn < 0 || vm->c->argstore) return;
  if (d->k == EK_ST && localmem(vm, d) && (uint32_t)d->a < vm->hi) setlv(vm, (uint32_t)d->a, words(vm, d->t), lv);
  else if (lv) fail(vm, "this view of the proc's own memory would outlive the call: make that memory global, or have the caller pass it in");
}
#define RO_REF 4 /* Ex.ro: an &place, not yet given to a & parameter or name := &place */
/* a value in the compiler's scratch slots (a call's or an expression's result), not a variable */
static int istemp(Rio *vm, Ex *e) { return e->k == EK_ST && (uint32_t)e->a >= vm->c->nact * 4 && (uint32_t)e->a < vm->hi; }
static void needval(Rio *vm, Ex *e) {
  if (e->k > EK_MEM) fail(vm, e->k == EK_VOID ? "no value" : "not a value");
  if (e->ro & RO_REF) fail(vm, "&x only goes to a & parameter or name := &x");
}
static void coerce(Rio *vm, Ex *e, int t) {
  int et;
  needval(vm, e);
  et = vt(e->t); t = vt(t);
  if (e->k == EK_CONST && et == TY_I32 && t == TY_F32) { RioVal v; v.f = (float)e->a; e->a = v.i; e->t = TY_F32; return; }
  if (et == t) return;
  if (TY(et)->k == K_ARR && TY(t)->k == K_SLICE && slice_of(vm, TY(et)->elem) == t) { if (istemp(vm, e)) fail(vm, "a computed array has no home to view: store it in a variable first"); return; }
  if (TY(et)->k == K_LIST && TY(t)->k == K_BUILD && TY(t)->elem == TY(et)->elem) return;
  if ((TY(et)->k == K_ARR || TY(et)->k == K_SLICE) && TY(t)->k == K_BUILD)
    fail(vm, "a builder needs somewhere to keep its length: declare one first, name: [..]T = array");
  fail(vm, "type mismatch");
}
/* scalar value -> slot holding it */
static int toslot(Rio *vm, Ex *e, int reuse) {
  int k, d;
  needval(vm, e); k = TY(e->t)->k;
  if (k != K_I32 && k != K_F32 && k != K_BYTE && k != K_BOOL) fail(vm, "expected a number");
  if (e->k == EK_CONST) return kslot(vm, (uint32_t)e->a);
  if (e->k == EK_ST) {
    if (k != K_BYTE && e->a < 0x40000) return e->a >> 2;
    e->k = EK_MEM; e->a = kaddr(vm, (uint32_t)e->a, k == K_BYTE ? 1 : 4); e->off = 0;
  }
  if (reuse && lastidx(vm, e->a)) { /* fuse index + load */
    vm->code[vm->pc - 2].op = k == K_BYTE ? OP_LDXB : OP_LDX; vm->code[vm->pc - 1].a = (uint16_t)e->off;
    return e->a;
  }
  d = reuse && e->a >= (int)vm->c->nact ? e->a : alloc(vm, 1);
  emit(vm, k == K_BYTE ? OP_LDB : OP_LDW, d, e->a, e->off);
  return d;
}
/* slice/string/blob/array value -> 2 consecutive slots (addr, len) */
static int toslot2(Rio *vm, Ex *e) {
  RioType *ty; int d;
  needval(vm, e); ty = TY(e->t);
  if (ty->k == K_ARR) {
    if (e->k == EK_ST) { uint32_t v[2]; v[0] = expose(vm, (uint32_t)e->a, ty->size); v[1] = ty->n; return kfind(vm, v, 2, 1); }
    d = alloc(vm, 2);
    emit(vm, OP_LEA, d, e->a, e->off); emit(vm, OP_MOV, d + 1, kslot(vm, ty->n), 0);
    return d;
  }
  if (ty->k != K_SLICE) fail(vm, "expected a slice");
  if (e->k == EK_ST) {
    if (e->a < 0x3FFFC) return e->a >> 2;
    e->k = EK_MEM; e->a = kaddr(vm, (uint32_t)e->a, 8); e->off = 0;
  }
  d = alloc(vm, 2);
  emit(vm, OP_LDW2, d, e->a, e->off);
  return d;
}
/* list or builder -> 3 consecutive slots: (address of the length word, address of the data, capacity) */
static int toslot3(Rio *vm, Ex *e) {
  RioType *ty; int d;
  needval(vm, e); ty = TY(e->t);
  if (ty->k == K_LIST) { /* the length word comes first, then the elements */
    if (e->k == EK_ST) { uint32_t v[3]; v[0] = expose(vm, (uint32_t)e->a, ty->size); v[1] = v[0] + 4; v[2] = ty->n; return kfind(vm, v, 3, 3); }
    d = alloc(vm, 3);
    emit(vm, OP_LEA, d, e->a, e->off); emit(vm, OP_LEA, d + 1, e->a, e->off + 4); emit(vm, OP_MOV, d + 2, kslot(vm, ty->n), 0);
    return d;
  }
  if (ty->k != K_BUILD) fail(vm, "expected a list");
  if (e->k == EK_ST) {
    if (e->a < 0x3FFF4) return e->a >> 2;
    e->k = EK_MEM; e->a = kaddr(vm, (uint32_t)e->a, 12); e->off = 0;
  }
  d = alloc(vm, 3);
  emit(vm, OP_LDW2, d, e->a, e->off); emit(vm, OP_LDW, d + 2, e->a, e->off + 8);
  return d;
}
/* the slots holding a slice-like or builder value */
static int toslotv(Rio *vm, Ex *e) { return TY(e->t)->k == K_BUILD ? toslot3(vm, e) : toslot2(vm, e); }
/* struct/array -> slot holding its address */
static int toaddr(Rio *vm, Ex *e) {
  int d;
  needval(vm, e);
  if (e->k == EK_ST) return kaddr(vm, (uint32_t)e->a, TY(e->t)->size);
  if (!e->off) return e->a;
  d = alloc(vm, 1);
  emit(vm, OP_LEA, d, e->a, e->off);
  return d;
}
static void store(Rio *vm, Ex *d, Ex *s) {
  RioType *ty = TY(d->t); int k = ty->k, ss;
  if (k == K_SLICE || k == K_BUILD) viewstore(vm, d, s);
  coerce(vm, s, d->t);
  if (k == K_STRUCT || k == K_ARR || k == K_LIST) {
    int sa, da;
    if (d->k == EK_ST && s->k == EK_ST && d->a < 0x40000 && s->a < 0x40000 && !((d->a | s->a | (int32_t)ty->size) & 3) && ty->size <= SMALLW * 4) {
      /* both in slots: copy the words, without taking either address */
      if (d->a != s->a) emit(vm, OP_MOVN, d->a >> 2, s->a >> 2, (int)(ty->size / 4));
      return;
    }
    sa = toaddr(vm, s); da = toaddr(vm, d);
    if (ty->size > 0xFFFF) fail(vm, "value too large to copy");
    emit(vm, OP_COPY, da, sa, (int)ty->size);
    return;
  }
  if (k == K_BUILD) { /* three words; ordered so an overlapping source is read before it's overwritten */
    ss = toslot3(vm, s);
    if (d->k == EK_ST && d->a < 0x3FFF4) {
      int a = d->a >> 2;
      if (a > ss) { emit(vm, OP_MOV, a + 2, ss + 2, 0); emit(vm, OP_MOV2, a, ss, 0); }
      else if (a < ss) { emit(vm, OP_MOV2, a, ss, 0); emit(vm, OP_MOV, a + 2, ss + 2, 0); }
      return;
    }
    if (d->k == EK_ST) { d->k = EK_MEM; d->a = kaddr(vm, (uint32_t)d->a, 12); d->off = 0; }
    emit(vm, OP_STW2, d->a, ss, d->off); emit(vm, OP_STW, d->a, ss + 2, d->off + 8);
    return;
  }
  if (k == K_SLICE) {
    ss = toslot2(vm, s);
    if (d->k == EK_ST && d->a < 0x3FFFC) { if (d->a >> 2 != ss) emit(vm, OP_MOV2, d->a >> 2, ss, 0); return; }
    if (d->k == EK_ST) { d->k = EK_MEM; d->a = kaddr(vm, (uint32_t)d->a, 8); d->off = 0; }
    emit(vm, OP_STW2, d->a, ss, d->off);
    return;
  }
  if (d->k == EK_ST && k != K_BYTE && d->a < 0x40000) {
    int dst = d->a >> 2;
    if (s->k == EK_ST && TY(s->t)->k != K_BYTE && s->a < 0x40000 && retarget(vm, s->a >> 2, dst)) return;
    ss = toslot(vm, s, 1);
    if (ss != dst) emit(vm, OP_MOV, dst, ss, 0);
    return;
  }
  ss = toslot(vm, s, 1);
  if (d->k == EK_MEM && lastidx(vm, d->a)) { /* fuse index + store */
    RioIns *x = &vm->code[vm->pc - 2];
    x->op = k == K_BYTE ? OP_STXB : OP_STX; x->a = x->b; x->b = x->c; x->c = (uint16_t)ss;
    vm->code[vm->pc - 1].a = (uint16_t)d->off;
    return;
  }
  if (d->k == EK_ST) { d->k = EK_MEM; d->a = kaddr(vm, (uint32_t)d->a, k == K_BYTE ? 1 : 4); d->off = 0; }
  emit(vm, k == K_BYTE ? OP_STB : OP_STW, d->a, ss, d->off);
}
/* emit a jump taken when e is false; fuses a preceding compare into a compare-and-branch */
static int invjump(RioIns *i) {
  uint16_t t;
  switch (i->op) {
  case OP_JZ: i->op = OP_JNZ; break; case OP_JNZ: i->op = OP_JZ; break;
  case OP_JEQ: i->op = OP_JNE; break; case OP_JNE: i->op = OP_JEQ; break;
  case OP_JFEQ: i->op = OP_JFNE; break; case OP_JFNE: i->op = OP_JFEQ; break;
  case OP_JFLT: i->op = OP_JFNLT; break; case OP_JFNLT: i->op = OP_JFLT; break;
  case OP_JFLE: i->op = OP_JFNLE; break; case OP_JFNLE: i->op = OP_JFLE; break;
  case OP_JLT: case OP_JLE: i->op = i->op == OP_JLT ? OP_JLE : OP_JLT; t = i->a; i->a = i->b; i->b = t; break;
  default: return 0;
  }
  return 1;
}
static int condjump(Rio *vm, Ex *e) {
  needval(vm, e);
  if (e->t != TY_BOOL) fail(vm, "condition must be Bool");
  if (e->k == EK_CONST) return e->a ? NONE : emit(vm, OP_JMP, 0, 0, NONE);
  if (e->k == EK_ST && e->a < 0x40000 && (e->a >> 2) >= (int)vm->c->nact && vm->pc && vm->c->lastlabel != vm->pc) {
    RioIns *i = &vm->code[vm->pc - 1];
    if (i->a == e->a >> 2 && i->op >= OP_EQ && i->op <= OP_FLE) {
      i->op = (uint8_t)(OP_JEQ + (i->op - OP_EQ)); i->a = i->b; i->b = i->c; i->c = NONE;
      invjump(i);
      return (int)vm->pc - 1;
    }
  }
  return emit(vm, OP_JZ, toslot(vm, e, 1), 0, NONE);
}

/* ---------------------------------------------------------------- operators */
static int32_t ftoi(float f) {
  if (f != f) return 0;
  if (f >= 2147483648.f) return 0x7FFFFFFF;
  if (f <= -2147483648.f) return (int32_t)0x80000000u;
  return (int32_t)f;
}
static void fold(Rio *vm, int op, Ex *l, Ex *r, int isf) {
  RioVal a, b, c; int32_t x, y;
  a.i = l->a; b.i = r->a; x = a.i; y = b.i;
  if (isf) switch (op) {
    case '+': c.f = a.f + b.f; break; case '-': c.f = a.f - b.f; break;
    case '*': c.f = a.f * b.f; break; case '/': c.f = a.f / b.f; break;
    case TK_EQ: c.i = a.f == b.f; break; case TK_NE: c.i = a.f != b.f; break;
    case '<': c.i = a.f < b.f; break; case '>': c.i = a.f > b.f; break;
    case TK_LE: c.i = a.f <= b.f; break; case TK_GE: c.i = a.f >= b.f; break;
    default: fail(vm, "integer operator on floats");
  }
  else switch (op) {
    case '+': c.u = a.u + b.u; break; case '-': c.u = a.u - b.u; break; case '*': c.u = a.u * b.u; break;
    case '/': case '%':
      if (!y) fail(vm, "division by zero");
      c.i = y == -1 ? (op == '/' ? (int32_t)(0u - a.u) : 0) : op == '/' ? x / y : x % y; break;
    case '&': c.u = a.u & b.u; break; case '|': c.u = a.u | b.u; break; case '^': c.u = a.u ^ b.u; break;
    case TK_SHL: c.u = a.u << (b.u & 31); break; case TK_SHR: c.i = x >> (b.u & 31); break;
    case TK_EQ: c.i = x == y; break; case TK_NE: c.i = x != y; break;
    case '<': c.i = x < y; break; case '>': c.i = x > y; break;
    case TK_LE: c.i = x <= y; break; default: c.i = x >= y; break;
  }
  l->a = c.i;
}
static void vecarith(Rio *vm, int op, Ex *l, Ex *r, Ex *into);
static void binop(Rio *vm, int op, Ex *l, Ex *r) {
  int lt, rt, isf, cmp, o, ls, rs, d, sw = 0;
  needval(vm, l); needval(vm, r);
  if (TY(l->t)->k == K_ARR || TY(l->t)->k == K_STRUCT || TY(r->t)->k == K_ARR || TY(r->t)->k == K_STRUCT) { vecarith(vm, op, l, r, 0); return; }
  lt = vt(l->t); rt = vt(r->t);
  if (l->k == EK_CONST && lt == TY_I32 && rt == TY_F32) { coerce(vm, l, TY_F32); lt = TY_F32; }
  if (r->k == EK_CONST && rt == TY_I32 && lt == TY_F32) { coerce(vm, r, TY_F32); rt = TY_F32; }
  if (lt != rt) fail(vm, "type mismatch");
  cmp = op == TK_EQ || op == TK_NE || op == '<' || op == '>' || op == TK_LE || op == TK_GE;
  if ((lt == TY_STR || lt == TY_BLOB) && (op == TK_EQ || op == TK_NE)) {
    ls = toslot2(vm, l); rs = toslot2(vm, r); o = op == TK_EQ ? OP_SEQ : OP_SNE;
  } else {
    isf = lt == TY_F32;
    if (!isf && lt != TY_I32 && !(lt == TY_BOOL && (op == TK_EQ || op == TK_NE))) fail(vm, "bad operand types");
    if (l->k == EK_CONST && r->k == EK_CONST) { fold(vm, op, l, r, isf); l->t = (uint16_t)(cmp ? TY_BOOL : lt); l->ro = 0; return; }
    switch (op) {
    case '+': o = isf ? OP_FADD : OP_ADD; break;
    case '-': o = isf ? OP_FSUB : OP_SUB; break;
    case '*': o = isf ? OP_FMUL : OP_MUL; break;
    case '/': o = isf ? OP_FDIV : OP_DIV; break;
    case TK_EQ: o = isf ? OP_FEQ : OP_EQ; break;
    case TK_NE: o = isf ? OP_FNE : OP_NE; break;
    case '>': sw = 1; /* fallthrough */
    case '<': o = isf ? OP_FLT : OP_LT; break;
    case TK_GE: sw = 1; /* fallthrough */
    case TK_LE: o = isf ? OP_FLE : OP_LE; break;
    default:
      if (isf) fail(vm, "integer operator on floats");
      o = op == '%' ? OP_MOD : op == '&' ? OP_AND : op == '|' ? OP_OR : op == '^' ? OP_XOR : op == TK_SHL ? OP_SHL : OP_SHR;
    }
    ls = toslot(vm, l, 1); rs = toslot(vm, r, 1);
    if (sw) { d = ls; ls = rs; rs = d; }
  }
  vm->c->fr = l->t0 < r->t0 ? l->t0 : r->t0;
  d = alloc(vm, 1);
  emit(vm, o, d, ls, rs);
  l->k = EK_ST; l->a = d * 4; l->t = (uint16_t)(cmp ? TY_BOOL : lt); l->ro = 0; l->t0 = (uint16_t)d;
}
/* an array or struct made of one number type (Int or Float, any array nesting): that type, and
   how many numbers in *n; 0 if t is anything else */
static int vecinfo(Rio *vm, int t, int *n) {
  RioType *ty = TY(t); uint32_t c = 1; int k, i;
  if (ty->k != K_ARR && ty->k != K_STRUCT) return 0;
  while (ty->k == K_ARR) { c *= ty->n; ty = TY(ty->elem); }
  k = ty->k;
  if (k == K_STRUCT) {
    RioField *f = &vm->c->field[ty->f0];
    if (!ty->nf || ty->size != ty->nf * 4u) return 0;
    k = TY(f[0].t)->k;
    for (i = 1; i < ty->nf; i++) if (TY(f[i].t)->k != k) return 0;
    c *= ty->nf;
  }
  if (k != K_I32 && k != K_F32) return 0;
  *n = (int)c;
  return k == K_F32 ? TY_F32 : TY_I32;
}
/* l op r element by element, either side may be a scalar (used for every element); the result
   goes into `into` (a op= b), else into a temporary that replaces l */
static void vecarith(Rio *vm, int op, Ex *l, Ex *r, Ex *into) {
  int ln = 0, rn = 0, le, re, et, n, x, o, ls, rs, da, d = 0, t;
  needval(vm, l); needval(vm, r);
  le = vecinfo(vm, l->t, &ln); re = vecinfo(vm, r->t, &rn);
  if (!le && !re) fail(vm, "arithmetic needs numbers, or arrays and structs made of one number type");
  t = le ? l->t : r->t; et = le ? le : re; n = le ? ln : rn;
  if (le && re) { if (l->t != r->t) fail(vm, "type mismatch"); }
  else {
    Ex *s = le ? r : l;
    if (s->k == EK_CONST && vt(s->t) == TY_I32 && et == TY_F32) coerce(vm, s, TY_F32);
    if (vt(s->t) != et) fail(vm, "type mismatch");
  }
  x = op == '+' ? 0 : op == '-' ? 1 : op == '*' ? 2 : op == '/' ? 3 : op == '%' ? 4 : -1;
  if (x < 0) fail(vm, "arrays and number structs only do + - * / %");
  if (x == 4 && et == TY_F32) fail(vm, "integer operator on floats");
  o = (et == TY_F32 ? OP_FVADD : OP_VADD) + x;
  x = (le ? 0 : 16) | (re ? 0 : 32);
  ls = le ? toaddr(vm, l) : toslot(vm, l, 1);
  rs = re ? toaddr(vm, r) : toslot(vm, r, 1);
  if (into) da = into == l ? ls : toaddr(vm, into); /* a op= b: l is the target */
  else { /* at or below every operand's temporaries: safe, since elements are done in order */
    if (n > SMALLW) fail(vm, "too big for a temporary (over 16 words): change it in place with += -= *= /=");
    vm->c->fr = l->t0 < r->t0 ? l->t0 : r->t0; d = alloc(vm, n);
    da = kaddr(vm, (uint32_t)d * 4, (uint32_t)n * 4);
  }
  emit(vm, o, da, ls, rs); vm->code[vm->pc - 1].x = (uint8_t)x; emitw(vm, 0, (uint32_t)n);
  if (!into) { l->k = EK_ST; l->a = d * 4; l->off = 0; l->t = (uint16_t)t; l->ro = 0; l->t0 = (uint16_t)d; }
}
static void unop(Rio *vm, int op, Ex *e) {
  int t, s, d;
  needval(vm, e);
  if (op == '&') {
    if (e->k == EK_CONST || e->ro || (e->k == EK_ST && (e->a < (int32_t)vm->kcap * 4 || istemp(vm, e))))
      fail(vm, "& needs a variable, field or element that can be changed");
    e->ro = RO_REF;
    return;
  }
  if (op == '-' && (t = vecinfo(vm, e->t, &s)) != 0) { /* -v is 0 - v */
    Ex z = mkex(EK_CONST, t, 0); z.t0 = e->t0;
    vecarith(vm, '-', &z, e, 0); *e = z;
    return;
  }
  t = vt(e->t);
  if (op == '!' ? t != TY_BOOL : t != TY_I32 && !(t == TY_F32 && op == '-')) fail(vm, "bad operand for unary operator");
  if (e->k == EK_CONST) {
    RioVal v; v.i = e->a;
    if (t == TY_F32) v.f = -v.f; else v.i = op == '-' ? (int32_t)(0u - v.u) : op == '!' ? !v.i : ~v.i;
    e->a = v.i; e->ro = 0;
    return;
  }
  s = toslot(vm, e, 1);
  vm->c->fr = e->t0; d = alloc(vm, 1);
  emit(vm, op == '-' ? (t == TY_F32 ? OP_FNEG : OP_NEG) : op == '!' ? OP_NOT : OP_BNOT, d, s, 0);
  e->k = EK_ST; e->a = d * 4; e->t = (uint16_t)t; e->ro = 0;
}

/* ---------------------------------------------------------------- expression parser (shunting-yard) */
static Ex *vtop(Rio *vm) { return &vm->c->vs[vm->c->nvs - 1]; }
static Ex vpop(Rio *vm) { return vm->c->vs[--vm->c->nvs]; }
static void vpush(Rio *vm, Ex e) { if (vm->c->nvs >= (int)vm->c->lim.expr) fail(vm, "expression too complex"); vm->c->vs[vm->c->nvs++] = e; }
static Op *opush(Rio *vm, int k, int prec, int op) {
  Op *o;
  if (vm->c->nos >= (int)vm->c->lim.expr) fail(vm, "expression too complex");
  o = &vm->c->os[vm->c->nos++];
  o->k = (uint8_t)k; o->prec = (uint8_t)prec; o->op = (int16_t)op; o->a = o->b = o->c = o->n = o->pun = 0; o->set = 0;
  o->vb = (uint16_t)vm->c->nvs; o->fr0 = (uint16_t)vm->c->fr;
  return o;
}
static void vres(Rio *vm, Ex e, int t0) { e.t0 = (uint16_t)t0; vpush(vm, e); }
static int binprec(int t) {
  switch (t) {
  case TK_OR: return 1;
  case TK_AND: return 2;
  case TK_EQ: case TK_NE: case '<': case '>': case TK_LE: case TK_GE: return 3;
  case '+': case '-': case '|': case '^': return 4;
  case '*': case '/': case '%': case '&': case TK_SHL: case TK_SHR: return 5;
  }
  return 0;
}
static void reduce1(Rio *vm) {
  Op o = vm->c->os[--vm->c->nos]; Ex r;
  if (o.k == OK_UN) { unop(vm, o.op, vtop(vm)); return; }
  r = vpop(vm);
  if (o.k == OK_BIN) { binop(vm, o.op, vtop(vm), &r); return; }
  {
    int s, d = o.b;
    needval(vm, &r);
    if (r.t != TY_BOOL) fail(vm, "expected Bool");
    s = toslot(vm, &r, 1);
    vm->c->fr = (uint32_t)d + 1;
    if (s != d && !retarget(vm, s, d)) emit(vm, OP_MOV, d, s, 0);
    patch(vm, o.a, here(vm));
  }
}
static Ex symex(Rio *vm, int i) {
  RioSym *y = &vm->c->sym[i];
  switch (y->k) {
  case S_MOD: return mkex(EK_MOD, 0, y->v);
  case S_VAR: {
    Ex e = mkex(EK_ST, y->t, y->v);
    if (localmem(vm, &e) && (uint32_t)e.a < vm->hi) e.lv = (uint8_t)(vm->c->lvs[e.a / 4 >> 3] >> (e.a / 4 & 7) & 1);
    return e;
  }
  case S_CONST: { Ex e = mkex(TY(y->t)->k <= K_F32 || TY(y->t)->k == K_BOOL ? EK_CONST : EK_ST, y->t, y->v); e.ro = 1; return e; }
  case S_FN: if (!vm->c->func[y->v].done) fail(vm, "recursion is not allowed"); return mkex(EK_FN, 0, y->v);
  case S_SELF: case S_ROREF: { /* v: the slot holding the receiver's (or loop element's) address */
    Ex e = mkex(EK_MEM, y->t, y->v); e.ro = y->k == S_ROREF; return e;
  }
  case S_FFI: return mkex(EK_FFI, 0, y->v);
  case S_BI: return mkex(EK_BI, 0, y->v);
  default: return mkex(EK_TY, y->t, 0);
  }
}
static Ex ident(Rio *vm) {
  int i = lookup(vm, TK.s, TK.n);
  if (i < 0) failtok(vm, "undefined name");
  return symex(vm, i);
}
static Ex strconst(Rio *vm, const char *s, int n) {
  RioC *c = vm->c; RioVal *R = (RioVal *)vm->mem; uint8_t *d = c->strs + c->pool; uint32_t k = vm->nk; int i, j = 0; Ex e;
  if ((uint32_t)n > c->poolcap - c->pool) fail(vm, "out of compiler memory");
  if (k + 2 > vm->kcap) fail(vm, "too many constants");
  for (i = 0; i < n; i++) d[j++] = (uint8_t)(s[i] == '\\' && i + 1 < n ? esc(s[++i]) : s[i]);
  R[k].u = 0x80000000u | c->pool; R[k + 1].u = (uint32_t)j; c->kfix[k >> 3] |= (uint8_t)(1u << (k & 7));
  vm->nk += 2; c->pool += (uint32_t)j;
  e = mkex(EK_ST, TY_STR, (int32_t)k * 4); e.ro = 1;
  return e;
}
static Ex strlit(Rio *vm) { return strconst(vm, TK.s, TK.n); }
/* parse a -D value as a (signed) number literal using the lexer */
static int defnum(Rio *vm, const char *v, RioTok *t) {
  RioC *c = vm->c; const char *sp = c->sp, *se = c->se; int line = c->line, neg = *v == '-', ok;
  c->sp = v + neg; c->se = v + strlen(v);
  lex(vm, t);
  ok = (t->t == TK_INT || t->t == TK_FLT) && c->sp == c->se;
  c->sp = sp; c->se = se; c->line = line;
  if (ok && neg) { if (t->t == TK_INT) t->v.u = 0u - t->v.u; else t->v.f = -t->v.f; }
  return ok;
}
/* constant for define di as type t (t < 0: infer from the text) */
static Ex defval(Rio *vm, int di, int t) {
  const char *name = vm->defs[di][0], *v = vm->defs[di][1] ? vm->defs[di][1] : ""; int n = (int)strlen(v); RioTok k;
  if (t < 0) t = !n || !strcmp(v, "true") || !strcmp(v, "false") ? TY_BOOL : !defnum(vm, v, &k) ? TY_STR : k.t == TK_INT ? TY_I32 : TY_F32;
  if (t == TY_STR) return n >= 2 && v[0] == '"' && v[n - 1] == '"' ? strconst(vm, v + 1, n - 2) : strconst(vm, v, n);
  if (t == TY_BOOL) {
    if (!n || !strcmp(v, "true")) return mkex(EK_CONST, TY_BOOL, 1);
    if (!strcmp(v, "false")) return mkex(EK_CONST, TY_BOOL, 0);
    if (defnum(vm, v, &k) && k.t == TK_INT) return mkex(EK_CONST, TY_BOOL, k.v.i != 0);
  } else if (defnum(vm, v, &k) && (t == TY_F32 || k.t == TK_INT)) {
    if (t == TY_F32 && k.t == TK_INT) k.v.f = (float)k.v.i;
    return mkex(EK_CONST, t, k.v.i);
  }
  {
    char b[RIO_ERRBUF]; int m = cat(b, 0, "-D "); m = cat(b, m, name); m = cat(b, m, ": value doesn't fit ");
    m = cat(b, m, t == TY_I32 ? "Int" : t == TY_F32 ? "Float" : "Bool"); b[m] = 0;
    seterr(vm, RIO_ECOMPILE, 0, 0, 0, b);
    longjmp(vm->c->jb, 1);
  }
}
static void field(Rio *vm, Ex *e) {
  RioType *st; RioField *f = 0; int i;
  needval(vm, e); st = TY(e->t);
  if (st->k != K_STRUCT) fail(vm, "not a struct");
  for (i = 0; i < st->nf; i++) {
    f = &vm->c->field[st->f0 + i];
    if (f->len == TK.n && !memcmp(vm->c->names + f->name, TK.s, (size_t)TK.n)) break;
  }
  if (i == st->nf) fail(vm, "no such field");
  if (e->k == EK_ST) e->a += f->off; else e->off += f->off;
  e->t = f->t;
}
static int structfield(Rio *vm, RioType *ty, const char *s, int n) {
  int i;
  for (i = 0; ty->k == K_STRUCT && i < ty->nf; i++) {
    RioField *f = &vm->c->field[ty->f0 + i];
    if (f->len == n && !memcmp(vm->c->names + f->name, s, (size_t)n)) return i;
  }
  return -1;
}
static int convname(const char *s, int n) {
  int k;
  for (k = BI_TOINT; k <= BI_ASSTRING; k++) if ((int)strlen(bi[k]) == n && !memcmp(bi[k], s, (size_t)n)) return k;
  return -1;
}
/* v.x, v.zy, v.rgba on an array of 1-4 Ints, Floats or Bools: one letter is that element (it can be
   assigned), more build a new read-only array of those elements. 0: the name isn't a swizzle */
static int swizzle(Rio *vm, Ex *e) {
  RioType *ty = TY(e->t); int ix[4], j, n = TK.n, el = ty->elem, ek = TY(el)->k, d, mix = 0;
  if (ty->k != K_ARR || ty->n > 4 || (ek != K_I32 && ek != K_F32 && ek != K_BOOL) || n > 4) return 0;
  for (j = 0; j < n; j++) {
    const char *p = memchr("xyzwrgba", TK.s[j], 8);
    if (!p) return 0;
    ix[j] = (int)(p - "xyzwrgba"); mix |= ix[j] < 4 ? 1 : 2; ix[j] &= 3;
  }
  if (mix == 3) failtok(vm, "use xyzw or rgba, not both");
  for (j = 0; j < n; j++) if ((uint32_t)ix[j] >= ty->n) failtok(vm, "no such element: the array is shorter");
  if (n == 1) { if (e->k == EK_ST) e->a += ix[0] * 4; else e->off += ix[0] * 4; e->t = (uint16_t)el; return 1; }
  d = alloc(vm, n);
  for (j = 0; j < n; j++) {
    Ex s = *e, dd = mkex(EK_ST, el, (d + j) * 4);
    if (s.k == EK_ST) s.a += ix[j] * 4; else s.off += ix[j] * 4;
    s.t = (uint16_t)el; store(vm, &dd, &s);
  }
  e->k = EK_ST; e->a = d * 4; e->off = 0; e->t = (uint16_t)array_of(vm, el, (uint32_t)n); e->ro = 1;
  return 1;
}
static void member(Rio *vm) {
  Ex *e = vtop(vm), m; RioType *ty; int i, k;
  if (e->k == EK_MOD) { /* mod.name: one of the module's exported names */
    uint16_t t0 = e->t0;
    if ((i = modsym(vm, e->a, TK.s, TK.n)) < 0) fail(vm, "not exported by that module");
    *e = symex(vm, i); e->t0 = t0;
    return;
  }
  needval(vm, e); ty = TY(e->t);
  if (structfield(vm, ty, TK.s, TK.n) >= 0) { field(vm, e); return; }
  if (ty->k == K_ARR && swizzle(vm, e)) return;
  if (ty->k != K_STRUCT && ty->k != K_LIST && ty->k != K_BUILD && (k = convname(TK.s, TK.n)) >= 0) m = mkex(EK_BI, 0, k);
  else if (ty->k == K_LIST || ty->k == K_BUILD) {
    for (k = BI_PUSH; k <= BI_FORMAT; k++)
      if (k != BI_CAP && (int)strlen(bi[k]) == TK.n && !memcmp(bi[k], TK.s, (size_t)TK.n)) break;
    if (k > BI_FORMAT) fail(vm, "no such method");
    m = mkex(EK_BI, 0, k);
  } else {
    if ((i = lookup_meth(vm, vt(e->t), TK.s, TK.n)) < 0)
      fail(vm, ty->k == K_STRUCT ? "no such field or method" : ty->k == K_ARR ? "no such method (swizzles like .x .zy .rgba need 1-4 letters, on arrays of 1-4 numbers)" : "no such method");
    if (!vm->c->func[vm->c->sym[i].v].done) fail(vm, "recursion is not allowed");
    m = mkex(EK_FN, 0, vm->c->sym[i].v);
  }
  if (vm->c->nx.t != '(') fail(vm, "method calls need ()");
  m.ro = 2; /* bound: the receiver is just below on the stack */
  vpush(vm, m);
}
static void doindex(Rio *vm, Ex *o, Ex *i) {
  RioType *ty; int el, sz, s, ix, d, ro;
  needval(vm, o); needval(vm, i); ty = TY(o->t);
  o->lv = (uint8_t)localview(vm, o);
  if (ty->k != K_ARR && ty->k != K_SLICE && ty->k != K_LIST && ty->k != K_BUILD) fail(vm, "cannot index this");
  if (vt(i->t) != TY_I32) fail(vm, "index must be Int");
  el = ty->elem; sz = (int)TY(el)->size; ro = o->ro || o->t == TY_STR;
  if (ty->k == K_LIST || ty->k == K_BUILD) { /* checked against the current length */
    s = toslot3(vm, o); ix = toslot(vm, i, 1);
    vm->c->fr = o->t0; d = alloc(vm, 1);
    emit(vm, OP_LIDX, d, s, ix); emitw(vm, 0, (uint32_t)sz);
    o->k = EK_MEM; o->a = d; o->off = 0; o->t = (uint16_t)el; o->ro = 0;
    return;
  }
  if (ty->k == K_ARR && o->k == EK_ST && i->k == EK_CONST) {
    if ((uint32_t)i->a >= ty->n) fail(vm, "index out of bounds");
    o->a += i->a * sz; o->t = (uint16_t)el;
    return;
  }
  s = toslot2(vm, o); ix = toslot(vm, i, 1);
  vm->c->fr = o->t0; d = alloc(vm, 1);
  emit(vm, OP_IDX, d, s, ix); emitw(vm, 0, (uint32_t)sz);
  o->k = EK_MEM; o->a = d; o->off = 0; o->t = (uint16_t)el; o->ro = (uint8_t)ro;
}
static void doslice(Rio *vm, Ex *o, Ex *lo, Ex *hi) {
  RioType *ty; int s, l, h, d, lv;
  needval(vm, o); ty = TY(o->t); lv = localview(vm, o);
  if (ty->k == K_LIST || ty->k == K_BUILD) { /* a slice of the live elements */
    int b = toslot3(vm, o), v = alloc(vm, 2), t0 = o->t0;
    emit(vm, OP_LVIEW, v, b, 0);
    *o = mkex(EK_ST, slice_of(vm, ty->elem), v * 4); o->t0 = (uint16_t)t0; ty = TY(o->t);
  }
  if (ty->k != K_ARR && ty->k != K_SLICE) fail(vm, "cannot slice this");
  if (ty->k == K_ARR && istemp(vm, o)) fail(vm, "a computed array has no home to view: store it in a variable first");
  if (vt(lo->t) != TY_I32 || (hi->k != EK_LEN && vt(hi->t) != TY_I32)) fail(vm, "slice bounds must be Int");
  s = toslot2(vm, o); l = toslot(vm, lo, 1); h = hi->k == EK_LEN ? s + 1 : toslot(vm, hi, 1);
  vm->c->fr = o->t0; d = alloc(vm, 2);
  emit(vm, OP_SLICE, d, s, l); emitw(vm, h, TY(ty->elem)->size);
  o->k = EK_ST; o->a = d * 4; o->off = 0; o->lv = (uint8_t)lv;
  if (ty->k == K_ARR) o->t = (uint16_t)slice_of(vm, ty->elem);
}
static void punfield(Rio *vm, Op *m);
/* one argument of a call / struct literal is complete (on top of the value stack) */
static void argdone(Rio *vm, Op *m) {
  Ex *a = vtop(vm), e; RioParam *pp = 0;
  if (m->k == OK_CALL && m->a == EK_FN && m->n < vm->c->func[m->b].np) pp = &vm->c->param[vm->c->func[m->b].p0 + m->n];
  if (pp && pp->ref) {
    if (!(a->ro & RO_REF)) fail(vm, "this parameter is a reference: pass &x");
    a->ro = 0;
    if (a->t != pp->ref) fail(vm, "type mismatch");
    m->n++;
    return;
  }
  needval(vm, a);
  if (m->k == OK_LIT) {
    RioType *st = TY(m->b); RioField *f; Ex d;
    if (m->a < 0) punfield(vm, m);
    f = &vm->c->field[st->f0 + m->a];
    e = vpop(vm); d = mkex(EK_ST, f->t, m->c + f->off);
    store(vm, &d, &e);
    vm->c->fr = (uint32_t)(m->c / 4 + words(vm, m->b));
  } else if (m->a == EK_FN) {
    int k = TY(a->t)->k;
    if (m->n >= vm->c->func[m->b].np) fail(vm, "too many arguments");
    if (a->k == EK_MEM && k != K_STRUCT && k != K_ARR && k != K_LIST) {
      int s = k == K_SLICE || k == K_BUILD ? toslotv(vm, a) : toslot(vm, a, 1);
      a->k = EK_ST; a->a = s * 4; a->t = (uint16_t)vt(a->t);
    }
  } else if (m->a == EK_FFI) {
    RioCFfi *f = &vm->c->ffi[m->b]; int pt, w, s, d;
    if (m->n >= f->np) fail(vm, "too many arguments");
    e = vpop(vm); pt = vm->c->param[f->p0 + m->n].t;
    coerce(vm, &e, pt); w = words(vm, pt);
    s = w == 2 ? toslot2(vm, &e) : toslot(vm, &e, 1);
    vm->c->fr = (uint32_t)(m->fr0 + m->c); d = alloc(vm, w);
    if (s != d && !(w == 1 && retarget(vm, s, d))) emit(vm, w == 2 ? OP_MOV2 : OP_MOV, d, s, 0);
    m->c += w;
  } else if (m->a == EK_BI && m->b == BI_FORMAT) {
    int k;
    e = vpop(vm); k = TY(e.t)->k;
    if (m->n == 0) { /* remember the length so a format that doesn't fit can be undone */
      int t;
      if ((k != K_LIST && k != K_BUILD) || TY(e.t)->elem != TY_BYTE) fail(vm, "format needs a Blob list");
      m->c = toslot3(vm, &e); t = alloc(vm, 2);
      emit(vm, OP_LDW, t, m->c, 0); emit(vm, OP_MOV, t + 1, kslot(vm, 1), 0);
      m->set = (uint64_t)t;
    } else {
      int ok = (int)m->set + 1, s, d;
      if ((k == K_SLICE || k == K_ARR) && TY(e.t)->elem == TY_BYTE) {
        s = toslot2(vm, &e); d = alloc(vm, 1); emit(vm, OP_PUSHS, d, m->c, s); emitw(vm, 0, 1);
      } else if (k == K_I32 || k == K_F32 || k == K_BOOL || k == K_BYTE) {
        s = toslot(vm, &e, 1); d = alloc(vm, 1); emit(vm, OP_PUSHT, d, m->c, s);
        vm->code[vm->pc - 1].x = (uint8_t)(k == K_F32 ? 1 : k == K_BOOL ? 2 : 0);
      } else fail(vm, "format takes Strings, Blobs and numbers");
      emit(vm, OP_AND, ok, ok, d);
      vm->c->fr = e.t0;
    }
  } else if (m->a == EK_BI && m->b == BI_LOG) {
    int k;
    e = vpop(vm); k = TY(e.t)->k;
    if (k == K_SLICE && TY(e.t)->elem == TY_BYTE) emitx(vm, OP_LOGS, m->n > 0, toslot2(vm, &e));
    else if (k == K_BOOL) emitx(vm, OP_LOGB, m->n > 0, toslot(vm, &e, 1));
    else if (k == K_F32) emitx(vm, OP_LOGF, m->n > 0, toslot(vm, &e, 1));
    else if (k == K_I32 || k == K_BYTE) emitx(vm, OP_LOGI, m->n > 0, toslot(vm, &e, 1));
    else fail(vm, "cannot log this type");
    vm->c->fr = e.t0;
  }
  m->n++;
}
static void builtin(Rio *vm, Op *m) {
  int id = m->b, n = vm->c->nvs - m->vb, t, o = 0, x = 0, s1, s2 = 0, d;
  Ex *a = &vm->c->vs[m->vb], r;
  if (id == BI_LOG) { emit(vm, OP_LOGE, 0, 0, 0); vm->c->fr = m->fr0; vres(vm, mkex(EK_VOID, TY_VOID, 0), m->fr0); return; }
  if (n != (id >= BI_ATAN2 ? 2 : 1)) fail(vm, "wrong number of arguments");
  if (id == BI_LEN) {
    int k = TY(a->t)->k;
    if (k == K_ARR) r = mkex(EK_CONST, TY_I32, (int32_t)TY(a->t)->n);
    else if (k == K_SLICE) { r = *a; if (r.k == EK_ST) r.a += 4; else r.off += 4; r.t = TY_I32; r.ro = 1; }
    else if (k == K_LIST) { r = *a; r.t = TY_I32; r.ro = 1; } /* the length word comes first */
    else if (k == K_BUILD) { r = mkex(EK_MEM, TY_I32, toslot3(vm, a)); r.ro = 1; }
    else fail(vm, "len needs a slice, array or list");
    vm->c->nvs = m->vb; vres(vm, r, m->fr0);
    return;
  }
  t = vt(a->t);
  if (id == BI_ABS && t == TY_I32) {
    if (a->k == EK_CONST) { r = *a; r.a = a->a < 0 ? (int32_t)(0u - (uint32_t)a->a) : a->a; vm->c->nvs = m->vb; vres(vm, r, m->fr0); return; }
    o = OP_IABS;
  } else if (id == BI_ABS || (id >= BI_SQRT && id <= BI_ROUND)) {
    coerce(vm, a, TY_F32); o = OP_M1; x = id == BI_ABS ? 12 : id - BI_SQRT;
    if (a->k == EK_CONST) { RioVal v; v.i = a->a; v.f = mf1[x](v.f); r = mkex(EK_CONST, TY_F32, v.i); vm->c->nvs = m->vb; vres(vm, r, m->fr0); return; }
  } else {
    coerce(vm, a, TY_F32); coerce(vm, a + 1, TY_F32); o = OP_M2; x = id - BI_ATAN2;
    s2 = toslot(vm, a + 1, 1);
  }
  s1 = toslot(vm, a, 1);
  vm->c->nvs = m->vb; vm->c->fr = m->fr0; d = alloc(vm, 1);
  emit(vm, o, d, s1, s2); vm->code[vm->pc - 1].x = (uint8_t)x;
  vres(vm, mkex(EK_ST, o == OP_IABS ? TY_I32 : TY_F32, d * 4), m->fr0);
}
static void minmax(Rio *vm, Op *m) {
  Ex *a = &vm->c->vs[m->vb]; int t, s1, s2, d, mx = m->b == BI_MAX;
  if (vm->c->nvs - m->vb != 2) fail(vm, "wrong number of arguments");
  needval(vm, a); needval(vm, a + 1);
  if (a[0].k == EK_CONST && vt(a[1].t) == TY_F32) coerce(vm, a, TY_F32);
  if (a[1].k == EK_CONST && vt(a[0].t) == TY_F32) coerce(vm, a + 1, TY_F32);
  t = vt(a->t);
  if (t != vt(a[1].t) || (t != TY_I32 && t != TY_F32)) fail(vm, "type mismatch");
  s1 = toslot(vm, a, 1); s2 = toslot(vm, a + 1, 1);
  vm->c->nvs = m->vb; vm->c->fr = m->fr0; d = alloc(vm, 1);
  emit(vm, t == TY_I32 ? (mx ? OP_IMAX : OP_IMIN) : (mx ? OP_FMAX : OP_FMIN), d, s1, s2);
  vres(vm, mkex(EK_ST, t, d * 4), m->fr0);
}
/* push pop clear cap remove swapRemove pushAll */
static void listop(Rio *vm, Op *m) {
  static const signed char argc[] = {2, 1, 1, 1, 2, 2, 2};
  int id = m->b, k, el, b, d, s, sz, j; Ex *a = &vm->c->vs[m->vb], r = mkex(EK_VOID, TY_VOID, 0);
  if (vm->c->nvs - m->vb != argc[id - BI_PUSH]) fail(vm, "wrong number of arguments");
  needval(vm, a); k = TY(a->t)->k;
  if (id == BI_CAP) {
    if (k == K_LIST || k == K_ARR) r = mkex(EK_CONST, TY_I32, (int32_t)TY(a->t)->n);
    else if (k == K_BUILD) { r = mkex(EK_ST, TY_I32, (toslot3(vm, a) + 2) * 4); r.ro = 1; }
    else if (k == K_SLICE) { r = *a; if (r.k == EK_ST) r.a += 4; else r.off += 4; r.t = TY_I32; r.ro = 1; }
    else fail(vm, "cap needs a list, array or slice");
    vm->c->nvs = m->vb; vres(vm, r, m->fr0);
    return;
  }
  if (k != K_LIST && k != K_BUILD) fail(vm, "expected a list");
  el = TY(a->t)->elem; sz = (int)TY(el)->size; b = toslot3(vm, a);
  if (id == BI_CLEAR) emit(vm, OP_STW, b, kslot(vm, 0), 0);
  else if (id == BI_POP) {
    vm->c->fr = m->fr0; d = alloc(vm, 1);
    emit(vm, OP_POPA, d, b, 0); emitw(vm, 0, (uint32_t)sz);
    r = mkex(EK_MEM, el, d);
  } else if (id == BI_REMOVE || id == BI_SWAPREMOVE) {
    coerce(vm, a + 1, TY_I32); s = toslot(vm, a + 1, 1);
    emit(vm, id == BI_REMOVE ? OP_LREM : OP_LSWAP, b, s, 0); emitw(vm, 0, (uint32_t)sz);
  } else if (id == BI_PUSHALL) {
    Ex *x = a + 1; int xk;
    needval(vm, x); xk = TY(x->t)->k;
    if (xk == K_LIST || xk == K_BUILD) { int b2 = toslot3(vm, x); s = alloc(vm, 2); emit(vm, OP_LVIEW, s, b2, 0); }
    else if (xk == K_SLICE || xk == K_ARR) s = toslot2(vm, x);
    else fail(vm, "pushAll needs a slice, array or list");
    if (TY(x->t)->elem != el) fail(vm, "type mismatch");
    vm->c->fr = m->fr0; d = alloc(vm, 1);
    emit(vm, OP_PUSHS, d, b, s); emitw(vm, 0, (uint32_t)sz);
    r = mkex(EK_ST, TY_BOOL, d * 4);
  } else {
    Ex *x = a + 1;
    { /* reserve a slot (0 when full), store into it */
      Ex dst; int t = alloc(vm, 1);
      emit(vm, OP_PUSHA, t, b, 0); emitw(vm, 0, (uint32_t)sz);
      j = emit(vm, OP_JZ, t, 0, NONE);
      dst = mkex(EK_MEM, el, t); store(vm, &dst, x);
      patch(vm, j, here(vm));
      vm->c->fr = m->fr0; d = alloc(vm, 1); emit(vm, OP_NE, d, t, kslot(vm, 0));
      r = mkex(EK_ST, TY_BOOL, d * 4);
    }
  }
  if (r.k == EK_VOID) vm->c->fr = m->fr0;
  vm->c->nvs = m->vb; vres(vm, r, m->fr0);
}
/* x.toInt() x.toFloat() x.toBool() b.asString() */
static void convert(Rio *vm, Op *m) {
  Ex *a = &vm->c->vs[m->vb], r; int from, s, d;
  int to = m->b == BI_TOINT ? TY_I32 : m->b == BI_TOFLOAT ? TY_F32 : m->b == BI_TOBOOL ? TY_BOOL : TY_STR;
  if (vm->c->nvs - m->vb != 1) fail(vm, "conversions take no arguments");
  needval(vm, a); from = vt(a->t); r = *a;
  if (to == from) { /* no-op */ }
  else if (to == TY_STR && from == TY_BLOB) { r.t = TY_STR; r.ro = 1; }
  else if (to == TY_I32 && from == TY_BOOL) r.t = TY_I32;
  else if (to == TY_BOOL && from == TY_I32) {
    if (a->k == EK_CONST) r = mkex(EK_CONST, TY_BOOL, a->a != 0);
    else { s = toslot(vm, a, 1); vm->c->fr = m->fr0; d = alloc(vm, 1); emit(vm, OP_NE, d, s, kslot(vm, 0)); r = mkex(EK_ST, TY_BOOL, d * 4); }
  }
  else if ((to == TY_F32 && from == TY_I32) || (to == TY_I32 && from == TY_F32)) {
    if (a->k == EK_CONST) { RioVal v; v.i = a->a; if (to == TY_F32) v.f = (float)v.i; else v.i = ftoi(v.f); r = mkex(EK_CONST, to, v.i); }
    else { s = toslot(vm, a, 1); vm->c->fr = m->fr0; d = alloc(vm, 1); emit(vm, to == TY_F32 ? OP_ITOF : OP_FTOI, d, s, 0); r = mkex(EK_ST, to, d * 4); }
  } else fail(vm, "no such conversion");
  vm->c->nvs = m->vb; vres(vm, r, m->fr0);
}
static void finish_call(Rio *vm, Op *m) {
  if (m->a == EK_FN) {
    RioCFunc *f = &vm->c->func[m->b]; int i, d;
    if (m->n != f->np) fail(vm, "wrong number of arguments");
    for (i = 0; i < f->np; i++) {
      Ex p = mkex(EK_ST, vm->c->param[f->p0 + i].t, (int32_t)vm->c->param[f->p0 + i].addr);
      if ((!i && f->selfref) || vm->c->param[f->p0 + i].ref) { emit(vm, OP_MOV, (int)(p.a / 4), toaddr(vm, &vm->c->vs[m->vb + i]), 0); continue; }
      vm->c->argstore = 1; store(vm, &p, &vm->c->vs[m->vb + i]); vm->c->argstore = 0;
    }
    vm->c->nvs = m->vb;
    emit(vm, OP_CALL, 0, 0, f->pc);
    vm->c->fr = m->fr0;
    if (f->ret == TY_VOID) { vres(vm, mkex(EK_VOID, TY_VOID, 0), m->fr0); return; }
    {
      Ex src = mkex(EK_ST, f->ret, (int32_t)f->retaddr), dst;
      d = alloc(vm, words(vm, f->ret)); dst = mkex(EK_ST, f->ret, d * 4);
      store(vm, &dst, &src); vres(vm, dst, m->fr0);
    }
  } else if (m->a == EK_FFI) {
    RioCFfi *f = &vm->c->ffi[m->b];
    if (m->n != f->np) fail(vm, "wrong number of arguments");
    emit(vm, OP_FFI, m->fr0, 0, m->b);
    vm->c->fr = m->fr0;
    if (f->ret == TY_VOID) vres(vm, mkex(EK_VOID, TY_VOID, 0), m->fr0);
    else vres(vm, mkex(EK_ST, f->ret, alloc(vm, words(vm, f->ret)) * 4), m->fr0);
  } else if (m->a == EK_BI) {
    if (m->b == BI_MIN || m->b == BI_MAX) minmax(vm, m);
    else if (m->b == BI_FORMAT) {
      int t = (int)m->set, j;
      if (m->n < 1) fail(vm, "format needs a Blob list");
      j = emit(vm, OP_JNZ, t + 1, 0, NONE); emit(vm, OP_STW, m->c, t, 0); patch(vm, j, here(vm));
      vm->c->fr = (uint32_t)t + 2; vres(vm, mkex(EK_ST, TY_BOOL, (t + 1) * 4), m->fr0);
    } else if (m->b >= BI_TOINT) convert(vm, m);
    else if (m->b >= BI_PUSH) listop(vm, m);
    else builtin(vm, m);
  }
}
static void finish_lit(Rio *vm, Op *m) {
  RioType *st = TY(m->b); RioField *fs = &vm->c->field[st->f0]; int i = 0, j, w, a, b;
  while (st->nf <= 64 && i < st->nf) { /* (structs with more fields were zeroed up front) */
    if (m->set >> i & 1) { i++; continue; }
    for (j = i; j < st->nf && !(m->set >> j & 1); j++) {}
    a = fs[i].off; b = j < st->nf ? fs[j].off : (int)st->size;
    if (b - a <= 64) for (w = a / 4; w < b / 4; w++) emit(vm, OP_MOV, m->c / 4 + w, kslot(vm, 0), 0);
    else emit(vm, OP_ZERO, kaddr(vm, (uint32_t)(m->c + a), (uint32_t)(b - a)), b - a, 0);
    i = j;
  }
  vm->c->fr = (uint32_t)(m->c / 4 + words(vm, m->b));
  vres(vm, mkex(EK_ST, m->b, m->c), m->fr0);
}
/* inside Type{...}: each value must be preceded by `field =` */
static void setfield(Rio *vm, Op *m, const char *s, int n) {
  int i = structfield(vm, TY(m->b), s, n);
  if (i < 0) fail(vm, "no such field");
  if (i < 64 && (m->set >> i & 1)) fail(vm, "field set twice");
  if (i < 64) m->set |= (uint64_t)1 << i;
  m->a = i;
}
static void litfield(Rio *vm, Op *m) {
  if (TK.t == '}') return;
  if (TK.t != TK_ID) fail(vm, "struct literal fields must be named: Type{field = value}");
  if (vm->c->nx.t == '=') { setfield(vm, m, TK.s, TK.n); next(vm); next(vm); return; }
  m->a = -1; m->pun = (int32_t)(TK.s - vm->c->src); /* {x} or {a.x}: named after the path's last name */
}
/* a punned value must be a name or a dotted path; its last name is the field */
static void punfield(Rio *vm, Op *m) {
  const char *p = vm->c->src + m->pun, *e = TK.s, *nm = 0; int nn = 0;
  while (e > p && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) e--;
  while (p < e) {
    if (isal(*p)) { nm = p; while (p < e && (isal(*p) || isdg(*p))) p++; nn = (int)(p - nm); }
    else if (*p == '.' || *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    else if (p + 1 < e && p[0] == '/' && p[1] == '/') { while (p < e && *p != '\n') p++; }
    else fail(vm, "only a name or a.b path can stand for a field; write field = value");
  }
  setfield(vm, m, nm, nn);
}
static void closer(Rio *vm, int t, int hasarg) {
  Op *m = &vm->c->os[vm->c->nos - 1], o;
  if (t == ')' && m->k == OK_PAREN) { vm->c->nos--; return; }
  if (t == ']') {
    Ex i, hi;
    if (m->k != OK_IDX) fail(vm, "unexpected ']'");
    o = vm->c->os[--vm->c->nos];
    if (o.a) { hi = vpop(vm); i = vpop(vm); doslice(vm, vtop(vm), &i, &hi); }
    else { i = vpop(vm); doindex(vm, vtop(vm), &i); }
    return;
  }
  if (m->k != (t == ')' ? OK_CALL : OK_LIT)) fail(vm, t == ')' ? "unexpected ')'" : "unexpected '}'");
  if (hasarg) argdone(vm, m);
  o = vm->c->os[--vm->c->nos];
  if (t == ')') finish_call(vm, &o); else finish_lit(vm, &o);
}
/* the type a value is headed for right now, or -1: what the whole expression is for, or the
   argument / field being filled in by the innermost call or struct literal */
static int target(Rio *vm, int ob, int whole) {
  Op *m;
  if (vm->c->nos == ob) return whole;
  m = &vm->c->os[vm->c->nos - 1];
  if (m->k == OK_LIT) return vm->c->field[TY(m->b)->f0 + m->a].t;
  if (m->k == OK_CALL && m->a == EK_FN && m->n < vm->c->func[m->b].np) return vm->c->param[vm->c->func[m->b].p0 + m->n].t;
  if (m->k == OK_CALL && m->a == EK_BI && m->b == BI_PUSH && m->n == 1) return TY(vm->c->vs[m->vb].t)->elem;
  return -1;
}
static Ex expr(Rio *vm) {
  int ob = vm->c->nos, vb = vm->c->nvs, want = 1, depth = 0, t, p, whole = vm->c->target;
  vm->c->target = -1;
  for (;;) {
    t = TK.t;
    if (want) {
      Ex e; int t0 = (int)vm->c->fr;
      if (t == TK_INT || t == TK_FLT) e = mkex(EK_CONST, t == TK_INT ? TY_I32 : TY_F32, TK.v.i);
      else if (t == TK_STR) e = strlit(vm);
      else if (t == TK_ID) e = ident(vm);
      else if (t == '(') { opush(vm, OK_PAREN, 0, 0); depth++; next(vm); continue; }
      else if (t == '-' || t == '!' || t == '~' || t == '&') { opush(vm, OK_UN, 6, t); next(vm); continue; }
      else if ((t == ')' || t == '}') && vm->c->nos > ob && vm->c->os[vm->c->nos - 1].k == (t == ')' ? OK_CALL : OK_LIT) &&
               (t == '}' || vm->c->os[vm->c->nos - 1].n == 0) && vm->c->nvs == vm->c->os[vm->c->nos - 1].vb) {
        closer(vm, t, 0); depth--; next(vm); want = 0; continue;
      } else if (t == '{') { /* {field = ...} takes its struct type from where the value is going */
        int tt = target(vm, ob, whole);
        if (tt < 0 || TY(tt)->k != K_STRUCT) failtok(vm, "can't tell which struct this is; write Type{...}");
        vres(vm, mkex(EK_TY, tt, 0), t0); want = 0; continue;
      } else failtok(vm, M_EXPR);
      vres(vm, e, t0); next(vm); want = 0;
      continue;
    }
    if (TK.nl && !depth) break;
    if (t == '(') {
      Ex c = *vtop(vm); Op *o;
      if (c.ro == 2 && (c.k == EK_FN || c.k == EK_BI)) { /* receiver.method( */
        vm->c->nvs--;
        o = opush(vm, OK_CALL, 0, 0); o->a = c.k; o->b = c.a;
        o->vb = (uint16_t)(vm->c->nvs - 1); o->fr0 = vtop(vm)->t0;
        if (c.k == EK_BI && c.a == BI_FORMAT) argdone(vm, o); else o->n = 1;
        depth++; next(vm);
        if (TK.t == ')') { closer(vm, ')', 0); depth--; next(vm); want = 0; } else want = 1;
        continue;
      }
      if (c.k < EK_FN || c.k > EK_TY) fail(vm, "not callable");
      if (c.k == EK_TY) fail(vm, TY(c.t)->k == K_STRUCT ? "use Struct{...} to build a struct" : "convert with x.toInt(), x.toFloat(), x.toBool() or b.asString()");
      vm->c->nvs--;
      o = opush(vm, OK_CALL, 0, 0); o->a = c.k; o->b = c.k == EK_TY ? c.t : c.a;
      depth++; next(vm); want = 1;
    } else if (t == '[') {
      opush(vm, OK_IDX, 0, 0); depth++; next(vm);
      if (TK.t == ':') { vres(vm, mkex(EK_CONST, TY_I32, 0), (int)vm->c->fr); want = 0; } else want = 1;
    } else if (t == '.') {
      next(vm);
      if (TK.t != TK_ID) fail(vm, "field name expected");
      member(vm); next(vm);
    } else if (t == '{') {
      Ex c = *vtop(vm); Op *o;
      if (c.k != EK_TY || TY(c.t)->k != K_STRUCT) break;
      vm->c->nvs--;
      o = opush(vm, OK_LIT, 0, 0); o->b = c.t; o->c = alloc(vm, words(vm, c.t)) * 4;
      if (TY(c.t)->nf > 64) emit(vm, OP_ZERO, kaddr(vm, (uint32_t)o->c, TY(c.t)->size), (int)TY(c.t)->size, 0);
      depth++; next(vm); litfield(vm, o); want = 1;
    } else if (t == ',' || t == ')' || t == ']' || t == '}' || t == ':') {
      Op *m;
      if (!depth) break;
      while (vm->c->os[vm->c->nos - 1].k < OK_PAREN) reduce1(vm);
      m = &vm->c->os[vm->c->nos - 1];
      if (t == ',') {
        if (m->k != OK_CALL && m->k != OK_LIT) fail(vm, "unexpected ','");
        argdone(vm, m); next(vm);
        if (m->k == OK_LIT) litfield(vm, m);
        want = 1;
      } else if (t == ':') {
        if (m->k != OK_IDX || m->a) fail(vm, "unexpected ':'");
        m->a = 1; next(vm);
        if (TK.t == ']') { vres(vm, mkex(EK_LEN, TY_I32, 0), (int)vm->c->fr); want = 0; } else want = 1;
      } else { closer(vm, t, 1); depth--; next(vm); }
    } else {
      if (!(p = binprec(t))) break;
      while (vm->c->nos > ob && vm->c->os[vm->c->nos - 1].k < OK_PAREN && vm->c->os[vm->c->nos - 1].prec >= p) reduce1(vm);
      if (t == TK_AND || t == TK_OR) {
        Ex *l = vtop(vm); int s, d; Op *o;
        needval(vm, l);
        if (l->t != TY_BOOL) fail(vm, "expected Bool");
        s = toslot(vm, l, 1); vm->c->fr = l->t0; d = alloc(vm, 1);
        if (s != d) emit(vm, OP_MOV, d, s, 0);
        o = opush(vm, t == TK_AND ? OK_AND : OK_OR, p, t);
        o->a = emit(vm, t == TK_AND ? OP_JZ : OP_JNZ, d, 0, NONE); o->b = d;
        l->k = EK_ST; l->a = d * 4; l->t = TY_BOOL; l->ro = 0;
      } else opush(vm, OK_BIN, p, t);
      next(vm); want = 1;
    }
  }
  while (vm->c->nos > ob) {
    if (vm->c->os[vm->c->nos - 1].k >= OK_PAREN) fail(vm, M_BRACKET);
    reduce1(vm);
  }
  if (vm->c->nvs != vb + 1) fail(vm, "bad expression");
  return vpop(vm);
}
static int32_t constexpr_i(Rio *vm) {
  Ex e = expr(vm);
  if (e.k != EK_CONST || e.t != TY_I32 || e.a <= 0) fail(vm, "positive integer constant expected");
  return e.a;
}

/* ---------------------------------------------------------------- statements */
static int parse_type(Rio *vm) {
  int32_t pre[16]; int np = 0, t, i;
  while (TK.t == '[') {
    if (np >= 16) fail(vm, "type too deep");
    next(vm);
    if (TK.t == ']') pre[np++] = -1;
    else if (TK.t == '.' && vm->c->nx.t == '.') { next(vm); next(vm); pre[np++] = TK.t == ']' ? -2 : -3 - constexpr_i(vm); }
    else pre[np++] = constexpr_i(vm);
    expect(vm, ']', "']' expected");
  }
  if (TK.t != TK_ID || (i = lookup(vm, TK.s, TK.n)) < 0) failtok(vm, "type expected");
  while (vm->c->sym[i].k == S_MOD) { /* mod.Type */
    int m = vm->c->sym[i].v;
    next(vm);
    if (TK.t != '.') fail(vm, "'.' expected");
    next(vm);
    if (TK.t != TK_ID) failtok(vm, "type expected");
    if ((i = modsym(vm, m, TK.s, TK.n)) < 0) failtok(vm, "not exported by that module");
  }
  if (vm->c->sym[i].k != S_TYPE) failtok(vm, "type expected");
  t = vm->c->sym[i].t; next(vm);
  if (t == TY_BLOB && TK.t == '[' && !TK.nl) {
    next(vm);
    if (TK.t == '.' && vm->c->nx.t == '.') { next(vm); next(vm); t = TK.t == ']' ? build_of(vm, TY_BYTE) : list_of(vm, TY_BYTE, (uint32_t)constexpr_i(vm)); }
    else t = array_of(vm, TY_BYTE, (uint32_t)constexpr_i(vm));
    expect(vm, ']', "']' expected");
  }
  while (np) {
    int32_t n = pre[--np];
    t = n == -1 ? slice_of(vm, t) : n == -2 ? build_of(vm, t) : n < -2 ? list_of(vm, t, (uint32_t)(-3 - n)) : array_of(vm, t, (uint32_t)n);
  }
  return t;
}
static RioBlk *bpush(Rio *vm, int k) {
  RioBlk *b;
  if (vm->c->nblk >= (int)vm->c->lim.blocks) fail(vm, "blocks nested too deep");
  b = &vm->c->blk[vm->c->nblk++];
  b->k = (uint8_t)k; b->nsym = (uint16_t)vm->c->nsym; b->nnames = (uint16_t)vm->c->nnames; b->nact = (uint16_t)vm->c->nact;
  b->a = b->b = b->brk = b->cont = b->cj = NONE;
  return b;
}
/* a block's names end and its slots get reused, except any whose address was taken: a view of them
   may outlive the block, so they keep their memory (and their one type) for good */
static void bscope(Rio *vm, RioBlk *b) {
  uint32_t s, top = b->nact;
  for (s = b->nact; s < vm->c->nact; s++) if (vm->c->exposed[s >> 3] & (1u << (s & 7))) top = s + 1;
  vm->c->nsym = b->nsym; vm->c->nnames = b->nnames; vm->c->nact = vm->c->fr = top;
}
static int namelist(Rio *vm, const char **ns, int *nl) {
  int c = 0;
  for (;;) {
    if (TK.t != TK_ID || c >= 16) fail(vm, "name expected");
    ns[c] = TK.s; nl[c++] = TK.n; next(vm);
    if (TK.t != ',') break;
    next(vm);
  }
  expect(vm, ':', "':' expected");
  return c;
}
static int isbig(RioType *ty) { return (ty->k == K_ARR || ty->k == K_STRUCT || ty->k == K_LIST) && ty->size > SMALLW * 4; }
static void proc_def(Rio *vm, const char *name, int nlen, int recv) {
  RioCFunc *f; RioBlk *b; int fi, skip;
  if (vm->c->nblk || vm->c->curfn >= 0) fail(vm, "procs must be top-level");
  if (vm->nfunc >= (int)vm->c->lim.procs) fail(vm, "too many procs");
  next(vm); expect(vm, '(', "'(' expected");
  skip = emit(vm, OP_JMP, 0, 0, NONE);
  fi = vm->nfunc++; f = &vm->c->func[fi];
  f->p0 = (uint16_t)vm->c->nparam; f->np = 0; f->done = 0; f->ret = TY_VOID; f->retaddr = 0; f->big = 0; f->shared = 0; f->hi0 = vm->hi;
  addsym(vm, name, nlen, recv < 0 ? S_FN : S_METH, recv < 0 ? 0 : recv, fi);
  f->selfref = (uint8_t)(recv >= 0 && TY(recv)->k == K_STRUCT);
  b = bpush(vm, B_PROC); b->a = (uint16_t)skip; b->b = (uint16_t)fi;
  /* a frame starts above every slot used so far, so each slot has one owner (top-level code or one
     proc): frame sharing moves a proc's slots, and must not move a dead top-level block's with them */
  vm->c->nact = vm->c->fr = vm->c->hwm; f->fs = (uint16_t)vm->c->nact;
  if (recv >= 0) { /* self: a struct receiver by address, other receivers as a copy */
    int addr = alloc(vm, f->selfref ? 1 : words(vm, recv)) * 4;
    if (vm->c->nparam >= (int)vm->c->lim.params) fail(vm, "too many parameters");
    vm->c->param[vm->c->nparam].t = (uint16_t)(f->selfref ? TY_I32 : recv); vm->c->param[vm->c->nparam].ref = 0;
    vm->c->param[vm->c->nparam++].addr = (uint32_t)addr;
    addsym(vm, "self", 4, f->selfref ? S_SELF : S_VAR, recv, f->selfref ? addr / 4 : addr); f->np++;
  }
  while (TK.t != ')') {
    const char *ns[16]; int nl[16], c = namelist(vm, ns, nl), isref = TK.t == '&', t, j;
    RioType *ty;
    if (isref) next(vm);
    t = parse_type(vm); ty = TY(t);
    if (ty->k == K_VOID || (!isref && (ty->k == K_LIST || ((ty->k == K_STRUCT || ty->k == K_ARR) && ty->ref))))
      fail(vm, "invalid parameter type (pass lists as [..]T)");
    if (!isref && isbig(ty)) fail(vm, "a big value (over 16 words) goes by reference: use &T, or []T for an array");
    for (j = 0; j < c; j++) { /* a & parameter is one word holding the address, read through like self */
      int addr = alloc(vm, isref ? 1 : words(vm, t)) * 4;
      if (vm->c->nparam >= (int)vm->c->lim.params) fail(vm, "too many parameters");
      vm->c->param[vm->c->nparam].t = (uint16_t)(isref ? TY_I32 : t); vm->c->param[vm->c->nparam].ref = (uint16_t)(isref ? t : 0);
      vm->c->param[vm->c->nparam++].addr = (uint32_t)addr;
      addsym(vm, ns[j], nl[j], isref ? S_SELF : S_VAR, t, isref ? addr / 4 : addr); f->np++;
    }
    if (TK.t != ',') break;
    next(vm);
  }
  expect(vm, ')', "')' expected");
  if (TK.t == TK_ARROW) {
    RioType *ty;
    next(vm); f->ret = (uint16_t)parse_type(vm); ty = TY(f->ret);
    if (ty->k == K_LIST || ((ty->k == K_ARR || ty->k == K_STRUCT) && ty->ref)) fail(vm, "invalid return type");
    if (isbig(ty)) fail(vm, "a big value (over 16 words) can't be returned: fill in a &T parameter instead");
    f->retaddr = (uint32_t)alloc(vm, words(vm, f->ret)) * 4;
  }
  vm->c->nact = vm->c->fr; vm->c->curfn = fi; f->pc = (uint16_t)here(vm);
}
/* Type.name :: proc(...) */
static void method_def(Rio *vm, int recv) {
  const char *ns; int nn;
  next(vm); next(vm);
  if (TK.t != TK_ID) fail(vm, "method name expected");
  ns = TK.s; nn = TK.n;
  if (structfield(vm, TY(recv), ns, nn) >= 0) failtok(vm, "a method can't have the same name as a field");
  if (lookup_meth(vm, recv, ns, nn) >= 0) failtok(vm, "method already defined");
  if (TY(recv)->k != K_STRUCT && convname(ns, nn) >= 0) failtok(vm, "that conversion is built in");
  next(vm);
  if (TK.t == '*') { vm->c->exporting = 1; next(vm); }
  expect(vm, TK_DCOLON, "'::' expected");
  if (TK.t != TK_PROC) fail(vm, "'proc' expected");
  proc_def(vm, ns, nn, recv);
}
static void struct_def(Rio *vm, const char *name, int nlen) {
  int ti, off = 0; RioType *st;
  if (vm->c->nblk || vm->c->curfn >= 0) fail(vm, "structs must be top-level");
  next(vm);
  ti = newtype(vm, K_STRUCT, 0, 0, 0); st = TY(ti); st->f0 = (uint16_t)vm->c->nfield;
  while (TK.t != TK_END) {
    const char *ns[16]; int nl[16], c = namelist(vm, ns, nl), t = parse_type(vm), j;
    if (TY(t)->k == K_VOID) fail(vm, "invalid field type");
    for (j = 0; j < c; j++) {
      RioField *f;
      if (vm->c->nfield >= (int)vm->c->lim.fields) fail(vm, "too many fields");
      f = &vm->c->field[vm->c->nfield++];
      f->name = (uint16_t)addname(vm, ns[j], nl[j]); f->len = (uint16_t)nl[j]; f->t = (uint16_t)t; f->off = (uint16_t)off;
      off += (int)TY(t)->size; st->nf++; st->ref |= TY(t)->ref;
      if (off > 0xFFFF) fail(vm, "struct too large");
    }
  }
  next(vm);
  st->size = (uint32_t)off;
  addsym(vm, name, nlen, S_TYPE, ti, 0);
}
/* declare a variable of type t (-1: from the value), initialized from *pe or zeroed. adopt: the
   value may already sit in the next free slot and simply become the variable */
/* name: [..]T = an array or slice: a builder over that memory, starting empty. Its length word is a
   fourth slot right after the builder's three */
static void buildover(Rio *vm, const char *ns, int nn, int t, Ex *e) {
  int s, d;
  if (TY(e->t)->elem != TY(t)->elem) fail(vm, "type mismatch");
  if (e->ro || e->t == TY_STR) fail(vm, "can't build on read-only data");
  if (TY(e->t)->k == K_ARR && istemp(vm, e)) fail(vm, "a computed array has no home to view: store it in a variable first");
  s = toslot2(vm, e);
  vm->c->fr = vm->c->nact; d = alloc(vm, 4); vm->c->nact = vm->c->fr;
  emit(vm, OP_MOV2, d + 1, s, 0); /* first: s may sit where the builder now goes */
  emit(vm, OP_MOV, d, kaddr(vm, (uint32_t)(d + 3) * 4, 4), 0);
  emit(vm, OP_MOV, d + 3, kslot(vm, 0), 0);
  if (vm->c->curfn >= 0) setlv(vm, (uint32_t)d * 4, 3, 1); /* its length word is in this frame */
  addsym(vm, ns, nn, S_VAR, t, d * 4);
}
static void declare(Rio *vm, const char *ns, int nn, int t, Ex *pe, int adopt) {
  int has = pe != 0, k, big, lv = 0; uint32_t addr; RioType *ty; Ex e = has ? *pe : mkex(EK_VOID, TY_VOID, 0);
  if (has) {
    needval(vm, &e); lv = localview(vm, &e);
    if (t >= 0 && TY(t)->k == K_BUILD && (TY(e.t)->k == K_ARR || TY(e.t)->k == K_SLICE)) { buildover(vm, ns, nn, t, &e); return; }
    if (t < 0) {
      t = vt(e.t);
      if (TY(t)->k == K_LIST) t = build_of(vm, TY(t)->elem);
      else if (isbig(TY(t)) && !istemp(vm, &e))
        fail(vm, TY(t)->k == K_ARR ? "this copies a big array (over 16 words): write its type to copy it (x: [N]T = a), or view it with a[:]"
                                   : "this copies a big struct (over 16 words): write its type to copy it (x: T = s), or use &s");
    }
    coerce(vm, &e, t);
  }
  ty = TY(t); k = ty->k;
  if (k == K_VOID) fail(vm, "variable has no type");
  if (vm->c->curfn >= 0 && (k == K_STRUCT || k == K_ARR || k == K_LIST) && ty->ref) fail(vm, "structs/arrays/lists holding slices or strings must be global");
  big = isbig(ty);
  if (big) addr = halloc(vm, ty->size);
  else {
    if (has && e.k == EK_MEM && (k == K_I32 || k == K_F32 || k == K_BOOL || k == K_SLICE || k == K_BUILD)) {
      int s = k == K_SLICE || k == K_BUILD ? toslotv(vm, &e) : toslot(vm, &e, 1);
      e.k = EK_ST; e.a = s * 4; e.t = (uint16_t)t;
    }
    if (adopt && has && e.k == EK_ST && e.a == (int32_t)vm->c->nact * 4 && !e.ro) {
      vm->c->nact += (uint32_t)words(vm, t); vm->c->fr = vm->c->nact;
      if ((k == K_SLICE || k == K_BUILD) && vm->c->curfn >= 0) setlv(vm, (uint32_t)e.a, words(vm, t), lv);
      addsym(vm, ns, nn, S_VAR, t, e.a);
      return;
    }
    addr = (uint32_t)alloc(vm, words(vm, t)) * 4;
  }
  if (has) { Ex d = mkex(EK_ST, t, (int32_t)addr); store(vm, &d, &e); }
  else if (!big && addr < 0x40000) { int w; for (w = 0; w < words(vm, t); w++) emit(vm, OP_MOV, (int)(addr >> 2) + w, kslot(vm, 0), 0); }
  else if (!(big && vm->c->curfn < 0 && !vm->c->nblk)) emit(vm, OP_ZERO, kaddr(vm, addr, ty->size), (int)(ty->size & 0xFFFF), (int)(ty->size >> 16));
  if (!big) vm->c->nact = vm->c->fr; /* after the store, so it can still write straight into the variable */
  if ((k == K_SLICE || k == K_BUILD) && vm->c->curfn >= 0 && !big) setlv(vm, addr, words(vm, t), lv);
  addsym(vm, ns, nn, S_VAR, t, (int32_t)addr);
}
static void decl_var(Rio *vm) {
  const char *ns = TK.s; int nn = TK.n, t = -1, has = 0; Ex e;
  next(vm);
  if (TK.t == ':') { next(vm); t = parse_type(vm); if (TK.t == '=') { next(vm); has = 1; } }
  else { next(vm); has = 1; }
  if (!has) { declare(vm, ns, nn, t, 0, 1); return; }
  vm->c->target = t; e = expr(vm);
  if (e.ro & RO_REF) { /* name := &place: bound once to that place, like a for-each element */
    int s, d;
    if (t >= 0) fail(vm, "a reference takes its type from its place: write name := &x");
    if (vm->c->exporting) fail(vm, "a reference can't be exported");
    e.ro = 0; s = toaddr(vm, &e);
    vm->c->fr = vm->c->nact; d = alloc(vm, 1); vm->c->nact = vm->c->fr;
    if (s != d) emit(vm, OP_MOV, d, s, 0);
    addsym(vm, ns, nn, S_SELF, e.t, d);
    return;
  }
  declare(vm, ns, nn, t, &e, 1);
}
/* {a, b as c} := value: new variables holding copies of some of a struct's fields */
static void destructure(Rio *vm) {
  const char *fs[32], *ls[32]; int fn[32], ln[32], n = 0, i; Ex e;
  next(vm);
  while (TK.t != '}') {
    if (TK.t != TK_ID) fail(vm, "field name expected");
    if (n >= 32) fail(vm, "too many names");
    fs[n] = ls[n] = TK.s; fn[n] = ln[n] = TK.n; next(vm);
    if (TK.t == TK_ID && TK.n == 2 && !memcmp(TK.s, "as", 2)) { /* 'as' only means something here */
      next(vm);
      if (TK.t != TK_ID) fail(vm, "name expected after 'as'");
      ls[n] = TK.s; ln[n] = TK.n; next(vm);
    }
    n++;
    if (TK.t != ',') break;
    next(vm);
  }
  expect(vm, '}', "'}' expected");
  if (TK.t != TK_DECL) fail(vm, "':=' expected: destructuring declares new names");
  next(vm);
  e = expr(vm); needval(vm, &e);
  if (TY(e.t)->k != K_STRUCT) fail(vm, "only a struct can be destructured");
  for (i = 0; i < n; i++) {
    RioType *st = TY(e.t); Ex fe = e; int fi = structfield(vm, st, fs[i], fn[i]), k; RioField *f;
    if (fi < 0) fail(vm, "no such field to destructure");
    f = &vm->c->field[st->f0 + fi];
    if (fe.k == EK_ST) fe.a += f->off; else fe.off += f->off;
    fe.t = f->t; k = TY(f->t)->k;
    if (fe.k == EK_MEM && k != K_STRUCT && k != K_ARR && k != K_LIST) { /* load without disturbing the struct's address */
      int s = k == K_SLICE || k == K_BUILD ? toslotv(vm, &fe) : toslot(vm, &fe, 0);
      fe = mkex(EK_ST, vt(f->t), s * 4);
    }
    declare(vm, ls[i], ln[i], -1, &fe, 0);
  }
}
static void autoprint(Rio *vm, Ex *e) {
  int k = TY(e->t)->k;
  if (k == K_SLICE && TY(e->t)->elem == TY_BYTE) emitx(vm, OP_LOGS, 0, toslot2(vm, e));
  else if (k == K_F32) emitx(vm, OP_LOGF, 0, toslot(vm, e, 1));
  else if (k == K_BOOL) emitx(vm, OP_LOGB, 0, toslot(vm, e, 1));
  else if (k == K_I32 || k == K_BYTE) emitx(vm, OP_LOGI, 0, toslot(vm, e, 1));
  else return;
  emit(vm, OP_LOGE, 0, 0, 0);
}
static void stmt_expr(Rio *vm) {
  Ex l = expr(vm), r, cur;
  if (TK.t != '=' && TK.t != TK_OPEQ) {
    if (vm->repl && vm->c->curfn < 0 && !vm->c->nblk && l.k <= EK_MEM) autoprint(vm, &l);
    return;
  }
  if ((l.k != EK_ST && l.k != EK_MEM) || l.ro || (l.k == EK_ST && l.a < (int32_t)vm->kcap * 4)) fail(vm, "cannot assign to this");
  if (TK.t == '=') { next(vm); vm->c->target = l.t; r = expr(vm); store(vm, &l, &r); return; }
  {
    int op = TK.op, k = TY(l.t)->k;
    next(vm);
    if (k == K_ARR || k == K_STRUCT) { r = expr(vm); vecarith(vm, op, &l, &r, &l); return; }
    cur = l;
    if (!(l.k == EK_ST && (k == K_I32 || k == K_F32) && l.a < 0x40000)) {
      int s = toslot(vm, &cur, 0);
      cur = mkex(EK_ST, vt(l.t), s * 4); cur.t0 = (uint16_t)s;
    }
    r = expr(vm); binop(vm, op, &cur, &r); store(vm, &l, &cur);
  }
}
/* the token after the lookahead, without consuming anything */
static int peek3(Rio *vm) {
  RioC *c = vm->c; const char *sp = c->sp, *ls = c->ls; int line = c->line; RioTok t;
  lex(vm, &t);
  c->sp = sp; c->ls = ls; c->line = line;
  return t.t;
}
static int findfile(Rio *vm, int pkg, int inc, const char *key, int n) {
  int k;
  for (k = 2; k < vm->c->nmod; k++) {
    RioMod *m = &vm->c->mods[k];
    if (m->pkg == pkg && m->inc == inc && m->keylen == n && !memcmp(vm->c->names + m->key, key, (size_t)n)) return k;
  }
  return -1;
}
/* start reading another file (a module to import, or a part to include) after saving where to
   resume. No recursion: compile_all's loop switches files. */
static void openfile(Rio *vm, int pkg, int inc, char *key, int klen, const char *pos, const char *ls, int line) {
  RioC *c = vm->c; RioSource s; RioMod *m; RioImp *f; int r, dl; char b[300];
  if (c->nis >= RIO_MAX_IMPORT_DEPTH) fail(vm, "imports and includes nested too deep");
  if (c->nmod >= (int)c->lim.modules) fail(vm, "too many modules");
  key[klen] = 0;
  memset(&s, 0, sizeof s);
  r = vm->loader ? vm->loader(vm->loadud, key, (pkg ? RIO_LOAD_PKG : 0) | (inc ? RIO_LOAD_FILE : 0), &s) : 1;
  if (r) {
    int n = cat(b, 0, r == 2 ? "found in more than one library path: " : inc ? "file not found: " : "module not found: ");
    n = cat(b, n, key); b[n] = 0;
    fail(vm, b);
  }
  m = &c->mods[c->nmod];
  m->key = (uint16_t)addname(vm, key, klen); m->keylen = (uint16_t)klen;
  m->pkg = (uint8_t)pkg; m->inc = (uint8_t)inc; m->state = 1;
  if (s.isdir && !inc) { memcpy(b, key, (size_t)klen); b[klen] = '/'; dl = klen + 1; }
  else for (dl = klen; dl > 0 && key[dl - 1] != '/'; dl--) {} /* a file's directory: up to its last '/' */
  m->dir = (uint16_t)addname(vm, s.isdir && !inc ? b : key, dl); m->dirlen = (uint16_t)dl;
  { /* a name for error messages */
    int n = s.name ? cat(b, 0, s.name) : inc ? cat(b, 0, key) : cat(b, cat(b, 0, key), ".rio");
    b[n] = 0; m->file = (uint16_t)addname(vm, b, n + 1);
  }
  m->pc0 = (uint16_t)vm->pc;
  f = &c->is[c->nis++];
  f->src = c->src; f->se = c->se; f->ls = ls; f->pos = pos; f->line = line; f->mod = c->curmod; f->f = c->curf; f->inc = inc;
  c->curf = c->nmod++;
  if (!inc) c->curmod = c->curf; /* a module gets its own names; an included part shares them */
  c->src = c->sp = c->ls = s.src; c->se = s.src + s.len; c->line = 1; c->pline = 0;
  lex(vm, &c->nx); next(vm);
  TK.nl = 1; /* the file's first token starts a statement */
}
/* end of a file: back to whoever imported or included it */
static void endfile(Rio *vm) {
  RioC *c = vm->c; RioImp *f = &c->is[--c->nis];
  c->mods[c->curf].pc1 = (uint16_t)vm->pc; c->mods[c->curf].state = 2;
  c->src = f->src; c->se = f->se; c->ls = f->ls; c->sp = f->pos; c->line = f->line; c->curmod = f->mod; c->curf = f->f;
  lex(vm, &c->nx); next(vm);
  if (f->inc) TK.nl = 1; /* carry on after the include statement */
  c->lastlabel = vm->pc;
}
static int isas(Rio *vm) { return TK.t == TK_ID && TK.n == 2 && !memcmp(TK.s, "as", 2); }
/* import .local.path or import package.path, then [as name][*] or .{a, b as c, ...}.
   A package import reaches its root only; deeper names go through what the root publishes. */
static void import_stmt(Rio *vm) {
  RioC *c = vm->c; RioMod *cf = &c->mods[c->curf];
  const char *start = TK.s, *startls = TK.s - (TK.col - 1), *seg[8]; char key[256];
  int startline = TK.line, local, pkg, klen = 0, nseg = 0, segn[8], m, k;
  next(vm);
  if (c->curfn >= 0 || c->nblk) fail(vm, "imports must be at the top level");
  local = TK.t == '.';
  if (local) next(vm);
  pkg = local ? cf->pkg : 1;
  if (local) { memcpy(key, c->names + cf->dir, cf->dirlen); klen = cf->dirlen; }
  for (;;) {
    if (TK.t != TK_ID) fail(vm, "module name expected");
    if (nseg >= 8) fail(vm, "module path too long");
    seg[nseg] = TK.s; segn[nseg] = TK.n;
    if (local || !nseg) { /* a package import names just its root */
      if (klen + TK.n + 2 > (int)sizeof key) fail(vm, "module path too long");
      if (local && nseg) key[klen++] = '/';
      memcpy(key + klen, TK.s, (size_t)TK.n); klen += TK.n;
    }
    nseg++; next(vm);
    if (TK.t == '.' && c->nx.t == TK_ID) { next(vm); continue; }
    break;
  }
  if ((m = findfile(vm, pkg, 0, key, klen)) < 0) { openfile(vm, pkg, 0, key, klen, start, startls, startline); return; }
  if (c->mods[m].state != 2) fail(vm, "import cycle");
  for (k = 1; !local && k < nseg; k++) { /* tween.easing: only if tween publishes easing */
    int i = modsym(vm, m, seg[k], segn[k]);
    if (i < 0 || c->sym[i].k != S_MOD) {
      char b[200]; int n = cat(b, 0, "that package doesn't publish ");
      memcpy(b + n, seg[k], (size_t)(segn[k] < 64 ? segn[k] : 64)); b[n + (segn[k] < 64 ? segn[k] : 64)] = 0;
      fail(vm, b);
    }
    m = c->sym[i].v;
  }
  if (TK.t == '.' && c->nx.t == '{') { /* .{a, b as c} */
    next(vm); next(vm);
    while (TK.t != '}') {
      const char *as; int an, i;
      if (TK.t != TK_ID) fail(vm, "name expected");
      if ((i = modsym(vm, m, TK.s, TK.n)) < 0) failtok(vm, "not exported by that module");
      as = TK.s; an = TK.n; next(vm);
      if (isas(vm)) { next(vm); if (TK.t != TK_ID) fail(vm, "name expected after 'as'"); as = TK.s; an = TK.n; next(vm); }
      if (TK.t == '*') failtok(vm, "to export an imported name, write Name* :: mod.Name");
      { RioSym y = c->sym[i]; addsym(vm, as, an, y.k, y.t, y.v); }
      if (TK.t != ',') break;
      next(vm);
    }
    expect(vm, '}', "'}' expected");
  } else {
    const char *bn = seg[nseg - 1]; int bl = segn[nseg - 1];
    if (isas(vm)) { next(vm); if (TK.t != TK_ID) fail(vm, "name expected after 'as'"); bn = TK.s; bl = TK.n; next(vm); }
    addsym(vm, bn, bl, S_MOD, 0, m);
    if (TK.t == '*') { /* publish your own submodule */
      if (!local) failtok(vm, "only your own submodules can be published (import .name*)");
      c->sym[c->nsym - 1].ex = 1; next(vm);
    }
  }
}
/* include "parts/a.rio": that file becomes part of this module (same names, private ones too).
   Always relative to this file and downward; each file once. */
static void include_stmt(Rio *vm) {
  RioC *c = vm->c; RioMod *cf = &c->mods[c->curf]; char key[256]; int klen, i; const char *pos;
  next(vm);
  if (c->curfn >= 0 || c->nblk) fail(vm, "includes must be at the top level");
  if (TK.t != TK_STR) fail(vm, "include needs a file path in quotes");
  if (TK.n < 5 || memcmp(TK.s + TK.n - 4, ".rio", 4)) failtok(vm, "include paths name a .rio file");
  for (i = 0; i < TK.n; i++) { /* plain names separated by '/': no "", ".", "..", absolute or drive paths */
    int seg = !i || TK.s[i - 1] == '/', end = i + 1 == TK.n || TK.s[i + 1] == '/';
    if (TK.s[i] == '\\' || TK.s[i] == ':' || (TK.s[i] == '/' && seg) || (TK.s[i] == '.' && seg && (end || TK.s[i + 1] == '.')))
      failtok(vm, "include paths stay inside this file's directory");
  }
  if (cf->dirlen + TK.n + 1 > (int)sizeof key) failtok(vm, "include path too long");
  memcpy(key, c->names + cf->dir, cf->dirlen); memcpy(key + cf->dirlen, TK.s, (size_t)TK.n); klen = cf->dirlen + TK.n;
  if (findfile(vm, cf->pkg, 1, key, klen) >= 0) failtok(vm, "file already included");
  if (!c->nx.nl && c->nx.t != TK_EOF && c->nx.t != ';') { next(vm); fail(vm, "expected end of statement"); }
  pos = c->nx.t == TK_STR ? c->nx.s - 1 : c->nx.s; /* resume at the token after the path */
  openfile(vm, cf->pkg, 1, key, klen, pos, pos - (c->nx.col - 1), c->nx.line);
}
static void statement(Rio *vm) {
  int t = TK.t, i; RioBlk *b; Ex e;
  vm->c->fr = vm->c->nact;
  if (t == ';') { next(vm); return; }
  if (t == TK_IMPORT) { import_stmt(vm); goto done; }
  if (t == TK_INCLUDE) { include_stmt(vm); goto done; }
  if (t == TK_ID && vm->c->nx.t == '*') { /* name* :: / name* := / name*: exports the name */
    int t3 = peek3(vm);
    if (t3 == TK_DCOLON || t3 == TK_DECL || t3 == ':') {
      RioTok nm = TK;
      if (vm->c->curfn >= 0 || vm->c->nblk) failtok(vm, "only top-level names can be exported");
      next(vm); vm->c->tk = nm; vm->c->exporting = 1;
    }
  }
  if (t == TK_ID && vm->c->nx.t == '.' && (i = lookup(vm, TK.s, TK.n)) >= 0 && vm->c->sym[i].k == S_TYPE) method_def(vm, vm->c->sym[i].t);
  else if (t == TK_ID && vm->c->nx.t == TK_DCOLON) {
    const char *ns = TK.s; int nn = TK.n;
    next(vm); next(vm);
    if (TK.t == TK_PROC) proc_def(vm, ns, nn, -1);
    else if (TK.t == TK_STRUCT) struct_def(vm, ns, nn);
    else {
      e = expr(vm);
      if (vm->c->curfn < 0 && !vm->c->nblk && (e.k == EK_CONST || e.t == TY_STR)) {
        int d = lookup(vm, ns, nn) - vm->c->def0;
        if (d >= 0 && d < vm->ndefs) e = defval(vm, d, e.t);
      }
      if (e.k == EK_TY) addsym(vm, ns, nn, S_TYPE, e.t, 0);
      else if (e.k == EK_FN) addsym(vm, ns, nn, S_FN, 0, e.a);
      else if (e.k == EK_MOD) addsym(vm, ns, nn, S_MOD, 0, e.a);
      else if (e.k == EK_CONST) addsym(vm, ns, nn, S_CONST, e.t, e.a);
      else if (e.k == EK_ST && e.ro && e.t == TY_STR) addsym(vm, ns, nn, S_CONST, TY_STR, e.a);
      else fail(vm, "constant expression expected");
    }
  } else if (t == TK_ID && (vm->c->nx.t == ':' || vm->c->nx.t == TK_DECL)) decl_var(vm);
  else switch (t) {
  case TK_IF:
    next(vm); e = expr(vm); i = condjump(vm, &e);
    b = bpush(vm, B_IF); b->a = (uint16_t)i;
    break;
  case TK_ELSE:
    if (!vm->c->nblk || vm->c->blk[vm->c->nblk - 1].k != B_IF) failtok(vm, "'else' without 'if'");
    b = &vm->c->blk[vm->c->nblk - 1];
    next(vm); bscope(vm, b);
    b->b = (uint16_t)jappend(vm, b->b, emit(vm, OP_JMP, 0, 0, NONE));
    patch(vm, b->a, here(vm));
    if (TK.t == TK_IF) { next(vm); e = expr(vm); b->a = (uint16_t)condjump(vm, &e); }
    else { b->k = B_ELSE; b->a = NONE; }
    break;
  case TK_FOR:
    next(vm);
    if (TK.nl || TK.t == TK_END) { b = bpush(vm, B_LOOP); b->a = (uint16_t)here(vm); }
    else if (TK.t == TK_ID && (vm->c->nx.t == TK_IN || vm->c->nx.t == ',')) {
      const char *ns = TK.s, *is = 0; int nn = TK.n, in = 0, iv, lim, ix = NONE, incl; Ex d;
      next(vm);
      if (TK.t == ',') { next(vm); if (TK.t != TK_ID) failtok(vm, "index name expected"); is = TK.s; in = TK.n; next(vm); }
      if (TK.t != TK_IN) failtok(vm, "'in' expected");
      next(vm);
      b = bpush(vm, B_FOR);
      vm->c->fr = vm->c->nact; iv = alloc(vm, 1); lim = alloc(vm, 2); if (is) ix = alloc(vm, 1); vm->c->nact = vm->c->fr;
      e = expr(vm);
      if (TK.t != TK_RLT && TK.t != TK_RLE) { /* for x in xs: walk an element pointer iv up to lim, by lim+1 bytes */
        RioType *ty; int s, sz, ro;
        needval(vm, &e); ty = TY(e.t);
        if (ty->k == K_LIST || ty->k == K_BUILD) { /* the elements there when the loop starts */
          int lb = toslot3(vm, &e), v = alloc(vm, 2);
          emit(vm, OP_LVIEW, v, lb, 0);
          e = mkex(EK_ST, slice_of(vm, ty->elem), v * 4); ty = TY(e.t);
        }
        if (ty->k != K_ARR && ty->k != K_SLICE) fail(vm, "for needs a range, array, slice or list");
        if (ty->k == K_ARR && istemp(vm, &e)) fail(vm, "a computed array has no home to view: store it in a variable first");
        sz = (int)TY(ty->elem)->size; ro = e.ro || e.t == TY_STR;
        s = toslot2(vm, &e);
        emit(vm, OP_MOV, iv, s, 0);
        if (sz != 1) { emit(vm, OP_MUL, lim, s + 1, kslot(vm, sz)); emit(vm, OP_ADD, lim, s, lim); }
        else emit(vm, OP_ADD, lim, s, s + 1);
        emit(vm, OP_MOV, lim + 1, kslot(vm, sz), 0);
        if (is) emit(vm, OP_MOV, ix, kslot(vm, 0), 0);
        vm->c->fr = vm->c->nact;
        addsym(vm, ns, nn, ro ? S_ROREF : S_SELF, ty->elem, iv);
        if (is) addsym(vm, is, in, S_VAR, TY_I32, ix * 4);
        b->k = B_EACH; b->i = (uint16_t)iv; b->lim = (uint16_t)lim; b->b = (uint16_t)ix;
        b->brk = (uint16_t)emit(vm, OP_JLE, lim, iv, NONE); b->a = (uint16_t)here(vm);
        break;
      }
      if (is) fail(vm, "a range loop has no index: its variable is the index");
      d = mkex(EK_ST, TY_I32, iv * 4); store(vm, &d, &e); vm->c->fr = vm->c->nact;
      incl = TK.t == TK_RLE; next(vm);
      e = expr(vm);
      if (incl) { Ex one = mkex(EK_CONST, TY_I32, 1); one.t0 = (uint16_t)vm->c->fr; binop(vm, '+', &e, &one); }
      d = mkex(EK_ST, TY_I32, lim * 4); store(vm, &d, &e); vm->c->fr = vm->c->nact;
      addsym(vm, ns, nn, S_VAR, TY_I32, iv * 4);
      b->i = (uint16_t)iv; b->lim = (uint16_t)lim;
      b->brk = (uint16_t)emit(vm, OP_JLE, lim, iv, NONE); b->a = (uint16_t)here(vm);
    } else {
      int top = here(vm);
      e = expr(vm); i = condjump(vm, &e);
      b = bpush(vm, B_LOOP); b->a = (uint16_t)top; b->brk = b->cj = (uint16_t)i;
    }
    break;
  case TK_END:
    if (!vm->c->nblk) failtok(vm, "'end' without block");
    b = &vm->c->blk[vm->c->nblk - 1];
    next(vm);
    if (b->k == B_PROC) {
      emit(vm, OP_RET, 0, 0, 0); patch(vm, b->a, here(vm));
      vm->c->func[b->b].done = 1; vm->c->func[b->b].end = (uint16_t)vm->pc; vm->c->func[b->b].fe = (uint16_t)vm->c->hwm; vm->c->curfn = -1;
      vm->c->nsym = b->nsym; vm->c->nnames = b->nnames; vm->c->nact = vm->c->fr = vm->c->hwm; vm->c->nblk--;
      break;
    }
    if (b->k == B_LOOP) {
      int n, cj = b->cj, ok = cj != NONE && vm->code[cj].op != OP_JMP && cj - b->a <= 24;
      for (n = b->a; ok && n < cj; n++) if (vm->code[n].op >= OP_JMP && vm->code[n].op <= OP_FORI) ok = 0;
      if (ok) { /* rotate: repeat the condition at the bottom, inverted, jumping back into the body */
        patch(vm, b->cont, here(vm));
        vm->c->lineovr = cline(vm, b->a);
        for (n = b->a; n <= cj; n++) { RioIns c = vm->code[n]; emit(vm, c.op, c.a, c.b, c.c); vm->code[vm->pc - 1].x = c.x; }
        vm->c->lineovr = 0;
        invjump(&vm->code[vm->pc - 1]); vm->code[vm->pc - 1].c = (uint16_t)(cj + 1);
      } else { patch(vm, b->cont, b->a); emit(vm, OP_JMP, 0, 0, b->a); }
    }
    else if (b->k == B_FOR) { patch(vm, b->cont, here(vm)); emit(vm, OP_FORI, b->i, b->lim, b->a); }
    else if (b->k == B_EACH) {
      patch(vm, b->cont, here(vm));
      if (b->b != NONE) emit(vm, OP_ADD, b->b, b->b, kslot(vm, 1));
      emit(vm, OP_EACH, b->i, b->lim, b->a);
    }
    else patch(vm, b->b, here(vm));
    patch(vm, b->k >= B_LOOP ? b->brk : b->a, here(vm));
    bscope(vm, b); vm->c->nblk--;
    break;
  case TK_RETURN: {
    RioCFunc *f;
    if (vm->c->curfn < 0) failtok(vm, "return outside proc");
    next(vm); f = &vm->c->func[vm->c->curfn];
    if (f->ret != TY_VOID) {
      Ex d = mkex(EK_ST, f->ret, (int32_t)f->retaddr); vm->c->target = f->ret; e = expr(vm);
      if ((TY(f->ret)->k == K_SLICE || TY(f->ret)->k == K_BUILD) && localview(vm, &e))
        fail(vm, "this returns a view of the proc's own memory, which the next call rewrites: make that memory global, or have the caller pass it in");
      store(vm, &d, &e);
    }
    emit(vm, OP_RET, 0, 0, 0);
    break;
  }
  case TK_BREAK: case TK_CONTINUE:
    for (i = vm->c->nblk - 1; i >= 0 && vm->c->blk[i].k != B_PROC; i--) if (vm->c->blk[i].k >= B_LOOP) break;
    if (i < 0 || vm->c->blk[i].k == B_PROC) failtok(vm, "not inside a loop");
    b = &vm->c->blk[i];
    if (t == TK_BREAK) b->brk = (uint16_t)jappend(vm, b->brk, emit(vm, OP_JMP, 0, 0, NONE));
    else b->cont = (uint16_t)jappend(vm, b->cont, emit(vm, OP_JMP, 0, 0, NONE));
    next(vm);
    break;
  case '{': destructure(vm); break;
  default: stmt_expr(vm);
  }
done:
  if (vm->c->exporting) { vm->c->exporting = 0; fail(vm, "nothing here to export"); }
  vm->c->fr = vm->c->nact;
  if (TK.t != TK_EOF && TK.t != ';' && !TK.nl) fail(vm, "expected end of statement");
}

/* ---------------------------------------------------------------- api */
int rio_init(Rio *vm, void *mem, uint32_t memsize, RioIns *code, uint32_t codecap) {
  uintptr_t pad = (16 - ((uintptr_t)mem & 15)) & 15;
  memset(vm, 0, sizeof *vm);
  if (!mem || memsize < pad + 1024 || !code || codecap < 16) return -1;
  vm->mem = (uint8_t *)mem + pad; vm->memsize = (uint32_t)(memsize - pad) & ~15u;
  vm->code = code; vm->codecap = codecap > 65535 ? 65535 : codecap;
  return 0;
}
void rio_set_log(Rio *vm, RioLogFn fn, void *ud) { vm->logfn = fn; vm->logud = ud; }
void rio_set_loader(Rio *vm, RioLoadFn fn, void *ud) { vm->loader = fn; vm->loadud = ud; }
int rio_ffi(Rio *vm, const char *name, const char *sig, RioFn fn) {
  RioFfi *f;
  if (vm->nffi >= RIO_MAX_FFI) return -1;
  f = &vm->ffi[vm->nffi++]; f->name = name; f->sig = sig; f->fn = fn; f->aw = f->rw = 0;
  return 0;
}
int rio_define(Rio *vm, const char *name, const char *value) {
  if (vm->ndefs >= RIO_MAX_DEFINES || !name) return -1;
  vm->defs[vm->ndefs][0] = name; vm->defs[vm->ndefs++][1] = value;
  return 0;
}
static uint32_t clampu(uint32_t v, uint32_t lo, uint32_t hi) { return v < lo ? lo : v > hi ? hi : v; }
RioLimits rio_limits_for(uint32_t bytes) {
  RioLimits l; uint32_t fixed, per, budget = bytes / 5 * 3;
  l.expr = clampu(bytes / 1024, 32, 256);
  l.blocks = clampu(bytes / 2048, 16, 64);
  l.slots = clampu(bytes / 8, 2048, 65536);
  fixed = (uint32_t)sizeof(RioC) + l.expr * (uint32_t)(sizeof(RioEx) + sizeof(RioOp)) + l.blocks * (uint32_t)sizeof(RioBlk) + l.slots / 8 + 64;
  /* per symbol: its entry, ~12 bytes of name, a share of fields, types, procs and params, and
     two constant words (programs use about two constants per symbol) */
  per = (uint32_t)(sizeof(RioSym) + 12 + sizeof(RioField) / 2 + sizeof(RioType) / 8 + sizeof(RioCFunc) / 4 + sizeof(RioParam) / 2 + 8);
  l.syms = clampu(budget > fixed ? (budget - fixed) / per : 0, 64, 65535);
  l.consts = clampu(l.syms * 2, 128, 16384);
  if (l.slots < l.consts + 1024) l.slots = clampu(l.consts + 1024, 2048, 65536);
  l.names = clampu(l.syms * 12, 512, 65535);
  l.fields = clampu(l.syms / 2, 16, 65535);
  l.types = clampu(l.syms / 8, 16, 65535);
  l.procs = clampu(l.syms / 4, 8, 65535);
  l.params = clampu(l.syms / 2, 16, 65535);
  l.modules = clampu(l.syms / 32, 8, 1024);
  return l;
}
static uint32_t carve(uint32_t *at, uint32_t n, uint32_t size, uint32_t align) {
  uint32_t p = (*at + align - 1) & ~(align - 1);
  *at = p + n * size;
  return p;
}
/* lay the tables out after the RioC header; returns the end offset (the string pool starts there) */
static uint32_t layout(RioC *c, const RioLimits *l) {
  uint32_t at = (uint32_t)sizeof(RioC); uint8_t *b = (uint8_t *)c;
  uint32_t sym = carve(&at, l->syms, sizeof(RioSym), 4), type = carve(&at, l->types, sizeof(RioType), 4);
  uint32_t field = carve(&at, l->fields, sizeof(RioField), 2), func = carve(&at, l->procs, sizeof(RioCFunc), 4);
  uint32_t param = carve(&at, l->params, sizeof(RioParam), 4), blk = carve(&at, l->blocks, sizeof(RioBlk), 2);
  uint32_t vs = carve(&at, l->expr, sizeof(RioEx), 4), os = carve(&at, l->expr, sizeof(RioOp), 8);
  uint32_t mods = carve(&at, l->modules, sizeof(RioMod), 2);
  uint32_t names = carve(&at, l->names, 1, 1), exposed = carve(&at, (l->slots + 7) / 8, 1, 1), kfix = carve(&at, (l->consts + 7) / 8, 1, 1);
  uint32_t kadr = carve(&at, (l->consts + 7) / 8, 1, 1), lvs = carve(&at, (l->slots + 7) / 8, 1, 1), smap = carve(&at, l->slots, 2, 2);
  if (c) {
    c->sym = (RioSym *)(void *)(b + sym); c->type = (RioType *)(void *)(b + type); c->field = (RioField *)(void *)(b + field);
    c->func = (RioCFunc *)(void *)(b + func); c->param = (RioParam *)(void *)(b + param); c->blk = (RioBlk *)(void *)(b + blk);
    c->vs = (RioEx *)(void *)(b + vs); c->os = (RioOp *)(void *)(b + os); c->names = (char *)b + names;
    c->exposed = b + exposed; c->kfix = b + kfix; c->kadr = b + kadr; c->lvs = b + lvs; c->smap = (uint16_t *)(void *)(b + smap);
    c->mods = (RioMod *)(void *)(b + mods);
    memset(c->exposed, 0, lvs + (l->slots + 7) / 8 - exposed);
  }
  return (at + 15) & ~15u;
}
uint32_t rio_limits_size(const RioLimits *l) { return layout(0, l) + 16; }
/* builtin types and names, and the host's ffi functions */
static void setup(Rio *vm) {
  static const uint8_t tk[] = {K_VOID, K_I32, K_F32, K_BYTE, K_SLICE, K_SLICE, K_BOOL};
  static const uint8_t ts[] = {0, 4, 4, 1, 8, 8, 4};
  static const char *tn[] = {"Int", "Float", "String", "Blob", "Bool"};
  RioC *c = vm->c; int i;
  c->nact = c->fr = c->hwm = vm->kcap; c->curfn = -1; c->lastlabel = NONE; c->target = -1;
  c->curmod = 0; c->nmod = 2; memset(c->mods, 0, 2 * sizeof(RioMod));
  for (i = 0; i < 7; i++) { c->type[i].k = tk[i]; c->type[i].size = ts[i]; c->type[i].elem = TY_BYTE; c->type[i].ref = i == TY_STR || i == TY_BLOB; }
  c->ntype = 7;
  for (i = 0; i < 5; i++) addsym(vm, tn[i], (int)strlen(tn[i]), S_TYPE, i < 2 ? TY_I32 + i : TY_STR + i - 2, 0);
  addsym(vm, "true", 4, S_CONST, TY_BOOL, 1); addsym(vm, "false", 5, S_CONST, TY_BOOL, 0);
  for (i = 0; i < (int)(sizeof bi / sizeof bi[0]); i++)
    if (i < BI_PUSH || i == BI_CAP) addsym(vm, bi[i], (int)strlen(bi[i]), S_BI, 0, i); /* the rest are list methods */
  for (i = 0; i < vm->nffi; i++) {
    RioFfi *f = &vm->ffi[i]; RioCFfi *cf = &c->ffi[i]; const char *g;
    cf->p0 = (uint16_t)c->nparam; cf->np = 0; cf->ret = TY_VOID; f->aw = f->rw = 0;
    for (g = f->sig; *g; g++) {
      int ret = *g == '>', ch = ret ? g[1] : *g;
      int t = ch == 'i' ? TY_I32 : ch == 'f' ? TY_F32 : ch == 's' ? TY_STR : ch == 'b' ? TY_BLOB : -1;
      if (t < 0 || c->nparam >= (int)c->lim.params) fail(vm, "bad ffi signature");
      if (ret) { cf->ret = (uint16_t)t; f->rw = (uint8_t)words(vm, t); break; }
      c->param[c->nparam].ref = 0; c->param[c->nparam++].t = (uint16_t)t; cf->np++; f->aw = (uint8_t)(f->aw + words(vm, t));
    }
    addsym(vm, f->name, (int)strlen(f->name), S_FFI, 0, i);
  }
  c->def0 = c->nsym;
  for (i = 0; i < vm->ndefs; i++) {
    const char *nm = vm->defs[i][0]; int j, n = (int)strlen(nm); Ex e;
    for (j = 0; j < n; j++) if (!(isal(nm[j]) || (j && isdg(nm[j])))) break;
    if (!n || j < n) { char b[RIO_ERRBUF]; int m = cat(b, 0, "-D: bad name "); m = cat(b, m, nm); b[m] = 0; seterr(vm, RIO_ECOMPILE, 0, 0, 0, b); longjmp(c->jb, 1); }
    e = defval(vm, i, -1);
    addsym(vm, nm, n, S_CONST, e.t, e.a);
  }
  c->curmod = c->curf = 1; c->mods[1].state = 1; /* the main source */
}
static void compile_all(Rio *vm) {
  RioC *c = vm->c;
  for (;;) {
    if (TK.t != TK_EOF) { statement(vm); continue; }
    if (c->nblk) fail(vm, M_END);
    if (!c->nis) break;
    endfile(vm);
  }
}
/* lay out the final memory: string pool + export table go just below the big arrays, string
   constants get their real addresses, and everything the compiler used is zeroed */
static void rtfunc(Rio *vm, RioFunc *rf, int k) {
  RioC *c = vm->c; RioCFunc *f = &c->func[k]; uint32_t pw = 0; int j;
  for (j = 0; j < f->np; j++) pw += (uint32_t)words(vm, c->param[f->p0 + j].t);
  rf->pc = f->pc; rf->end = f->end; rf->fs = f->fs; rf->fe = f->fe; rf->pend = (uint16_t)(f->fs + pw);
  rf->ret = (uint16_t)(f->retaddr / 4); rf->retw = (uint8_t)(f->ret ? words(vm, f->ret) : 0);
}
/* which operands of an instruction are slots: bit 1 a, 2 b, 4 c (unused ones are 0, which never moves) */
static int slotops(int op) {
  switch (op) {
  case OP_LDW: case OP_LDB: case OP_LEA: case OP_LDW2: case OP_STW: case OP_STB: case OP_STW2: case OP_COPY: case OP_MOVN: return 3;
  case OP_ZERO: case OP_FFI: return 1;
  case OP_CALL: return 0;
  default: return op >= OP_JMP && op <= OP_FORI ? 3 : 7;
  }
}
/* the proc whose code starts at pc: one defined before proc `before` */
static int funcat(Rio *vm, int pc, int before) {
  int lo = 0, hi = before - 1, m;
  while (lo <= hi) { /* procs are compiled in order, so their code is too */
    m = (lo + hi) / 2;
    if (vm->c->func[m].pc == pc) return m;
    if (vm->c->func[m].pc < pc) lo = m + 1; else hi = m - 1;
  }
  for (m = 0; m < before && vm->c->func[m].pc != pc; m++) {}
  if (m == before) fail(vm, "internal error: call to an unknown proc");
  return m;
}
/* Procs that are never running at the same time share memory. No recursion means the call graph is
   known and has no cycles, and callees are compiled first. A proc's frame moves into the shared area,
   just above the shared frames of everything it can call, unless a slot of it ever has its address
   taken: those must keep their memory for good (a view of them may live on). The slots that stay are
   packed together, starting right after the constants actually used (the room reserved for constants is
   only needed while compiling); every operand, address constant and table that names a slot is
   renumbered. Returns the longest chain of calls, which sizes the return stack. */
static uint32_t shareframes(Rio *vm) {
  RioC *c = vm->c; RioVal *R = (RioVal *)vm->mem; uint16_t *map = c->smap;
  uint32_t lo = vm->kcap, hi = c->hwm, s, keep, top = 0, depth = 0, i, start = vm->repl ? lo : vm->nk; int k, j, pc;
  for (k = 0; k < vm->nfunc; k++) {
    RioCFunc *f = &c->func[k]; uint32_t ob = 0, dep = 0;
    for (pc = f->pc; pc < f->end; pc++) {
      if (vm->code[pc].op != OP_CALL) continue;
      j = funcat(vm, vm->code[pc].c, k);
      if (c->func[j].oe > ob) ob = c->func[j].oe;
      if (c->func[j].dep > dep) dep = c->func[j].dep;
    }
    f->shared = !vm->repl && f->fe > f->fs;
    for (s = f->fs; f->shared && s < f->fe; s++) if (c->exposed[s >> 3] & (1u << (s & 7))) f->shared = 0;
    f->ob = (uint16_t)ob; f->oe = (uint16_t)(ob + (f->shared ? f->fe - f->fs : 0)); f->dep = (uint16_t)(dep + 1);
    if (f->oe > top) top = f->oe;
    if (f->dep > depth) depth = f->dep;
  }
  if (!top && start == lo) return depth;
  for (s = lo; s < hi; s++) map[s] = 0;
  for (k = 0; k < vm->nfunc; k++) for (s = c->func[k].fs; c->func[k].shared && s < c->func[k].fe; s++) map[s] = 1;
  for (s = lo, keep = start; s < hi; s++) if (!map[s]) map[s] = (uint16_t)keep++;
  for (k = 0; k < vm->nfunc; k++) {
    RioCFunc *f = &c->func[k];
    for (s = f->fs; f->shared && s < f->fe; s++) map[s] = (uint16_t)(keep + f->ob + (s - f->fs));
  }
#define MAPS(x) ((uint32_t)(x) >= lo && (uint32_t)(x) < hi ? map[x] : (x))
#define MAPA(a) ((uint32_t)(a) / 4 >= lo && (uint32_t)(a) / 4 < hi ? (uint32_t)map[(a) / 4] * 4 + ((a) & 3) : (uint32_t)(a))
  for (pc = 0; pc < (int)vm->pc; pc++) {
    RioIns *x = &vm->code[pc]; int m;
    if (x->op == OP_DATA) { if (pc && vm->code[pc - 1].op == OP_SLICE) x->a = (uint16_t)MAPS(x->a); continue; }
    m = slotops(x->op);
    if (m & 1) x->a = (uint16_t)MAPS(x->a);
    if (m & 2) x->b = (uint16_t)MAPS(x->b);
    if (m & 4) x->c = (uint16_t)MAPS(x->c);
  }
  for (i = 1; i < vm->nk; i++) if (KADR(i)) R[i].u = MAPA(R[i].u);
  for (s = start; s < lo; s++) c->exposed[s >> 3] &= (uint8_t)~(1u << (s & 7)); /* unused constant room */
  for (s = lo; s < hi; s++) { /* address-taken marks follow their slots, which only move down */
    int b = c->exposed[s >> 3] >> (s & 7) & 1;
    c->exposed[s >> 3] &= (uint8_t)~(1u << (s & 7));
    if (b && map[s] < keep) c->exposed[map[s] >> 3] |= (uint8_t)(1u << (map[s] & 7));
  }
  for (k = 0; k < c->nsym; k++) if (c->sym[k].k == S_VAR) c->sym[k].v = (int32_t)MAPA(c->sym[k].v);
  for (k = 0; k < vm->nfunc; k++) {
    RioCFunc *f = &c->func[k];
    if (f->fe > f->fs) { uint32_t n = f->fe - f->fs; f->fs = map[f->fs]; f->fe = (uint16_t)(f->fs + n); }
    if (f->retaddr) f->retaddr = MAPA(f->retaddr);
  }
#undef MAPS
#undef MAPA
  c->hwm = c->nact = c->fr = keep + top;
  vm->kcap = start;
  return depth;
}
/* Pack the pc->line table in place, from the 4-byte (pc, line) entries written while compiling. It's
   read only for runtime errors (and by the C backend), so it trades lookup speed for size: one byte for
   a step of 0-14 instructions and -4..+11 lines, which is nearly every entry, else four bytes:
   0xF0 | step >> 8, step & 255, then the line (low byte first). markline keeps steps under 4096, and no
   entry grows, so packing in place never overtakes what's still to be read. Returns the size. */
static uint32_t packlines(RioC *c) {
  uint8_t *t = c->strs + c->poolcap; uint32_t i, w = 0, pc = 0; int line = 0, dl; uint16_t e[2];
  for (i = 0; i < c->nline; i++) {
    uint32_t step;
    memcpy(e, t + i * 4, 4);
    step = e[0] - pc; dl = (int)e[1] - line;
    if (step <= 14 && dl >= -4 && dl <= 11) t[w++] = (uint8_t)(step << 4 | (uint32_t)(dl + 4));
    else { t[w++] = (uint8_t)(0xF0 | step >> 8); t[w++] = (uint8_t)step; t[w++] = (uint8_t)e[1]; t[w++] = (uint8_t)(e[1] >> 8); }
    pc = e[0]; line = e[1];
  }
  return w;
}
/* the names the host can look up (rio_func, rio_global): main and the main file's names marked name*,
   or when built with RIO_EXPORTS_ALL every top-level proc and global of the main file */
static int hostname(RioC *c, RioSym *y) {
  if ((y->k != S_FN && y->k != S_VAR) || y->mod != 1) return 0;
#ifdef RIO_EXPORTS_ALL
  (void)c;
  return 1;
#else
  return y->ex || (y->len == 4 && !memcmp(c->names + y->name, "main", 4));
#endif
}
static void finalize(Rio *vm) {
  RioC *c = vm->c; RioVal *R = (RioVal *)vm->mem; RioExport *x;
  uint32_t i, n = 0, p, ex, lo, fo, sz, base, first, depth = shareframes(vm), lsz; int k;
  first = vm->kcap * 4; /* after sharing: the slots now start right after the constants */
  c->fr = c->hwm; vm->csaddr = (uint32_t)alloc(vm, depth + 1) * 4; /* return stack: the longest chain of calls */
  uint32_t mo, mf, nm = (uint32_t)(c->nmod - 2);
  p = c->pool;
  for (k = 0; k < c->nsym; k++) {
    RioSym *y = &c->sym[k];
    if (!hostname(c, y)) continue;
    if (y->len > c->poolcap - p) fail(vm, "out of compiler memory");
    memcpy(c->strs + p, c->names + y->name, y->len); p += y->len; n++;
  }
  mf = p; /* module file names, for runtime errors */
  for (k = 2; k < c->nmod; k++) {
    uint32_t fl = (uint32_t)strlen(c->names + c->mods[k].file) + 1;
    if (fl > c->poolcap - p) fail(vm, "out of compiler memory");
    memcpy(c->strs + p, c->names + c->mods[k].file, fl); p += fl;
  }
  { /* the line table was written top-down: reverse it in place so it ascends by pc */
    uint8_t *a = c->strs + c->poolcap, *z = c->strs + c->linetop - 4, t[4];
    for (; a < z; a += 4, z -= 4) { memcpy(t, a, 4); memcpy(a, z, 4); memcpy(z, t, 4); }
  }
  lsz = packlines(c);
  /* block layout: strings and names | exports | pc->line table | per-proc table */
  ex = (p + 3) & ~3u; lo = ex + n * (uint32_t)sizeof(RioExport); fo = (lo + lsz + 3) & ~3u;
  mo = (fo + (uint32_t)vm->nfunc * (uint32_t)sizeof(RioFunc) + 3) & ~3u;
  sz = mo + nm * (uint32_t)sizeof(RioModRt);
  if (lo > c->poolcap || sz > c->linetop) fail(vm, "out of compiler memory");
  if (vm->hi < sz || ((vm->hi - sz) & ~3u) < c->hwm * 4) fail(vm, "out of memory");
  base = (vm->hi - sz) & ~3u;
  x = (RioExport *)(void *)(c->strs + ex); p = c->pool;
  for (k = 0; k < c->nsym; k++) {
    RioSym *y = &c->sym[k];
    if (!hostname(c, y)) continue;
    x->name = base + p; x->len = y->len; x->k = y->k; x->pad = 0; x->v = y->v; p += y->len; x++;
  }
  memmove(c->strs + lo, c->strs + c->poolcap, lsz); /* lines go right after the exports */
  for (i = 1; i < vm->nk; i++) if (KFIX(i)) R[i].u = base + (R[i].u & 0x7FFFFFFFu);
  for (k = 0; k < vm->nfunc; k++) rtfunc(vm, (RioFunc *)(void *)(c->strs + fo) + k, k);
  for (k = 2, p = mf; k < c->nmod; k++) {
    RioModRt *r = (RioModRt *)(void *)(c->strs + mo) + (k - 2);
    r->pc0 = c->mods[k].pc0; r->pc1 = c->mods[k].pc1; r->file = base + p; p += (uint32_t)strlen(c->names + c->mods[k].file) + 1;
  }
  vm->mods = base + mo; vm->nmods = nm;
  vm->exports = base + ex; vm->nexports = n; vm->hi = base;
  vm->lines = base + lo; vm->nlines = lsz; vm->lcpos = vm->lcpc = vm->lcline = 0; vm->func = (RioFunc *)(void *)(vm->mem + base + fo);
  /* nothing in c is read after this: the move and the zeroing may overwrite it */
  memmove(vm->mem + base, c->strs, sz);
  memset(vm->mem + first, 0, base - first);
  memset(vm->mem + base + sz, 0, vm->memsize - base - sz);
}
/* set up the compiler's state in scratch, or overlaid on mem when scratch is NULL */
static int cstart(Rio *vm, void *scratch, uint32_t size, const RioLimits *lim) {
  int inmem = !scratch; uintptr_t pad; RioC *c; RioLimits l; uint32_t used;
  vm->ok = 0; vm->c = 0; vm->repl = 0; vm->ekind = RIO_ENONE; vm->err[0] = 0; vm->nlines = 0; vm->func = 0;
  l = lim ? *lim : rio_limits_for(inmem ? vm->memsize : size);
  if (l.consts < 16) l.consts = 16;
  if (l.slots > 65536) l.slots = 65536;
  if (inmem) { /* overlay: everything above the constants is free until the program runs */
    if (l.consts * 4 + 1024 > vm->memsize) { seterr(vm, RIO_ECOMPILE, 0, 0, 0, "not enough memory for the compiler"); return -1; }
    scratch = vm->mem + l.consts * 4; size = vm->memsize - l.consts * 4;
  }
  pad = (16 - ((uintptr_t)scratch & 15)) & 15;
  used = rio_limits_size(&l);
  if (l.slots < l.consts + 64 || l.consts * 4 + 256 > vm->memsize || size < pad + used + 64) {
    seterr(vm, RIO_ECOMPILE, 0, 0, 0, inmem ? "not enough memory for the compiler" : "scratch too small for these limits");
    return -1;
  }
  c = vm->c = (RioC *)(void *)((uint8_t *)scratch + pad);
  memset(c, 0, sizeof *c);
  c->lim = l; used = layout(c, &l);
  c->strs = (uint8_t *)c + used; c->poolcap = c->linetop = (size - (uint32_t)pad - used) & ~3u;
  vm->kcap = l.consts; vm->hi = vm->memsize; vm->nk = 1; vm->pc = 0; vm->nfunc = 0; ((RioVal *)vm->mem)[0].u = 0;
  return 0;
}
int rio_compile_ex(Rio *vm, const char *src, uint32_t len, void *scratch, uint32_t size, const RioLimits *lim) {
  int inmem = !scratch; RioC *c;
  if (cstart(vm, scratch, size, lim)) return -1;
  c = vm->c;
  if (setjmp(c->jb)) { if (inmem) vm->c = 0; return -1; }
  setup(vm);
  c->src = c->sp = c->ls = src; c->se = src + len; c->line = 1;
  lex(vm, &c->nx); next(vm);
  compile_all(vm);
  emit(vm, OP_HALT, 0, 0, 0);
  finalize(vm);
  if (inmem) vm->c = 0;
  vm->ok = 1;
  return 0;
}
int rio_compile(Rio *vm, const char *src, uint32_t len) { return rio_compile_ex(vm, src, len, 0, 0, 0); }
int rio_compile_scratch(Rio *vm, const char *src, uint32_t len, void *scratch, uint32_t size) {
  if (!scratch) { seterr(vm, RIO_ECOMPILE, 0, 0, 0, "scratch too small for these limits"); vm->ok = 0; return -1; }
  return rio_compile_ex(vm, src, len, scratch, size, 0);
}
static int run(Rio *vm, uint32_t pc);
/* frames are shared by procs that can't run at the same time: starting rio again from inside an FFI
   function would break that, so it isn't allowed */
static int start(Rio *vm, uint32_t pc) {
  int r;
  if (vm->running) { seterr(vm, RIO_ERUNTIME, 0, 0, 0, "rio is already running (called from an FFI function?)"); return -1; }
  vm->running = 1; r = run(vm, pc); vm->running = 0;
  return r;
}
/* REPL: after each piece compiles, its strings move into mem (below the arrays) and its procs
   get runtime entries; the compiler state stays in scratch */
static void commit(Rio *vm, uint32_t k0, int f0) {
  RioC *c = vm->c; RioVal *R = (RioVal *)vm->mem; uint32_t i; int k;
  if (c->pool) {
    uint32_t sz = (c->pool + 3) & ~3u, base;
    if (vm->hi < sz || vm->hi - sz < c->hwm * 4) fail(vm, "out of memory");
    base = vm->hi -= sz;
    memcpy(vm->mem + base, c->strs, c->pool);
    for (i = k0; i < vm->nk; i++)
      if (KFIX(i)) { R[i].u = base + (R[i].u & 0x7FFFFFFFu); c->kfix[i >> 3] &= (uint8_t)~(1u << (i & 7)); }
    c->pool = 0;
  }
  for (k = f0; k < vm->nfunc; k++) rtfunc(vm, &vm->func[k], k);
}
int rio_repl_begin(Rio *vm, void *scratch, uint32_t size, const RioLimits *lim) {
  RioC *c; uint32_t fsz;
  if (!scratch) { seterr(vm, RIO_ECOMPILE, 0, 0, 0, "a REPL needs its own scratch buffer"); return -1; }
  if (cstart(vm, scratch, size, lim)) return -1;
  c = vm->c;
  memset(vm->mem, 0, vm->memsize);
  if (setjmp(c->jb)) { vm->c = 0; return -1; }
  setup(vm);
  /* reserved up front, since later pieces can add procs: their runtime table and the return stack */
  fsz = (c->lim.procs * (uint32_t)sizeof(RioFunc) + 3) & ~3u;
  if (vm->hi < fsz + (c->lim.procs + 1) * 4 + c->hwm * 4 + 64) fail(vm, "out of memory");
  vm->hi -= fsz; vm->func = (RioFunc *)(void *)(vm->mem + vm->hi);
  vm->hi -= (c->lim.procs + 1) * 4; vm->csaddr = vm->hi;
  commit(vm, 1, 0); /* string values from -D defines */
  vm->repl = 1; vm->ok = 1;
  return 0;
}
int rio_repl_eval(Rio *vm, const char *src, uint32_t len) {
  RioC *c = vm->c; uint32_t pc0 = vm->pc, nk0 = vm->nk, hi0 = vm->hi, nact0, hwm0, poolcap0, nline0, lastline0;
  int nsym0, ntype0, nfield0, nparam0, nnames0, nfunc0 = vm->nfunc, nmod0 = c ? c->nmod : 0;
  if (!vm->repl || !c) { seterr(vm, RIO_ECOMPILE, 0, 0, 0, "no REPL session"); return -1; }
  vm->ekind = RIO_ENONE; vm->err[0] = 0;
  nact0 = c->nact; hwm0 = c->hwm; poolcap0 = c->poolcap; nline0 = c->nline; lastline0 = c->lastline;
  nsym0 = c->nsym; ntype0 = c->ntype; nfield0 = c->nfield; nparam0 = c->nparam; nnames0 = c->nnames;
  if (setjmp(c->jb)) { /* undo everything this piece added: all the tables only ever grow */
    int more = c->sp >= c->se && (c->failmsg == M_END || c->failmsg == M_BRACKET || c->failmsg == M_EXPR || c->failmsg == M_STRING);
    uint32_t i;
    for (i = nk0; i < vm->nk; i++) c->kfix[i >> 3] &= (uint8_t)~(1u << (i & 7));
    vm->pc = pc0; vm->nk = nk0; vm->hi = hi0; vm->nfunc = nfunc0;
    c->nact = c->fr = nact0; c->hwm = hwm0; c->poolcap = poolcap0; c->nline = nline0; c->lastline = lastline0; c->pool = 0;
    c->nsym = nsym0; c->ntype = ntype0; c->nfield = nfield0; c->nparam = nparam0; c->nnames = nnames0;
    c->nblk = c->nvs = c->nos = 0; c->curfn = -1; c->target = -1; c->lineovr = 0;
    c->nmod = nmod0; c->nis = 0; c->curmod = c->curf = 1; c->exporting = 0;
    if (more) { vm->ekind = RIO_ENONE; vm->err[0] = 0; return RIO_MORE; }
    return -1;
  }
  c->src = c->sp = c->ls = src; c->se = src + len; c->line = 1; c->pline = c->pcol = c->pw = 0; c->lastline = c->lastlinepc = 0;
  lex(vm, &c->nx); next(vm);
  c->lastlabel = vm->pc; /* nothing gets fused with the previous piece's last instruction */
  compile_all(vm);
  emit(vm, OP_HALT, 0, 0, 0);
  commit(vm, nk0, nfunc0);
  vm->brk = 0;
  return start(vm, pc0) ? -1 : 0;
}
void rio_interrupt(Rio *vm) { vm->brk = 1; }
const char *rio_error(Rio *vm) { return vm->err; }
RioError rio_error_info(Rio *vm) {
  RioError e; e.kind = vm->ekind; e.line = vm->eline; e.col = vm->ecol; e.len = vm->elen; e.msg = vm->err + vm->emsg; e.file = vm->efile;
  return e;
}
int rio_pc_line(Rio *vm, uint32_t pc) { /* decode the packed table (see packlines) up to pc */
  const uint8_t *t = vm->mem + vm->lines; uint32_t i, at, line;
  if (vm->repl && vm->c) return (int)cline(vm, pc); /* REPL: the table is still in scratch */
  if (pc < vm->lcpc) vm->lcpos = vm->lcpc = vm->lcline = 0; /* behind the last lookup: start over */
  i = vm->lcpos; at = vm->lcpc; line = vm->lcline;
  while (i < vm->nlines) {
    uint32_t b = t[i], step, nl, n;
    if (b < 0xF0) { step = b >> 4; nl = line + (b & 15) - 4; n = 1; }
    else { step = (b & 15) << 8 | t[i + 1]; nl = (uint32_t)t[i + 2] | (uint32_t)t[i + 3] << 8; n = 4; }
    if (at + step > pc) break;
    at += step; line = nl; i += n;
  }
  vm->lcpos = i; vm->lcpc = at; vm->lcline = line;
  return (int)line;
}
void rio_trap(Rio *vm, const char *msg) { int n = cat(vm->err, 0, msg); vm->err[n] = 0; vm->trap = 1; }
static RioExport *findexp(Rio *vm, const char *name, int k) {
  RioExport *x = (RioExport *)(void *)(vm->mem + vm->exports); uint32_t i, n = (uint32_t)strlen(name);
  if (!vm->ok) return 0;
  for (i = vm->nexports; i-- > 0;)
    if (x[i].k == k && x[i].len == n && !memcmp(vm->mem + x[i].name, name, n)) return &x[i];
  return 0;
}
static int replsym(Rio *vm, const char *name, int k) { /* REPL: look names up in the live compiler tables */
  int i = lookup(vm, name, (int)strlen(name));
  return i >= 0 && vm->c->sym[i].k == k ? vm->c->sym[i].v : -1;
}
int rio_func(Rio *vm, const char *name) {
  RioExport *x;
  if (vm->repl && vm->c) return replsym(vm, name, S_FN);
  x = findexp(vm, name, S_FN); return x ? x->v : -1;
}
void *rio_global(Rio *vm, const char *name) {
  RioExport *x; int v;
  if (vm->repl && vm->c) return (v = replsym(vm, name, S_VAR)) >= 0 ? vm->mem + v : 0;
  x = findexp(vm, name, S_VAR); return x ? vm->mem + x->v : 0;
}
RioVal *rio_args(Rio *vm, int fn) { return (RioVal *)(void *)vm->mem + vm->func[fn].fs; }
RioVal *rio_ret(Rio *vm, int fn) { return (RioVal *)(void *)vm->mem + vm->func[fn].ret; }
void *rio_ptr(Rio *vm, const RioVal *s) {
  return (uint32_t)s[0].i <= vm->memsize && (uint32_t)s[1].i <= vm->memsize - (uint32_t)s[0].i ? vm->mem + s[0].i : 0;
}

/* ---------------------------------------------------------------- vm */
static void logput(Rio *vm, const char *s, int n) { while (n-- > 0 && vm->logn < RIO_LOGBUF) vm->logbuf[vm->logn++] = *s++; }
static const char *pcfile(Rio *vm, uint32_t pc) {
  const char *f = 0; uint32_t best = 0x10000, k;
  if (vm->repl && vm->c) {
    for (k = 2; k < (uint32_t)vm->c->nmod; k++) {
      RioMod *m = &vm->c->mods[k];
      if (pc >= m->pc0 && pc < m->pc1 && (uint32_t)(m->pc1 - m->pc0) < best) { best = m->pc1 - m->pc0; f = vm->c->names + m->file; }
    }
    return f;
  }
  for (k = 0; k < vm->nmods; k++) {
    RioModRt *m = (RioModRt *)(void *)(vm->mem + vm->mods) + k;
    if (pc >= m->pc0 && pc < m->pc1 && (uint32_t)(m->pc1 - m->pc0) < best) { best = m->pc1 - m->pc0; f = (const char *)vm->mem + m->file; }
  }
  return f;
}
static int rterr(Rio *vm, const char *m, uint32_t pc) {
  char b[RIO_ERRBUF];
  if (m == vm->err) { memcpy(b, m, sizeof b); m = b; } /* a message from rio_trap */
  seterr(vm, RIO_ERUNTIME, rio_pc_line(vm, pc), 0, 0, m);
  vm->efile = pcfile(vm, pc);
  return -1;
}
static int run(Rio *vm, uint32_t pc) {
  const RioIns *code = vm->code, *ip = code + pc;
  uint32_t *cs = (uint32_t *)(void *)(vm->mem + vm->csaddr);
  uint8_t *M = vm->mem; RioVal *R = (RioVal *)M; int sp = 0;
#define A ip->a
#define B ip->b
#define C ip->c
#define I(x) R[x].i
#define U(x) R[x].u
#define F(x) R[x].f
#define MV(p) (*(RioVal *)(M + (p)))
#define SZ(w) ((uint32_t)(w).b | (uint32_t)(w).c << 16)
#ifdef RIO_CGOTO
#define LBL(o) &&L_##o,
  static const void *const L[] = {OPS(LBL)};
#define CASE(o) L_##o:
#define DISPATCH() goto *L[ip->op]
#define NEXT() do { ip++; DISPATCH(); } while (0)
  DISPATCH();
#else
#define CASE(o) case OP_##o:
#define DISPATCH() continue
#define NEXT() { ip++; continue; }
  for (;;) switch (ip->op) {
#endif
#define JUMP() { if (C <= (uint32_t)(ip - code) && vm->brk) goto stop; ip = code + C; DISPATCH(); }
  CASE(MOV) R[A] = R[B]; NEXT();
  CASE(ADD) U(A) = U(B) + U(C); NEXT();
  CASE(SUB) U(A) = U(B) - U(C); NEXT();
  CASE(MUL) U(A) = U(B) * U(C); NEXT();
  CASE(DIV) { int32_t y = I(C); if (!y) return rterr(vm, "division by zero", (uint32_t)(ip - code)); I(A) = y == -1 ? (int32_t)(0u - U(B)) : I(B) / y; NEXT(); }
  CASE(MOD) { int32_t y = I(C); if (!y) return rterr(vm, "division by zero", (uint32_t)(ip - code)); I(A) = y == -1 ? 0 : I(B) % y; NEXT(); }
  CASE(AND) U(A) = U(B) & U(C); NEXT();
  CASE(OR) U(A) = U(B) | U(C); NEXT();
  CASE(XOR) U(A) = U(B) ^ U(C); NEXT();
  CASE(SHL) U(A) = U(B) << (U(C) & 31); NEXT();
  CASE(SHR) I(A) = I(B) >> (U(C) & 31); NEXT();
  CASE(NEG) U(A) = 0u - U(B); NEXT();
  CASE(BNOT) U(A) = ~U(B); NEXT();
  CASE(NOT) I(A) = !I(B); NEXT();
  CASE(FADD) F(A) = F(B) + F(C); NEXT();
  CASE(FSUB) F(A) = F(B) - F(C); NEXT();
  CASE(FMUL) F(A) = F(B) * F(C); NEXT();
  CASE(FDIV) F(A) = F(B) / F(C); NEXT();
  CASE(FNEG) F(A) = -F(B); NEXT();
  CASE(EQ) I(A) = I(B) == I(C); NEXT();
  CASE(NE) I(A) = I(B) != I(C); NEXT();
  CASE(LT) I(A) = I(B) < I(C); NEXT();
  CASE(LE) I(A) = I(B) <= I(C); NEXT();
  CASE(FEQ) I(A) = F(B) == F(C); NEXT();
  CASE(FNE) I(A) = F(B) != F(C); NEXT();
  CASE(FLT) I(A) = F(B) < F(C); NEXT();
  CASE(FLE) I(A) = F(B) <= F(C); NEXT();
  CASE(SEQ) I(A) = I(B + 1) == I(C + 1) && !memcmp(M + I(B), M + I(C), (size_t)I(B + 1)); NEXT();
  CASE(SNE) I(A) = !(I(B + 1) == I(C + 1) && !memcmp(M + I(B), M + I(C), (size_t)I(B + 1))); NEXT();
  CASE(ITOF) F(A) = (float)I(B); NEXT();
  CASE(FTOI) I(A) = ftoi(F(B)); NEXT();
  CASE(LDW) R[A] = MV(I(B) + C); NEXT();
  CASE(LDB) I(A) = M[I(B) + C]; NEXT();
  CASE(LEA) I(A) = I(B) + C; NEXT();
  CASE(M1) F(A) = mf1[ip->x](F(B)); NEXT();
  CASE(M2) F(A) = mf2[ip->x](F(B), F(C)); NEXT();
  CASE(IABS) U(A) = I(B) < 0 ? 0u - U(B) : U(B); NEXT();
  CASE(IMIN) I(A) = I(B) < I(C) ? I(B) : I(C); NEXT();
  CASE(IMAX) I(A) = I(B) > I(C) ? I(B) : I(C); NEXT();
  CASE(FMIN) F(A) = F(B) < F(C) ? F(B) : F(C); NEXT();
  CASE(FMAX) F(A) = F(B) > F(C) ? F(B) : F(C); NEXT();
  CASE(LDX) {
    uint32_t i = U(C);
    if (i >= U(B + 1)) return rterr(vm, "index out of bounds", (uint32_t)(ip - code));
    R[A] = MV(U(B) + i * SZ(ip[1]) + ip[1].a); ip += 2; DISPATCH();
  }
  CASE(LDXB) {
    uint32_t i = U(C);
    if (i >= U(B + 1)) return rterr(vm, "index out of bounds", (uint32_t)(ip - code));
    U(A) = M[U(B) + i * SZ(ip[1]) + ip[1].a]; ip += 2; DISPATCH();
  }
  CASE(MOV2) { RioVal x = R[B], y = R[B + 1]; R[A] = x; R[A + 1] = y; NEXT(); }
  CASE(LDW2) { int32_t p = I(B) + C; RioVal x = MV(p), y = MV(p + 4); R[A] = x; R[A + 1] = y; NEXT(); }
  CASE(STW) MV(I(A) + C) = R[B]; NEXT();
  CASE(STB) M[I(A) + C] = (uint8_t)U(B); NEXT();
  CASE(STW2) { int32_t p = I(A) + C; MV(p) = R[B]; MV(p + 4) = R[B + 1]; NEXT(); }
  CASE(STX) {
    uint32_t i = U(B);
    if (i >= U(A + 1)) return rterr(vm, "index out of bounds", (uint32_t)(ip - code));
    MV(U(A) + i * SZ(ip[1]) + ip[1].a) = R[C]; ip += 2; DISPATCH();
  }
  CASE(STXB) {
    uint32_t i = U(B);
    if (i >= U(A + 1)) return rterr(vm, "index out of bounds", (uint32_t)(ip - code));
    M[U(A) + i * SZ(ip[1]) + ip[1].a] = (uint8_t)U(C); ip += 2; DISPATCH();
  }
  CASE(IDX) {
    uint32_t i = U(C);
    if (i >= U(B + 1)) return rterr(vm, "index out of bounds", (uint32_t)(ip - code));
    U(A) = U(B) + i * SZ(ip[1]); ip += 2; DISPATCH();
  }
  CASE(SLICE) {
    int32_t lo = I(C), hi = I(ip[1].a), n = I(B + 1);
    if (lo < 0 || hi < lo || hi > n) return rterr(vm, "slice out of bounds", (uint32_t)(ip - code));
    I(A) = I(B) + lo * (int32_t)SZ(ip[1]); I(A + 1) = hi - lo; ip += 2; DISPATCH();
  }
  CASE(COPY) memmove(M + I(A), M + I(B), C); NEXT();
  CASE(MOVN) memmove(R + A, R + B, C * sizeof(RioVal)); NEXT();
  CASE(ZERO) memset(M + I(A), 0, SZ(*ip)); NEXT();
/* n numbers d[i] = l[i] op r[i]; x: 16 l is a scalar, 32 r is a scalar (the value itself, read once:
   it steps by 0), else l and r are addresses */
#define VLOOP(body) { \
    RioVal lv_ = R[B], rv_ = R[C], *D_ = &MV(U(A)), *L_ = ip->x & 16 ? &lv_ : &MV(lv_.u), *Q_ = ip->x & 32 ? &rv_ : &MV(rv_.u); \
    uint32_t n_ = SZ(ip[1]), i_, la_ = ip->x & 16 ? 0 : 1, ra_ = ip->x & 32 ? 0 : 1; \
    for (i_ = 0; i_ < n_; i_++) body; \
    ip += 2; DISPATCH(); }
#define VL L_[i_ * la_]
#define VR Q_[i_ * ra_]
  CASE(VADD) VLOOP(D_[i_].u = VL.u + VR.u)
  CASE(VSUB) VLOOP(D_[i_].u = VL.u - VR.u)
  CASE(VMUL) VLOOP(D_[i_].u = VL.u * VR.u)
  CASE(VDIV) VLOOP({ int32_t y = VR.i; if (!y) return rterr(vm, "division by zero", (uint32_t)(ip - code)); D_[i_].i = y == -1 ? (int32_t)(0u - VL.u) : VL.i / y; })
  CASE(VMOD) VLOOP({ int32_t y = VR.i; if (!y) return rterr(vm, "division by zero", (uint32_t)(ip - code)); D_[i_].i = y == -1 ? 0 : VL.i % y; })
  CASE(FVADD) VLOOP(D_[i_].f = VL.f + VR.f)
  CASE(FVSUB) VLOOP(D_[i_].f = VL.f - VR.f)
  CASE(FVMUL) VLOOP(D_[i_].f = VL.f * VR.f)
  CASE(FVDIV) VLOOP(D_[i_].f = VL.f / VR.f)
/* a builder in slots s..s+2: (address of the length word, address of the data, capacity) */
#define LLEN(s) (MV(U(s)).u < U((s) + 2) ? MV(U(s)).u : U((s) + 2)) /* its length, never above capacity */
  CASE(LIDX) {
    uint32_t i = U(C);
    if (i >= LLEN(B)) return rterr(vm, "index out of bounds", (uint32_t)(ip - code));
    U(A) = U(B + 1) + i * SZ(ip[1]); ip += 2; DISPATCH();
  }
  CASE(PUSHA) {
    uint32_t lw = U(B), cap = U(B + 2), n = LLEN(B);
    if (n >= cap) U(A) = 0; else { MV(lw).u = n + 1; U(A) = U(B + 1) + n * SZ(ip[1]); }
    ip += 2; DISPATCH();
  }
  CASE(PUSHS) {
    uint32_t lw = U(B), cap = U(B + 2), n = LLEN(B), p = U(C), k = U(C + 1), sz = SZ(ip[1]);
    if (k > cap - n) I(A) = 0; else { memmove(M + U(B + 1) + n * sz, M + p, k * sz); MV(lw).u = n + k; I(A) = 1; }
    ip += 2; DISPATCH();
  }
  CASE(PUSHT) {
    char t[40]; uint32_t lw = U(B), cap = U(B + 2), n = LLEN(B), k;
    if (ip->x == 1) k = (uint32_t)fmtf(t, F(C));
    else if (ip->x == 2) { k = I(C) ? 4u : 5u; memcpy(t, I(C) ? "true" : "false", k); }
    else k = (uint32_t)fmti(t, I(C));
    if (k > cap - n) I(A) = 0; else { memcpy(M + U(B + 1) + n, t, k); MV(lw).u = n + k; I(A) = 1; }
    NEXT();
  }
  CASE(POPA) {
    uint32_t n = LLEN(B);
    if (!n) return rterr(vm, "pop from empty list", (uint32_t)(ip - code));
    MV(U(B)).u = --n; U(A) = U(B + 1) + n * SZ(ip[1]); ip += 2; DISPATCH();
  }
  CASE(LREM) {
    uint32_t data = U(A + 1), n = LLEN(A), i = U(B), sz = SZ(ip[1]);
    if (i >= n) return rterr(vm, "index out of bounds", (uint32_t)(ip - code));
    memmove(M + data + i * sz, M + data + (i + 1) * sz, (n - i - 1) * sz); MV(U(A)).u = n - 1;
    ip += 2; DISPATCH();
  }
  CASE(LSWAP) {
    uint32_t data = U(A + 1), n = LLEN(A), i = U(B), sz = SZ(ip[1]);
    if (i >= n) return rterr(vm, "index out of bounds", (uint32_t)(ip - code));
    if (i != --n) memmove(M + data + i * sz, M + data + n * sz, sz);
    MV(U(A)).u = n; ip += 2; DISPATCH();
  }
  CASE(LVIEW) { uint32_t data = U(B + 1), n = LLEN(B); U(A) = data; U(A + 1) = n; NEXT(); }
  CASE(JMP) JUMP();
  CASE(JZ) if (!I(A)) JUMP(); NEXT();
  CASE(JNZ) if (I(A)) JUMP(); NEXT();
  CASE(JLT) if (I(A) < I(B)) JUMP(); NEXT();
  CASE(JLE) if (I(A) <= I(B)) JUMP(); NEXT();
  CASE(JEQ) if (I(A) == I(B)) JUMP(); NEXT();
  CASE(JNE) if (I(A) != I(B)) JUMP(); NEXT();
  CASE(JFLT) if (F(A) < F(B)) JUMP(); NEXT();
  CASE(JFNLT) if (!(F(A) < F(B))) JUMP(); NEXT();
  CASE(JFNLE) if (!(F(A) <= F(B))) JUMP(); NEXT();
  CASE(JFLE) if (F(A) <= F(B)) JUMP(); NEXT();
  CASE(JFEQ) if (F(A) == F(B)) JUMP(); NEXT();
  CASE(JFNE) if (F(A) != F(B)) JUMP(); NEXT();
  CASE(EACH) if ((U(A) += U(B + 1)) < U(B)) JUMP(); NEXT();
  CASE(FORI) if ((I(A) = (int32_t)(U(A) + 1u)) < I(B)) JUMP(); NEXT();
  CASE(CALL) cs[sp++] = (uint32_t)(ip + 1 - code); JUMP();
  CASE(RET) if (!sp) return 0; ip = code + cs[--sp]; DISPATCH();
  CASE(FFI) vm->ffi[C].fn(vm, R + A); if (vm->trap) { vm->trap = 0; return rterr(vm, vm->err, (uint32_t)(ip - code)); } NEXT();
  CASE(LOGI) { char b[16]; if (ip->x) logput(vm, " ", 1); logput(vm, b, fmti(b, I(A))); NEXT(); }
  CASE(LOGF) { char b[32]; if (ip->x) logput(vm, " ", 1); logput(vm, b, fmtf(b, F(A))); NEXT(); }
  CASE(LOGS) { if (ip->x) logput(vm, " ", 1); logput(vm, (const char *)M + I(A), I(A + 1)); NEXT(); }
  CASE(LOGB) if (ip->x) logput(vm, " ", 1); if (I(A)) logput(vm, "true", 4); else logput(vm, "false", 5); NEXT();
  CASE(LOGE) if (vm->logfn) vm->logfn(vm->logud, vm->logbuf, vm->logn); vm->logn = 0; NEXT();
  CASE(HALT) return 0;
#ifndef RIO_CGOTO
  default: return rterr(vm, "bad opcode", (uint32_t)(ip - code));
  }
#endif
stop:
  vm->brk = 0;
  return rterr(vm, "interrupted", (uint32_t)(ip - code));
}
int rio_run(Rio *vm) { if (!vm->running) vm->ekind = RIO_ENONE; vm->brk = 0; return vm->ok ? start(vm, 0) : -1; }
int rio_call(Rio *vm, int fn) { if (!vm->running) vm->ekind = RIO_ENONE; vm->brk = 0; return vm->ok && fn >= 0 && fn < vm->nfunc ? start(vm, vm->func[fn].pc) : -1; }
