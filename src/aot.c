/* aot.c - translate compiled rio bytecode to a standalone C program.
   No recursion => every proc becomes a plain C function, and its frame slots become C locals
   (unless their address is taken). Constants are emitted inline. */
#define _CRT_SECURE_NO_WARNINGS
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rio.h"
#include "aot.h"

#define OPENUM(o) A_##o,
enum { RIO_OPS(OPENUM) A_DATA = 255 };

static Rio *vm;
static FILE *o;
static int cur = -1;            /* function being emitted, -1 = top level */
static uint8_t *label;          /* jump targets */
static uint8_t used[8192];      /* locals referenced by the current proc */
static int lastline;             /* last #line emitted */

static RioVal kval(int s) { return ((RioVal *)vm->mem)[s]; }
static int isret(int s) {
  RioFunc *f = &vm->func[cur];
  return s >= f->ret && s < f->ret + f->retw;
}
static int islocal(int s) {
  RioFunc *f;
  if (cur < 0) return 0;
  f = &vm->func[cur];
  return s >= f->fs && s < f->fe && !(vm->c->exposed[s >> 3] & (1 << (s & 7))) && !isret(s);
}
/* operand text for slot s viewed as t: i(nt) u(nsigned) f(loat) v(whole value) */
static const char *V(int s, int t) {
  static char buf[16][48]; static int n;
  char *b = buf[n++ & 15];
  if (s < (int)vm->nk) {
    RioVal k = kval(s);
    if (t == 'i') { if (k.i == (int32_t)0x80000000u) strcpy(b, "(-2147483647-1)"); else sprintf(b, k.i < 0 ? "(%d)" : "%d", k.i); }
    else if (t == 'u') sprintf(b, "%uu", k.u);
    else if (t == 'f') {
      char *q; float back;
      sprintf(b, "%.9g", k.f); back = strtof(b, &q);
      if (k.f != k.f || back != k.f || k.f - k.f != 0) sprintf(b, "kf(%uu)", k.u);
      else { if (!strpbrk(b, ".e")) strcat(b, ".0"); strcat(b, "f"); if (k.f < 0) { memmove(b + 1, b, strlen(b) + 1); b[0] = '('; strcat(b, ")"); } }
    } else sprintf(b, "((RioVal){.u = %uu})", k.u);
  } else if (islocal(s)) {
    used[s >> 3] |= (uint8_t)(1 << (s & 7));
    if (t == 'v') sprintf(b, "r%d", s); else sprintf(b, "r%d.%c", s, t); }
  else { if (t == 'v') sprintf(b, "R[%d]", s); else sprintf(b, "R[%d].%c", s, t); }
  return b;
}
static uint32_t sz(const RioIns *d) { return (uint32_t)d->b | (uint32_t)d->c << 16; }
static const char *symname(int kind, int idx) {
  static char b[64]; int i;
  for (i = 0; i < vm->c->nsym; i++)
    if (vm->c->sym[i].k == kind && vm->c->sym[i].v == idx) {
      int n = vm->c->sym[i].len < 60 ? vm->c->sym[i].len : 60;
      memcpy(b, vm->c->names + vm->c->sym[i].name, (size_t)n); b[n] = 0;
      return b;
    }
  sprintf(b, "anon%d", idx);
  return b;
}
/* print to the output, or only evaluate the operands (marking used locals) during the dry run */
static void P(const char *fmt, ...) { va_list ap; va_start(ap, fmt); if (o) vfprintf(o, fmt, ap); va_end(ap); }
/* C name for proc/method i: p<i>_<name>, unique even when methods of different types share a name */
static const char *fname(int idx) {
  static char b[80]; int i;
  for (i = 0; i < vm->c->nsym; i++)
    if ((vm->c->sym[i].k == RIO_S_FN || vm->c->sym[i].k == RIO_S_METH) && vm->c->sym[i].v == idx) {
      int n = vm->c->sym[i].len < 60 ? vm->c->sym[i].len : 60;
      sprintf(b, "p%d_%.*s", idx, n, vm->c->names + vm->c->sym[i].name);
      return b;
    }
  sprintf(b, "p%d", idx);
  return b;
}
static int fnat(int pc) { int i; for (i = 0; i < vm->nfunc; i++) if (vm->func[i].pc == pc) return i; return -1; }

static void ins(int pc) {
  const RioIns *x = &vm->code[pc], *d = x + 1;
  int a = x->a, b = x->b, c = x->c;
  static const char *bin[] = {"+", "-", "*", 0, 0, "&", "|", "^"};
  static const char *cmp[] = {"==", "!=", "<", "<="};
  static const char *m1[] = {"sqrtf", "sinf", "cosf", "tanf", "asinf", "acosf", "atanf", "expf", "logf", "floorf", "ceilf", "rt_round", "fabsf"};
  static const char *m2[] = {"atan2f", "powf", "fmodf"};
  if (x->op == A_DATA) return;
  if (label[pc]) P("L%d:;\n", pc);
  { int ln = rio_pc_line(vm, (uint32_t)pc); if (ln && ln != lastline) { P("#line %d\n", ln); lastline = ln; } }
  P("  ");
  switch (x->op) {
  case A_MOV: P("%s = %s;\n", V(a, 'v'), V(b, 'v')); break;
  case A_ADD: case A_SUB: case A_MUL: case A_AND: case A_OR: case A_XOR:
    P("%s = %s %s %s;\n", V(a, 'u'), V(b, 'u'), bin[x->op - A_ADD], V(c, 'u')); break;
  case A_DIV: case A_MOD: {
    const char *op = x->op == A_DIV ? "/" : "%";
    if (c < (int)vm->nk && kval(c).i != 0 && kval(c).i != -1) P("%s = %s %s %s;\n", V(a, 'i'), V(b, 'i'), op, V(c, 'i'));
    else P("{ int32_t y_ = %s, x_ = %s; if (!y_) RT_ERR(\"division by zero\"); %s = y_ == -1 ? %s : x_ %s y_; }\n",
                 V(c, 'i'), V(b, 'i'), V(a, 'i'), x->op == A_DIV ? "(int32_t)(0u - (uint32_t)x_)" : "0", op);
    break;
  }
  case A_SHL: P("%s = %s << (%s & 31);\n", V(a, 'u'), V(b, 'u'), V(c, 'u')); break;
  case A_SHR: P("%s = %s >> (%s & 31);\n", V(a, 'i'), V(b, 'i'), V(c, 'u')); break;
  case A_NEG: P("%s = 0u - %s;\n", V(a, 'u'), V(b, 'u')); break;
  case A_BNOT: P("%s = ~%s;\n", V(a, 'u'), V(b, 'u')); break;
  case A_NOT: P("%s = !%s;\n", V(a, 'i'), V(b, 'i')); break;
  case A_FADD: case A_FSUB: case A_FMUL: case A_FDIV:
    P("%s = %s %c %s;\n", V(a, 'f'), V(b, 'f'), "+-*/"[x->op - A_FADD], V(c, 'f')); break;
  case A_FNEG: P("%s = -%s;\n", V(a, 'f'), V(b, 'f')); break;
  case A_EQ: case A_NE: case A_LT: case A_LE:
    P("%s = %s %s %s;\n", V(a, 'i'), V(b, 'i'), cmp[x->op - A_EQ], V(c, 'i')); break;
  case A_FEQ: case A_FNE: case A_FLT: case A_FLE:
    P("%s = %s %s %s;\n", V(a, 'i'), V(b, 'f'), cmp[x->op - A_FEQ], V(c, 'f')); break;
  case A_SEQ: case A_SNE:
    P("%s = %sseq(%s, %s, %s, %s);\n", V(a, 'i'), x->op == A_SNE ? "!" : "", V(b, 'i'), V(b + 1, 'i'), V(c, 'i'), V(c + 1, 'i')); break;
  case A_ITOF: P("%s = (float)%s;\n", V(a, 'f'), V(b, 'i')); break;
  case A_FTOI: P("%s = ftoi(%s);\n", V(a, 'i'), V(b, 'f')); break;
  case A_LDW: P("%s = MV(%s + %du);\n", V(a, 'v'), V(b, 'u'), c); break;
  case A_LDB: P("%s = M[%s + %du];\n", V(a, 'u'), V(b, 'u'), c); break;
  case A_LEA: P("%s = %s + %du;\n", V(a, 'u'), V(b, 'u'), c); break;
  case A_M1: P("%s = %s(%s);\n", V(a, 'f'), m1[x->x], V(b, 'f')); break;
  case A_M2: P("%s = %s(%s, %s);\n", V(a, 'f'), m2[x->x], V(b, 'f'), V(c, 'f')); break;
  case A_IABS: P("%s = %s < 0 ? 0u - %s : %s;\n", V(a, 'u'), V(b, 'i'), V(b, 'u'), V(b, 'u')); break;
  case A_IMIN: case A_IMAX: case A_FMIN: case A_FMAX: {
    int t = x->op <= A_IMAX ? 'i' : 'f';
    P("%s = %s %c %s ? %s : %s;\n", V(a, t), V(b, t), x->op == A_IMIN || x->op == A_FMIN ? '<' : '>', V(c, t), V(b, t), V(c, t));
    break;
  }
  case A_LDX: case A_LDXB:
    P("{ uint32_t i_ = %s; if (i_ >= %s) RT_ERR(\"index out of bounds\"); ", V(c, 'u'), V(b + 1, 'u'));
    if (x->op == A_LDX) P("%s = MV(%s + i_ * %uu + %du); }\n", V(a, 'v'), V(b, 'u'), sz(d), d->a);
    else P("%s = M[%s + i_ * %uu + %du]; }\n", V(a, 'u'), V(b, 'u'), sz(d), d->a);
    break;
  case A_MOV2: P("{ RioVal x_ = %s, y_ = %s; %s = x_; %s = y_; }\n", V(b, 'v'), V(b + 1, 'v'), V(a, 'v'), V(a + 1, 'v')); break;
  case A_LDW2: P("{ uint32_t p_ = %s + %du; RioVal x_ = MV(p_), y_ = MV(p_ + 4); %s = x_; %s = y_; }\n", V(b, 'u'), c, V(a, 'v'), V(a + 1, 'v')); break;
  case A_STW: P("MV(%s + %du) = %s;\n", V(a, 'u'), c, V(b, 'v')); break;
  case A_STB: P("M[%s + %du] = (uint8_t)%s;\n", V(a, 'u'), c, V(b, 'u')); break;
  case A_STW2: P("{ uint32_t p_ = %s + %du; MV(p_) = %s; MV(p_ + 4) = %s; }\n", V(a, 'u'), c, V(b, 'v'), V(b + 1, 'v')); break;
  case A_STX: case A_STXB:
    P("{ uint32_t i_ = %s; if (i_ >= %s) RT_ERR(\"index out of bounds\"); ", V(b, 'u'), V(a + 1, 'u'));
    if (x->op == A_STX) P("MV(%s + i_ * %uu + %du) = %s; }\n", V(a, 'u'), sz(d), d->a, V(c, 'v'));
    else P("M[%s + i_ * %uu + %du] = (uint8_t)%s; }\n", V(a, 'u'), sz(d), d->a, V(c, 'u'));
    break;
  case A_IDX:
    P("{ uint32_t i_ = %s; if (i_ >= %s) RT_ERR(\"index out of bounds\"); %s = %s + i_ * %uu; }\n",
            V(c, 'u'), V(b + 1, 'u'), V(a, 'u'), V(b, 'u'), sz(d));
    break;
  case A_SLICE:
    P("{ int32_t lo_ = %s, hi_ = %s, n_ = %s, p_ = %s; if (lo_ < 0 || hi_ < lo_ || hi_ > n_) RT_ERR(\"slice out of bounds\"); ",
            V(c, 'i'), V(d->a, 'i'), V(b + 1, 'i'), V(b, 'i'));
    P("%s = p_ + lo_ * %d; %s = hi_ - lo_; }\n", V(a, 'i'), (int)sz(d), V(a + 1, 'i'));
    break;
  case A_COPY: P("memmove(M + %s, M + %s, %d);\n", V(a, 'u'), V(b, 'u'), c); break;
  case A_VADD: case A_VSUB: case A_VMUL: case A_VDIV: case A_VMOD: case A_FVADD: case A_FVSUB: case A_FVMUL: case A_FVDIV: {
    /* a loop over the numbers; a scalar side is read once */
    int m = x->x | (x->op >= A_FVADD ? 8 : 0), k = x->op >= A_FVADD ? x->op - A_FVADD : x->op - A_VADD; const char *L = m & 16 ? "l_" : "MV(l_ + 4u * i_)", *Q = m & 32 ? "r_" : "MV(r_ + 4u * i_)";
    P("{ uint32_t d_ = %s, i_; ", V(a, 'u'));
    if (m & 16) P("RioVal l_ = %s; ", V(b, 'v')); else P("uint32_t l_ = %s; ", V(b, 'u'));
    if (m & 32) P("RioVal r_ = %s; ", V(c, 'v')); else P("uint32_t r_ = %s; ", V(c, 'u'));
    P("for (i_ = 0; i_ < %uu; i_++) ", sz(d));
    if (k <= 2 || (m & 8)) P("MV(d_ + 4u * i_).%c = %s.%c %c %s.%c; }\n", m & 8 ? 'f' : 'u', L, m & 8 ? 'f' : 'u', "+-*/"[k], Q, m & 8 ? 'f' : 'u');
    else P("{ int32_t y_ = %s.i, v_ = %s.i; if (!y_) RT_ERR(\"division by zero\"); MV(d_ + 4u * i_).i = y_ == -1 ? %s : v_ %c y_; } }\n",
           Q, L, k == 3 ? "(int32_t)(0u - (uint32_t)v_)" : "0", k == 3 ? '/' : '%');
    break;
  }
  case A_ZERO: P("memset(M + %s, 0, %uu);\n", V(a, 'u'), sz(x)); break;
  case A_LIDX:
    P("{ uint32_t i_ = %s; if (i_ >= rt_llen(%s, %s)) RT_ERR(\"index out of bounds\"); %s = %s + i_ * %uu; }\n",
      V(c, 'u'), V(b, 'u'), V(b + 2, 'u'), V(a, 'u'), V(b + 1, 'u'), sz(d));
    break;
  case A_PUSHA:
    P("{ uint32_t b_ = %s, d_ = %s, c_ = %s, n_ = rt_llen(b_, c_); if (n_ >= c_) %s = 0; else { MV(b_).u = n_ + 1; %s = d_ + n_ * %uu; } }\n",
      V(b, 'u'), V(b + 1, 'u'), V(b + 2, 'u'), V(a, 'u'), V(a, 'u'), sz(d));
    break;
  case A_PUSHS: P("%s = rt_pushb(%s, %s, %s, M + %s, %s, %uu);\n", V(a, 'i'), V(b, 'u'), V(b + 1, 'u'), V(b + 2, 'u'), V(c, 'u'), V(c + 1, 'u'), sz(d)); break;
  case A_PUSHT: P("%s = rt_pusht(%s, %s, %s, %d, %s);\n", V(a, 'i'), V(b, 'u'), V(b + 1, 'u'), V(b + 2, 'u'), x->x, V(c, 'v')); break;
  case A_POPA:
    P("{ uint32_t b_ = %s, d_ = %s, n_ = rt_llen(b_, %s); if (!n_) RT_ERR(\"pop from empty list\"); MV(b_).u = --n_; %s = d_ + n_ * %uu; }\n",
      V(b, 'u'), V(b + 1, 'u'), V(b + 2, 'u'), V(a, 'u'), sz(d));
    break;
  case A_LREM: case A_LSWAP:
    P("rt_lrem(%s, %s, %s, %s, %uu, %d, __LINE__);\n", V(a, 'u'), V(a + 1, 'u'), V(a + 2, 'u'), V(b, 'u'), sz(d), x->op == A_LSWAP);
    break;
  case A_LVIEW: P("{ uint32_t d_ = %s, n_ = rt_llen(%s, %s); %s = d_; %s = n_; }\n", V(b + 1, 'u'), V(b, 'u'), V(b + 2, 'u'), V(a, 'u'), V(a + 1, 'u')); break;
  case A_JMP: P("goto L%d;\n", c); break;
  case A_JZ: P("if (!%s) goto L%d;\n", V(a, 'i'), c); break;
  case A_JNZ: P("if (%s) goto L%d;\n", V(a, 'i'), c); break;
  case A_JEQ: case A_JNE: case A_JLT: case A_JLE:
    P("if (%s %s %s) goto L%d;\n", V(a, 'i'), cmp[x->op - A_JEQ], V(b, 'i'), c); break;
  case A_JFEQ: case A_JFNE: case A_JFLT: case A_JFLE:
    P("if (%s %s %s) goto L%d;\n", V(a, 'f'), cmp[x->op - A_JFEQ], V(b, 'f'), c); break;
  case A_JFNLT: case A_JFNLE:
    P("if (!(%s %s %s)) goto L%d;\n", V(a, 'f'), x->op == A_JFNLT ? "<" : "<=", V(b, 'f'), c); break;
  case A_EACH: P("if ((%s += %s) < %s) goto L%d;\n", V(a, 'u'), V(b + 1, 'u'), V(b, 'u'), c); break;
  case A_FORI: P("if ((%s = (int32_t)(%s + 1u)) < %s) goto L%d;\n", V(a, 'i'), V(a, 'u'), V(b, 'i'), c); break;
  case A_CALL: P("%s();\n", fname(fnat(c))); break;
  case A_RET: case A_HALT: P("return;\n"); break;
  case A_FFI: {
    RioFfi *f = &vm->ffi[c]; int n = f->aw, i, rw = f->rw;
    P("{ RioVal a_[%d];", (n > rw ? n : rw) + 1);
    for (i = 0; i < n; i++) P(" a_[%d] = %s;", i, V(a + i, 'v'));
    P(" rio_ffi_%s(a_);", symname(RIO_S_FFI, c));
    for (i = 0; i < rw; i++) P(" %s = a_[%d];", V(a + i, 'v'), i);
    P(" }\n");
    break;
  }
  case A_LOGI: P("log_i(%d, %s);\n", x->x, V(a, 'i')); break;
  case A_LOGF: P("log_f(%d, %s);\n", x->x, V(a, 'f')); break;
  case A_LOGB: P("log_s(%d, %s ? \"true\" : \"false\", -1);\n", x->x, V(a, 'i')); break;
  case A_LOGS: P("log_s(%d, (const char *)M + %s, %s);\n", x->x, V(a, 'u'), V(a + 1, 'i')); break;
  case A_LOGE: P("log_e();\n"); break;
  default: P("RT_ERR(\"bad op\");\n");
  }
}

static const char *prelude =
  "#define _CRT_SECURE_NO_WARNINGS\n#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <math.h>\n#include <time.h>\n"
  "#ifdef __GNUC__\n#pragma GCC diagnostic ignored \"-Wunused\"\n#endif\n"
  "typedef union { int32_t i; float f; uint32_t u; } RioVal;\n"
  "static union { RioVal v[RIO_MEM / 4]; uint8_t b[RIO_MEM]; } rio_mem;\n"
  "#define M rio_mem.b\n#define R rio_mem.v\n#define MV(p) (*(RioVal *)(M + (uint32_t)(p)))\n"
  "static inline float kf(uint32_t u) { RioVal v; v.u = u; return v.f; }\n"
  "static inline void rt_err(const char *m, int line) { fflush(stdout); fprintf(stderr, \"%d: runtime error: %s\\n\", line, m); exit(1); }\n"
  "#define RT_ERR(m) rt_err(m, __LINE__) /* #line directives make __LINE__ the rio source line */\n"
  "static inline int32_t ftoi(float f) { return f != f ? 0 : f >= 2147483648.f ? 0x7FFFFFFF : f <= -2147483648.f ? (int32_t)0x80000000u : (int32_t)f; }\n"
  "static inline float rt_round(float x) { return x < 0 ? -floorf(-x + 0.5f) : floorf(x + 0.5f); }\n"
  "static inline int seq(int32_t a, int32_t an, int32_t b, int32_t bn) { return an == bn && !memcmp(M + a, M + b, (size_t)an); }\n"
  "static char lb[256]; static int ln;\n"
  "static inline void log_s(int sp, const char *s, int n) { if (n < 0) n = (int)strlen(s); if (sp && ln < 256) lb[ln++] = ' '; while (n-- > 0 && ln < 256) lb[ln++] = *s++; }\n"
  "static inline void log_i(int sp, int32_t v) { char b[16]; sprintf(b, \"%d\", (int)v); log_s(sp, b, -1); }\n"
  "static inline int fmt_f(char *b, float f) {\n"
  "  int i = 0, e = 0, nd = 1, dec, k; double d = f; uint32_t ip, fp, sc = 1, t;\n"
  "  if (f != f) { memcpy(b, \"nan\", 4); return 3; }\n"
  "  if (d < 0) { b[i++] = '-'; d = -d; }\n"
  "  if (d > 3.5e38) { memcpy(b + i, \"inf\", 4); return i + 3; }\n"
  "  if (d != 0 && (d >= 1e9 || d < 1e-4)) { while (d >= 10) { d /= 10; e++; } while (d < 1) { d *= 10; e--; } }\n"
  "  ip = (uint32_t)d; for (t = ip; t >= 10; t /= 10) nd++;\n"
  "  dec = 7 - nd; if (dec < 1) dec = 1; if (dec > 6) dec = 6; for (k = 0; k < dec; k++) sc *= 10;\n"
  "  fp = (uint32_t)((d - ip) * sc + 0.5); if (fp >= sc) { ip++; fp -= sc; }\n"
  "  i += sprintf(b + i, \"%u.\", (unsigned)ip);\n"
  "  for (k = dec - 1; k >= 0; k--) { b[i + k] = (char)('0' + fp % 10); fp /= 10; }\n"
  "  while (dec > 1 && b[i + dec - 1] == '0') dec--; i += dec;\n"
  "  if (e) i += sprintf(b + i, \"e%d\", e);\n"
  "  b[i] = 0; return i;\n"
  "}\n"
  "static inline void log_f(int sp, float f) { char b[40]; log_s(sp, b, fmt_f(b, f)); }\n"
  "static inline void log_e(void) { fwrite(lb, 1, (size_t)ln, stdout); fputc('\\n', stdout); ln = 0; }\n"
  /* lists: a length word followed by the elements; views are (address, capacity) */
  "static inline uint32_t rt_llen(uint32_t a, uint32_t cap) { uint32_t n = MV(a).u; return n < cap ? n : cap; }\n"
  "static inline int32_t rt_pushb(uint32_t a, uint32_t d, uint32_t cap, const void *p, uint32_t k, uint32_t sz) {\n"
  "  uint32_t n = rt_llen(a, cap); if (k > cap - n) return 0;\n"
  "  memmove(M + d + n * sz, p, k * sz); MV(a).u = n + k; return 1;\n"
  "}\n"
  "static inline int32_t rt_pusht(uint32_t a, uint32_t d, uint32_t cap, int kind, RioVal v) {\n"
  "  char b[40]; int k = kind == 1 ? fmt_f(b, v.f) : kind == 2 ? sprintf(b, \"%s\", v.i ? \"true\" : \"false\") : sprintf(b, \"%d\", (int)v.i);\n"
  "  return rt_pushb(a, d, cap, b, (uint32_t)k, 1);\n"
  "}\n"
  "static inline void rt_lrem(uint32_t a, uint32_t d, uint32_t cap, uint32_t i, uint32_t sz, int swap, int line) {\n"
  "  uint32_t n = rt_llen(a, cap); if (i >= n) rt_err(\"index out of bounds\", line);\n"
  "  if (swap) { if (i != --n) memmove(M + d + i * sz, M + d + n * sz, sz); MV(a).u = n; }\n"
  "  else { memmove(M + d + i * sz, M + d + (i + 1) * sz, (n - i - 1) * sz); MV(a).u = n - 1; }\n"
  "}\n";

int rio_aot(Rio *v, FILE *out, const char *host_ffi) {
  static uint8_t lab[65536];
  int pc, i, s, fmain;
  uint8_t *mem;
  vm = v; o = out; label = lab; cur = -1;
  if (!vm->ok || !vm->c) return -1; /* needs the compiler tables: compile with rio_compile_scratch */
  memset(lab, 0, sizeof lab);
  for (pc = 0; pc < (int)vm->pc; pc++) {
    int op = vm->code[pc].op;
    if (op >= A_JMP && op <= A_FORI) lab[vm->code[pc].c] = 1;
    if (op == A_IDX || (op >= A_VADD && op <= A_FVDIV) || op == A_SLICE || op == A_LDX || op == A_LDXB || op == A_STX || op == A_STXB) pc++;
  }
  fprintf(o, "/* generated by rio -c */\n#define RIO_MEM %uu\n%s", vm->memsize, prelude);
  /* memory image: constants + string literals */
  mem = vm->mem;
  fprintf(o, "static const uint32_t rio_k[%u] = {", vm->nk);
  for (i = 0; i < (int)vm->nk; i++) fprintf(o, "%s%uu", i ? (i % 12 ? ", " : ",\n  ") : "\n  ",((RioVal *)mem)[i].u);
  fprintf(o, "\n};\nstatic void rio_load(void) {\n  memcpy(R, rio_k, sizeof rio_k);\n");
  for (i = (int)vm->hi; i < (int)vm->memsize; i++) {
    int e = i, z = 0, st = i;
    if (!mem[i]) continue;
    while (e < (int)vm->memsize && z < 8) { z = mem[e] ? 0 : z + 1; e++; }
    e -= z;
    fprintf(o, "  memcpy(M + %d, \"", i);
    for (; i < e; i++) fprintf(o, "\\%03o", mem[i]);
    fprintf(o, "\", %d);\n", e - st);
    i--;
  }
  fprintf(o, "}\n");
  /* ffi */
  for (i = 0; i < vm->nffi; i++) fprintf(o, "void rio_ffi_%s(RioVal *a);\n", symname(RIO_S_FFI, i));
  if (host_ffi) fprintf(o, "#ifndef RIO_NO_HOST_FFI\n%s#endif\n", host_ffi);
  /* procs: defined in order, and calls only go backwards, so no prototypes are needed */
  for (cur = 0; cur < vm->nfunc; cur++) {
    RioFunc *f = &vm->func[cur];
    memset(used, 0, sizeof used);
    o = 0; lastline = 0; /* dry run: learn which locals the body uses */
    for (pc = f->pc; pc < f->end; pc++) ins(pc);
    o = out; lastline = 0;
    fprintf(o, "\nstatic void %s(void) {\n", fname(cur));
    for (s = f->fs; s < f->fe; s++) {
      if (!islocal(s) || !(used[s >> 3] & (1 << (s & 7)))) continue;
      if (s < f->pend) fprintf(o, "  RioVal r%d = R[%d];\n", s, s); else fprintf(o, "  RioVal r%d = {0};\n", s);
    }
    for (pc = f->pc; pc < f->end; pc++) ins(pc);
    fprintf(o, "}\n");
  }
  cur = -1;
  fprintf(o, "\nstatic void rio_top(void) {\n");
  for (pc = 0; pc < (int)vm->pc; pc++) {
    for (i = 0; i < vm->nfunc; i++) if (pc == vm->func[i].pc) { pc = vm->func[i].end; break; }
    if (pc < (int)vm->pc) ins(pc);
  }
  fprintf(o, "}\n\nint main(void) {\n  rio_load();\n  rio_top();\n");
  if ((fmain = rio_func(vm, "main")) >= 0) fprintf(o, "  %s();\n", fname(fmain));
  fprintf(o, "  return 0;\n}\n");
  return ferror(o) ? -1 : 0;
}
