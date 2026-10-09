/* nib.c - compiler + vm. Single pass, no recursion, no allocation: everything lives in Nib and the
   caller's buffers. All locals are statically allocated (no recursion => one frame per proc), so
   bytecode operands are absolute word slots into the memory buffer. */
#include "nib.h"
#include <math.h>
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
#ifndef NIB_NO_CGOTO
#define NIB_CGOTO 1
#endif
#define NORET __attribute__((noreturn))
#elif defined(_MSC_VER)
#define NORET __declspec(noreturn)
#else
#define NORET
#endif

#define OPS(X) X(MOV) X(ADD) X(SUB) X(MUL) X(DIV) X(MOD) X(AND) X(OR) X(XOR) X(SHL) X(SHR) \
  X(NEG) X(BNOT) X(NOT) X(FADD) X(FSUB) X(FMUL) X(FDIV) X(FNEG) \
  X(EQ) X(NE) X(LT) X(LE) X(FEQ) X(FNE) X(FLT) X(FLE) X(SEQ) X(SNE) \
  X(ITOF) X(FTOI) X(LDW) X(LDB) X(LEA) X(M1) X(M2) X(IABS) X(IMIN) X(IMAX) X(FMIN) X(FMAX) \
  X(MOV2) X(LDW2) X(STW) X(STB) X(STW2) X(IDX) X(SLICE) X(COPY) X(ZERO) \
  X(JMP) X(JZ) X(JNZ) X(JLT) X(JLE) X(JEQ) X(JNE) X(JFLT) X(JFLE) X(JFEQ) X(JFNE) X(FORI) \
  X(CALL) X(RET) X(FFI) X(LOGI) X(LOGF) X(LOGS) X(LOGB) X(LOGE) X(HALT)
#define OPENUM(o) OP_##o,
enum { OPS(OPENUM) OP_DATA = 255 };
#define OP_LASTDST OP_FMAX /* ops up to here write one scalar to slot a */

enum { TK_EOF = 256, TK_ID, TK_INT, TK_FLT, TK_STR, TK_PROC, TK_STRUCT, TK_IF, TK_ELSE, TK_FOR, TK_IN,
  TK_END, TK_RETURN, TK_BREAK, TK_CONTINUE, TK_DCOLON, TK_DECL, TK_EQ, TK_NE, TK_LE, TK_GE, TK_AND,
  TK_OR, TK_SHL, TK_SHR, TK_ARROW, TK_RLT, TK_RLE, TK_OPEQ };
enum { TY_VOID, TY_I32, TY_F32, TY_BYTE, TY_STR, TY_BLOB, TY_BOOL };
enum { K_VOID, K_I32, K_F32, K_BYTE, K_SLICE, K_ARR, K_STRUCT, K_BOOL };
enum { S_VAR, S_CONST, S_TYPE, S_FN, S_FFI, S_BI };
enum { EK_CONST, EK_ST, EK_MEM, EK_VOID, EK_FN, EK_FFI, EK_BI, EK_TY, EK_LEN };
enum { OK_BIN, OK_UN, OK_AND, OK_OR, OK_PAREN, OK_CALL, OK_IDX, OK_LIT };
enum { B_PROC, B_IF, B_ELSE, B_LOOP, B_FOR };
enum { BI_LOG, BI_LEN, BI_MIN, BI_MAX, BI_ABS, BI_SQRT, BI_ROUND = BI_SQRT + 11, BI_ATAN2, BI_FMOD = BI_ATAN2 + 2 };
#define NONE 0xFFFF
#define TY(t) (&vm->type[t])
#define TK (vm->tk)
typedef NibEx Ex;
typedef NibOp Op;

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
static int cat(char *b, int n, const char *s) { while (*s && n < 180) b[n++] = *s++; return n; }

NORET static void fail(Nib *vm, const char *m) {
  char *b = vm->err; int n = 0, i;
  n = cat(b, n, "line "); n += fmti(b + n, TK.nl ? vm->pline : TK.line); n = cat(b, n, ": "); n = cat(b, n, m);
  if (TK.t != TK_EOF && TK.n > 0 && !TK.nl) {
    n = cat(b, n, " near '");
    for (i = 0; i < TK.n && i < 24; i++) b[n++] = TK.s[i];
    n = cat(b, n, "'");
  }
  b[n] = 0;
  longjmp(vm->jb, 1);
}

/* ---------------------------------------------------------------- lexer */
static int isal(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
static int isdg(int c) { return c >= '0' && c <= '9'; }
static int esc(int c) { return c == 'n' ? '\n' : c == 't' ? '\t' : c == 'r' ? '\r' : c == '0' ? 0 : c; }
static const char ops2[] = "::" ":=" "==" "!=" "<=" ">=" "&&" "||" "<<" ">>" "->" "+=" "-=" "*=" "/=" "%=" "&=" "|=" "^=";
static const short tok2[] = {TK_DCOLON, TK_DECL, TK_EQ, TK_NE, TK_LE, TK_GE, TK_AND, TK_OR, TK_SHL, TK_SHR,
  TK_ARROW, -'+', -'-', -'*', -'/', -'%', -'&', -'|', -'^'};

static void lex(Nib *vm, NibTok *t) {
  const char *p = vm->sp, *e = vm->se, *s;
  int nl = 0, i;
  for (;;) {
    while (p < e && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) { if (*p == '\n') { nl = 1; vm->line++; } p++; }
    if (p + 1 < e && p[0] == '/' && p[1] == '/') { while (p < e && *p != '\n') p++; continue; }
    if (p + 1 < e && p[0] == '/' && p[1] == '*') {
      for (p += 2; p + 1 < e && !(p[0] == '*' && p[1] == '/'); p++) if (*p == '\n') { nl = 1; vm->line++; }
      p = p + 1 < e ? p + 2 : e;
      continue;
    }
    break;
  }
  t->nl = (uint8_t)nl; t->line = vm->line; t->s = s = p; t->op = 0; t->v.i = 0;
  if (p >= e) { t->t = TK_EOF; t->n = 0; vm->sp = e; return; }
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
    for (s = ++p; p < e && *p != '"'; p++) { if (*p == '\\') p++; if (p < e && *p == '\n') vm->line++; }
    if (p >= e) { vm->sp = e; fail(vm, "unterminated string"); }
    t->t = TK_STR; t->s = s; t->n = (int)(p - s); vm->sp = p + 1;
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
  t->n = (int)(p - s); vm->sp = p;
}
static void next(Nib *vm) { vm->pline = vm->tk.line; vm->tk = vm->nx; lex(vm, &vm->nx); }
static void expect(Nib *vm, int t, const char *m) { if (TK.t != t) fail(vm, m); next(vm); }

/* ---------------------------------------------------------------- tables */
static int addname(Nib *vm, const char *s, int n) {
  if (vm->nnames + n > NIB_NAMES) fail(vm, "out of name space");
  memcpy(vm->names + vm->nnames, s, (size_t)n); vm->nnames += n;
  return vm->nnames - n;
}
static void addsym(Nib *vm, const char *s, int n, int k, int t, int32_t v) {
  NibSym *y;
  if (vm->nsym >= NIB_MAX_SYMS) fail(vm, "too many symbols");
  y = &vm->sym[vm->nsym++];
  y->name = (uint16_t)addname(vm, s, n); y->len = (uint16_t)n; y->k = (uint8_t)k; y->t = (uint16_t)t; y->v = v;
}
static int lookup(Nib *vm, const char *s, int n) {
  int i;
  for (i = vm->nsym - 1; i >= 0; i--)
    if (vm->sym[i].len == n && !memcmp(vm->names + vm->sym[i].name, s, (size_t)n)) return i;
  return -1;
}
static int newtype(Nib *vm, int k, int elem, uint32_t n, uint32_t size) {
  NibType *y;
  if (vm->ntype >= NIB_MAX_TYPES) fail(vm, "too many types");
  y = &vm->type[vm->ntype];
  y->k = (uint8_t)k; y->elem = (uint16_t)elem; y->n = n; y->size = size; y->f0 = y->nf = 0;
  y->ref = (uint8_t)(k == K_SLICE || (k == K_ARR && vm->type[elem].ref));
  return vm->ntype++;
}
static int slice_of(Nib *vm, int e) {
  int i;
  if (e == TY_BYTE) return TY_BLOB;
  for (i = TY_BLOB + 1; i < vm->ntype; i++) if (vm->type[i].k == K_SLICE && vm->type[i].elem == e) return i;
  return newtype(vm, K_SLICE, e, 0, 8);
}
static int array_of(Nib *vm, int e, uint32_t n) {
  int i; uint32_t es = TY(e)->size;
  for (i = TY_BLOB + 1; i < vm->ntype; i++) if (vm->type[i].k == K_ARR && vm->type[i].elem == e && vm->type[i].n == n) return i;
  if (es && n > 0x7FFFFFF0u / es) fail(vm, "array too large");
  return newtype(vm, K_ARR, e, n, (es * n + 3) & ~3u);
}
static int vt(int t) { return t == TY_BYTE ? TY_I32 : t; }
static int words(Nib *vm, int t) { return (int)((TY(t)->size + 3) / 4); }

/* ---------------------------------------------------------------- code + storage */
static int emit(Nib *vm, int op, int a, int b, int c) {
  NibIns *i;
  if (vm->pc >= vm->codecap) fail(vm, "code too large");
  i = &vm->code[vm->pc];
  i->op = (uint8_t)op; i->x = 0; i->a = (uint16_t)a; i->b = (uint16_t)b; i->c = (uint16_t)c;
  return (int)vm->pc++;
}
static void emitx(Nib *vm, int op, int x, int a) { emit(vm, op, a, 0, 0); vm->code[vm->pc - 1].x = (uint8_t)x; }
static void emitw(Nib *vm, int a, uint32_t w) { emit(vm, OP_DATA, a, (int)(w & 0xFFFF), (int)(w >> 16)); }
static int here(Nib *vm) { return (int)(vm->lastlabel = vm->pc); }
static int jappend(Nib *vm, int list, int j) { if (j == NONE) return list; vm->code[j].c = (uint16_t)list; return j; }
static void patch(Nib *vm, int list, int to) {
  while (list != NONE) { int n = vm->code[list].c; vm->code[list].c = (uint16_t)to; list = n; }
}
static int alloc(Nib *vm, int n) {
  int r = (int)vm->fr;
  vm->fr += (uint32_t)n;
  if (vm->fr > vm->hwm) {
    vm->hwm = vm->fr;
    if (vm->hwm > 0xFFFF || vm->hwm * 4 > vm->hi) fail(vm, "out of slots");
  }
  return r;
}
static uint32_t halloc(Nib *vm, uint32_t n) {
  n = (n + 3) & ~3u;
  if (vm->hi < n || vm->hi - n < vm->hwm * 4) fail(vm, "out of memory");
  return vm->hi -= n;
}
static int kslot(Nib *vm, uint32_t v) {
  NibVal *R = (NibVal *)vm->mem; uint32_t i;
  for (i = 1; i < vm->nk; i++) if (R[i].u == v) return (int)i;
  if (vm->nk >= NIB_MAX_CONSTS) fail(vm, "too many constants");
  R[vm->nk].u = v;
  return (int)vm->nk++;
}
static int kslot2(Nib *vm, uint32_t a, uint32_t b) {
  NibVal *R = (NibVal *)vm->mem; uint32_t i;
  for (i = 1; i + 1 < vm->nk; i++) if (R[i].u == a && R[i + 1].u == b) return (int)i;
  if (vm->nk + 2 > NIB_MAX_CONSTS) fail(vm, "too many constants");
  R[vm->nk].u = a; R[vm->nk + 1].u = b; vm->nk += 2;
  return (int)vm->nk - 2;
}
/* if the last instruction wrote temp `from`, make it write `to` instead */
static int retarget(Nib *vm, int from, int to) {
  NibIns *i;
  if (!vm->pc || vm->lastlabel == vm->pc || from < (int)vm->nact) return 0;
  i = &vm->code[vm->pc - 1];
  if (i->op > OP_LASTDST || i->a != from) return 0;
  i->a = (uint16_t)to;
  return 1;
}

/* ---------------------------------------------------------------- expression values */
static Ex mkex(int k, int t, int32_t a) { Ex e; e.k = (uint8_t)k; e.ro = 0; e.t = (uint16_t)t; e.t0 = 0; e.a = a; e.off = 0; return e; }
static void needval(Nib *vm, Ex *e) { if (e->k > EK_MEM) fail(vm, e->k == EK_VOID ? "no value" : "not a value"); }
static void coerce(Nib *vm, Ex *e, int t) {
  int et;
  needval(vm, e);
  et = vt(e->t); t = vt(t);
  if (e->k == EK_CONST && et == TY_I32 && t == TY_F32) { NibVal v; v.f = (float)e->a; e->a = v.i; e->t = TY_F32; return; }
  if (et == t) return;
  if (TY(et)->k == K_ARR && TY(t)->k == K_SLICE && slice_of(vm, TY(et)->elem) == t) return;
  fail(vm, "type mismatch");
}
/* scalar value -> slot holding it */
static int toslot(Nib *vm, Ex *e, int reuse) {
  int k, d;
  needval(vm, e); k = TY(e->t)->k;
  if (k != K_I32 && k != K_F32 && k != K_BYTE && k != K_BOOL) fail(vm, "expected a number");
  if (e->k == EK_CONST) return kslot(vm, (uint32_t)e->a);
  if (e->k == EK_ST) {
    if (k != K_BYTE && e->a < 0x40000) return e->a >> 2;
    e->k = EK_MEM; e->a = kslot(vm, (uint32_t)e->a); e->off = 0;
  }
  d = reuse && e->a >= (int)vm->nact ? e->a : alloc(vm, 1);
  emit(vm, k == K_BYTE ? OP_LDB : OP_LDW, d, e->a, e->off);
  return d;
}
/* slice/string/blob/array value -> 2 consecutive slots (addr, len) */
static int toslot2(Nib *vm, Ex *e) {
  NibType *ty; int d;
  needval(vm, e); ty = TY(e->t);
  if (ty->k == K_ARR) {
    if (e->k == EK_ST) return kslot2(vm, (uint32_t)e->a, ty->n);
    d = alloc(vm, 2);
    emit(vm, OP_LEA, d, e->a, e->off); emit(vm, OP_MOV, d + 1, kslot(vm, ty->n), 0);
    return d;
  }
  if (ty->k != K_SLICE) fail(vm, "expected a slice");
  if (e->k == EK_ST) {
    if (e->a < 0x3FFFC) return e->a >> 2;
    e->k = EK_MEM; e->a = kslot(vm, (uint32_t)e->a); e->off = 0;
  }
  d = alloc(vm, 2);
  emit(vm, OP_LDW2, d, e->a, e->off);
  return d;
}
/* struct/array -> slot holding its address */
static int toaddr(Nib *vm, Ex *e) {
  int d;
  needval(vm, e);
  if (e->k == EK_ST) return kslot(vm, (uint32_t)e->a);
  if (!e->off) return e->a;
  d = alloc(vm, 1);
  emit(vm, OP_LEA, d, e->a, e->off);
  return d;
}
static void store(Nib *vm, Ex *d, Ex *s) {
  NibType *ty = TY(d->t); int k = ty->k, ss;
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
    if (d->k == EK_ST) { d->k = EK_MEM; d->a = kslot(vm, (uint32_t)d->a); d->off = 0; }
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
  if (d->k == EK_ST) { d->k = EK_MEM; d->a = kslot(vm, (uint32_t)d->a); d->off = 0; }
  emit(vm, k == K_BYTE ? OP_STB : OP_STW, d->a, ss, d->off);
}
/* emit a jump taken when e is false; fuses a preceding compare into a compare-and-branch */
static int condjump(Nib *vm, Ex *e) {
  static const uint8_t inv[] = {OP_JNE, OP_JEQ, OP_JLE, OP_JLT, OP_JFNE, OP_JFEQ, OP_JFLE, OP_JFLT};
  needval(vm, e);
  if (e->t != TY_BOOL) fail(vm, "condition must be bool");
  if (e->k == EK_CONST) return e->a ? NONE : emit(vm, OP_JMP, 0, 0, NONE);
  if (e->k == EK_ST && e->a < 0x40000 && (e->a >> 2) >= (int)vm->nact && vm->pc && vm->lastlabel != vm->pc) {
    NibIns *i = &vm->code[vm->pc - 1];
    if (i->a == e->a >> 2 && i->op >= OP_EQ && i->op <= OP_FLE) {
      int o = i->op - OP_EQ, b = i->b, c = i->c, sw = o == 2 || o == 3 || o == 6 || o == 7;
      i->op = inv[o]; i->a = (uint16_t)(sw ? c : b); i->b = (uint16_t)(sw ? b : c); i->c = NONE;
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
static void fold(Nib *vm, int op, Ex *l, Ex *r, int isf) {
  NibVal a, b, c; int32_t x, y;
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
static void binop(Nib *vm, int op, Ex *l, Ex *r) {
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
  vm->fr = l->t0 < r->t0 ? l->t0 : r->t0;
  d = alloc(vm, 1);
  emit(vm, o, d, ls, rs);
  l->k = EK_ST; l->a = d * 4; l->t = (uint16_t)(cmp ? TY_BOOL : lt); l->ro = 0; l->t0 = (uint16_t)d;
}
static void unop(Nib *vm, int op, Ex *e) {
  int t, s, d;
  needval(vm, e); t = vt(e->t);
  if (op == '!' ? t != TY_BOOL : t != TY_I32 && !(t == TY_F32 && op == '-')) fail(vm, "bad operand for unary operator");
  if (e->k == EK_CONST) {
    NibVal v; v.i = e->a;
    if (t == TY_F32) v.f = -v.f; else v.i = op == '-' ? (int32_t)(0u - v.u) : op == '!' ? !v.i : ~v.i;
    e->a = v.i; e->ro = 0;
    return;
  }
  s = toslot(vm, e, 1);
  vm->fr = e->t0; d = alloc(vm, 1);
  emit(vm, op == '-' ? (t == TY_F32 ? OP_FNEG : OP_NEG) : op == '!' ? OP_NOT : OP_BNOT, d, s, 0);
  e->k = EK_ST; e->a = d * 4; e->t = (uint16_t)t; e->ro = 0;
}

/* ---------------------------------------------------------------- expression parser (shunting-yard) */
static Ex *vtop(Nib *vm) { return &vm->vs[vm->nvs - 1]; }
static Ex vpop(Nib *vm) { return vm->vs[--vm->nvs]; }
static void vpush(Nib *vm, Ex e) { if (vm->nvs >= NIB_MAX_EXPR) fail(vm, "expression too complex"); vm->vs[vm->nvs++] = e; }
static Op *opush(Nib *vm, int k, int prec, int op) {
  Op *o;
  if (vm->nos >= NIB_MAX_EXPR) fail(vm, "expression too complex");
  o = &vm->os[vm->nos++];
  o->k = (uint8_t)k; o->prec = (uint8_t)prec; o->op = (int16_t)op; o->a = o->b = o->c = o->n = 0;
  o->vb = (uint16_t)vm->nvs; o->fr0 = (uint16_t)vm->fr;
  return o;
}
static void vres(Nib *vm, Ex e, int t0) { e.t0 = (uint16_t)t0; vpush(vm, e); }
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
static void reduce1(Nib *vm) {
  Op o = vm->os[--vm->nos]; Ex r;
  if (o.k == OK_UN) { unop(vm, o.op, vtop(vm)); return; }
  r = vpop(vm);
  if (o.k == OK_BIN) { binop(vm, o.op, vtop(vm), &r); return; }
  {
    int s, d = o.b;
    needval(vm, &r);
    if (r.t != TY_BOOL) fail(vm, "expected bool");
    s = toslot(vm, &r, 1);
    vm->fr = (uint32_t)d + 1;
    if (s != d && !retarget(vm, s, d)) emit(vm, OP_MOV, d, s, 0);
    patch(vm, o.a, here(vm));
  }
}
static Ex ident(Nib *vm) {
  int i = lookup(vm, TK.s, TK.n); NibSym *y;
  if (i < 0) fail(vm, "undefined name");
  y = &vm->sym[i];
  switch (y->k) {
  case S_VAR: return mkex(EK_ST, y->t, y->v);
  case S_CONST: { Ex e = mkex(TY(y->t)->k <= K_F32 || TY(y->t)->k == K_BOOL ? EK_CONST : EK_ST, y->t, y->v); e.ro = 1; return e; }
  case S_FN: if (!vm->func[y->v].done) fail(vm, "recursion is not allowed"); return mkex(EK_FN, 0, y->v);
  case S_FFI: return mkex(EK_FFI, 0, y->v);
  case S_BI: return mkex(EK_BI, 0, y->v);
  default: return mkex(EK_TY, y->t, 0);
  }
}
static Ex strlit(Nib *vm) {
  int i, j = 0; uint32_t a = halloc(vm, (uint32_t)TK.n + 1);
  uint8_t *d = vm->mem + a; Ex e;
  for (i = 0; i < TK.n; i++) d[j++] = (uint8_t)(TK.s[i] == '\\' && i + 1 < TK.n ? esc(TK.s[++i]) : TK.s[i]);
  e = mkex(EK_ST, TY_STR, kslot2(vm, a, (uint32_t)j) * 4); e.ro = 1;
  return e;
}
static void field(Nib *vm, Ex *e) {
  NibType *st; NibField *f = 0; int i;
  needval(vm, e); st = TY(e->t);
  if (st->k != K_STRUCT) fail(vm, "not a struct");
  for (i = 0; i < st->nf; i++) {
    f = &vm->field[st->f0 + i];
    if (f->len == TK.n && !memcmp(vm->names + f->name, TK.s, (size_t)TK.n)) break;
  }
  if (i == st->nf) fail(vm, "no such field");
  if (e->k == EK_ST) e->a += f->off; else e->off += f->off;
  e->t = f->t;
}
static void doindex(Nib *vm, Ex *o, Ex *i) {
  NibType *ty; int el, sz, s, ix, d, ro;
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
  vm->fr = o->t0; d = alloc(vm, 1);
  emit(vm, OP_IDX, d, s, ix); emitw(vm, 0, (uint32_t)sz);
  o->k = EK_MEM; o->a = d; o->off = 0; o->t = (uint16_t)el; o->ro = (uint8_t)ro;
}
static void doslice(Nib *vm, Ex *o, Ex *lo, Ex *hi) {
  NibType *ty; int s, l, h, d;
  needval(vm, o); ty = TY(o->t);
  if (ty->k != K_ARR && ty->k != K_SLICE) fail(vm, "cannot slice this");
  if (vt(lo->t) != TY_I32 || (hi->k != EK_LEN && vt(hi->t) != TY_I32)) fail(vm, "slice bounds must be i32");
  s = toslot2(vm, o); l = toslot(vm, lo, 1); h = hi->k == EK_LEN ? s + 1 : toslot(vm, hi, 1);
  vm->fr = o->t0; d = alloc(vm, 2);
  emit(vm, OP_SLICE, d, s, l); emitw(vm, h, TY(ty->elem)->size);
  o->k = EK_ST; o->a = d * 4; o->off = 0;
  if (ty->k == K_ARR) o->t = (uint16_t)slice_of(vm, ty->elem);
}
/* one argument of a call / struct literal is complete (on top of the value stack) */
static void argdone(Nib *vm, Op *m) {
  Ex *a = vtop(vm), e;
  needval(vm, a);
  if (m->k == OK_LIT) {
    NibType *st = TY(m->b); NibField *f; Ex d;
    if (m->n >= st->nf) fail(vm, "too many fields");
    f = &vm->field[st->f0 + m->n];
    e = vpop(vm); d = mkex(EK_ST, f->t, m->c + f->off);
    store(vm, &d, &e);
    vm->fr = (uint32_t)(m->c / 4 + words(vm, m->b));
  } else if (m->a == EK_FN) {
    int k = TY(a->t)->k;
    if (m->n >= vm->func[m->b].np) fail(vm, "too many arguments");
    if (a->k == EK_MEM && k != K_STRUCT && k != K_ARR) {
      int s = k == K_SLICE ? toslot2(vm, a) : toslot(vm, a, 1);
      a->k = EK_ST; a->a = s * 4; a->t = (uint16_t)vt(a->t);
    }
  } else if (m->a == EK_FFI) {
    NibFfi *f = &vm->ffi[m->b]; int pt, w, s, d;
    if (m->n >= f->np) fail(vm, "too many arguments");
    e = vpop(vm); pt = vm->param[f->p0 + m->n].t;
    coerce(vm, &e, pt); w = words(vm, pt);
    s = w == 2 ? toslot2(vm, &e) : toslot(vm, &e, 1);
    vm->fr = (uint32_t)(m->fr0 + m->c); d = alloc(vm, w);
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
    vm->fr = e.t0;
  }
  m->n++;
}
static void builtin(Nib *vm, Op *m) {
  int id = m->b, n = vm->nvs - m->vb, t, o = 0, x = 0, s1, s2 = 0, d;
  Ex *a = &vm->vs[m->vb], r;
  if (id == BI_LOG) { emit(vm, OP_LOGE, 0, 0, 0); vm->fr = m->fr0; vres(vm, mkex(EK_VOID, TY_VOID, 0), m->fr0); return; }
  if (n != (id >= BI_ATAN2 ? 2 : 1)) fail(vm, "wrong number of arguments");
  if (id == BI_LEN) {
    int k = TY(a->t)->k;
    if (k == K_ARR) r = mkex(EK_CONST, TY_I32, (int32_t)TY(a->t)->n);
    else if (k == K_SLICE) { r = *a; if (r.k == EK_ST) r.a += 4; else r.off += 4; r.t = TY_I32; r.ro = 1; }
    else fail(vm, "len needs a slice or array");
    vm->nvs = m->vb; vres(vm, r, m->fr0);
    return;
  }
  t = vt(a->t);
  if (id == BI_ABS && t == TY_I32) {
    if (a->k == EK_CONST) { r = *a; r.a = a->a < 0 ? (int32_t)(0u - (uint32_t)a->a) : a->a; vm->nvs = m->vb; vres(vm, r, m->fr0); return; }
    o = OP_IABS;
  } else if (id == BI_ABS || (id >= BI_SQRT && id <= BI_ROUND)) {
    coerce(vm, a, TY_F32); o = OP_M1; x = id == BI_ABS ? 12 : id - BI_SQRT;
    if (a->k == EK_CONST) { NibVal v; v.i = a->a; v.f = mf1[x](v.f); r = mkex(EK_CONST, TY_F32, v.i); vm->nvs = m->vb; vres(vm, r, m->fr0); return; }
  } else {
    coerce(vm, a, TY_F32); coerce(vm, a + 1, TY_F32); o = OP_M2; x = id - BI_ATAN2;
    s2 = toslot(vm, a + 1, 1);
  }
  s1 = toslot(vm, a, 1);
  vm->nvs = m->vb; vm->fr = m->fr0; d = alloc(vm, 1);
  emit(vm, o, d, s1, s2); vm->code[vm->pc - 1].x = (uint8_t)x;
  vres(vm, mkex(EK_ST, o == OP_IABS ? TY_I32 : TY_F32, d * 4), m->fr0);
}
static void minmax(Nib *vm, Op *m) {
  Ex *a = &vm->vs[m->vb]; int t, s1, s2, d, mx = m->b == BI_MAX;
  if (vm->nvs - m->vb != 2) fail(vm, "wrong number of arguments");
  needval(vm, a); needval(vm, a + 1);
  if (a[0].k == EK_CONST && vt(a[1].t) == TY_F32) coerce(vm, a, TY_F32);
  if (a[1].k == EK_CONST && vt(a[0].t) == TY_F32) coerce(vm, a + 1, TY_F32);
  t = vt(a->t);
  if (t != vt(a[1].t) || (t != TY_I32 && t != TY_F32)) fail(vm, "type mismatch");
  s1 = toslot(vm, a, 1); s2 = toslot(vm, a + 1, 1);
  vm->nvs = m->vb; vm->fr = m->fr0; d = alloc(vm, 1);
  emit(vm, t == TY_I32 ? (mx ? OP_IMAX : OP_IMIN) : (mx ? OP_FMAX : OP_FMIN), d, s1, s2);
  vres(vm, mkex(EK_ST, t, d * 4), m->fr0);
}
static void cast(Nib *vm, Op *m) {
  Ex *a = &vm->vs[m->vb], r; int to = m->b, from, s, d;
  if (vm->nvs - m->vb != 1) fail(vm, "cast takes one value");
  needval(vm, a); from = vt(a->t); r = *a;
  if (to == from) { /* no-op */ }
  else if (to == TY_STR && from == TY_BLOB) { r.t = TY_STR; r.ro = 1; }
  else if (to == TY_I32 && from == TY_BOOL) r.t = TY_I32;
  else if (to == TY_BOOL && from == TY_I32) {
    if (a->k == EK_CONST) r = mkex(EK_CONST, TY_BOOL, a->a != 0);
    else { s = toslot(vm, a, 1); vm->fr = m->fr0; d = alloc(vm, 1); emit(vm, OP_NE, d, s, kslot(vm, 0)); r = mkex(EK_ST, TY_BOOL, d * 4); }
  }
  else if ((to == TY_F32 && from == TY_I32) || (to == TY_I32 && from == TY_F32)) {
    if (a->k == EK_CONST) { NibVal v; v.i = a->a; if (to == TY_F32) v.f = (float)v.i; else v.i = ftoi(v.f); r = mkex(EK_CONST, to, v.i); }
    else { s = toslot(vm, a, 1); vm->fr = m->fr0; d = alloc(vm, 1); emit(vm, to == TY_F32 ? OP_ITOF : OP_FTOI, d, s, 0); r = mkex(EK_ST, to, d * 4); }
  } else fail(vm, "invalid cast");
  vm->nvs = m->vb; vres(vm, r, m->fr0);
}
static void finish_call(Nib *vm, Op *m) {
  if (m->a == EK_FN) {
    NibFunc *f = &vm->func[m->b]; int i, d;
    if (m->n != f->np) fail(vm, "wrong number of arguments");
    for (i = 0; i < f->np; i++) {
      Ex p = mkex(EK_ST, vm->param[f->p0 + i].t, (int32_t)vm->param[f->p0 + i].addr);
      store(vm, &p, &vm->vs[m->vb + i]);
    }
    vm->nvs = m->vb;
    emit(vm, OP_CALL, 0, 0, f->pc);
    vm->fr = m->fr0;
    if (f->ret == TY_VOID) { vres(vm, mkex(EK_VOID, TY_VOID, 0), m->fr0); return; }
    {
      Ex src = mkex(EK_ST, f->ret, (int32_t)f->retaddr), dst;
      d = alloc(vm, words(vm, f->ret)); dst = mkex(EK_ST, f->ret, d * 4);
      store(vm, &dst, &src); vres(vm, dst, m->fr0);
    }
  } else if (m->a == EK_FFI) {
    NibFfi *f = &vm->ffi[m->b];
    if (m->n != f->np) fail(vm, "wrong number of arguments");
    emit(vm, OP_FFI, m->fr0, 0, m->b);
    vm->fr = m->fr0;
    if (f->ret == TY_VOID) vres(vm, mkex(EK_VOID, TY_VOID, 0), m->fr0);
    else vres(vm, mkex(EK_ST, f->ret, alloc(vm, words(vm, f->ret)) * 4), m->fr0);
  } else if (m->a == EK_BI) {
    if (m->b == BI_MIN || m->b == BI_MAX) minmax(vm, m); else builtin(vm, m);
  } else cast(vm, m);
}
static void finish_lit(Nib *vm, Op *m) {
  NibType *st = TY(m->b);
  if (m->n < st->nf) {
    int off = vm->field[st->f0 + m->n].off;
    emit(vm, OP_ZERO, kslot(vm, (uint32_t)(m->c + off)), (int)st->size - off, 0);
  }
  vm->fr = (uint32_t)(m->c / 4 + words(vm, m->b));
  vres(vm, mkex(EK_ST, m->b, m->c), m->fr0);
}
static void closer(Nib *vm, int t, int hasarg) {
  Op *m = &vm->os[vm->nos - 1], o;
  if (t == ')' && m->k == OK_PAREN) { vm->nos--; return; }
  if (t == ']') {
    Ex i, hi;
    if (m->k != OK_IDX) fail(vm, "unexpected ']'");
    o = vm->os[--vm->nos];
    if (o.a) { hi = vpop(vm); i = vpop(vm); doslice(vm, vtop(vm), &i, &hi); }
    else { i = vpop(vm); doindex(vm, vtop(vm), &i); }
    return;
  }
  if (m->k != (t == ')' ? OK_CALL : OK_LIT)) fail(vm, t == ')' ? "unexpected ')'" : "unexpected '}'");
  if (hasarg) argdone(vm, m);
  o = vm->os[--vm->nos];
  if (t == ')') finish_call(vm, &o); else finish_lit(vm, &o);
}
static Ex expr(Nib *vm) {
  int ob = vm->nos, vb = vm->nvs, want = 1, depth = 0, t, p;
  for (;;) {
    t = TK.t;
    if (want) {
      Ex e; int t0 = (int)vm->fr;
      if (t == TK_INT || t == TK_FLT) e = mkex(EK_CONST, t == TK_INT ? TY_I32 : TY_F32, TK.v.i);
      else if (t == TK_STR) e = strlit(vm);
      else if (t == TK_ID) e = ident(vm);
      else if (t == '(') { opush(vm, OK_PAREN, 0, 0); depth++; next(vm); continue; }
      else if (t == '-' || t == '!' || t == '~') { opush(vm, OK_UN, 6, t); next(vm); continue; }
      else if ((t == ')' || t == '}') && vm->nos > ob && vm->os[vm->nos - 1].n == 0 &&
               vm->os[vm->nos - 1].k == (t == ')' ? OK_CALL : OK_LIT) && vm->nvs == vm->os[vm->nos - 1].vb) {
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
      vm->nvs--;
      o = opush(vm, OK_CALL, 0, 0); o->a = c.k; o->b = c.k == EK_TY ? c.t : c.a;
      depth++; next(vm); want = 1;
    } else if (t == '[') {
      opush(vm, OK_IDX, 0, 0); depth++; next(vm);
      if (TK.t == ':') { vres(vm, mkex(EK_CONST, TY_I32, 0), (int)vm->fr); want = 0; } else want = 1;
    } else if (t == '.') {
      next(vm);
      if (TK.t != TK_ID) fail(vm, "field name expected");
      field(vm, vtop(vm)); next(vm);
    } else if (t == '{') {
      Ex c = *vtop(vm); Op *o;
      if (c.k != EK_TY || TY(c.t)->k != K_STRUCT) break;
      vm->nvs--;
      o = opush(vm, OK_LIT, 0, 0); o->b = c.t; o->c = alloc(vm, words(vm, c.t)) * 4;
      depth++; next(vm); want = 1;
    } else if (t == ',' || t == ')' || t == ']' || t == '}' || t == ':') {
      Op *m;
      if (!depth) break;
      while (vm->os[vm->nos - 1].k < OK_PAREN) reduce1(vm);
      m = &vm->os[vm->nos - 1];
      if (t == ',') {
        if (m->k != OK_CALL && m->k != OK_LIT) fail(vm, "unexpected ','");
        argdone(vm, m); next(vm); want = 1;
      } else if (t == ':') {
        if (m->k != OK_IDX || m->a) fail(vm, "unexpected ':'");
        m->a = 1; next(vm);
        if (TK.t == ']') { vres(vm, mkex(EK_LEN, TY_I32, 0), (int)vm->fr); want = 0; } else want = 1;
      } else { closer(vm, t, 1); depth--; next(vm); }
    } else {
      if (!(p = binprec(t))) break;
      while (vm->nos > ob && vm->os[vm->nos - 1].k < OK_PAREN && vm->os[vm->nos - 1].prec >= p) reduce1(vm);
      if (t == TK_AND || t == TK_OR) {
        Ex *l = vtop(vm); int s, d; Op *o;
        needval(vm, l);
        if (l->t != TY_BOOL) fail(vm, "expected bool");
        s = toslot(vm, l, 1); vm->fr = l->t0; d = alloc(vm, 1);
        if (s != d) emit(vm, OP_MOV, d, s, 0);
        o = opush(vm, t == TK_AND ? OK_AND : OK_OR, p, t);
        o->a = emit(vm, t == TK_AND ? OP_JZ : OP_JNZ, d, 0, NONE); o->b = d;
        l->k = EK_ST; l->a = d * 4; l->t = TY_BOOL; l->ro = 0;
      } else opush(vm, OK_BIN, p, t);
      next(vm); want = 1;
    }
  }
  while (vm->nos > ob) {
    if (vm->os[vm->nos - 1].k >= OK_PAREN) fail(vm, "unclosed bracket");
    reduce1(vm);
  }
  if (vm->nvs != vb + 1) fail(vm, "bad expression");
  return vpop(vm);
}
static int32_t constexpr_i(Nib *vm) {
  Ex e = expr(vm);
  if (e.k != EK_CONST || e.t != TY_I32 || e.a <= 0) fail(vm, "positive integer constant expected");
  return e.a;
}

/* ---------------------------------------------------------------- statements */
static int parse_type(Nib *vm) {
  int32_t pre[16]; int np = 0, t, i;
  while (TK.t == '[') {
    if (np >= 16) fail(vm, "type too deep");
    next(vm);
    if (TK.t == ']') pre[np++] = -1; else pre[np++] = constexpr_i(vm);
    expect(vm, ']', "']' expected");
  }
  if (TK.t != TK_ID || (i = lookup(vm, TK.s, TK.n)) < 0 || vm->sym[i].k != S_TYPE) fail(vm, "type expected");
  t = vm->sym[i].t; next(vm);
  if (t == TY_BLOB && TK.t == '[' && !TK.nl) { next(vm); t = array_of(vm, TY_BYTE, (uint32_t)constexpr_i(vm)); expect(vm, ']', "']' expected"); }
  while (np) { int32_t n = pre[--np]; t = n < 0 ? slice_of(vm, t) : array_of(vm, t, (uint32_t)n); }
  return t;
}
static NibBlk *bpush(Nib *vm, int k) {
  NibBlk *b;
  if (vm->nblk >= NIB_MAX_BLOCKS) fail(vm, "blocks nested too deep");
  b = &vm->blk[vm->nblk++];
  b->k = (uint8_t)k; b->nsym = (uint16_t)vm->nsym; b->nnames = (uint16_t)vm->nnames; b->nact = (uint16_t)vm->nact;
  b->a = b->b = b->brk = b->cont = NONE;
  return b;
}
static void bscope(Nib *vm, NibBlk *b) { vm->nsym = b->nsym; vm->nnames = b->nnames; vm->nact = vm->fr = b->nact; }
static int namelist(Nib *vm, const char **ns, int *nl) {
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
static int isbig(NibType *ty) { return (ty->k == K_ARR || ty->k == K_STRUCT) && ty->size > 64; }
static void proc_def(Nib *vm, const char *name, int nlen) {
  NibFunc *f; NibBlk *b; int fi, skip;
  if (vm->nblk || vm->curfn >= 0) fail(vm, "procs must be top-level");
  if (vm->nfunc >= NIB_MAX_FUNCS) fail(vm, "too many procs");
  next(vm); expect(vm, '(', "'(' expected");
  skip = emit(vm, OP_JMP, 0, 0, NONE);
  fi = vm->nfunc++; f = &vm->func[fi];
  f->p0 = (uint16_t)vm->nparam; f->np = 0; f->done = 0; f->ret = TY_VOID; f->retaddr = 0;
  addsym(vm, name, nlen, S_FN, 0, fi);
  b = bpush(vm, B_PROC); b->a = (uint16_t)skip; b->b = (uint16_t)fi;
  vm->fr = vm->nact;
  while (TK.t != ')') {
    const char *ns[16]; int nl[16], c = namelist(vm, ns, nl), t = parse_type(vm), j;
    NibType *ty = TY(t);
    if (ty->k == K_VOID || ty->k == K_ARR || (ty->k == K_STRUCT && ty->ref)) fail(vm, "invalid parameter type (use a slice for arrays)");
    for (j = 0; j < c; j++) {
      int addr = alloc(vm, words(vm, t)) * 4;
      if (vm->nparam >= NIB_MAX_PARAMS) fail(vm, "too many parameters");
      vm->param[vm->nparam].t = (uint16_t)t; vm->param[vm->nparam++].addr = (uint32_t)addr;
      addsym(vm, ns[j], nl[j], S_VAR, t, addr); f->np++;
    }
    if (TK.t != ',') break;
    next(vm);
  }
  expect(vm, ')', "')' expected");
  if (TK.t == TK_ARROW) {
    NibType *ty;
    next(vm); f->ret = (uint16_t)parse_type(vm); ty = TY(f->ret);
    if (ty->k == K_ARR || (ty->k == K_STRUCT && ty->ref)) fail(vm, "invalid return type");
    f->retaddr = (uint32_t)alloc(vm, words(vm, f->ret)) * 4;
  }
  vm->nact = vm->fr; vm->curfn = fi; f->pc = (uint16_t)here(vm);
}
static void struct_def(Nib *vm, const char *name, int nlen) {
  int ti, off = 0; NibType *st;
  if (vm->nblk || vm->curfn >= 0) fail(vm, "structs must be top-level");
  next(vm);
  ti = newtype(vm, K_STRUCT, 0, 0, 0); st = TY(ti); st->f0 = (uint16_t)vm->nfield;
  while (TK.t != TK_END) {
    const char *ns[16]; int nl[16], c = namelist(vm, ns, nl), t = parse_type(vm), j;
    if (TY(t)->k == K_VOID) fail(vm, "invalid field type");
    for (j = 0; j < c; j++) {
      NibField *f;
      if (vm->nfield >= NIB_MAX_FIELDS) fail(vm, "too many fields");
      f = &vm->field[vm->nfield++];
      f->name = (uint16_t)addname(vm, ns[j], nl[j]); f->len = (uint16_t)nl[j]; f->t = (uint16_t)t; f->off = (uint16_t)off;
      off += (int)TY(t)->size; st->nf++; st->ref |= TY(t)->ref;
      if (off > 0xFFFF) fail(vm, "struct too large");
    }
  }
  next(vm);
  st->size = (uint32_t)off;
  addsym(vm, name, nlen, S_TYPE, ti, 0);
}
static void decl_var(Nib *vm) {
  const char *ns = TK.s; int nn = TK.n, t = -1, has = 0, k, big; uint32_t addr; NibType *ty; Ex e = mkex(EK_VOID, TY_VOID, 0);
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
  if (vm->curfn >= 0 && (k == K_STRUCT || k == K_ARR) && ty->ref) fail(vm, "structs/arrays holding slices or strings must be global");
  big = isbig(ty);
  if (big) addr = halloc(vm, ty->size);
  else {
    if (has && e.k == EK_MEM && (k == K_I32 || k == K_F32 || k == K_BOOL || k == K_SLICE)) {
      int s = k == K_SLICE ? toslot2(vm, &e) : toslot(vm, &e, 1);
      e.k = EK_ST; e.a = s * 4; e.t = (uint16_t)t;
    }
    if (has && e.k == EK_ST && e.a == (int32_t)vm->nact * 4 && !e.ro && k != K_ARR) {
      vm->nact += (uint32_t)words(vm, t); vm->fr = vm->nact;
      addsym(vm, ns, nn, S_VAR, t, e.a);
      return;
    }
    addr = (uint32_t)alloc(vm, words(vm, t)) * 4; vm->nact = vm->fr;
  }
  if (has) { Ex d = mkex(EK_ST, t, (int32_t)addr); store(vm, &d, &e); }
  else if ((k == K_I32 || k == K_F32 || k == K_BOOL) && addr < 0x40000) emit(vm, OP_MOV, (int)addr >> 2, kslot(vm, 0), 0);
  else if (!(big && vm->curfn < 0 && !vm->nblk)) emit(vm, OP_ZERO, kslot(vm, addr), (int)(ty->size & 0xFFFF), (int)(ty->size >> 16));
  addsym(vm, ns, nn, S_VAR, t, (int32_t)addr);
}
static void stmt_expr(Nib *vm) {
  Ex l = expr(vm), r, cur;
  if (TK.t != '=' && TK.t != TK_OPEQ) return;
  if ((l.k != EK_ST && l.k != EK_MEM) || l.ro || (l.k == EK_ST && l.a < NIB_MAX_CONSTS * 4)) fail(vm, "cannot assign to this");
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
static void statement(Nib *vm) {
  int t = TK.t, i; NibBlk *b; Ex e;
  vm->fr = vm->nact;
  if (t == ';') { next(vm); return; }
  if (t == TK_ID && vm->nx.t == TK_DCOLON) {
    const char *ns = TK.s; int nn = TK.n;
    next(vm); next(vm);
    if (TK.t == TK_PROC) proc_def(vm, ns, nn);
    else if (TK.t == TK_STRUCT) struct_def(vm, ns, nn);
    else {
      e = expr(vm);
      if (e.k == EK_CONST) addsym(vm, ns, nn, S_CONST, e.t, e.a);
      else if (e.k == EK_ST && e.ro && e.t == TY_STR) addsym(vm, ns, nn, S_CONST, TY_STR, e.a);
      else fail(vm, "constant expression expected");
    }
  } else if (t == TK_ID && (vm->nx.t == ':' || vm->nx.t == TK_DECL)) decl_var(vm);
  else switch (t) {
  case TK_IF:
    next(vm); e = expr(vm); i = condjump(vm, &e);
    b = bpush(vm, B_IF); b->a = (uint16_t)i;
    break;
  case TK_ELSE:
    if (!vm->nblk || vm->blk[vm->nblk - 1].k != B_IF) fail(vm, "'else' without 'if'");
    b = &vm->blk[vm->nblk - 1];
    next(vm); bscope(vm, b);
    b->b = (uint16_t)jappend(vm, b->b, emit(vm, OP_JMP, 0, 0, NONE));
    patch(vm, b->a, here(vm));
    if (TK.t == TK_IF) { next(vm); e = expr(vm); b->a = (uint16_t)condjump(vm, &e); }
    else { b->k = B_ELSE; b->a = NONE; }
    break;
  case TK_FOR:
    next(vm);
    if (TK.nl || TK.t == TK_END) { b = bpush(vm, B_LOOP); b->a = (uint16_t)here(vm); }
    else if (TK.t == TK_ID && vm->nx.t == TK_IN) {
      const char *ns = TK.s; int nn = TK.n, iv, lim, incl; Ex d;
      next(vm); next(vm);
      b = bpush(vm, B_FOR);
      vm->fr = vm->nact; iv = alloc(vm, 1); lim = alloc(vm, 1); vm->nact = vm->fr;
      e = expr(vm); d = mkex(EK_ST, TY_I32, iv * 4); store(vm, &d, &e); vm->fr = vm->nact;
      if (TK.t != TK_RLT && TK.t != TK_RLE) fail(vm, "'..<' or '..=' expected");
      incl = TK.t == TK_RLE; next(vm);
      e = expr(vm);
      if (incl) { Ex one = mkex(EK_CONST, TY_I32, 1); one.t0 = (uint16_t)vm->fr; binop(vm, '+', &e, &one); }
      d = mkex(EK_ST, TY_I32, lim * 4); store(vm, &d, &e); vm->fr = vm->nact;
      addsym(vm, ns, nn, S_VAR, TY_I32, iv * 4);
      b->i = (uint16_t)iv; b->lim = (uint16_t)lim;
      b->brk = (uint16_t)emit(vm, OP_JLE, lim, iv, NONE); b->a = (uint16_t)here(vm);
    } else {
      int top = here(vm);
      e = expr(vm); i = condjump(vm, &e);
      b = bpush(vm, B_LOOP); b->a = (uint16_t)top; b->brk = (uint16_t)i;
    }
    break;
  case TK_END:
    if (!vm->nblk) fail(vm, "'end' without block");
    b = &vm->blk[vm->nblk - 1];
    next(vm);
    if (b->k == B_PROC) {
      emit(vm, OP_RET, 0, 0, 0); patch(vm, b->a, here(vm));
      vm->func[b->b].done = 1; vm->curfn = -1;
      vm->nsym = b->nsym; vm->nnames = b->nnames; vm->nact = vm->fr = vm->hwm; vm->nblk--;
      break;
    }
    if (b->k == B_LOOP) { patch(vm, b->cont, b->a); emit(vm, OP_JMP, 0, 0, b->a); }
    else if (b->k == B_FOR) { patch(vm, b->cont, here(vm)); emit(vm, OP_FORI, b->i, b->lim, b->a); }
    else patch(vm, b->b, here(vm));
    patch(vm, b->k == B_LOOP || b->k == B_FOR ? b->brk : b->a, here(vm));
    bscope(vm, b); vm->nblk--;
    break;
  case TK_RETURN: {
    NibFunc *f;
    if (vm->curfn < 0) fail(vm, "return outside proc");
    next(vm); f = &vm->func[vm->curfn];
    if (f->ret != TY_VOID) { Ex d = mkex(EK_ST, f->ret, (int32_t)f->retaddr); e = expr(vm); store(vm, &d, &e); }
    emit(vm, OP_RET, 0, 0, 0);
    break;
  }
  case TK_BREAK: case TK_CONTINUE:
    for (i = vm->nblk - 1; i >= 0 && vm->blk[i].k != B_PROC; i--) if (vm->blk[i].k == B_LOOP || vm->blk[i].k == B_FOR) break;
    if (i < 0 || vm->blk[i].k == B_PROC) fail(vm, "not inside a loop");
    b = &vm->blk[i];
    if (t == TK_BREAK) b->brk = (uint16_t)jappend(vm, b->brk, emit(vm, OP_JMP, 0, 0, NONE));
    else b->cont = (uint16_t)jappend(vm, b->cont, emit(vm, OP_JMP, 0, 0, NONE));
    next(vm);
    break;
  default: stmt_expr(vm);
  }
  vm->fr = vm->nact;
  if (TK.t != TK_EOF && TK.t != ';' && !TK.nl) fail(vm, "expected end of statement");
}

/* ---------------------------------------------------------------- api */
int nib_init(Nib *vm, void *mem, uint32_t memsize, NibIns *code, uint32_t codecap) {
  static const uint8_t tk[] = {K_VOID, K_I32, K_F32, K_BYTE, K_SLICE, K_SLICE, K_BOOL};
  static const uint8_t ts[] = {0, 4, 4, 1, 8, 8, 4};
  static const char *tn[] = {"i32", "f32", "string", "blob", "bool"};
  int i; uintptr_t pad = (4 - ((uintptr_t)mem & 3)) & 3;
  memset(vm, 0, sizeof *vm);
  if (memsize < pad + NIB_MAX_CONSTS * 4 + 4096 || !code || codecap < 16) return -1;
  vm->mem = (uint8_t *)mem + pad; vm->memsize = vm->hi = (uint32_t)(memsize - pad) & ~3u;
  memset(vm->mem, 0, vm->memsize);
  vm->code = code; vm->codecap = codecap > 65535 ? 65535 : codecap;
  vm->nk = 1; vm->nact = vm->fr = vm->hwm = NIB_MAX_CONSTS; vm->curfn = -1; vm->lastlabel = NONE;
  for (i = 0; i < 7; i++) { vm->type[i].k = tk[i]; vm->type[i].size = ts[i]; vm->type[i].elem = TY_BYTE; vm->type[i].ref = i == TY_STR || i == TY_BLOB; }
  vm->ntype = 7;
  if (setjmp(vm->jb)) return -1;
  for (i = 0; i < 5; i++) addsym(vm, tn[i], (int)strlen(tn[i]), S_TYPE, i < 2 ? TY_I32 + i : TY_STR + i - 2, 0);
  addsym(vm, "true", 4, S_CONST, TY_BOOL, 1); addsym(vm, "false", 5, S_CONST, TY_BOOL, 0);
  for (i = 0; i < (int)(sizeof bi / sizeof bi[0]); i++) addsym(vm, bi[i], (int)strlen(bi[i]), S_BI, 0, i);
  return 0;
}
void nib_set_log(Nib *vm, NibLogFn fn, void *ud) { vm->logfn = fn; vm->logud = ud; }
int nib_ffi(Nib *vm, const char *name, const char *sig, NibFn fn) {
  NibFfi *f;
  if (setjmp(vm->jb)) return -1;
  if (vm->nffi >= NIB_MAX_FFI) fail(vm, "too many ffi functions");
  f = &vm->ffi[vm->nffi]; f->fn = fn; f->p0 = (uint16_t)vm->nparam; f->np = 0; f->ret = TY_VOID;
  for (; *sig; sig++) {
    int ret = *sig == '>', c = ret ? sig[1] : *sig;
    int t = c == 'i' ? TY_I32 : c == 'f' ? TY_F32 : c == 's' ? TY_STR : c == 'b' ? TY_BLOB : -1;
    if (t < 0 || vm->nparam >= NIB_MAX_PARAMS) fail(vm, "bad ffi signature");
    if (ret) { f->ret = (uint16_t)t; break; }
    vm->param[vm->nparam++].t = (uint16_t)t; f->np++;
  }
  addsym(vm, name, (int)strlen(name), S_FFI, 0, vm->nffi++);
  return 0;
}
int nib_compile(Nib *vm, const char *src, uint32_t len) {
  if (setjmp(vm->jb)) return -1;
  vm->sp = src; vm->se = src + len; vm->line = 1;
  lex(vm, &vm->nx); next(vm);
  while (TK.t != TK_EOF) statement(vm);
  if (vm->nblk) fail(vm, "missing 'end'");
  emit(vm, OP_HALT, 0, 0, 0);
  vm->ok = 1;
  return 0;
}
const char *nib_error(Nib *vm) { return vm->err; }
void nib_trap(Nib *vm, const char *msg) { int n = cat(vm->err, 0, msg); vm->err[n] = 0; vm->trap = 1; }
int nib_func(Nib *vm, const char *name) {
  int i = lookup(vm, name, (int)strlen(name));
  return i >= 0 && vm->sym[i].k == S_FN ? vm->sym[i].v : -1;
}
void *nib_global(Nib *vm, const char *name) {
  int i = lookup(vm, name, (int)strlen(name));
  return i >= 0 && vm->sym[i].k == S_VAR ? vm->mem + vm->sym[i].v : 0;
}
NibVal *nib_arg(Nib *vm, int fn, int i) { return (NibVal *)(vm->mem + vm->param[vm->func[fn].p0 + i].addr); }
NibVal *nib_ret(Nib *vm, int fn) { return (NibVal *)(vm->mem + vm->func[fn].retaddr); }
void *nib_ptr(Nib *vm, const NibVal *s) {
  return (uint32_t)s[0].i <= vm->memsize && (uint32_t)s[1].i <= vm->memsize - (uint32_t)s[0].i ? vm->mem + s[0].i : 0;
}

/* ---------------------------------------------------------------- vm */
static void logput(Nib *vm, const char *s, int n) { while (n-- > 0 && vm->logn < NIB_LOGBUF) vm->logbuf[vm->logn++] = *s++; }
static int rterr(Nib *vm, const char *m, uint32_t pc) {
  int n = cat(vm->err, 0, "runtime error: "); n = cat(vm->err, n, m);
  n = cat(vm->err, n, " (pc "); n += fmti(vm->err + n, (int32_t)pc); n = cat(vm->err, n, ")");
  vm->err[n] = 0;
  return -1;
}
static int run(Nib *vm, uint32_t pc) {
  const NibIns *code = vm->code, *ip = code + pc, **cs = vm->cs;
  uint8_t *M = vm->mem; NibVal *R = (NibVal *)M; int sp = 0;
#define A ip->a
#define B ip->b
#define C ip->c
#define I(x) R[x].i
#define U(x) R[x].u
#define F(x) R[x].f
#define MV(p) (*(NibVal *)(M + (p)))
#define SZ(w) ((uint32_t)(w).b | (uint32_t)(w).c << 16)
#ifdef NIB_CGOTO
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
  CASE(MOV2) { NibVal x = R[B], y = R[B + 1]; R[A] = x; R[A + 1] = y; NEXT(); }
  CASE(LDW2) { int32_t p = I(B) + C; NibVal x = MV(p), y = MV(p + 4); R[A] = x; R[A + 1] = y; NEXT(); }
  CASE(STW) MV(I(A) + C) = R[B]; NEXT();
  CASE(STB) M[I(A) + C] = (uint8_t)U(B); NEXT();
  CASE(STW2) { int32_t p = I(A) + C; MV(p) = R[B]; MV(p + 4) = R[B + 1]; NEXT(); }
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
  CASE(JFLE) if (F(A) <= F(B)) JUMP(); NEXT();
  CASE(JFEQ) if (F(A) == F(B)) JUMP(); NEXT();
  CASE(JFNE) if (F(A) != F(B)) JUMP(); NEXT();
  CASE(FORI) if ((I(A) = (int32_t)(U(A) + 1u)) < I(B)) JUMP(); NEXT();
  CASE(CALL) cs[sp++] = ip + 1; JUMP();
  CASE(RET) if (!sp) return 0; ip = cs[--sp]; DISPATCH();
  CASE(FFI) vm->ffi[C].fn(vm, R + A); if (vm->trap) { vm->trap = 0; return -1; } NEXT();
  CASE(LOGI) { char b[16]; if (ip->x) logput(vm, " ", 1); logput(vm, b, fmti(b, I(A))); NEXT(); }
  CASE(LOGF) { char b[32]; if (ip->x) logput(vm, " ", 1); logput(vm, b, fmtf(b, F(A))); NEXT(); }
  CASE(LOGS) { if (ip->x) logput(vm, " ", 1); logput(vm, (const char *)M + I(A), I(A + 1)); NEXT(); }
  CASE(LOGB) if (ip->x) logput(vm, " ", 1); if (I(A)) logput(vm, "true", 4); else logput(vm, "false", 5); NEXT();
  CASE(LOGE) if (vm->logfn) vm->logfn(vm->logud, vm->logbuf, vm->logn); vm->logn = 0; NEXT();
  CASE(HALT) return 0;
#ifndef NIB_CGOTO
  default: return rterr(vm, "bad opcode", (uint32_t)(ip - code));
  }
#endif
}
int nib_run(Nib *vm) { return vm->ok ? run(vm, 0) : -1; }
int nib_call(Nib *vm, int fn) { return vm->ok && fn >= 0 && fn < vm->nfunc ? run(vm, vm->func[fn].pc) : -1; }
