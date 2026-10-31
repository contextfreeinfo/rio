/* aot.c - translates compiled rio bytecode to a standalone C program. No recursion: every proc becomes a
   plain C function, and its frame slots become C locals (unless their address is taken). Constants are
   written inline, and each instruction becomes one C statement. Memory is a static image identical to
   the vm's, so behavior is the same, bounds checks, wrapping and all. #line directives keep __LINE__ on
   the rio line, which runtime errors report. */
#define _CRT_SECURE_NO_WARNINGS
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"
#include "aot.h"

typedef struct {
  rio_Vm *vm;
  FILE *out;            /* NULL during a dry run, which only notes the locals a proc uses */
  int proc;             /* the proc being written, or -1 for top-level code */
  uint8_t isLabel[65536]; /* jump targets */
  uint8_t used[8192];   /* the current proc's locals that the code reads or writes */
  int lastLine;         /* what __LINE__ is on the next output line; 0: unknown */
} rio_Aot;

static rio_Val rio_constant(rio_Aot *a, int s) { return ((rio_Val *)a->vm->mem)[s]; }

static int rio_isResultSlot(rio_Aot *a, int s) {
  rio_Proc *p = &a->vm->procs[a->proc];
  return s >= p->result && s < p->result + p->resultWords;
}

/* a slot of the current proc's frame that can be a C local: its address is never taken, and it isn't the
   result (which callers read) */
static int rio_isLocal(rio_Aot *a, int s) {
  rio_Proc *p;
  if (a->proc < 0) return 0;
  p = &a->vm->procs[a->proc];
  return s >= p->frameStart && s < p->frameEnd && !rio_bitGet(a->vm->compiler->exposed, (uint32_t)s) && !rio_isResultSlot(a, s);
}

/* The C text for slot s viewed as view: i(nt) u(nsigned) f(loat) v(whole value). Constants are written
   as literals. (Returns one of 16 buffers in turn: enough for any one statement.) */
static const char *rio_operand(rio_Aot *a, int s, int view) {
  static char buffers[16][48];
  static int next;
  char *b = buffers[next++ & 15];
  if (s < (int)a->vm->constCount) {
    rio_Val k = rio_constant(a, s);
    if (view == 'i') {
      if (k.i == (int32_t)0x80000000u) strcpy(b, "(-2147483647-1)");
      else sprintf(b, k.i < 0 ? "(%d)" : "%d", k.i);
    } else if (view == 'u') sprintf(b, "%uu", k.u);
    else if (view == 'f') { /* exactly, or as bits when no literal reads back the same */
      char *end;
      float back;
      sprintf(b, "%.9g", k.f);
      back = strtof(b, &end);
      if (k.f != k.f || back != k.f || k.f - k.f != 0) sprintf(b, "rio_floatBits(%uu)", k.u);
      else {
        if (!strpbrk(b, ".e")) strcat(b, ".0");
        strcat(b, "f");
        if (k.f < 0) {
          memmove(b + 1, b, strlen(b) + 1);
          b[0] = '(';
          strcat(b, ")");
        }
      }
    } else sprintf(b, "((rio_Val){.u = %uu})", k.u);
  } else if (rio_isLocal(a, s)) {
    rio_bitSet(a->used, (uint32_t)s);
    if (view == 'v') sprintf(b, "r%d", s);
    else sprintf(b, "r%d.%c", s, view);
  } else {
    if (view == 'v') sprintf(b, "rio_R[%d]", s);
    else sprintf(b, "rio_R[%d].%c", s, view);
  }
  return b;
}

/* host proc i's name as part of a C name: pc.cls is pc_cls */
static const char *rio_ffiName(rio_Aot *a, int i) {
  static char b[64];
  const char *s = a->vm->ffi[i].name;
  int n;
  for (n = 0; s[n] && n < 63; n++) b[n] = s[n] == '.' ? '_' : s[n];
  b[n] = 0;
  return b;
}

/* prints to the output, or only evaluates the operands (noting the locals used) during the dry run */
#if defined(__GNUC__) || defined(__clang__)
static void rio_print(rio_Aot *a, const char *fmt, ...) __attribute__((format(printf, 2, 3))); /* lets the compiler check every call */
#endif
static void rio_print(rio_Aot *a, const char *fmt, ...) {
  static char b[4096];
  va_list ap;
  int n, i;
  if (!a->out) return;
  va_start(ap, fmt);
  n = vsnprintf(b, sizeof b, fmt, ap);
  va_end(ap);
  if (n < 0) return;
  if (n >= (int)sizeof b) {
    va_start(ap, fmt);
    vfprintf(a->out, fmt, ap);
    va_end(ap);
    n = (int)sizeof b - 1;
  } else fputs(b, a->out);
  for (i = 0; i < n && a->lastLine; i++) a->lastLine += b[i] == '\n'; /* every line written moves __LINE__ on */
}

/* the C name for proc or method i: rio_p<i>_<name>, unique even when methods of different types share a name */
static const char *rio_procName(rio_Aot *a, int proc) {
  static char b[80];
  rio_Compiler *c = a->vm->compiler;
  int i;
  for (i = 0; i < c->symCount; i++)
    if ((c->syms[i].kind == rio_SymKind_proc || c->syms[i].kind == rio_SymKind_method) && c->syms[i].value == proc) {
      int n = c->syms[i].nameLen < 60 ? c->syms[i].nameLen : 60;
      sprintf(b, "rio_p%d_%.*s", proc, n, c->names + c->syms[i].name);
      return b;
    }
  sprintf(b, "rio_p%d", proc);
  return b;
}

static int rio_procAtPc(rio_Aot *a, int pc) {
  int i;
  for (i = 0; i < a->vm->procCount; i++)
    if (a->vm->procs[i].entry == pc) return i;
  return -1;
}

/* ------------------------------------------------------------------ instructions */

#define rio_V(s, view) rio_operand(a, s, view)

/* a builder op's element size: its data's, or a slot's (a generic proc's T) */
static const char *rio_elemSize(rio_Aot *a, const rio_Instr *data) {
  static char b[16];
  if (data->a) return rio_operand(a, data->a, 'u');
  sprintf(b, "%uu", rio_dataSize(data));
  return b;
}

/* the statement for the instruction at pc */
static void rio_instruction(rio_Aot *a, int pc) {
  static const char *const binary[] = {"+", "-", "*", 0, 0, "&", "|", "^"};
  static const char *const compare[] = {"==", "!=", "<", "<="};
  static const char *const math1[] = {"sqrtf", "sinf", "cosf", "tanf", "asinf", "acosf", "atanf", "expf", "logf",
                                      "floorf", "ceilf", "rio_round", "fabsf"};
  static const char *const math2[] = {"atan2f", "powf", "fmodf"};
  rio_Vm *vm = a->vm;
  const rio_Instr *in = &vm->code[pc], *data = in + 1;
  int A = in->a, B = in->b, C = in->c;
  if (in->op == rio_Op_data) return;
  if (a->isLabel[pc]) rio_print(a, "L%d:;\n", pc);
  if (a->out) { /* keep __LINE__ on the rio line, however many C lines a line becomes (not needed in the dry run) */
    int line = rio_pcLine(vm, (uint32_t)pc);
    if (line && line != a->lastLine) {
      rio_print(a, "#line %d\n", line);
      a->lastLine = line;
    }
  }
  rio_print(a, "  ");
  switch (in->op) {
  case rio_Op_mov: rio_print(a, "%s = %s;\n", rio_V(A, 'v'), rio_V(B, 'v')); break;
  case rio_Op_add: case rio_Op_sub: case rio_Op_mul: case rio_Op_and: case rio_Op_or: case rio_Op_xor:
    rio_print(a, "%s = %s %s %s;\n", rio_V(A, 'u'), rio_V(B, 'u'), binary[in->op - rio_Op_add], rio_V(C, 'u'));
    break;
  case rio_Op_div: case rio_Op_mod: {
    const char *op = in->op == rio_Op_div ? "/" : "%";
    if (C < (int)vm->constCount && rio_constant(a, C).i != 0 && rio_constant(a, C).i != -1) /* a safe constant divisor */
      rio_print(a, "%s = %s %s %s;\n", rio_V(A, 'i'), rio_V(B, 'i'), op, rio_V(C, 'i'));
    else
      rio_print(a, "{ int32_t y_ = %s, x_ = %s; if (!y_) rio_RT_ERROR(\"division by zero\"); %s = y_ == -1 ? %s : x_ %s y_; }\n",
                rio_V(C, 'i'), rio_V(B, 'i'), rio_V(A, 'i'), in->op == rio_Op_div ? "(int32_t)(0u - (uint32_t)x_)" : "0", op);
    break;
  }
  case rio_Op_shl: rio_print(a, "%s = %s << (%s & 31);\n", rio_V(A, 'u'), rio_V(B, 'u'), rio_V(C, 'u')); break;
  case rio_Op_shr: rio_print(a, "%s = %s >> (%s & 31);\n", rio_V(A, 'i'), rio_V(B, 'i'), rio_V(C, 'u')); break;
  case rio_Op_ushr: rio_print(a, "%s = %s >> (%s & 31);\n", rio_V(A, 'u'), rio_V(B, 'u'), rio_V(C, 'u')); break;
  case rio_Op_neg: rio_print(a, "%s = 0u - %s;\n", rio_V(A, 'u'), rio_V(B, 'u')); break;
  case rio_Op_bnot: rio_print(a, "%s = ~%s;\n", rio_V(A, 'u'), rio_V(B, 'u')); break;
  case rio_Op_not: rio_print(a, "%s = !%s;\n", rio_V(A, 'i'), rio_V(B, 'i')); break;
  case rio_Op_fadd: case rio_Op_fsub: case rio_Op_fmul: case rio_Op_fdiv:
    rio_print(a, "%s = %s %c %s;\n", rio_V(A, 'f'), rio_V(B, 'f'), "+-*/"[in->op - rio_Op_fadd], rio_V(C, 'f'));
    break;
  case rio_Op_fneg: rio_print(a, "%s = -%s;\n", rio_V(A, 'f'), rio_V(B, 'f')); break;
  case rio_Op_eq: case rio_Op_ne: case rio_Op_lt: case rio_Op_le:
    rio_print(a, "%s = %s %s %s;\n", rio_V(A, 'i'), rio_V(B, 'i'), compare[in->op - rio_Op_eq], rio_V(C, 'i'));
    break;
  case rio_Op_feq: case rio_Op_fne: case rio_Op_flt: case rio_Op_fle:
    rio_print(a, "%s = %s %s %s;\n", rio_V(A, 'i'), rio_V(B, 'f'), compare[in->op - rio_Op_feq], rio_V(C, 'f'));
    break;
  case rio_Op_seq: case rio_Op_sne:
    rio_print(a, "%s = %srio_stringEq(%s, %s, %s, %s);\n", rio_V(A, 'i'), in->op == rio_Op_sne ? "!" : "", rio_V(B, 'i'),
              rio_V(B + 1, 'i'), rio_V(C, 'i'), rio_V(C + 1, 'i'));
    break;
  case rio_Op_itof: rio_print(a, "%s = (float)%s;\n", rio_V(A, 'f'), rio_V(B, 'i')); break;
  case rio_Op_ftoi: rio_print(a, "%s = rio_floatToInt(%s);\n", rio_V(A, 'i'), rio_V(B, 'f')); break;
  case rio_Op_ldw: rio_print(a, "%s = rio_WORD(%s + %du);\n", rio_V(A, 'v'), rio_V(B, 'u'), C); break;
  case rio_Op_ldb: rio_print(a, "%s = rio_M[%s + %du];\n", rio_V(A, 'u'), rio_V(B, 'u'), C); break;
  case rio_Op_lea: rio_print(a, "%s = %s + %du;\n", rio_V(A, 'u'), rio_V(B, 'u'), C); break;
  case rio_Op_m1: rio_print(a, "%s = %s(%s);\n", rio_V(A, 'f'), math1[in->x], rio_V(B, 'f')); break;
  case rio_Op_m2: rio_print(a, "%s = %s(%s, %s);\n", rio_V(A, 'f'), math2[in->x], rio_V(B, 'f'), rio_V(C, 'f')); break;
  case rio_Op_iabs: rio_print(a, "%s = %s < 0 ? 0u - %s : %s;\n", rio_V(A, 'u'), rio_V(B, 'i'), rio_V(B, 'u'), rio_V(B, 'u')); break;
  case rio_Op_imin: case rio_Op_imax: case rio_Op_fmin: case rio_Op_fmax: {
    int view = in->op <= rio_Op_imax ? 'i' : 'f';
    rio_print(a, "%s = %s %c %s ? %s : %s;\n", rio_V(A, view), rio_V(B, view), in->op == rio_Op_imin || in->op == rio_Op_fmin ? '<' : '>',
              rio_V(C, view), rio_V(B, view), rio_V(C, view));
    break;
  }
  case rio_Op_ldx: case rio_Op_ldxb:
    rio_print(a, "{ uint32_t i_ = %s; if (i_ >= %s) rio_RT_ERROR(\"index out of bounds\"); ", rio_V(C, 'u'), rio_V(B + 1, 'u'));
    if (in->op == rio_Op_ldx) rio_print(a, "%s = rio_WORD(%s + i_ * %uu + %du); }\n", rio_V(A, 'v'), rio_V(B, 'u'), rio_dataSize(data), data->a);
    else rio_print(a, "%s = rio_M[%s + i_ * %uu + %du]; }\n", rio_V(A, 'u'), rio_V(B, 'u'), rio_dataSize(data), data->a);
    break;
  case rio_Op_mov2:
    rio_print(a, "{ rio_Val x_ = %s, y_ = %s; %s = x_; %s = y_; }\n", rio_V(B, 'v'), rio_V(B + 1, 'v'), rio_V(A, 'v'), rio_V(A + 1, 'v'));
    break;
  case rio_Op_ldw2:
    rio_print(a, "{ uint32_t p_ = %s + %du; rio_Val x_ = rio_WORD(p_), y_ = rio_WORD(p_ + 4); %s = x_; %s = y_; }\n", rio_V(B, 'u'), C,
              rio_V(A, 'v'), rio_V(A + 1, 'v'));
    break;
  case rio_Op_stw: rio_print(a, "rio_WORD(%s + %du) = %s;\n", rio_V(A, 'u'), C, rio_V(B, 'v')); break;
  case rio_Op_stb: rio_print(a, "rio_M[%s + %du] = (uint8_t)%s;\n", rio_V(A, 'u'), C, rio_V(B, 'u')); break;
  case rio_Op_stw2:
    rio_print(a, "{ uint32_t p_ = %s + %du; rio_WORD(p_) = %s; rio_WORD(p_ + 4) = %s; }\n", rio_V(A, 'u'), C, rio_V(B, 'v'), rio_V(B + 1, 'v'));
    break;
  case rio_Op_stx: case rio_Op_stxb:
    rio_print(a, "{ uint32_t i_ = %s; if (i_ >= %s) rio_RT_ERROR(\"index out of bounds\"); ", rio_V(B, 'u'), rio_V(A + 1, 'u'));
    if (in->op == rio_Op_stx) rio_print(a, "rio_WORD(%s + i_ * %uu + %du) = %s; }\n", rio_V(A, 'u'), rio_dataSize(data), data->a, rio_V(C, 'v'));
    else rio_print(a, "rio_M[%s + i_ * %uu + %du] = (uint8_t)%s; }\n", rio_V(A, 'u'), rio_dataSize(data), data->a, rio_V(C, 'u'));
    break;
  case rio_Op_idx:
    rio_print(a, "{ uint32_t i_ = %s; if (i_ >= %s) rio_RT_ERROR(\"index out of bounds\"); %s = %s + i_ * %uu; }\n", rio_V(C, 'u'),
              rio_V(B + 1, 'u'), rio_V(A, 'u'), rio_V(B, 'u'), rio_dataSize(data));
    break;
  case rio_Op_slice:
    rio_print(a, "{ int32_t lo_ = %s, hi_ = %s, n_ = %s, p_ = %s; if (lo_ < 0 || hi_ < lo_ || hi_ > n_) rio_RT_ERROR(\"slice out of bounds\"); ",
              rio_V(C, 'i'), rio_V(data->a, 'i'), rio_V(B + 1, 'i'), rio_V(B, 'i'));
    rio_print(a, "%s = p_ + lo_ * %d; %s = hi_ - lo_; }\n", rio_V(A, 'i'), (int)rio_dataSize(data), rio_V(A + 1, 'i'));
    break;
  case rio_Op_movn: { /* word by word, in the order that's safe when the ranges overlap */
    int i;
    if (A > B)
      for (i = C - 1; i >= 0; i--) rio_print(a, "%s = %s;\n", rio_V(A + i, 'v'), rio_V(B + i, 'v'));
    else
      for (i = 0; i < C; i++) rio_print(a, "%s = %s;\n", rio_V(A + i, 'v'), rio_V(B + i, 'v'));
    break;
  }
  case rio_Op_enumname:
    rio_print(a, "{ uint32_t i_ = %s; if (i_ < %uu) { %s = rio_R[%d + 2 * i_]; %s = rio_R[%d + 2 * i_ + 1]; } else { %s.u = 0; %s.u = 0; } }\n",
              rio_V(B, 'u'), rio_dataSize(data), rio_V(A, 'v'), data->a, rio_V(A + 1, 'v'), data->a, rio_V(A, 'v'), rio_V(A + 1, 'v'));
    break;
  case rio_Op_enumck:
    rio_print(a, "{ uint32_t i_ = %s; ", rio_V(B, 'u'));
    if (in->x) rio_print(a, "if (i_ >= %uu) rio_RT_ERROR(\"not a value of that enum\"); }\n", rio_dataSize(data));
    else rio_print(a, "%s = (int32_t)(i_ < %uu); }\n", rio_V(A, 'i'), rio_dataSize(data));
    break;
  case rio_Op_copy: rio_print(a, "memmove(rio_M + %s, rio_M + %s, %d);\n", rio_V(A, 'u'), rio_V(B, 'u'), C); break;
  case rio_Op_copyv: rio_print(a, "memmove(rio_M + %s, rio_M + %s, %s);\n", rio_V(A, 'u'), rio_V(B, 'u'), rio_V(C, 'u')); break;
  case rio_Op_idxv:
    rio_print(a, "{ uint32_t i_ = %s; if (i_ >= %s) rio_RT_ERROR(\"index out of bounds\"); %s = %s + i_ * %s; }\n", rio_V(C, 'u'),
              rio_V(B + 1, 'u'), rio_V(A, 'u'), rio_V(B, 'u'), rio_V(A, 'u'));
    break;
  case rio_Op_swapv:
    rio_print(a, "{ uint8_t *x_ = rio_M + %s, *y_ = rio_M + %s, t_; uint32_t n_ = %s; while (n_--) t_ = *x_, *x_++ = *y_, *y_++ = t_; }\n",
              rio_V(A, 'u'), rio_V(B, 'u'), rio_V(C, 'u'));
    break;
  case rio_Op_vadd: case rio_Op_vsub: case rio_Op_vmul: case rio_Op_vdiv: case rio_Op_vmod:
  case rio_Op_fvadd: case rio_Op_fvsub: case rio_Op_fvmul: case rio_Op_fvdiv: { /* a loop over the numbers; a scalar side is read once */
    int isFloat = in->op >= rio_Op_fvadd, which = isFloat ? in->op - rio_Op_fvadd : in->op - rio_Op_vadd, view = isFloat ? 'f' : 'u';
    int lScalar = in->x & 16, rScalar = in->x & 32;
    const char *l = lScalar ? "l_" : "rio_WORD(l_ + 4u * i_)", *r = rScalar ? "r_" : "rio_WORD(r_ + 4u * i_)";
    rio_print(a, "{ uint32_t d_ = %s, i_; ", rio_V(A, 'u'));
    if (lScalar) rio_print(a, "rio_Val l_ = %s; ", rio_V(B, 'v'));
    else rio_print(a, "uint32_t l_ = %s; ", rio_V(B, 'u'));
    if (rScalar) rio_print(a, "rio_Val r_ = %s; ", rio_V(C, 'v'));
    else rio_print(a, "uint32_t r_ = %s; ", rio_V(C, 'u'));
    rio_print(a, "for (i_ = 0; i_ < %uu; i_++) ", rio_dataSize(data));
    if (which <= 2 || isFloat) rio_print(a, "rio_WORD(d_ + 4u * i_).%c = %s.%c %c %s.%c; }\n", view, l, view, "+-*/"[which], r, view);
    else
      rio_print(a, "{ int32_t y_ = %s.i, v_ = %s.i; if (!y_) rio_RT_ERROR(\"division by zero\"); rio_WORD(d_ + 4u * i_).i = y_ == -1 ? %s : v_ %c y_; } }\n",
                r, l, which == 3 ? "(int32_t)(0u - (uint32_t)v_)" : "0", which == 3 ? '/' : '%');
    break;
  }
  case rio_Op_zero: rio_print(a, "memset(rio_M + %s, 0, %uu);\n", rio_V(A, 'u'), rio_dataSize(in)); break;
  case rio_Op_lidx:
    rio_print(a, "{ uint32_t i_ = %s; if (i_ >= rio_builderLen(%s, %s)) rio_RT_ERROR(\"index out of bounds\"); %s = %s + i_ * %s; }\n",
              rio_V(C, 'u'), rio_V(B, 'u'), rio_V(B + 2, 'u'), rio_V(A, 'u'), rio_V(B + 1, 'u'), rio_elemSize(a, data));
    break;
  case rio_Op_pusha:
    rio_print(a, "{ uint32_t b_ = %s, d_ = %s, c_ = %s, n_ = rio_builderLen(b_, c_); if (n_ >= c_) %s = 0; else { rio_WORD(b_).u = n_ + 1; %s = d_ + n_ * %s; } }\n",
              rio_V(B, 'u'), rio_V(B + 1, 'u'), rio_V(B + 2, 'u'), rio_V(A, 'u'), rio_V(A, 'u'), rio_elemSize(a, data));
    break;
  case rio_Op_pushs:
    rio_print(a, "%s = rio_listPush(%s, %s, %s, rio_M + %s, %s, %s);\n", rio_V(A, 'i'), rio_V(B, 'u'), rio_V(B + 1, 'u'), rio_V(B + 2, 'u'),
              rio_V(C, 'u'), rio_V(C + 1, 'u'), rio_elemSize(a, data));
    break;
  case rio_Op_pusht:
    rio_print(a, "%s = rio_listPushText(%s, %s, %s, %d, %s);\n", rio_V(A, 'i'), rio_V(B, 'u'), rio_V(B + 1, 'u'), rio_V(B + 2, 'u'), in->x,
              rio_V(C, 'v'));
    break;
  case rio_Op_pushf:
    rio_print(a, "%s = rio_pushFormatted(%s, %s, %s, %s, %d, %s, %s);\n", rio_V(A, 'i'), rio_V(B, 'u'), rio_V(B + 1, 'u'), rio_V(B + 2, 'u'),
              rio_V((int)rio_dataSize(data), 'u'), in->x, rio_V(C, 'u'), in->x == 3 ? rio_V(C + 1, 'u') : "0u");
    break;
  case rio_Op_popa:
    rio_print(a, "{ uint32_t b_ = %s, d_ = %s, n_ = rio_builderLen(b_, %s); if (!n_) rio_RT_ERROR(\"pop from empty builder\"); rio_WORD(b_).u = --n_; %s = d_ + n_ * %s; }\n",
              rio_V(B, 'u'), rio_V(B + 1, 'u'), rio_V(B + 2, 'u'), rio_V(A, 'u'), rio_elemSize(a, data));
    break;
  case rio_Op_lrem: case rio_Op_lswap:
    rio_print(a, "rio_listRemove(%s, %s, %s, %s, %s, %d, __LINE__);\n", rio_V(A, 'u'), rio_V(A + 1, 'u'), rio_V(A + 2, 'u'), rio_V(B, 'u'),
              rio_elemSize(a, data), in->op == rio_Op_lswap);
    break;
  case rio_Op_lview:
    rio_print(a, "{ uint32_t d_ = %s, n_ = rio_builderLen(%s, %s); %s = d_; %s = n_; }\n", rio_V(B + 1, 'u'), rio_V(B, 'u'), rio_V(B + 2, 'u'),
              rio_V(A, 'u'), rio_V(A + 1, 'u'));
    break;
  case rio_Op_bpack:
    rio_print(a, "{ int32_t at_ = %s; uint32_t f_ = %s, n_ = %s, len_ = %s; if (at_ < 0 || n_ > len_ || (uint32_t)at_ > len_ - n_) rio_RT_ERROR(\"%s out of bounds\"); ",
              rio_V(B, 'i'), rio_V(C, 'u'), rio_V(C + 1, 'u'), rio_V(A + 1, 'u'), in->x ? "unpack" : "pack");
    if (in->x) rio_print(a, "memmove(rio_M + f_, rio_M + %s + (uint32_t)at_, n_); }\n", rio_V(A, 'u'));
    else rio_print(a, "memmove(rio_M + %s + (uint32_t)at_, rio_M + f_, n_); }\n", rio_V(A, 'u'));
    break;
  case rio_Op_bchk:
    rio_print(a, "{ uint32_t at_ = %s, n_ = %s, size_ = %s, i_; for (i_ = 0; size_ && i_ + size_ <= n_; i_ += size_) if (!rio_holdsOk(rio_M, %du, at_ + i_)) rio_RT_ERROR(\"unpacked a value its type doesn't have\"); }\n",
              rio_V(A, 'u'), rio_V(A + 1, 'u'), rio_V(C, 'u'), B);
    break;
  case rio_Op_geti: case rio_Op_seti: { /* (the blob's slots: geti's b, seti's a) */
    int get = in->op == rio_Op_geti, blob = get ? B : A;
    const char *name = get ? "getInt" : "setInt";
    rio_print(a, "{ int32_t at_ = %s, bits_ = %s; uint32_t len_ = %s, n_ = (uint32_t)(bits_ < 0 ? -bits_ : bits_) / 8, i_, v_ = %s; uint8_t *p_; ",
              rio_V(C, 'i'), rio_V(C + 1, 'i'), rio_V(blob + 1, 'u'), get ? "0u" : rio_V(B, 'u'));
    rio_print(a, "if (n_ < 1 || n_ > 4 || bits_ %% 8) rio_RT_ERROR(\"%s takes 8, 16, 24 or 32 bits\"); ", name);
    rio_print(a, "if (at_ < 0 || n_ > len_ || (uint32_t)at_ > len_ - n_) rio_RT_ERROR(\"%s out of bounds\"); p_ = rio_M + %s + (uint32_t)at_; ", name,
              rio_V(blob, 'u'));
    if (get)
      rio_print(a, "for (i_ = 0; i_ < n_; i_++) v_ |= (uint32_t)p_[i_] << (8 * i_); if (bits_ < 0 && n_ < 4 && (v_ >> (8 * n_ - 1) & 1)) v_ |= ~0u << (8 * n_); %s = v_; }\n",
                rio_V(A, 'u'));
    else rio_print(a, "for (i_ = 0; i_ < n_; i_++) p_[i_] = (uint8_t)(v_ >> (8 * i_)); }\n");
    break;
  }
  case rio_Op_pnum:
    rio_print(a, "{ int32_t base_ = %s; uint32_t v_ = 0; int has_; ", in->x ? "0" : rio_V(C, 'i'));
    if (!in->x) rio_print(a, "if (!base_) base_ = 10; if (base_ < 2 || base_ > 36) rio_RT_ERROR(\"toInt's base is 2 to 36\"); ");
    rio_print(a, "has_ = rio_parseNumber(rio_M + %s, %s, base_, &v_); ", rio_V(B, 'u'), rio_V(B + 1, 'u'));
    rio_print(a, "%s = has_; %s = has_ ? v_ : 0u; }\n", rio_V(A, 'i'), rio_V(A + 1, 'u'));
    break;
  case rio_Op_jmp: rio_print(a, "goto L%d;\n", C); break;
  case rio_Op_jz: rio_print(a, "if (!%s) goto L%d;\n", rio_V(A, 'i'), C); break;
  case rio_Op_jnz: rio_print(a, "if (%s) goto L%d;\n", rio_V(A, 'i'), C); break;
  case rio_Op_jeq: case rio_Op_jne: case rio_Op_jlt: case rio_Op_jle:
    rio_print(a, "if (%s %s %s) goto L%d;\n", rio_V(A, 'i'), compare[in->op - rio_Op_jeq], rio_V(B, 'i'), C);
    break;
  case rio_Op_jfeq: case rio_Op_jfne: case rio_Op_jflt: case rio_Op_jfle:
    rio_print(a, "if (%s %s %s) goto L%d;\n", rio_V(A, 'f'), compare[in->op - rio_Op_jfeq], rio_V(B, 'f'), C);
    break;
  case rio_Op_jfnlt: case rio_Op_jfnle:
    rio_print(a, "if (!(%s %s %s)) goto L%d;\n", rio_V(A, 'f'), in->op == rio_Op_jfnlt ? "<" : "<=", rio_V(B, 'f'), C);
    break;
  case rio_Op_each: rio_print(a, "if ((%s += %s) < %s) goto L%d;\n", rio_V(A, 'u'), rio_V(B + 1, 'u'), rio_V(B, 'u'), C); break;
  case rio_Op_fori: rio_print(a, "if ((%s = (int32_t)(%s + 1u)) < %s) goto L%d;\n", rio_V(A, 'i'), rio_V(A, 'u'), rio_V(B, 'i'), C); break;
  case rio_Op_forby:
    rio_print(a, "{ uint32_t v_ = %s, e_ = %s, k_ = %s; if ((int32_t)k_ > 0 ? e_ - v_ >= k_ : v_ - e_ >= 0u - k_) { %s = v_ + k_; goto L%d; } }\n",
              rio_V(A, 'u'), rio_V(B, 'u'), rio_V(B + 1, 'u'), rio_V(A, 'u'), C);
    break;
  case rio_Op_call: rio_print(a, "%s();\n", rio_procName(a, rio_procAtPc(a, C))); break;
  case rio_Op_calli: { /* through a proc value: the tables say where its parameters go */
    int i;
    rio_print(a, "{ uint32_t f_ = %s - 1u; if (f_ >= %du) rio_RT_ERROR(\"this proc value isn't set\"); ", rio_V(A, 'u'), vm->procCount);
    for (i = 0; i < C; i++) rio_print(a, "rio_R[rio_fs[f_] + %d] = %s; ", i, rio_V(B + i, 'v'));
    rio_print(a, "rio_fn[f_](); }\n");
    break;
  }
  case rio_Op_getret: {
    int i;
    rio_print(a, "{ uint32_t f_ = %s - 1u; ", rio_V(B, 'u'));
    for (i = 0; i < C; i++) rio_print(a, "%s = rio_R[rio_ret[f_] + %d]; ", rio_V(A + i, 'v'), i);
    rio_print(a, "}\n");
    break;
  }
  case rio_Op_ret: case rio_Op_halt: rio_print(a, "return;\n"); break;
  case rio_Op_ffi: { /* through a copy of the argument words; the result comes back in the first */
    rio_Ffi *f = &vm->ffi[C];
    int argWords = f->argWords, resultWords = f->resultWords, i;
    rio_print(a, "{ rio_Val a_[%d];", (argWords > resultWords ? argWords : resultWords) + 1);
    for (i = 0; i < argWords; i++) rio_print(a, " a_[%d] = %s;", i, rio_V(A + i, 'v'));
    rio_print(a, " rio_ffi_%s(a_);", rio_ffiName(a, C));
    for (i = 0; i < resultWords; i++) rio_print(a, " %s = a_[%d];", rio_V(A + i, 'v'), i);
    rio_print(a, " }\n");
    break;
  }
  case rio_Op_logi: rio_print(a, "rio_logInt(%d, %s);\n", in->x, rio_V(A, 'i')); break;
  case rio_Op_logf: rio_print(a, "rio_logFloat(%d, %s);\n", in->x, rio_V(A, 'f')); break;
  case rio_Op_logb: rio_print(a, "rio_logText(%d, %s ? \"true\" : \"false\", -1);\n", in->x, rio_V(A, 'i')); break;
  case rio_Op_logs: rio_print(a, "rio_logShown(%d, %s, %s);\n", in->x, rio_V(A, 'u'), rio_V(A + 1, 'u')); break;
  case rio_Op_logn:
    rio_print(a, "{ int32_t v_ = %s; uint32_t i_ = (uint32_t)v_; if (i_ < %uu) rio_logText(%d, (const char *)rio_M + rio_R[%d + 2 * i_].u, rio_R[%d + 2 * i_ + 1].i); else rio_logInt(%d, v_); }\n",
              rio_V(A, 'i'), rio_dataSize(data), in->x, C, C, in->x);
    break;
  case rio_Op_fail:
    if (in->x) rio_print(a, "{ fflush(stdout); fprintf(stderr, \"%%d: runtime error: %%.*s\\n\", __LINE__, %s, (const char *)rio_M + %s); exit(1); }\n",
                         rio_V(A + 1, 'i'), rio_V(A, 'u'));
    else if (A) rio_print(a, "rio_rtAssert((const char *)rio_M + %s, %s, __LINE__);\n", rio_V(A, 'u'), rio_V(A + 1, 'i'));
    else rio_print(a, "rio_rtAssert(\"\", -1, __LINE__);\n");
    break;
  case rio_Op_logv: case rio_Op_pushv: { /* a slice's or builder view's words, else the value's address */
    int d = in->op == rio_Op_logv ? B : (int)rio_dataSize(data), v = in->op == rio_Op_logv ? A : C;
    uint32_t k = rio_constant(a, d).u & 255, words = k == rio_Print_slice ? 2 : k == rio_Print_builderView ? 3 : 1;
    char w[3][48];
    int j;
    for (j = 0; j < 3; j++) strcpy(w[j], j < (int)words ? rio_V(v + j, 'u') : "0u");
    if (in->op == rio_Op_logv) rio_print(a, "rio_logValue(%d, %du, %s, %s, %s);\n", in->x, d, w[0], w[1], w[2]);
    else
      rio_print(a, "%s = rio_pushValue(%s, %s, %s, %s, %du, %s, %s, %s);\n", rio_V(A, 'i'), rio_V(B, 'u'), rio_V(B + 1, 'u'), rio_V(B + 2, 'u'),
                data->a ? rio_V(data->a, 'u') : "0u", d, w[0], w[1], w[2]);
    break;
  }
  case rio_Op_loge: rio_print(a, "rio_logEnd();\n"); break;
  default: rio_print(a, "rio_RT_ERROR(\"bad op\");\n");
  }
}

#undef rio_V

/* ------------------------------------------------------------------ the program */

/* the runtime the generated code needs: memory, errors, log, builders */
static const char *const rio_prelude =
  "#define _CRT_SECURE_NO_WARNINGS\n#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include <math.h>\n#include <time.h>\n"
  "#ifdef __GNUC__\n#pragma GCC diagnostic ignored \"-Wunused\"\n#endif\n"
  "typedef union { int32_t i; float f; uint32_t u; } rio_Val;\n"
  "static union { rio_Val v[rio_MEM_SIZE / 4]; uint8_t b[rio_MEM_SIZE]; } rio_mem;\n"
  "#define rio_M rio_mem.b\n#define rio_R rio_mem.v\n#define rio_WORD(p) (*(rio_Val *)(rio_M + (uint32_t)(p)))\n"
  "static inline float rio_floatBits(uint32_t u) { rio_Val v; v.u = u; return v.f; }\n"
  "static inline void rio_rtError(const char *m, int line) { fflush(stdout); fprintf(stderr, \"%d: runtime error: %s\\n\", line, m); exit(1); }\n"
  "#define rio_RT_ERROR(m) rio_rtError(m, __LINE__) /* #line directives make __LINE__ the rio source line */\n"
  "static void rio_rtAssert(const char *s, int n, int line) { fflush(stdout); fprintf(stderr, \"%d: runtime error: assertion failed%s%.*s\\n\", line, n < 0 ? \"\" : \": \", n < 0 ? 0 : n, s); exit(1); }\n"
  "static inline int32_t rio_floatToInt(float f) { return f != f ? 0 : f >= 2147483648.f ? 0x7FFFFFFF : f <= -2147483648.f ? (int32_t)0x80000000u : (int32_t)f; }\n"
  "static inline float rio_round(float x) { return x < 0 ? -floorf(-x + 0.5f) : floorf(x + 0.5f); }\n"
  "static inline int rio_stringEq(int32_t a, int32_t an, int32_t b, int32_t bn) { return an == bn && !memcmp(rio_M + a, rio_M + b, (size_t)an); }\n"
  "static char rio_logBuf[256]; static int rio_logLen;\n"
  "static inline void rio_logText(int sp, const char *s, int n) { if (n < 0) n = (int)strlen(s); if (sp && rio_logLen < 256) rio_logBuf[rio_logLen++] = ' '; while (n-- > 0 && rio_logLen < 256) rio_logBuf[rio_logLen++] = *s++; }\n"
  "static inline void rio_logInt(int sp, int32_t v) { char b[16]; sprintf(b, \"%d\", (int)v); rio_logText(sp, b, -1); }\n"
  "static inline int rio_formatFloat(char *b, float f) {\n"
  "  int i = 0, e = 0, nd = 1, dec, k; double d = f; uint32_t ip, fp, sc = 1, t;\n"
  "  if (f != f) { memcpy(b, \"nan\", 4); return 3; }\n"
  "  if (d < 0) { b[i++] = '-'; d = -d; }\n"
  "  if (d > 3.5e38) { memcpy(b + i, \"inf\", 4); return i + 3; }\n"
  "  if (d != 0 && (d >= 1e9 || d < 1e-4)) { while (d >= 10) { d /= 10; e++; } while (d < 1) { d *= 10; e--; } }\n"
  "  ip = (uint32_t)d; for (t = ip; t >= 10; t /= 10) nd++;\n"
  "  dec = 7 - nd; if (dec < 1) dec = 1; if (dec > 6) dec = 6; for (k = 0; k < dec; k++) sc *= 10;\n"
  "  fp = (uint32_t)((d - ip) * sc + 0.5); if (fp >= sc) { ip++; fp -= sc; if (e && ip == 10) { ip = 1; e++; } }\n"
  "  i += sprintf(b + i, \"%u.\", (unsigned)ip);\n"
  "  for (k = dec - 1; k >= 0; k--) { b[i + k] = (char)('0' + fp % 10); fp /= 10; }\n"
  "  while (dec > 1 && b[i + dec - 1] == '0') dec--; i += dec;\n"
  "  if (e) i += sprintf(b + i, \"e%d\", e);\n"
  "  b[i] = 0; return i;\n"
  "}\n"
  "static inline void rio_logFloat(int sp, float f) { char b[40]; rio_logText(sp, b, rio_formatFloat(b, f)); }\n"
  "static inline void rio_logEnd(void) { fwrite(rio_logBuf, 1, (size_t)rio_logLen, stdout); fputc('\\n', stdout); rio_logLen = 0; }\n"
  /* builders: a length word followed by the elements; views are (address, capacity) */
  "static inline uint32_t rio_builderLen(uint32_t a, uint32_t cap) { uint32_t n = rio_WORD(a).u; return n < cap ? n : cap; }\n"
  "static inline int32_t rio_listPush(uint32_t a, uint32_t d, uint32_t cap, const void *p, uint32_t k, uint32_t sz) {\n"
  "  uint32_t n = rio_builderLen(a, cap); if (k > cap - n) return 0;\n"
  "  memmove(rio_M + d + n * sz, p, k * sz); rio_WORD(a).u = n + k; return 1;\n"
  "}\n"
  "static inline int32_t rio_listPushText(uint32_t a, uint32_t d, uint32_t cap, int kind, rio_Val v) {\n"
  "  char b[40]; int k = kind == 1 ? rio_formatFloat(b, v.f) : kind == 2 ? sprintf(b, \"%s\", v.i ? \"true\" : \"false\") : sprintf(b, \"%d\", (int)v.i);\n"
  "  return rio_listPush(a, d, cap, b, (uint32_t)k, 1);\n"
  "}\n"
  /* values by their descriptors (print.h): the same printer the VM compiles */
  rio_PRINT_STRING(rio_PRINT_KINDS) "\n" rio_PRINTER(rio_PRINT_TEXT) "\n"
  "static inline void rio_logValue(int sp, uint32_t d, uint32_t w0, uint32_t w1, uint32_t w2) {\n"
  "  rio_Out o; if (sp && rio_logLen < 256) rio_logBuf[rio_logLen++] = ' ';\n"
  "  o.out = (uint8_t *)rio_logBuf; o.len = (uint32_t)rio_logLen; o.cap = 256; o.capped = 1; o.full = 0; o.spec = 0;\n"
  "  rio_printValue(rio_M, &o, d, w0, w1, w2); rio_logLen = (int)o.len;\n"
  "}\n"
  "static inline void rio_logShown(int sp, uint32_t at, uint32_t n) {\n"
  "  rio_Out o; if (sp && rio_logLen < 256) rio_logBuf[rio_logLen++] = ' ';\n"
  "  o.out = (uint8_t *)rio_logBuf; o.len = (uint32_t)rio_logLen; o.cap = 256; o.capped = 1; o.full = 0; o.spec = 0;\n"
  "  rio_outShown(&o, rio_M + at, n, 0); rio_logLen = (int)o.len;\n"
  "}\n"
  "static inline int32_t rio_pushValue(uint32_t a, uint32_t dd, uint32_t cap, uint32_t spec, uint32_t d, uint32_t w0, uint32_t w1, uint32_t w2) {\n"
  "  rio_Out o; o.out = rio_M + dd; o.len = rio_builderLen(a, cap); o.cap = cap; o.capped = 0; o.full = 0; o.spec = spec;\n"
  "  rio_printValue(rio_M, &o, d, w0, w1, w2); if (o.full) return 0; rio_WORD(a).u = o.len; return 1;\n"
  "}\n"
  "static inline int32_t rio_pushFormatted(uint32_t a, uint32_t dd, uint32_t cap, uint32_t spec, int kind, uint32_t w0, uint32_t w1) {\n"
  "  rio_Out o; o.out = rio_M + dd; o.len = rio_builderLen(a, cap); o.cap = cap; o.capped = 0; o.full = 0; o.spec = 0;\n"
  "  rio_outFormatted(rio_M, &o, spec, kind, w0, w1); if (o.full) return 0; rio_WORD(a).u = o.len; return 1;\n"
  "}\n"
  "static inline void rio_listRemove(uint32_t a, uint32_t d, uint32_t cap, uint32_t i, uint32_t sz, int swap, int line) {\n"
  "  uint32_t n = rio_builderLen(a, cap); if (i >= n) rio_rtError(\"index out of bounds\", line);\n"
  "  if (swap) { if (i != --n) memmove(rio_M + d + i * sz, rio_M + d + n * sz, sz); rio_WORD(a).u = n; }\n"
  "  else { memmove(rio_M + d + i * sz, rio_M + d + (i + 1) * sz, (n - i - 1) * sz); rio_WORD(a).u = n - 1; }\n"
  "}\n";

/* constants, and the nonzero bytes above top (strings and tables), as the program's start */
static void rio_writeMemory(rio_Aot *a) {
  rio_Vm *vm = a->vm;
  uint8_t *mem = vm->mem;
  int i;
  fprintf(a->out, "static const uint32_t rio_k[%u] = {", vm->constCap); /* (and the starting data after them) */
  for (i = 0; i < (int)vm->constCap; i++) fprintf(a->out, "%s%uu", i ? (i % 12 ? ", " : ",\n  ") : "\n  ", ((rio_Val *)mem)[i].u);
  fprintf(a->out, "\n};\nstatic void rio_load(void) {\n  memcpy(rio_R, rio_k, sizeof rio_k);\n");
  for (i = (int)vm->top; i < (int)vm->memSize; i++) { /* runs of bytes, broken at 8 zeros */
    int end = i, zeros = 0, start = i;
    if (!mem[i]) continue;
    while (end < (int)vm->memSize && zeros < 8) {
      zeros = mem[end] ? 0 : zeros + 1;
      end++;
    }
    end -= zeros;
    fprintf(a->out, "  memcpy(rio_M + %d, \"", i);
    for (; i < end; i++) fprintf(a->out, "\\%03o", mem[i]);
    fprintf(a->out, "\", %d);\n", end - start);
    i--;
  }
  fprintf(a->out, "}\n");
}

/* a proc as a C function: a dry run first, to learn which locals the body uses */
static void rio_writeProc(rio_Aot *a, FILE *out) {
  rio_Proc *p = &a->vm->procs[a->proc];
  int pc, s;
  memset(a->used, 0, sizeof a->used);
  a->out = 0;
  a->lastLine = 0;
  for (pc = p->entry; pc < p->end; pc++) rio_instruction(a, pc);
  a->out = out;
  a->lastLine = 0;
  fprintf(out, "\nstatic void %s(void) {\n", rio_procName(a, a->proc));
  for (s = p->frameStart; s < p->frameEnd; s++) {
    if (!rio_isLocal(a, s) || !rio_bitGet(a->used, (uint32_t)s)) continue;
    if (s < p->paramsEnd) fprintf(out, "  rio_Val r%d = rio_R[%d];\n", s, s); /* parameters come in through memory */
    else fprintf(out, "  rio_Val r%d = {0};\n", s);
  }
  for (pc = p->entry; pc < p->end; pc++) rio_instruction(a, pc);
  fprintf(out, "}\n");
}

int rio_aot(rio_Vm *vm, FILE *out, const char *hostFfi) {
  static rio_Aot aot; /* (big tables: not on the stack) */
  rio_Aot *a = &aot;
  int pc, i, mainProc;
  if (!vm->ready || !vm->compiler) return -1; /* needs the compiler tables: compile with rio_compileScratch */
  a->vm = vm;
  a->out = out;
  a->proc = -1;
  memset(a->isLabel, 0, sizeof a->isLabel);
  for (pc = 0; pc < (int)vm->codeLen; pc++) {
    int op = vm->code[pc].op;
    if (rio_isJump(op)) a->isLabel[vm->code[pc].c] = 1;
    if (op == rio_Op_idx || op == rio_Op_enumck || op == rio_Op_enumname || op == rio_Op_logn || op == rio_Op_pushv || op == rio_Op_pushf || (op >= rio_Op_vadd && op <= rio_Op_fvdiv) ||
        op == rio_Op_slice || op == rio_Op_ldx || op == rio_Op_ldxb || op == rio_Op_stx || op == rio_Op_stxb)
      pc++; /* (its data word) */
  }
  fprintf(out, "/* generated by rio -c */\n#define rio_MEM_SIZE %uu\n%s", vm->memSize, rio_prelude);
  rio_writeMemory(a);
  for (i = 0; i < vm->ffiCount; i++) fprintf(out, "void rio_ffi_%s(rio_Val *a);\n", rio_ffiName(a, i));
  if (hostFfi) fprintf(out, "#ifndef rio_NO_HOST_FFI\n%s#endif\n", hostFfi);
  /* procs: defined in order, and direct calls only go backwards; calls through proc values use a table
     of every proc, its parameter slots and its result slot */
  if (vm->procCount) {
    for (i = 0; i < vm->procCount; i++) fprintf(out, "static void %s(void);\n", rio_procName(a, i));
    fprintf(out, "static void (*const rio_fn[%d])(void) = {", vm->procCount);
    for (i = 0; i < vm->procCount; i++) fprintf(out, "%s%s", i ? ", " : "", rio_procName(a, i));
    fprintf(out, "};\nstatic const uint32_t rio_fs[%d] = {", vm->procCount);
    for (i = 0; i < vm->procCount; i++) fprintf(out, "%s%u", i ? ", " : "", vm->procs[i].frameStart);
    fprintf(out, "};\nstatic const uint32_t rio_ret[%d] = {", vm->procCount);
    for (i = 0; i < vm->procCount; i++) fprintf(out, "%s%u", i ? ", " : "", vm->procs[i].result);
    fprintf(out, "};\n");
  }
  for (a->proc = 0; a->proc < vm->procCount; a->proc++) rio_writeProc(a, out);
  a->proc = -1; /* top-level code: everything outside the procs */
  a->lastLine = 0;
  fprintf(out, "\nstatic void rio_top(void) {\n");
  for (pc = 0; pc < (int)vm->codeLen; pc++) {
    for (i = 0; i < vm->procCount; i++)
      if (pc == vm->procs[i].entry) {
        pc = vm->procs[i].end;
        break;
      }
    if (pc < (int)vm->codeLen) rio_instruction(a, pc);
  }
  fprintf(out, "}\n\nint main(void) {\n  rio_load();\n  rio_top();\n");
  if ((mainProc = rio_findProc(vm, "main")) >= 0) fprintf(out, "  %s();\n", rio_procName(a, mainProc));
  fprintf(out, "  return 0;\n}\n");
  return ferror(out) ? -1 : 0;
}
