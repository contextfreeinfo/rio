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

enum { TK_EOF = 256, TK_ID, TK_INT, TK_FLT, TK_STR, TK_PROC, TK_STRUCT, TK_IF, TK_ELSE, TK_FOR, TK_IN,
  TK_END, TK_RETURN, TK_BREAK, TK_CONTINUE, TK_DCOLON, TK_DECL, TK_EQ, TK_NE, TK_LE, TK_GE, TK_AND,
  TK_OR, TK_SHL, TK_SHR, TK_ARROW, TK_RLT, TK_RLE, TK_OPEQ };
enum { TY_VOID, TY_I32, TY_F32, TY_BYTE, TY_STR, TY_BLOB, TY_BOOL };
enum { K_VOID, K_I32, K_F32, K_BYTE, K_SLICE, K_ARR, K_STRUCT, K_BOOL };
enum { S_VAR = RIO_S_VAR, S_CONST, S_TYPE, S_FN, S_FFI, S_BI };
enum { EK_CONST, EK_ST, EK_MEM, EK_VOID, EK_FN, EK_FFI, EK_BI, EK_TY, EK_LEN };
enum { OK_BIN, OK_UN, OK_AND, OK_OR, OK_PAREN, OK_CALL, OK_IDX, OK_LIT };
enum { B_PROC, B_IF, B_ELSE, B_LOOP, B_FOR };
enum { BI_LOG, BI_LEN, BI_MIN, BI_MAX, BI_ABS, BI_SQRT, BI_ROUND = BI_SQRT + 11, BI_ATAN2, BI_FMOD = BI_ATAN2 + 2 };
#define NONE 0xFFFF
#define TY(t) (&vm->c->type[t])
#define TK (vm->c->tk)
typedef RioEx Ex;
typedef RioOp Op;

static const char *kw[] = {"proc", "struct", "if", "else", "for", "in", "end", "return", "break", "continue"};
static const char *bi[] = {"log", "len", "min", "max", "abs", "sqrt", "sin", "cos", "tan", "asin", "acos",
  "atan", "exp", "ln", "floor", "ceil", "round", "atan2", "pow", "fmod"};
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

NORET static void fail(Rio *vm, const char *m) {
  char *b = vm->err; int n = 0, i;
  n = cat(b, n, "line "); n += fmti(b + n, TK.nl ? vm->c->pline : TK.line); n = cat(b, n, ": "); n = cat(b, n, m);
  if (TK.t != TK_EOF && TK.n > 0 && !TK.nl) {
    n = cat(b, n, " near '");
    for (i = 0; i < TK.n && i < 24 && n < RIO_ERRBUF - 2; i++) b[n++] = TK.s[i];
    n = cat(b, n, "'");
  }
  b[n] = 0;
  longjmp(vm->c->jb, 1);
}

/* ---------------------------------------------------------------- lexer */
static int isal(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
static int isdg(int c) { return c >= '0' && c <= '9'; }
static int esc(int c) { return c == 'n' ? '\n' : c == 't' ? '\t' : c == 'r' ? '\r' : c == '0' ? 0 : c; }
static const char ops2[] = "::" ":=" "==" "!=" "<=" ">=" "&&" "||" "<<" ">>" "->" "+=" "-=" "*=" "/=" "%=" "&=" "|=" "^=";
static const short tok2[] = {TK_DCOLON, TK_DECL, TK_EQ, TK_NE, TK_LE, TK_GE, TK_AND, TK_OR, TK_SHL, TK_SHR,
  TK_ARROW, -'+', -'-', -'*', -'/', -'%', -'&', -'|', -'^'};

static void lex(Rio *vm, RioTok *t) {
  const char *p = vm->c->sp, *e = vm->c->se, *s;
  int nl = 0, i;
  for (;;) {
    while (p < e && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) { if (*p == '\n') { nl = 1; vm->c->line++; } p++; }
    if (p + 1 < e && p[0] == '/' && p[1] == '/') { while (p < e && *p != '\n') p++; continue; }
    if (p + 1 < e && p[0] == '/' && p[1] == '*') {
      for (p += 2; p + 1 < e && !(p[0] == '*' && p[1] == '/'); p++) if (*p == '\n') { nl = 1; vm->c->line++; }
      p = p + 1 < e ? p + 2 : e;
      continue;
    }
    break;
  }
  t->nl = (uint8_t)nl; t->line = vm->c->line; t->s = s = p; t->op = 0; t->v.i = 0;
  if (p >= e) { t->t = TK_EOF; t->n = 0; vm->c->sp = e; return; }
  if (isal(*p)) {
    while (p < e && (isal(*p) || isdg(*p))) p++;
    t->t = TK_ID;
    for (i = 0; i < 10; i++) if ((int)strlen(kw[i]) == p - s && !memcmp(kw[i], s, p - s)) t->t = TK_PROC + i;
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
    for (s = ++p; p < e && *p != '"'; p++) { if (*p == '\\') p++; if (p < e && *p == '\n') vm->c->line++; }
    if (p >= e) { vm->c->sp = e; fail(vm, "unterminated string"); }
    t->t = TK_STR; t->s = s; t->n = (int)(p - s); vm->c->sp = p + 1;
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
  t->n = (int)(p - s); vm->c->sp = p;
}
static void next(Rio *vm) { vm->c->pline = vm->c->tk.line; vm->c->tk = vm->c->nx; lex(vm, &vm->c->nx); }
static void expect(Rio *vm, int t, const char *m) { if (TK.t != t) fail(vm, m); next(vm); }

/* ---------------------------------------------------------------- tables */
static int addname(Rio *vm, const char *s, int n) {
  if (vm->c->nnames + n > RIO_NAMES) fail(vm, "out of name space");
  memcpy(vm->c->names + vm->c->nnames, s, (size_t)n); vm->c->nnames += n;
  return vm->c->nnames - n;
}
static void addsym(Rio *vm, const char *s, int n, int k, int t, int32_t v) {
  RioSym *y;
  if (vm->c->nsym >= RIO_MAX_SYMS) fail(vm, "too many symbols");
  y = &vm->c->sym[vm->c->nsym++];
  y->name = (uint16_t)addname(vm, s, n); y->len = (uint16_t)n; y->k = (uint8_t)k; y->t = (uint16_t)t; y->v = v;
}
static int lookup(Rio *vm, const char *s, int n) {
  int i;
  for (i = vm->c->nsym - 1; i >= 0; i--)
    if (vm->c->sym[i].len == n && !memcmp(vm->c->names + vm->c->sym[i].name, s, (size_t)n)) return i;
  return -1;
}
static int newtype(Rio *vm, int k, int elem, uint32_t n, uint32_t size) {
  RioType *y;
  if (vm->c->ntype >= RIO_MAX_TYPES) fail(vm, "too many types");
  y = &vm->c->type[vm->c->ntype];
  y->k = (uint8_t)k; y->elem = (uint16_t)elem; y->n = n; y->size = size; y->f0 = y->nf = 0;
  y->ref = (uint8_t)(k == K_SLICE || (k == K_ARR && vm->c->type[elem].ref));
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
static int vt(int t) { return t == TY_BYTE ? TY_I32 : t; }
static int words(Rio *vm, int t) { return (int)((TY(t)->size + 3) / 4); }

/* ---------------------------------------------------------------- code + storage */
static int emit(Rio *vm, int op, int a, int b, int c) {
  RioIns *i;
  if (vm->pc >= vm->codecap) fail(vm, "code too large");
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
    if (vm->c->hwm > RIO_MAX_SLOTS || vm->c->hwm * 4 > vm->hi) fail(vm, "out of slots");
  }
  return r;
}
static uint32_t halloc(Rio *vm, uint32_t n) {
  n = (n + 3) & ~3u;
  if (vm->hi < n || vm->hi - n < vm->c->hwm * 4) fail(vm, "out of memory");
  return vm->hi -= n;
}
#define KFIX(i) (vm->c->kfix[(i) >> 3] & (1u << ((i) & 7)))
static int kslot(Rio *vm, uint32_t v) {
  RioVal *R = (RioVal *)vm->mem; uint32_t i;
  for (i = 1; i < vm->nk; i++) if (R[i].u == v && !KFIX(i)) return (int)i;
  if (vm->nk >= RIO_MAX_CONSTS) fail(vm, "too many constants");
  R[vm->nk].u = v;
  return (int)vm->nk++;
}
static int kslot2(Rio *vm, uint32_t a, uint32_t b) {
  RioVal *R = (RioVal *)vm->mem; uint32_t i;
  for (i = 1; i + 1 < vm->nk; i++) if (R[i].u == a && R[i + 1].u == b && !KFIX(i)) return (int)i;
  if (vm->nk + 2 > RIO_MAX_CONSTS) fail(vm, "too many constants");
  R[vm->nk].u = a; R[vm->nk + 1].u = b; vm->nk += 2;
  return (int)vm->nk - 2;
}
/* constant holding a static address; marks those slots as address-taken (they can't become C locals in AOT) */
static uint32_t expose(Rio *vm, uint32_t a, uint32_t n) {
  uint32_t w;
  for (w = a / 4; w < RIO_MAX_SLOTS && w * 4 < a + n; w++) vm->c->exposed[w >> 3] |= (uint8_t)(1u << (w & 7));
  return a;
}
static int kaddr(Rio *vm, uint32_t a, uint32_t n) { return kslot(vm, expose(vm, a, n)); }
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
static Ex mkex(int k, int t, int32_t a) { Ex e; e.k = (uint8_t)k; e.ro = 0; e.t = (uint16_t)t; e.t0 = 0; e.a = a; e.off = 0; return e; }
static void needval(Rio *vm, Ex *e) { if (e->k > EK_MEM) fail(vm, e->k == EK_VOID ? "no value" : "not a value"); }
static void coerce(Rio *vm, Ex *e, int t) {
  int et;
  needval(vm, e);
  et = vt(e->t); t = vt(t);
  if (e->k == EK_CONST && et == TY_I32 && t == TY_F32) { RioVal v; v.f = (float)e->a; e->a = v.i; e->t = TY_F32; return; }
  if (et == t) return;
  if (TY(et)->k == K_ARR && TY(t)->k == K_SLICE && slice_of(vm, TY(et)->elem) == t) return;
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
    if (e->k == EK_ST) return kslot2(vm, expose(vm, (uint32_t)e->a, ty->size), ty->n);
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
  coerce(vm, s, d->t);
  if (k == K_STRUCT || k == K_ARR) {
    int sa = toaddr(vm, s), da = toaddr(vm, d);
    if (ty->size > 0xFFFF) fail(vm, "value too large to copy");
    emit(vm, OP_COPY, da, sa, (int)ty->size);
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
  if (e->t != TY_BOOL) fail(vm, "condition must be bool");
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
static void binop(Rio *vm, int op, Ex *l, Ex *r) {
  int lt, rt, isf, cmp, o, ls, rs, d, sw = 0;
  needval(vm, l); needval(vm, r);
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
static void unop(Rio *vm, int op, Ex *e) {
  int t, s, d;
  needval(vm, e); t = vt(e->t);
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
static void vpush(Rio *vm, Ex e) { if (vm->c->nvs >= RIO_MAX_EXPR) fail(vm, "expression too complex"); vm->c->vs[vm->c->nvs++] = e; }
static Op *opush(Rio *vm, int k, int prec, int op) {
  Op *o;
  if (vm->c->nos >= RIO_MAX_EXPR) fail(vm, "expression too complex");
  o = &vm->c->os[vm->c->nos++];
  o->k = (uint8_t)k; o->prec = (uint8_t)prec; o->op = (int16_t)op; o->a = o->b = o->c = o->n = 0;
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
    if (r.t != TY_BOOL) fail(vm, "expected bool");
    s = toslot(vm, &r, 1);
    vm->c->fr = (uint32_t)d + 1;
    if (s != d && !retarget(vm, s, d)) emit(vm, OP_MOV, d, s, 0);
    patch(vm, o.a, here(vm));
  }
}
static Ex ident(Rio *vm) {
  int i = lookup(vm, TK.s, TK.n); RioSym *y;
  if (i < 0) fail(vm, "undefined name");
  y = &vm->c->sym[i];
  switch (y->k) {
  case S_VAR: return mkex(EK_ST, y->t, y->v);
  case S_CONST: { Ex e = mkex(TY(y->t)->k <= K_F32 || TY(y->t)->k == K_BOOL ? EK_CONST : EK_ST, y->t, y->v); e.ro = 1; return e; }
  case S_FN: if (!vm->c->func[y->v].done) fail(vm, "recursion is not allowed"); return mkex(EK_FN, 0, y->v);
  case S_FFI: return mkex(EK_FFI, 0, y->v);
  case S_BI: return mkex(EK_BI, 0, y->v);
  default: return mkex(EK_TY, y->t, 0);
  }
}
static Ex strconst(Rio *vm, const char *s, int n) {
  RioC *c = vm->c; RioVal *R = (RioVal *)vm->mem; uint8_t *d = c->strs + c->pool; uint32_t k = vm->nk; int i, j = 0; Ex e;
  if ((uint32_t)n > c->poolcap - c->pool) fail(vm, "out of compiler memory");
  if (k + 2 > RIO_MAX_CONSTS) fail(vm, "too many constants");
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
    int m = cat(vm->err, 0, "-D "); m = cat(vm->err, m, name); m = cat(vm->err, m, ": value doesn't fit ");
    m = cat(vm->err, m, t == TY_I32 ? "i32" : t == TY_F32 ? "f32" : "bool"); vm->err[m] = 0;
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
static void doindex(Rio *vm, Ex *o, Ex *i) {
  RioType *ty; int el, sz, s, ix, d, ro;
  needval(vm, o); needval(vm, i); ty = TY(o->t);
  if (ty->k != K_ARR && ty->k != K_SLICE) fail(vm, "cannot index this");
  if (vt(i->t) != TY_I32) fail(vm, "index must be i32");
  el = ty->elem; sz = (int)TY(el)->size; ro = o->ro || o->t == TY_STR;
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
  RioType *ty; int s, l, h, d;
  needval(vm, o); ty = TY(o->t);
  if (ty->k != K_ARR && ty->k != K_SLICE) fail(vm, "cannot slice this");
  if (vt(lo->t) != TY_I32 || (hi->k != EK_LEN && vt(hi->t) != TY_I32)) fail(vm, "slice bounds must be i32");
  s = toslot2(vm, o); l = toslot(vm, lo, 1); h = hi->k == EK_LEN ? s + 1 : toslot(vm, hi, 1);
  vm->c->fr = o->t0; d = alloc(vm, 2);
  emit(vm, OP_SLICE, d, s, l); emitw(vm, h, TY(ty->elem)->size);
  o->k = EK_ST; o->a = d * 4; o->off = 0;
  if (ty->k == K_ARR) o->t = (uint16_t)slice_of(vm, ty->elem);
}
/* one argument of a call / struct literal is complete (on top of the value stack) */
static void argdone(Rio *vm, Op *m) {
  Ex *a = vtop(vm), e;
  needval(vm, a);
  if (m->k == OK_LIT) {
    RioType *st = TY(m->b); RioField *f; Ex d;
    if (m->n >= st->nf) fail(vm, "too many fields");
    f = &vm->c->field[st->f0 + m->n];
    e = vpop(vm); d = mkex(EK_ST, f->t, m->c + f->off);
    store(vm, &d, &e);
    vm->c->fr = (uint32_t)(m->c / 4 + words(vm, m->b));
  } else if (m->a == EK_FN) {
    int k = TY(a->t)->k;
    if (m->n >= vm->c->func[m->b].np) fail(vm, "too many arguments");
    if (a->k == EK_MEM && k != K_STRUCT && k != K_ARR) {
      int s = k == K_SLICE ? toslot2(vm, a) : toslot(vm, a, 1);
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
    else fail(vm, "len needs a slice or array");
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
static void cast(Rio *vm, Op *m) {
  Ex *a = &vm->c->vs[m->vb], r; int to = m->b, from, s, d;
  if (vm->c->nvs - m->vb != 1) fail(vm, "cast takes one value");
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
  } else fail(vm, "invalid cast");
  vm->c->nvs = m->vb; vres(vm, r, m->fr0);
}
static void finish_call(Rio *vm, Op *m) {
  if (m->a == EK_FN) {
    RioCFunc *f = &vm->c->func[m->b]; int i, d;
    if (m->n != f->np) fail(vm, "wrong number of arguments");
    for (i = 0; i < f->np; i++) {
      Ex p = mkex(EK_ST, vm->c->param[f->p0 + i].t, (int32_t)vm->c->param[f->p0 + i].addr);
      store(vm, &p, &vm->c->vs[m->vb + i]);
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
    if (m->b == BI_MIN || m->b == BI_MAX) minmax(vm, m); else builtin(vm, m);
  } else cast(vm, m);
}
static void finish_lit(Rio *vm, Op *m) {
  RioType *st = TY(m->b);
  if (m->n < st->nf) {
    int off = vm->c->field[st->f0 + m->n].off, w;
    if (st->size - off <= 64) for (w = off / 4; w < (int)st->size / 4; w++) emit(vm, OP_MOV, m->c / 4 + w, kslot(vm, 0), 0);
    else emit(vm, OP_ZERO, kaddr(vm, (uint32_t)(m->c + off), st->size - off), (int)st->size - off, 0);
  }
  vm->c->fr = (uint32_t)(m->c / 4 + words(vm, m->b));
  vres(vm, mkex(EK_ST, m->b, m->c), m->fr0);
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
static Ex expr(Rio *vm) {
  int ob = vm->c->nos, vb = vm->c->nvs, want = 1, depth = 0, t, p;
  for (;;) {
    t = TK.t;
    if (want) {
      Ex e; int t0 = (int)vm->c->fr;
      if (t == TK_INT || t == TK_FLT) e = mkex(EK_CONST, t == TK_INT ? TY_I32 : TY_F32, TK.v.i);
      else if (t == TK_STR) e = strlit(vm);
      else if (t == TK_ID) e = ident(vm);
      else if (t == '(') { opush(vm, OK_PAREN, 0, 0); depth++; next(vm); continue; }
      else if (t == '-' || t == '!' || t == '~') { opush(vm, OK_UN, 6, t); next(vm); continue; }
      else if ((t == ')' || t == '}') && vm->c->nos > ob && vm->c->os[vm->c->nos - 1].n == 0 &&
               vm->c->os[vm->c->nos - 1].k == (t == ')' ? OK_CALL : OK_LIT) && vm->c->nvs == vm->c->os[vm->c->nos - 1].vb) {
        closer(vm, t, 0); depth--; next(vm); want = 0; continue;
      } else fail(vm, "expected expression");
      vres(vm, e, t0); next(vm); want = 0;
      continue;
    }
    if (TK.nl && !depth) break;
    if (t == '(') {
      Ex c = *vtop(vm); Op *o;
      if (c.k < EK_FN || c.k > EK_TY) fail(vm, "not callable");
      if (c.k == EK_TY && TY(c.t)->k == K_STRUCT) fail(vm, "use Struct{...} to build a struct");
      vm->c->nvs--;
      o = opush(vm, OK_CALL, 0, 0); o->a = c.k; o->b = c.k == EK_TY ? c.t : c.a;
      depth++; next(vm); want = 1;
    } else if (t == '[') {
      opush(vm, OK_IDX, 0, 0); depth++; next(vm);
      if (TK.t == ':') { vres(vm, mkex(EK_CONST, TY_I32, 0), (int)vm->c->fr); want = 0; } else want = 1;
    } else if (t == '.') {
      next(vm);
      if (TK.t != TK_ID) fail(vm, "field name expected");
      field(vm, vtop(vm)); next(vm);
    } else if (t == '{') {
      Ex c = *vtop(vm); Op *o;
      if (c.k != EK_TY || TY(c.t)->k != K_STRUCT) break;
      vm->c->nvs--;
      o = opush(vm, OK_LIT, 0, 0); o->b = c.t; o->c = alloc(vm, words(vm, c.t)) * 4;
      depth++; next(vm); want = 1;
    } else if (t == ',' || t == ')' || t == ']' || t == '}' || t == ':') {
      Op *m;
      if (!depth) break;
      while (vm->c->os[vm->c->nos - 1].k < OK_PAREN) reduce1(vm);
      m = &vm->c->os[vm->c->nos - 1];
      if (t == ',') {
        if (m->k != OK_CALL && m->k != OK_LIT) fail(vm, "unexpected ','");
        argdone(vm, m); next(vm); want = 1;
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
        if (l->t != TY_BOOL) fail(vm, "expected bool");
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
    if (vm->c->os[vm->c->nos - 1].k >= OK_PAREN) fail(vm, "unclosed bracket");
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
    if (TK.t == ']') pre[np++] = -1; else pre[np++] = constexpr_i(vm);
    expect(vm, ']', "']' expected");
  }
  if (TK.t != TK_ID || (i = lookup(vm, TK.s, TK.n)) < 0 || vm->c->sym[i].k != S_TYPE) fail(vm, "type expected");
  t = vm->c->sym[i].t; next(vm);
  if (t == TY_BLOB && TK.t == '[' && !TK.nl) { next(vm); t = array_of(vm, TY_BYTE, (uint32_t)constexpr_i(vm)); expect(vm, ']', "']' expected"); }
  while (np) { int32_t n = pre[--np]; t = n < 0 ? slice_of(vm, t) : array_of(vm, t, (uint32_t)n); }
  return t;
}
static RioBlk *bpush(Rio *vm, int k) {
  RioBlk *b;
  if (vm->c->nblk >= RIO_MAX_BLOCKS) fail(vm, "blocks nested too deep");
  b = &vm->c->blk[vm->c->nblk++];
  b->k = (uint8_t)k; b->nsym = (uint16_t)vm->c->nsym; b->nnames = (uint16_t)vm->c->nnames; b->nact = (uint16_t)vm->c->nact;
  b->a = b->b = b->brk = b->cont = b->cj = NONE;
  return b;
}
static void bscope(Rio *vm, RioBlk *b) { vm->c->nsym = b->nsym; vm->c->nnames = b->nnames; vm->c->nact = vm->c->fr = b->nact; }
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
static int isbig(RioType *ty) { return (ty->k == K_ARR || ty->k == K_STRUCT) && ty->size > 64; }
static void proc_def(Rio *vm, const char *name, int nlen) {
  RioCFunc *f; RioBlk *b; int fi, skip;
  if (vm->c->nblk || vm->c->curfn >= 0) fail(vm, "procs must be top-level");
  if (vm->nfunc >= RIO_MAX_FUNCS) fail(vm, "too many procs");
  next(vm); expect(vm, '(', "'(' expected");
  skip = emit(vm, OP_JMP, 0, 0, NONE);
  fi = vm->nfunc++; f = &vm->c->func[fi];
  f->p0 = (uint16_t)vm->c->nparam; f->np = 0; f->done = 0; f->ret = TY_VOID; f->retaddr = 0;
  addsym(vm, name, nlen, S_FN, 0, fi);
  b = bpush(vm, B_PROC); b->a = (uint16_t)skip; b->b = (uint16_t)fi;
  vm->c->fr = vm->c->nact; f->fs = (uint16_t)vm->c->nact;
  while (TK.t != ')') {
    const char *ns[16]; int nl[16], c = namelist(vm, ns, nl), t = parse_type(vm), j;
    RioType *ty = TY(t);
    if (ty->k == K_VOID || ty->k == K_ARR || (ty->k == K_STRUCT && ty->ref)) fail(vm, "invalid parameter type (use a slice for arrays)");
    for (j = 0; j < c; j++) {
      int addr = alloc(vm, words(vm, t)) * 4;
      if (vm->c->nparam >= RIO_MAX_PARAMS) fail(vm, "too many parameters");
      vm->c->param[vm->c->nparam].t = (uint16_t)t; vm->c->param[vm->c->nparam++].addr = (uint32_t)addr;
      addsym(vm, ns[j], nl[j], S_VAR, t, addr); f->np++;
    }
    if (TK.t != ',') break;
    next(vm);
  }
  expect(vm, ')', "')' expected");
  if (TK.t == TK_ARROW) {
    RioType *ty;
    next(vm); f->ret = (uint16_t)parse_type(vm); ty = TY(f->ret);
    if (ty->k == K_ARR || (ty->k == K_STRUCT && ty->ref)) fail(vm, "invalid return type");
    f->retaddr = (uint32_t)alloc(vm, words(vm, f->ret)) * 4;
  }
  vm->c->nact = vm->c->fr; vm->c->curfn = fi; f->pc = (uint16_t)here(vm);
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
      if (vm->c->nfield >= RIO_MAX_FIELDS) fail(vm, "too many fields");
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
static void decl_var(Rio *vm) {
  const char *ns = TK.s; int nn = TK.n, t = -1, has = 0, k, big; uint32_t addr; RioType *ty; Ex e = mkex(EK_VOID, TY_VOID, 0);
  next(vm);
  if (TK.t == ':') { next(vm); t = parse_type(vm); if (TK.t == '=') { next(vm); has = 1; } }
  else { next(vm); has = 1; }
  if (has) {
    e = expr(vm); needval(vm, &e);
    if (t < 0) { t = vt(e.t); if (TY(t)->k == K_ARR) t = slice_of(vm, TY(t)->elem); }
    coerce(vm, &e, t);
  }
  ty = TY(t); k = ty->k;
  if (k == K_VOID) fail(vm, "variable has no type");
  if (vm->c->curfn >= 0 && (k == K_STRUCT || k == K_ARR) && ty->ref) fail(vm, "structs/arrays holding slices or strings must be global");
  big = isbig(ty);
  if (big) addr = halloc(vm, ty->size);
  else {
    if (has && e.k == EK_MEM && (k == K_I32 || k == K_F32 || k == K_BOOL || k == K_SLICE)) {
      int s = k == K_SLICE ? toslot2(vm, &e) : toslot(vm, &e, 1);
      e.k = EK_ST; e.a = s * 4; e.t = (uint16_t)t;
    }
    if (has && e.k == EK_ST && e.a == (int32_t)vm->c->nact * 4 && !e.ro && k != K_ARR) {
      vm->c->nact += (uint32_t)words(vm, t); vm->c->fr = vm->c->nact;
      addsym(vm, ns, nn, S_VAR, t, e.a);
      return;
    }
    addr = (uint32_t)alloc(vm, words(vm, t)) * 4; vm->c->nact = vm->c->fr;
  }
  if (has) { Ex d = mkex(EK_ST, t, (int32_t)addr); store(vm, &d, &e); }
  else if (!big && addr < 0x40000 && ty->size <= 64) { int w; for (w = 0; w < words(vm, t); w++) emit(vm, OP_MOV, (int)(addr >> 2) + w, kslot(vm, 0), 0); }
  else if (!(big && vm->c->curfn < 0 && !vm->c->nblk)) emit(vm, OP_ZERO, kaddr(vm, addr, ty->size), (int)(ty->size & 0xFFFF), (int)(ty->size >> 16));
  addsym(vm, ns, nn, S_VAR, t, (int32_t)addr);
}
static void stmt_expr(Rio *vm) {
  Ex l = expr(vm), r, cur;
  if (TK.t != '=' && TK.t != TK_OPEQ) return;
  if ((l.k != EK_ST && l.k != EK_MEM) || l.ro || (l.k == EK_ST && l.a < RIO_MAX_CONSTS * 4)) fail(vm, "cannot assign to this");
  if (TK.t == '=') { next(vm); r = expr(vm); store(vm, &l, &r); return; }
  {
    int op = TK.op, k = TY(l.t)->k;
    next(vm); cur = l;
    if (!(l.k == EK_ST && (k == K_I32 || k == K_F32) && l.a < 0x40000)) {
      int s = toslot(vm, &cur, 0);
      cur = mkex(EK_ST, vt(l.t), s * 4); cur.t0 = (uint16_t)s;
    }
    r = expr(vm); binop(vm, op, &cur, &r); store(vm, &l, &cur);
  }
}
static void statement(Rio *vm) {
  int t = TK.t, i; RioBlk *b; Ex e;
  vm->c->fr = vm->c->nact;
  if (t == ';') { next(vm); return; }
  if (t == TK_ID && vm->c->nx.t == TK_DCOLON) {
    const char *ns = TK.s; int nn = TK.n;
    next(vm); next(vm);
    if (TK.t == TK_PROC) proc_def(vm, ns, nn);
    else if (TK.t == TK_STRUCT) struct_def(vm, ns, nn);
    else {
      e = expr(vm);
      if (vm->c->curfn < 0 && !vm->c->nblk && (e.k == EK_CONST || e.t == TY_STR)) {
        int d = lookup(vm, ns, nn) - vm->c->def0;
        if (d >= 0 && d < vm->ndefs) e = defval(vm, d, e.t);
      }
      if (e.k == EK_CONST) addsym(vm, ns, nn, S_CONST, e.t, e.a);
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
    if (!vm->c->nblk || vm->c->blk[vm->c->nblk - 1].k != B_IF) fail(vm, "'else' without 'if'");
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
    else if (TK.t == TK_ID && vm->c->nx.t == TK_IN) {
      const char *ns = TK.s; int nn = TK.n, iv, lim, incl; Ex d;
      next(vm); next(vm);
      b = bpush(vm, B_FOR);
      vm->c->fr = vm->c->nact; iv = alloc(vm, 1); lim = alloc(vm, 1); vm->c->nact = vm->c->fr;
      e = expr(vm); d = mkex(EK_ST, TY_I32, iv * 4); store(vm, &d, &e); vm->c->fr = vm->c->nact;
      if (TK.t != TK_RLT && TK.t != TK_RLE) fail(vm, "'..<' or '..=' expected");
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
    if (!vm->c->nblk) fail(vm, "'end' without block");
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
        for (n = b->a; n <= cj; n++) { RioIns c = vm->code[n]; emit(vm, c.op, c.a, c.b, c.c); vm->code[vm->pc - 1].x = c.x; }
        invjump(&vm->code[vm->pc - 1]); vm->code[vm->pc - 1].c = (uint16_t)(cj + 1);
      } else { patch(vm, b->cont, b->a); emit(vm, OP_JMP, 0, 0, b->a); }
    }
    else if (b->k == B_FOR) { patch(vm, b->cont, here(vm)); emit(vm, OP_FORI, b->i, b->lim, b->a); }
    else patch(vm, b->b, here(vm));
    patch(vm, b->k == B_LOOP || b->k == B_FOR ? b->brk : b->a, here(vm));
    bscope(vm, b); vm->c->nblk--;
    break;
  case TK_RETURN: {
    RioCFunc *f;
    if (vm->c->curfn < 0) fail(vm, "return outside proc");
    next(vm); f = &vm->c->func[vm->c->curfn];
    if (f->ret != TY_VOID) { Ex d = mkex(EK_ST, f->ret, (int32_t)f->retaddr); e = expr(vm); store(vm, &d, &e); }
    emit(vm, OP_RET, 0, 0, 0);
    break;
  }
  case TK_BREAK: case TK_CONTINUE:
    for (i = vm->c->nblk - 1; i >= 0 && vm->c->blk[i].k != B_PROC; i--) if (vm->c->blk[i].k == B_LOOP || vm->c->blk[i].k == B_FOR) break;
    if (i < 0 || vm->c->blk[i].k == B_PROC) fail(vm, "not inside a loop");
    b = &vm->c->blk[i];
    if (t == TK_BREAK) b->brk = (uint16_t)jappend(vm, b->brk, emit(vm, OP_JMP, 0, 0, NONE));
    else b->cont = (uint16_t)jappend(vm, b->cont, emit(vm, OP_JMP, 0, 0, NONE));
    next(vm);
    break;
  default: stmt_expr(vm);
  }
  vm->c->fr = vm->c->nact;
  if (TK.t != TK_EOF && TK.t != ';' && !TK.nl) fail(vm, "expected end of statement");
}

/* ---------------------------------------------------------------- api */
int rio_init(Rio *vm, void *mem, uint32_t memsize, RioIns *code, uint32_t codecap) {
  uintptr_t pad = (16 - ((uintptr_t)mem & 15)) & 15;
  memset(vm, 0, sizeof *vm);
  if (!mem || memsize < pad + RIO_MAX_CONSTS * 4 + 256 || !code || codecap < 16) return -1;
  vm->mem = (uint8_t *)mem + pad; vm->memsize = (uint32_t)(memsize - pad) & ~15u;
  vm->code = code; vm->codecap = codecap > 65535 ? 65535 : codecap;
  return 0;
}
void rio_set_log(Rio *vm, RioLogFn fn, void *ud) { vm->logfn = fn; vm->logud = ud; }
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
uint32_t rio_scratch_min(void) { return (uint32_t)sizeof(RioC) + 16; }
/* builtin types and names, and the host's ffi functions */
static void setup(Rio *vm) {
  static const uint8_t tk[] = {K_VOID, K_I32, K_F32, K_BYTE, K_SLICE, K_SLICE, K_BOOL};
  static const uint8_t ts[] = {0, 4, 4, 1, 8, 8, 4};
  static const char *tn[] = {"i32", "f32", "string", "blob", "bool"};
  RioC *c = vm->c; int i;
  c->nact = c->fr = c->hwm = RIO_MAX_CONSTS; c->curfn = -1; c->lastlabel = NONE;
  for (i = 0; i < 7; i++) { c->type[i].k = tk[i]; c->type[i].size = ts[i]; c->type[i].elem = TY_BYTE; c->type[i].ref = i == TY_STR || i == TY_BLOB; }
  c->ntype = 7;
  for (i = 0; i < 5; i++) addsym(vm, tn[i], (int)strlen(tn[i]), S_TYPE, i < 2 ? TY_I32 + i : TY_STR + i - 2, 0);
  addsym(vm, "true", 4, S_CONST, TY_BOOL, 1); addsym(vm, "false", 5, S_CONST, TY_BOOL, 0);
  for (i = 0; i < (int)(sizeof bi / sizeof bi[0]); i++) addsym(vm, bi[i], (int)strlen(bi[i]), S_BI, 0, i);
  for (i = 0; i < vm->nffi; i++) {
    RioFfi *f = &vm->ffi[i]; RioCFfi *cf = &c->ffi[i]; const char *g;
    cf->p0 = (uint16_t)c->nparam; cf->np = 0; cf->ret = TY_VOID; f->aw = f->rw = 0;
    for (g = f->sig; *g; g++) {
      int ret = *g == '>', ch = ret ? g[1] : *g;
      int t = ch == 'i' ? TY_I32 : ch == 'f' ? TY_F32 : ch == 's' ? TY_STR : ch == 'b' ? TY_BLOB : -1;
      if (t < 0 || c->nparam >= RIO_MAX_PARAMS) fail(vm, "bad ffi signature");
      if (ret) { cf->ret = (uint16_t)t; f->rw = (uint8_t)words(vm, t); break; }
      c->param[c->nparam++].t = (uint16_t)t; cf->np++; f->aw = (uint8_t)(f->aw + words(vm, t));
    }
    addsym(vm, f->name, (int)strlen(f->name), S_FFI, 0, i);
  }
  c->def0 = c->nsym;
  for (i = 0; i < vm->ndefs; i++) {
    const char *nm = vm->defs[i][0]; int j, n = (int)strlen(nm); Ex e;
    for (j = 0; j < n; j++) if (!(isal(nm[j]) || (j && isdg(nm[j])))) break;
    if (!n || j < n) { int m = cat(vm->err, 0, "-D: bad name "); m = cat(vm->err, m, nm); vm->err[m] = 0; longjmp(c->jb, 1); }
    e = defval(vm, i, -1);
    addsym(vm, nm, n, S_CONST, e.t, e.a);
  }
}
/* lay out the final memory: string pool + export table go just below the big arrays, string
   constants get their real addresses, and everything the compiler used is zeroed */
static void finalize(Rio *vm) {
  RioC *c = vm->c; RioVal *R = (RioVal *)vm->mem; RioExport *x;
  uint32_t i, n = 0, p, ex, sz, base, first = RIO_MAX_CONSTS * 4; int k;
  c->fr = c->hwm; vm->csaddr = (uint32_t)alloc(vm, vm->nfunc + 1) * 4; /* return stack: depth <= #procs */
  p = c->pool;
  for (k = 0; k < c->nsym; k++) {
    RioSym *y = &c->sym[k];
    if (y->k != S_FN && y->k != S_VAR) continue;
    if (y->len > c->poolcap - p) fail(vm, "out of compiler memory");
    memcpy(c->strs + p, c->names + y->name, y->len); p += y->len; n++;
  }
  ex = (p + 3) & ~3u; sz = ex + n * (uint32_t)sizeof(RioExport);
  if (sz > c->poolcap) fail(vm, "out of compiler memory");
  if (vm->hi < sz || ((vm->hi - sz) & ~3u) < c->hwm * 4) fail(vm, "out of memory");
  base = (vm->hi - sz) & ~3u;
  x = (RioExport *)(void *)(c->strs + ex); p = c->pool;
  for (k = 0; k < c->nsym; k++) {
    RioSym *y = &c->sym[k];
    if (y->k != S_FN && y->k != S_VAR) continue;
    x->name = base + p; x->len = y->len; x->k = y->k; x->pad = 0; x->v = y->v; p += y->len; x++;
  }
  for (i = 1; i < vm->nk; i++) if (KFIX(i)) R[i].u = base + (R[i].u & 0x7FFFFFFFu);
  for (k = 0; k < vm->nfunc; k++) {
    RioCFunc *f = &c->func[k]; RioFunc *rf = &vm->func[k]; uint32_t pw = 0; int j;
    for (j = 0; j < f->np; j++) pw += (uint32_t)words(vm, c->param[f->p0 + j].t);
    rf->pc = f->pc; rf->end = f->end; rf->fs = f->fs; rf->fe = f->fe; rf->pend = (uint16_t)(f->fs + pw);
    rf->ret = (uint16_t)(f->retaddr / 4); rf->retw = (uint8_t)(f->ret ? words(vm, f->ret) : 0);
  }
  vm->exports = base + ex; vm->nexports = n; vm->hi = base;
  /* nothing in c is read after this: the move and the zeroing may overwrite it */
  memmove(vm->mem + base, c->strs, sz);
  memset(vm->mem + first, 0, base - first);
  memset(vm->mem + base + sz, 0, vm->memsize - base - sz);
}
static int compile(Rio *vm, const char *src, uint32_t len, uint8_t *scratch, uint32_t size, int inmem) {
  uintptr_t pad = scratch ? (16 - ((uintptr_t)scratch & 15)) & 15 : 0; RioC *c;
  vm->ok = 0; vm->c = 0;
  if (!scratch || size < pad + rio_scratch_min()) {
    int n = cat(vm->err, 0, inmem ? "not enough memory for the compiler" : "scratch too small"); vm->err[n] = 0;
    return -1;
  }
  c = vm->c = (RioC *)(void *)(scratch + pad);
  memset(c, 0, sizeof *c);
  c->strs = (uint8_t *)(c + 1); c->poolcap = size - (uint32_t)pad - (uint32_t)sizeof *c;
  vm->hi = vm->memsize; vm->nk = 1; vm->pc = 0; vm->nfunc = 0; ((RioVal *)vm->mem)[0].u = 0;
  if (setjmp(c->jb)) { if (inmem) vm->c = 0; return -1; }
  setup(vm);
  c->sp = src; c->se = src + len; c->line = 1;
  lex(vm, &c->nx); next(vm);
  while (TK.t != TK_EOF) statement(vm);
  if (c->nblk) fail(vm, "missing 'end'");
  emit(vm, OP_HALT, 0, 0, 0);
  finalize(vm);
  if (inmem) vm->c = 0;
  vm->ok = 1;
  return 0;
}
/* the compiler's state is overlaid on the part of mem that stays zero until the program runs */
int rio_compile(Rio *vm, const char *src, uint32_t len) {
  uint32_t k = RIO_MAX_CONSTS * 4;
  return compile(vm, src, len, vm->mem + k, vm->memsize - k, 1);
}
int rio_compile_scratch(Rio *vm, const char *src, uint32_t len, void *scratch, uint32_t size) {
  return compile(vm, src, len, (uint8_t *)scratch, size, 0);
}
const char *rio_error(Rio *vm) { return vm->err; }
void rio_trap(Rio *vm, const char *msg) { int n = cat(vm->err, 0, msg); vm->err[n] = 0; vm->trap = 1; }
static RioExport *findexp(Rio *vm, const char *name, int k) {
  RioExport *x = (RioExport *)(void *)(vm->mem + vm->exports); uint32_t i, n = (uint32_t)strlen(name);
  if (!vm->ok) return 0;
  for (i = vm->nexports; i-- > 0;)
    if (x[i].k == k && x[i].len == n && !memcmp(vm->mem + x[i].name, name, n)) return &x[i];
  return 0;
}
int rio_func(Rio *vm, const char *name) { RioExport *x = findexp(vm, name, S_FN); return x ? x->v : -1; }
void *rio_global(Rio *vm, const char *name) { RioExport *x = findexp(vm, name, S_VAR); return x ? vm->mem + x->v : 0; }
RioVal *rio_args(Rio *vm, int fn) { return (RioVal *)(void *)vm->mem + vm->func[fn].fs; }
RioVal *rio_ret(Rio *vm, int fn) { return (RioVal *)(void *)vm->mem + vm->func[fn].ret; }
void *rio_ptr(Rio *vm, const RioVal *s) {
  return (uint32_t)s[0].i <= vm->memsize && (uint32_t)s[1].i <= vm->memsize - (uint32_t)s[0].i ? vm->mem + s[0].i : 0;
}

/* ---------------------------------------------------------------- vm */
static void logput(Rio *vm, const char *s, int n) { while (n-- > 0 && vm->logn < RIO_LOGBUF) vm->logbuf[vm->logn++] = *s++; }
static int rterr(Rio *vm, const char *m, uint32_t pc) {
  int n = cat(vm->err, 0, "runtime error: "); n = cat(vm->err, n, m);
  n = cat(vm->err, n, " (pc "); n += fmti(vm->err + n, (int32_t)pc); n = cat(vm->err, n, ")");
  vm->err[n] = 0;
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
#define JUMP() { ip = code + C; DISPATCH(); }
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
  CASE(ZERO) memset(M + I(A), 0, SZ(*ip)); NEXT();
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
  CASE(FORI) if ((I(A) = (int32_t)(U(A) + 1u)) < I(B)) JUMP(); NEXT();
  CASE(CALL) cs[sp++] = (uint32_t)(ip + 1 - code); JUMP();
  CASE(RET) if (!sp) return 0; ip = code + cs[--sp]; DISPATCH();
  CASE(FFI) vm->ffi[C].fn(vm, R + A); if (vm->trap) { vm->trap = 0; return -1; } NEXT();
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
}
int rio_run(Rio *vm) { return vm->ok ? run(vm, 0) : -1; }
int rio_call(Rio *vm, int fn) { return vm->ok && fn >= 0 && fn < vm->nfunc ? run(vm, vm->func[fn].pc) : -1; }
