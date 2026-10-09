/* aot.c - translate compiled nib bytecode to a standalone C program.
   No recursion => every proc becomes a plain C function, and its frame slots become C locals
   (unless their address is taken). Constants are emitted inline. */
#define _CRT_SECURE_NO_WARNINGS
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nib.h"
#include "aot.h"

#define OPENUM(o) A_##o,
enum { NIB_OPS(OPENUM) A_DATA = 255 };

static Nib *vm;
static FILE *o;
static int cur = -1;            /* function being emitted, -1 = top level */
static uint8_t *label;          /* jump targets */
static uint8_t used[8192];      /* locals referenced by the current proc */

static NibVal kval(int s) { return ((NibVal *)vm->mem)[s]; }
static int isret(int s) {
  NibFunc *f = &vm->func[cur];
  return f->ret && s >= (int)(f->retaddr / 4) && s < (int)((f->retaddr + vm->type[f->ret].size + 3) / 4);
}
static int islocal(int s) {
  NibFunc *f;
  if (cur < 0) return 0;
  f = &vm->func[cur];
  return s >= f->fs && s < f->fe && !(vm->exposed[s >> 3] & (1 << (s & 7))) && !isret(s);
}
/* operand text for slot s viewed as t: i(nt) u(nsigned) f(loat) v(whole value) */
static const char *V(int s, int t) {
  static char buf[16][48]; static int n;
  char *b = buf[n++ & 15];
  if (s < (int)vm->nk) {
    NibVal k = kval(s);
    if (t == 'i') { if (k.i == (int32_t)0x80000000u) strcpy(b, "(-2147483647-1)"); else sprintf(b, k.i < 0 ? "(%d)" : "%d", k.i); }
    else if (t == 'u') sprintf(b, "%uu", k.u);
    else if (t == 'f') {
      char *q; float back;
      sprintf(b, "%.9g", k.f); back = strtof(b, &q);
      if (k.f != k.f || back != k.f || k.f - k.f != 0) sprintf(b, "kf(%uu)", k.u);
      else { if (!strpbrk(b, ".e")) strcat(b, ".0"); strcat(b, "f"); if (k.f < 0) { memmove(b + 1, b, strlen(b) + 1); b[0] = '('; strcat(b, ")"); } }
    } else sprintf(b, "((NibVal){.u = %uu})", k.u);
  } else if (islocal(s)) {
    used[s >> 3] |= (uint8_t)(1 << (s & 7));
    if (t == 'v') sprintf(b, "r%d", s); else sprintf(b, "r%d.%c", s, t); }
  else { if (t == 'v') sprintf(b, "R[%d]", s); else sprintf(b, "R[%d].%c", s, t); }
  return b;
}
static uint32_t sz(const NibIns *d) { return (uint32_t)d->b | (uint32_t)d->c << 16; }
static const char *symname(int kind, int idx) {
  static char b[64]; int i;
  for (i = 0; i < vm->nsym; i++)
    if (vm->sym[i].k == kind && vm->sym[i].v == idx) {
      int n = vm->sym[i].len < 60 ? vm->sym[i].len : 60;
      memcpy(b, vm->names + vm->sym[i].name, (size_t)n); b[n] = 0;
      return b;
    }
  sprintf(b, "anon%d", idx);
  return b;
}
/* print to the output, or only evaluate the operands (marking used locals) during the dry run */
static void P(const char *fmt, ...) { va_list ap; va_start(ap, fmt); if (o) vfprintf(o, fmt, ap); va_end(ap); }
static int fnat(int pc) { int i; for (i = 0; i < vm->nfunc; i++) if (vm->func[i].pc == pc) return i; return -1; }

static void ins(int pc) {
  const NibIns *x = &vm->code[pc], *d = x + 1;
  int a = x->a, b = x->b, c = x->c;
  static const char *bin[] = {"+", "-", "*", 0, 0, "&", "|", "^"};
  static const char *cmp[] = {"==", "!=", "<", "<="};
  static const char *m1[] = {"sqrtf", "sinf", "cosf", "tanf", "asinf", "acosf", "atanf", "expf", "logf", "floorf", "ceilf", "rt_round", "fabsf"};
  static const char *m2[] = {"atan2f", "powf", "fmodf"};
  if (x->op == A_DATA) return;
  if (label[pc]) P("L%d:;\n", pc);
  P("  ");
  switch (x->op) {
  case A_MOV: P("%s = %s;\n", V(a, 'v'), V(b, 'v')); break;
  case A_ADD: case A_SUB: case A_MUL: case A_AND: case A_OR: case A_XOR:
    P("%s = %s %s %s;\n", V(a, 'u'), V(b, 'u'), bin[x->op - A_ADD], V(c, 'u')); break;
  case A_DIV: case A_MOD: {
    const char *op = x->op == A_DIV ? "/" : "%";
    if (c < (int)vm->nk && kval(c).i != 0 && kval(c).i != -1) P("%s = %s %s %s;\n", V(a, 'i'), V(b, 'i'), op, V(c, 'i'));
    else P("{ int32_t y_ = %s, x_ = %s; if (!y_) rt_err(\"division by zero\"); %s = y_ == -1 ? %s : x_ %s y_; }\n",
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
    P("{ uint32_t i_ = %s; if (i_ >= %s) rt_err(\"index out of bounds\"); ", V(c, 'u'), V(b + 1, 'u'));
    if (x->op == A_LDX) P("%s = MV(%s + i_ * %uu + %du); }\n", V(a, 'v'), V(b, 'u'), sz(d), d->a);
    else P("%s = M[%s + i_ * %uu + %du]; }\n", V(a, 'u'), V(b, 'u'), sz(d), d->a);
    break;
  case A_MOV2: P("{ NibVal x_ = %s, y_ = %s; %s = x_; %s = y_; }\n", V(b, 'v'), V(b + 1, 'v'), V(a, 'v'), V(a + 1, 'v')); break;
  case A_LDW2: P("{ uint32_t p_ = %s + %du; NibVal x_ = MV(p_), y_ = MV(p_ + 4); %s = x_; %s = y_; }\n", V(b, 'u'), c, V(a, 'v'), V(a + 1, 'v')); break;
  case A_STW: P("MV(%s + %du) = %s;\n", V(a, 'u'), c, V(b, 'v')); break;
  case A_STB: P("M[%s + %du] = (uint8_t)%s;\n", V(a, 'u'), c, V(b, 'u')); break;
  case A_STW2: P("{ uint32_t p_ = %s + %du; MV(p_) = %s; MV(p_ + 4) = %s; }\n", V(a, 'u'), c, V(b, 'v'), V(b + 1, 'v')); break;
  case A_STX: case A_STXB:
    P("{ uint32_t i_ = %s; if (i_ >= %s) rt_err(\"index out of bounds\"); ", V(b, 'u'), V(a + 1, 'u'));
    if (x->op == A_STX) P("MV(%s + i_ * %uu + %du) = %s; }\n", V(a, 'u'), sz(d), d->a, V(c, 'v'));
    else P("M[%s + i_ * %uu + %du] = (uint8_t)%s; }\n", V(a, 'u'), sz(d), d->a, V(c, 'u'));
    break;
  case A_IDX:
    P("{ uint32_t i_ = %s; if (i_ >= %s) rt_err(\"index out of bounds\"); %s = %s + i_ * %uu; }\n",
            V(c, 'u'), V(b + 1, 'u'), V(a, 'u'), V(b, 'u'), sz(d));
    break;
  case A_SLICE:
    P("{ int32_t lo_ = %s, hi_ = %s, n_ = %s, p_ = %s; if (lo_ < 0 || hi_ < lo_ || hi_ > n_) rt_err(\"slice out of bounds\"); ",
            V(c, 'i'), V(d->a, 'i'), V(b + 1, 'i'), V(b, 'i'));
    P("%s = p_ + lo_ * %d; %s = hi_ - lo_; }\n", V(a, 'i'), (int)sz(d), V(a + 1, 'i'));
    break;
  case A_COPY: P("memmove(M + %s, M + %s, %d);\n", V(a, 'u'), V(b, 'u'), c); break;
  case A_ZERO: P("memset(M + %s, 0, %uu);\n", V(a, 'u'), sz(x)); break;
  case A_JMP: P("goto L%d;\n", c); break;
  case A_JZ: P("if (!%s) goto L%d;\n", V(a, 'i'), c); break;
  case A_JNZ: P("if (%s) goto L%d;\n", V(a, 'i'), c); break;
  case A_JEQ: case A_JNE: case A_JLT: case A_JLE:
    P("if (%s %s %s) goto L%d;\n", V(a, 'i'), cmp[x->op - A_JEQ], V(b, 'i'), c); break;
  case A_JFEQ: case A_JFNE: case A_JFLT: case A_JFLE:
    P("if (%s %s %s) goto L%d;\n", V(a, 'f'), cmp[x->op - A_JFEQ], V(b, 'f'), c); break;
  case A_JFNLT: case A_JFNLE:
    P("if (!(%s %s %s)) goto L%d;\n", V(a, 'f'), x->op == A_JFNLT ? "<" : "<=", V(b, 'f'), c); break;
  case A_FORI: P("if ((%s = (int32_t)(%s + 1u)) < %s) goto L%d;\n", V(a, 'i'), V(a, 'u'), V(b, 'i'), c); break;
  case A_CALL: P("p_%s();\n", symname(NIB_S_FN, fnat(c))); break;
  case A_RET: case A_HALT: P("return;\n"); break;
  case A_FFI: {
    NibFfi *f = &vm->ffi[c]; int n = 0, i, rw = f->ret ? (int)(vm->type[f->ret].size + 3) / 4 : 0;
    for (i = 0; i < f->np; i++) n += (int)(vm->type[vm->param[f->p0 + i].t].size + 3) / 4;
    P("{ NibVal a_[%d];", (n > rw ? n : rw) + 1);
    for (i = 0; i < n; i++) P(" a_[%d] = %s;", i, V(a + i, 'v'));
    P(" nib_ffi_%s(a_);", symname(NIB_S_FFI, c));
    for (i = 0; i < rw; i++) P(" %s = a_[%d];", V(a + i, 'v'), i);
    P(" }\n");
    break;
  }
  case A_LOGI: P("log_i(%d, %s);\n", x->x, V(a, 'i')); break;
  case A_LOGF: P("log_f(%d, %s);\n", x->x, V(a, 'f')); break;
  case A_LOGB: P("log_s(%d, %s ? \"true\" : \"false\", -1);\n", x->x, V(a, 'i')); break;
  case A_LOGS: P("log_s(%d, (const char *)M + %s, %s);\n", x->x, V(a, 'u'), V(a + 1, 'i')); break;
  case A_LOGE: P("log_e();\n"); break;
  default: P("rt_err(\"bad op\");\n");
  }
}

static const char *prelude =
  "#define _CRT_SECURE_NO_WARNINGS\n#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <math.h>\n#include <time.h>\n"
  "#ifdef __GNUC__\n#pragma GCC diagnostic ignored \"-Wunused\"\n#endif\n"
  "typedef union { int32_t i; float f; uint32_t u; } NibVal;\n"
  "static union { NibVal v[NIB_MEM / 4]; uint8_t b[NIB_MEM]; } nib_mem;\n"
  "#define M nib_mem.b\n#define R nib_mem.v\n#define MV(p) (*(NibVal *)(M + (uint32_t)(p)))\n"
  "static inline float kf(uint32_t u) { NibVal v; v.u = u; return v.f; }\n"
  "static inline void rt_err(const char *m) { fflush(stdout); fprintf(stderr, \"runtime error: %s\\n\", m); exit(1); }\n"
  "static inline int32_t ftoi(float f) { return f != f ? 0 : f >= 2147483648.f ? 0x7FFFFFFF : f <= -2147483648.f ? (int32_t)0x80000000u : (int32_t)f; }\n"
  "static inline float rt_round(float x) { return x < 0 ? -floorf(-x + 0.5f) : floorf(x + 0.5f); }\n"
  "static inline int seq(int32_t a, int32_t an, int32_t b, int32_t bn) { return an == bn && !memcmp(M + a, M + b, (size_t)an); }\n"
  "static char lb[256]; static int ln;\n"
  "static inline void log_s(int sp, const char *s, int n) { if (n < 0) n = (int)strlen(s); if (sp && ln < 256) lb[ln++] = ' '; while (n-- > 0 && ln < 256) lb[ln++] = *s++; }\n"
  "static inline void log_i(int sp, int32_t v) { char b[16]; sprintf(b, \"%d\", (int)v); log_s(sp, b, -1); }\n"
  "static inline void log_f(int sp, float f) {\n"
  "  char b[40]; int i = 0, e = 0, nd = 1, dec, k; double d = f; uint32_t ip, fp, sc = 1, t;\n"
  "  if (f != f) { log_s(sp, \"nan\", 3); return; }\n"
  "  if (d < 0) { b[i++] = '-'; d = -d; }\n"
  "  if (d > 3.5e38) { memcpy(b + i, \"inf\", 4); log_s(sp, b, -1); return; }\n"
  "  if (d != 0 && (d >= 1e9 || d < 1e-4)) { while (d >= 10) { d /= 10; e++; } while (d < 1) { d *= 10; e--; } }\n"
  "  ip = (uint32_t)d; for (t = ip; t >= 10; t /= 10) nd++;\n"
  "  dec = 7 - nd; if (dec < 1) dec = 1; if (dec > 6) dec = 6; for (k = 0; k < dec; k++) sc *= 10;\n"
  "  fp = (uint32_t)((d - ip) * sc + 0.5); if (fp >= sc) { ip++; fp -= sc; }\n"
  "  i += sprintf(b + i, \"%u.\", (unsigned)ip);\n"
  "  for (k = dec - 1; k >= 0; k--) { b[i + k] = (char)('0' + fp % 10); fp /= 10; }\n"
  "  while (dec > 1 && b[i + dec - 1] == '0') dec--; i += dec;\n"
  "  if (e) i += sprintf(b + i, \"e%d\", e);\n"
  "  b[i] = 0; log_s(sp, b, -1);\n"
  "}\n"
  "static inline void log_e(void) { fwrite(lb, 1, (size_t)ln, stdout); fputc('\\n', stdout); ln = 0; }\n";

int nib_aot(Nib *v, FILE *out, const char *host_ffi) {
  static uint8_t lab[65536];
  int pc, i, s, fmain;
  uint8_t *mem;
  vm = v; o = out; label = lab; cur = -1;
  memset(lab, 0, sizeof lab);
  for (pc = 0; pc < (int)vm->pc; pc++) {
    int op = vm->code[pc].op;
    if (op >= A_JMP && op <= A_FORI) lab[vm->code[pc].c] = 1;
    if (op == A_IDX || op == A_SLICE || op == A_LDX || op == A_LDXB || op == A_STX || op == A_STXB) pc++;
  }
  fprintf(o, "/* generated by nib -c */\n#define NIB_MEM %uu\n%s", vm->memsize, prelude);
  /* memory image: constants + string literals */
  mem = vm->mem;
  fprintf(o, "static const uint32_t nib_k[%u] = {", vm->nk);
  for (i = 0; i < (int)vm->nk; i++) fprintf(o, "%s%uu", i ? (i % 12 ? ", " : ",\n  ") : "\n  ",((NibVal *)mem)[i].u);
  fprintf(o, "\n};\nstatic void nib_load(void) {\n  memcpy(R, nib_k, sizeof nib_k);\n");
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
  for (i = 0; i < vm->nffi; i++) fprintf(o, "void nib_ffi_%s(NibVal *a);\n", symname(NIB_S_FFI, i));
  if (host_ffi) fprintf(o, "#ifndef NIB_NO_HOST_FFI\n%s#endif\n", host_ffi);
  /* procs: defined in order, and calls only go backwards, so no prototypes are needed */
  for (cur = 0; cur < vm->nfunc; cur++) {
    NibFunc *f = &vm->func[cur];
    memset(used, 0, sizeof used);
    o = 0; /* dry run: learn which locals the body uses */
    for (pc = f->pc; pc < f->end; pc++) ins(pc);
    o = out;
    fprintf(o, "\nstatic void p_%s(void) {\n", symname(NIB_S_FN, cur));
    for (s = f->fs; s < f->fe; s++) {
      if (!islocal(s) || !(used[s >> 3] & (1 << (s & 7)))) continue;
      for (i = 0; i < f->np; i++) {
        NibParam *p = &vm->param[f->p0 + i];
        if (s >= (int)(p->addr / 4) && s < (int)((p->addr + vm->type[p->t].size + 3) / 4)) break;
      }
      if (i < f->np) fprintf(o, "  NibVal r%d = R[%d];\n", s, s); else fprintf(o, "  NibVal r%d = {0};\n", s);
    }
    for (pc = f->pc; pc < f->end; pc++) ins(pc);
    fprintf(o, "}\n");
  }
  cur = -1;
  fprintf(o, "\nstatic void nib_top(void) {\n");
  for (pc = 0; pc < (int)vm->pc; pc++) {
    for (i = 0; i < vm->nfunc; i++) if (pc == vm->func[i].pc) { pc = vm->func[i].end; break; }
    if (pc < (int)vm->pc) ins(pc);
  }
  fprintf(o, "}\n\nint main(void) {\n  nib_load();\n  nib_top();\n");
  if ((fmain = nib_func(vm, "main")) >= 0) fprintf(o, "  p_%s();\n", symname(NIB_S_FN, fmain));
  fprintf(o, "  return 0;\n}\n");
  return ferror(o) ? -1 : 0;
}
