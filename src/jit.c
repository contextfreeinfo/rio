/* jit.c - compile simple loops to Thumb-2 machine code (Cortex-M33, and ARMv7 in Thumb mode), ahead of
   running: after compiling or loading, once. A loop qualifies when everything inside it is integer math,
   compares and branches; its first instruction becomes OP_JIT, which calls the native code. That code
   keeps the slots in memory (r0 holds their base, so nothing changes for the interpreter) and returns
   the bytecode position to carry on from, where the loop exits; or, when an instruction fails (a
   division by zero, an index out of bounds), which failure and where, for the vm to report as it would
   have. Each backward branch checks for rio_interrupt.
   Procs whose bodies qualify (and don't touch memory) are compiled whole, first, so loops and other procs can call them
   natively (no recursion: each has one frame, so a call is just a branch). Called from bytecode, a
   proc's first instruction becomes OP_JIT too, and its code returns the position of one of its RETs.
   Around a call, slots held in registers go back to memory and are loaded again after. Log and most
   math go to rio_jit_op in C; sqrt, abs (and floor and ceil, on ARMv8) are instructions.
   Elsewhere rio_jit compiles nothing. */
#include <stddef.h>
#include <string.h>
#include "rio.h"

#if defined(__ARM_ARCH_ISA_THUMB) && __ARM_ARCH_ISA_THUMB >= 2 && !defined(RIO_NO_JIT)

#define JOPENUM(o) J_##o,
enum { RIO_OPS(JOPENUM) J_NOPS };
enum { EQ = 0, NE = 1, GE = 10, LT = 11, GT = 12, LE = 13 };
#define R2 2
#define R3 3
#define R12 12

typedef struct {
  Rio *vm; uint32_t orig;                  /* orig: where code needs the address of a copy of the first instruction (OP_JIT takes its place), or 0 */
  uint16_t *b; uint32_t n, cap; int bad;   /* output, in halfwords */
  int held[16];                            /* register -> slot it holds, or -1 */
  const RioVal *R; uint32_t kcap;          /* constant slots: below kcap, values known */
} Gen;
/* where the native code finds things in Rio (r1 holds the vm) */
#define OFF_BRK ((uint32_t)offsetof(Rio, brk))
#define OFF_TRAP ((uint32_t)offsetof(Rio, trap))
#define OFF_FN(i) ((uint32_t)(offsetof(Rio, ffi) + (i) * sizeof(RioFfi) + offsetof(RioFfi, fn)))

static void h(Gen *g, uint32_t x) { if (g->n < g->cap) g->b[g->n] = (uint16_t)x; else g->bad = 1; g->n++; }
static void h2(Gen *g, uint32_t a, uint32_t b) { h(g, a); h(g, b); }
static void movw(Gen *g, int rd, uint32_t v) { h2(g, 0xF240 | ((v >> 1) & 0x400) | ((v >> 12) & 15), ((v << 4) & 0x7000) | (uint32_t)rd << 8 | (v & 0xFF)); }
static void movt(Gen *g, int rd, uint32_t v) { h2(g, 0xF2C0 | ((v >> 1) & 0x400) | ((v >> 12) & 15), ((v << 4) & 0x7000) | (uint32_t)rd << 8 | (v & 0xFF)); }
static void imm(Gen *g, int rd, uint32_t v) { movw(g, rd, v & 0xFFFF); if (v >> 16) movt(g, rd, v >> 16); }
/* rt = slot s (or its constant value) */
static void ld(Gen *g, int rt, uint32_t s) {
  if (s < g->kcap) { imm(g, rt, g->R[s].u); g->held[rt] = -1; return; }
  if (g->held[rt] == (int)s) return;
  if (s < 1024) h2(g, 0xF8D0, (uint32_t)rt << 12 | s * 4);              /* ldr.w rt, [r0, #s*4] */
  else { movw(g, rt, s); h2(g, 0xF850, (uint32_t)rt << 12 | 0x20 | (uint32_t)rt); } /* ldr.w rt, [r0, rt, lsl #2] (r12 untouched) */
  g->held[rt] = (int)s;
}
static void st(Gen *g, int rt, uint32_t s) {
  int r;
  if (s < 1024) h2(g, 0xF8C0, (uint32_t)rt << 12 | s * 4);
  else { movw(g, R12, s); h2(g, 0xF840, (uint32_t)rt << 12 | 0x20 | R12); }
  for (r = 0; r < 16; r++) if (g->held[r] == (int)s) g->held[r] = -1;
  g->held[rt] = (int)s;
}
static void forget(Gen *g) { int r; for (r = 0; r < 16; r++) g->held[r] = -1; }
static void dp(Gen *g, uint32_t op, int rd, int rn, int rm) { h2(g, op | (uint32_t)rn, (uint32_t)rd << 8 | (uint32_t)rm); } /* add.w and friends */
static void cmp(Gen *g, int rn, int rm) { h2(g, 0xEBB0 | (uint32_t)rn, 0x0F00 | (uint32_t)rm); }
static void cmp0(Gen *g, int rn) { h2(g, 0xF1B0 | (uint32_t)rn, 0x0F00); }
/* branches are patched once targets are known: record where */
typedef struct { uint32_t at; int cond; int to; } Fix; /* to >= 0: a bytecode pc */
static void bra(Gen *g, int cond, Fix *f, int *nf, int maxf, int to) { /* cond < 0: always */
  if (*nf >= maxf) { g->bad = 1; return; }
  f[*nf].at = g->n; f[*nf].cond = cond; f[*nf].to = to; (*nf)++;
  h2(g, 0, 0);
}
static int patch(Gen *g, uint32_t at, int cond, uint32_t target) {
  int32_t off = ((int32_t)target - (int32_t)(at + 2)) * 2; uint32_t S = off < 0;
  if (at + 1 >= g->cap) return 0;
  if (cond >= 0) {
    if (off < -(1 << 20) || off >= (1 << 20)) return -1;
    g->b[at] = (uint16_t)(0xF000 | S << 10 | (uint32_t)cond << 6 | ((uint32_t)off >> 12 & 0x3F));
    g->b[at + 1] = (uint16_t)(0x8000 | ((uint32_t)off >> 18 & 1) << 13 | ((uint32_t)off >> 19 & 1) << 11 | ((uint32_t)off >> 1 & 0x7FF));
  } else {
    uint32_t i1 = (uint32_t)off >> 23 & 1, i2 = (uint32_t)off >> 22 & 1;
    if (off < -(1 << 24) || off >= (1 << 24)) return -1;
    g->b[at] = (uint16_t)(0xF000 | S << 10 | ((uint32_t)off >> 12 & 0x3FF));
    g->b[at + 1] = (uint16_t)(0x9000 | ((!i1) ^ S) << 13 | ((!i2) ^ S) << 11 | ((uint32_t)off >> 1 & 0x7FF));
  }
  return 0;
}

/* instructions with a data word after them (element size and offset) */
static int wide(int op) { return op == J_LDX || op == J_LDXB || op == J_STX || op == J_STXB || op == J_IDX; }
#define MAXLOOPS 256 /* native loops and procs */
static int datawords(int op) { return wide(op) || op == J_LOGN; }
static int islog(int op) { return op >= J_LOGI && op <= J_LOGE; }
/* math done in place: sqrt, abs, and on ARMv8 (which has vrint) floor and ceil */
#if __ARM_ARCH >= 8
#define RINT 1
#else
#define RINT 0
#endif
static int fastm1(int x) { return x == 0 || x == 12 || (RINT && (x == 9 || x == 10)); }
/* the proc starting at pc, if it's native code: its entry; else 0 */
static uintptr_t native(Gen *g, uint32_t pc) {
  Rio *vm = g->vm; const RioIns *x = &vm->code[pc]; uint16_t (*done)[2];
  if (x->op != J_JIT) return 0;
  done = (uint16_t (*)[2])(void *)((uint8_t *)vm->jitbuf + MAXLOOPS * sizeof(RioJitFn));
  return done[x->a][0] == pc && vm->code[done[x->a][1]].op == J_RET ? (uintptr_t)vm->jit[x->a] : 0;
}
static int memop(int op) { /* (an ffi function may write through the pointers it's given) */
  return wide(op) || op == J_LDW || op == J_LDB || op == J_STW || op == J_STB || op == J_LDW2 || op == J_STW2 || op == J_FFI;
}
static int supported(int op) {
  switch (op) {
  case J_MOV: case J_ADD: case J_SUB: case J_MUL: case J_DIV: case J_MOD: case J_AND: case J_OR: case J_XOR:
  case J_SHL: case J_SHR: case J_NEG: case J_BNOT: case J_NOT: case J_EQ: case J_NE: case J_LT: case J_LE:
  case J_IABS: case J_IMIN: case J_IMAX: case J_LEA: case J_MOV2:
  case J_FADD: case J_FSUB: case J_FMUL: case J_FDIV: case J_FNEG: case J_FEQ: case J_FNE: case J_FLT: case J_FLE:
  case J_ITOF: case J_FTOI: case J_FMIN: case J_FMAX:
  case J_JMP: case J_JZ: case J_JNZ: case J_JLT: case J_JLE: case J_JEQ: case J_JNE: case J_FORI: case J_EACH:
  case J_JFEQ: case J_JFNE: case J_JFLT: case J_JFLE: case J_JFNLT: case J_JFNLE:
  case J_M1: case J_M2: case J_CALL: case J_RET: return 1;
  }
  return memop(op) || islog(op);
}
static int isjump(int op) {
  return op == J_JMP || op == J_JZ || op == J_JNZ || op == J_JLT || op == J_JLE || op == J_JEQ || op == J_JNE || op == J_FORI ||
         op == J_EACH || op == J_JFEQ || op == J_JFNE || op == J_JFLT || op == J_JFLE || op == J_JFNLT || op == J_JFNLE;
}
/* are [s, e] all instructions this compiles (data words skipped)? does it touch memory? */
static int compilable(Gen *g, const RioIns *code, uint32_t s, uint32_t e, int *mem) {
  uint32_t pc;
  *mem = 0;
  for (pc = s; pc <= e; pc++) {
    int op = code[pc].op;
    if (!supported(op)) return 0;
    if (memop(op)) *mem = 1;
    if (op == J_CALL && !native(g, code[pc].c)) return 0; /* (calls only to native procs) */
    if ((op == J_LDW || op == J_LDB || op == J_STW || op == J_STB || op == J_LDW2 || op == J_STW2) && code[pc].c >= 4088) return 0; /* (offsets this big: left to the vm) */
    if (datawords(op)) { if (pc + 1 > e || (wide(op) && code[pc + 1].a >= 4096)) return 0; pc++; }
    if (op == J_FFI && OFF_FN(code[pc].c) >= 4096) return 0;
  }
  return pc == e + 1;
}

#define MAXFIX 128 /* branches in one loop */
#define MAXLEN 1024 /* longest loop compiled, in instructions */
#define NREG 8      /* r4-r11 hold the loop's busiest slots and constants */
#define R14 14      /* lr: saved on entry, so one more scratch register */
/* returned with an instruction's position when it fails: the vm reports it (see OP_JIT) */
#define ERR_DIV (1u << 16)
#define ERR_INDEX (2u << 16)
#define ERR_TRAP (3u << 16) /* an ffi function called rio_trap */
#define ERR_INTR (4u << 16) /* rio_interrupt, in a proc */
#define PASS 0x7FFFFFFF /* (an exit that returns r12: a called proc's failure) */
enum { MI = 4, PL = 5, HS = 2, LO = 3, HI = 8, LS = 9 };
static uint32_t aslot[NREG]; static int nalloc;
static int areg(uint32_t s) { int i; for (i = 0; i < nalloc; i++) if (aslot[i] == s) return 4 + i; return -1; }
/* the register holding slot s: its own, or scratch loaded with it */
static int src(Gen *g, uint32_t s, int scratch) { int r = areg(s); if (r >= 0) return r; ld(g, scratch, s); return scratch; }
static int dst(uint32_t s) { int r = areg(s); return r >= 0 ? r : R2; }
static void mov(Gen *g, int rd, int rm) { if (rd != rm) h2(g, 0xEA4F, (uint32_t)rd << 8 | (uint32_t)rm); }
/* the result for slot s is in r: into s's register, or memory */
static void fin(Gen *g, uint32_t s, int r) {
  int a = areg(s);
  if (a >= 0) mov(g, a, r); else { if (r == R2 || r == R3 || r == R14) g->held[r] = -1; st(g, r, s); }
}
static void it(Gen *g, int cc) { h(g, 0xBF08 | (uint32_t)cc << 4); } /* it <cc>, for the one instruction after */
static void movi(Gen *g, int rd, uint32_t v) { h2(g, 0xF04F, (uint32_t)rd << 8 | v); } /* mov.w rd, #v (v < 256) */
/* rd = rn + v (v < 4096; else through r12) */
static void addi(Gen *g, int rd, int rn, uint32_t v) {
  if (v < 4096) h2(g, 0xF200 | (v >> 11 & 1) << 10 | (uint32_t)rn, (v >> 8 & 7) << 12 | (uint32_t)rd << 8 | (v & 0xFF)); /* addw */
  else { imm(g, R12, v); dp(g, 0xEB00, rd, rn, R12); }
}
/* memory: r12 = M + base + i * size (r12 may hold base or i); then ldr/str rt, [r12, #off] */
static void elem(Gen *g, int rb, int ri, uint32_t size) {
  uint32_t k = 0;
  while (k < 31 && (1u << k) < size) k++;
  if ((1u << k) == size) h2(g, 0xEB00 | (uint32_t)rb, (k >> 2) << 12 | R12 << 8 | (k & 3) << 6 | (uint32_t)ri); /* add.w r12, rb, ri, lsl #k */
  else { imm(g, R14, size); h2(g, 0xFB00 | R14, 0xF000 | R12 << 8 | (uint32_t)ri); dp(g, 0xEB00, R12, R12, rb); g->held[R14] = -1; }
  dp(g, 0xEB00, R12, R12, 0); /* + M (r0) */
  g->held[R12] = -1;
}
/* ldr / ldrb / str / strb rt, [r12, #off] (off < 4096: compilable checks) */
static void mem(Gen *g, uint32_t op, int rt, uint32_t off) { h2(g, op | R12, (uint32_t)rt << 12 | off); }
#define LDR 0xF8D0
#define LDRB 0xF890
#define STR 0xF8C0
#define STRB 0xF880
/* the VFP: s0 and s1 (free to use), through core registers */
static void s_from(Gen *g, int sn, int r) { h2(g, 0xEE00 | (uint32_t)(sn >> 1), (uint32_t)r << 12 | 0x0A10 | (uint32_t)(sn & 1) << 7); } /* vmov sn, r */
static void s_to(Gen *g, int r, int sn) { h2(g, 0xEE10 | (uint32_t)(sn >> 1), (uint32_t)r << 12 | 0x0A10 | (uint32_t)(sn & 1) << 7); }   /* vmov r, sn */
static void fcmp01(Gen *g) { h2(g, 0xEEB4, 0x0A60); h2(g, 0xEEF1, 0xFA10); } /* vcmp.f32 s0, s1; vmrs APSR_nzcv, fpscr */
/* which slots an instruction reads or writes (bit 0: a, 1: b, 2: c), for choosing registers */
static int uses(int op) {
  switch (op) {
  case J_JMP: case J_CALL: case J_RET: return 0;
  case J_M1: return 3;
  case J_JZ: case J_JNZ: return 1;
  case J_JLT: case J_JLE: case J_JEQ: case J_JNE: case J_FORI: case J_JFEQ: case J_JFNE: case J_JFLT: case J_JFLE: case J_JFNLT: case J_JFNLE: return 3;
  case J_MOV: case J_NEG: case J_BNOT: case J_NOT: case J_IABS: case J_FNEG: case J_ITOF: case J_FTOI: case J_LEA: case J_EACH: return 3;
  case J_LDW: case J_LDB: return 3;
  case J_STW: case J_STB: return 3;
  case J_MOV2: case J_LDW2: case J_STW2: return 0; /* (pairs: kept in memory) */
  }
  if (islog(op)) return 0;
  return 7;
}
/* mem: the loop touches memory, which may be a slot: then only constants get registers */
static void allocate(const RioIns *code, uint32_t s, uint32_t e, uint32_t kcap, int mem) {
  static uint32_t sl[64], cnt[64]; uint32_t n = 0, pc, i, k, best; /* (slots past the 64th aren't counted) */
  for (pc = s; pc <= e; pc++) {
    uint32_t v[3]; int u = uses(code[pc].op), j;
    v[0] = code[pc].a; v[1] = code[pc].b; v[2] = code[pc].c;
    for (j = 0; j < 3; j++) {
      if (!(u >> j & 1) || (mem && v[j] >= kcap)) continue;
      for (i = 0; i < n && sl[i] != v[j]; i++) {}
      if (i == n) { if (n == 64) continue; sl[n] = v[j]; cnt[n++] = 0; }
      cnt[i] += v[j] < kcap ? 2 : 4; /* (a variable saves a load and a store; a constant, a move or two, loaded once before the loop) */
    }
    if (datawords(code[pc].op)) pc++;
  }
  for (nalloc = 0; nalloc < NREG; nalloc++) {
    for (k = n, best = 0, i = 0; i < n; i++) if (cnt[i] > best) { best = cnt[i]; k = i; }
    if (k == n || best < 2) break;
    aslot[nalloc] = sl[k]; cnt[k] = 0;
  }
}
/* the slots registers hold, to memory and back, around code that reads or writes them there */
static void spill(Gen *g) { int k; for (k = 0; k < nalloc; k++) if (aslot[k] >= g->kcap) st(g, 4 + k, aslot[k]); }
static void reload(Gen *g) { int k; forget(g); for (k = 0; k < nalloc; k++) if (aslot[k] >= g->kcap) ld(g, 4 + k, aslot[k]); forget(g); }
static void spill1(Gen *g, uint32_t s) { int r = areg(s); if (r >= 0 && s >= g->kcap) st(g, r, s); }
/* rio_jit_op(slots, vm, the instruction at pc), with r0 and r1 kept */
static void helper(Gen *g, const RioIns *code, uint32_t pc, uint32_t s) {
  h(g, 0xB403);
  if (pc == s) { g->orig = g->n; movw(g, R2, 0); movt(g, R2, 0); } /* (filled in once the copy has a place) */
  else imm(g, R2, (uint32_t)(uintptr_t)(code + pc));
  imm(g, R12, (uint32_t)(uintptr_t)rio_jit_op); h(g, 0x47E0); h(g, 0xBC03); forget(g); /* push {r0, r1}; blx r12; pop */
}
/* native code for bytecode [s, e] (proc: a whole proc's body); returns 0, or -1 if it doesn't fit */
static int region(Gen *g, const RioIns *code, uint32_t s, uint32_t e, int proc) {
  /* static, not on the stack: an MCU's main stack can be just 2 KB (the RP2350's default) */
  static uint16_t lab[MAXLEN]; static uint32_t expc[64], exat[64]; static uint8_t tgt[MAXLEN]; static Fix f[MAXFIX]; /* (lab: halfwords, up to 128 KB of code) */
  int nf = 0, i, memory; uint32_t pc, nx = 0, head = s;
  if (e - s + 1 > MAXLEN || !compilable(g, code, s, e, &memory)) return -1;
  memset(tgt, 0, e - s + 1);
  for (pc = s; pc <= e; pc++) {
    if (isjump(code[pc].op) && code[pc].c >= s && code[pc].c <= e) tgt[code[pc].c - s] = 1;
    if (datawords(code[pc].op)) pc++;
  }
  allocate(code, s, e, g->kcap, memory); g->orig = 0;
  h2(g, 0xE92D, 0x4FF8); /* push.w {r3-r11, lr} (r3 too: the stack stays 8-byte aligned for calls) */
  forget(g);
  for (i = 0; i < nalloc; i++) ld(g, 4 + i, aslot[i]);
  forget(g);
  for (pc = s; pc <= e; pc++) {
    const RioIns *x = &code[pc]; uint32_t A = x->a, B = x->b, C = x->c; int rb, rc, rd, rv;
    if (tgt[pc - s]) { forget(g); if (g->n & 1) h(g, 0xBF00); } /* a jump target: 4-byte aligned (nop), so it's fetched at once */
    lab[pc - s] = g->n;
    switch (x->op) {
    case J_MOV: rb = src(g, B, R2); fin(g, A, rb); break;
    case J_ADD: case J_SUB: case J_AND: case J_OR: case J_XOR: {
      uint32_t op = x->op == J_ADD ? 0xEB00 : x->op == J_SUB ? 0xEBA0 : x->op == J_AND ? 0xEA00 : x->op == J_OR ? 0xEA40 : 0xEA80;
      rb = src(g, B, R2); rc = src(g, C, R3); rd = dst(A); dp(g, op, rd, rb, rc); fin(g, A, rd); break;
    }
    case J_MUL: rb = src(g, B, R2); rc = src(g, C, R3); rd = dst(A); h2(g, 0xFB00 | (uint32_t)rb, 0xF000 | (uint32_t)rd << 8 | (uint32_t)rc); fin(g, A, rd); break;
    case J_DIV: case J_MOD: /* a zero divisor: the interpreter runs this one and reports it */
      rc = src(g, C, R3); cmp0(g, rc); bra(g, EQ, f, &nf, MAXFIX, (int)(pc | ERR_DIV)); rb = src(g, B, R2); rd = dst(A);
      h2(g, 0xFB90 | (uint32_t)rb, 0xF0F0 | R12 << 8 | (uint32_t)rc);    /* sdiv r12, rb, rc (-1 and INT_MIN wrap, like the vm) */
      if (x->op == J_MOD) h2(g, 0xFB00 | R12, (uint32_t)rb << 12 | (uint32_t)rd << 8 | 0x10 | (uint32_t)rc); /* mls rd, r12, rc, rb */
      else mov(g, rd, R12);
      fin(g, A, rd); break;
    case J_SHL: case J_SHR:
      rb = src(g, B, R2); rc = src(g, C, R3); h2(g, 0xF000 | (uint32_t)rc, R3 << 8 | 0x1F); g->held[R3] = -1; /* and.w r3, rc, #31 */
      rd = dst(A); h2(g, (x->op == J_SHL ? 0xFA00 : 0xFA40) | (uint32_t)rb, 0xF000 | (uint32_t)rd << 8 | R3); fin(g, A, rd); break;
    case J_NEG: rb = src(g, B, R2); rd = dst(A); h2(g, 0xF1C0 | (uint32_t)rb, (uint32_t)rd << 8); fin(g, A, rd); break; /* rsb.w rd, rb, #0 */
    case J_BNOT: rb = src(g, B, R2); rd = dst(A); h2(g, 0xEA6F, (uint32_t)rd << 8 | (uint32_t)rb); fin(g, A, rd); break; /* mvn.w */
    case J_IABS: rb = src(g, B, R2); rd = dst(A); cmp0(g, rb); mov(g, rd, rb); it(g, LT); h2(g, 0xF1C0 | (uint32_t)rd, (uint32_t)rd << 8); fin(g, A, rd); break;
    case J_IMIN: case J_IMAX: /* in r2: rc is never r2 */
      rb = src(g, B, R2); rc = src(g, C, R3); cmp(g, rb, rc); mov(g, R2, rb); it(g, x->op == J_IMIN ? GT : LT); h2(g, 0xEA4F, R2 << 8 | (uint32_t)rc);
      fin(g, A, R2); break;
    case J_NOT: rb = src(g, B, R2); rd = dst(A); cmp0(g, rb); movi(g, rd, 0); it(g, EQ); movi(g, rd, 1); fin(g, A, rd); break;
    case J_EQ: case J_NE: case J_LT: case J_LE: {
      int cc = x->op == J_EQ ? EQ : x->op == J_NE ? NE : x->op == J_LT ? LT : LE;
      rb = src(g, B, R2); rc = src(g, C, R3); rd = dst(A); cmp(g, rb, rc); movi(g, rd, 0); it(g, cc); movi(g, rd, 1); fin(g, A, rd); break;
    }
    case J_LEA: rb = src(g, B, R2); rd = dst(A); addi(g, rd, rb, C); fin(g, A, rd); break;
    case J_FFI: /* fn(vm, slots + a), then a trap is an error here; r0 and r1 kept around the call */
      h(g, 0xB403); mov(g, R2, 1); h2(g, 0xF8D0 | R2, R12 << 12 | OFF_FN(C)); /* push {r0, r1}; r2 = vm; r12 = vm->ffi[c].fn */
      if (A * 4 < 4096) addi(g, 1, 0, A * 4); else { imm(g, R3, A * 4); dp(g, 0xEB00, 1, 0, R3); }
      mov(g, 0, R2); h(g, 0x47E0); h(g, 0xBC03); forget(g); /* blx r12; pop {r0, r1} (measured: keeping the slots in memory beats spilling registers around each call, for pset loops) */
      h2(g, 0xF8D1, R12 << 12 | OFF_TRAP); cmp0(g, R12); bra(g, NE, f, &nf, MAXFIX, (int)(pc | ERR_TRAP));
      break;
    case J_CALL: /* a native proc: it returns where it returned, or a failure (kind << 16 | pc) to pass on */
      spill(g); h(g, 0xB403); imm(g, R12, (uint32_t)native(g, C)); h(g, 0x47E0); mov(g, R12, 0); h(g, 0xBC03); /* push {r0, r1}; blx; r12 = r0; pop */
      reload(g); h2(g, 0xF5B0 | R12, 0x3F80); bra(g, HS, f, &nf, MAXFIX, (int)PASS); /* cmp.w r12, #0x10000; bhs */
      break;
    case J_RET: bra(g, -1, f, &nf, MAXFIX, (int)pc); break; /* (out to it: the vm returns, or a native caller carries on) */
    case J_M1:
      if (fastm1(x->x)) {
        rb = src(g, B, R2); s_from(g, 0, rb);
        if (x->x == 0) h2(g, 0xEEB1, 0x0AC0); else if (x->x == 12) h2(g, 0xEEB0, 0x0AC0); /* vsqrt / vabs */
        else h2(g, x->x == 9 ? 0xFEBB : 0xFEBA, 0x0A40); /* vrintm / vrintp: floor / ceil */
        rd = dst(A); s_to(g, rd, 0); if (rd == R2) g->held[R2] = -1; fin(g, A, rd); break;
      }
      /* fall through */
    case J_M2: /* the operands to memory, the call, and the result back */
      spill1(g, B); if (x->op == J_M2) spill1(g, C); helper(g, code, pc, s);
      if (areg(A) >= 0) ld(g, areg(A), A); forget(g); break;
    case J_LOGI: case J_LOGF: case J_LOGS: case J_LOGB: case J_LOGN: case J_LOGE:
      spill(g); helper(g, code, pc, s);
      if (x->op == J_LOGN) { pc++; lab[pc - s] = g->n; }
      break;
    case J_MOV2: /* both read before either is written: they may overlap */
      rb = src(g, B, R2); mov(g, R2, rb); rc = src(g, B + 1, R3); mov(g, R3, rc); g->held[R2] = g->held[R3] = -1;
      fin(g, A, R2); fin(g, A + 1, R3); break;
    /* memory (bytes of M, whose start is r0): a[b + c] and the like */
    case J_LDW: case J_LDB:
      rb = src(g, B, R2); dp(g, 0xEB00, R12, rb, 0); g->held[R12] = -1; rd = dst(A); /* add.w r12, rb, r0 */
      mem(g, x->op == J_LDW ? LDR : LDRB, rd, C); if (rd == R2) g->held[R2] = -1; fin(g, A, rd); break;
    case J_STW: case J_STB:
      rv = src(g, B, R14); rb = src(g, A, R2); dp(g, 0xEB00, R12, rb, 0); g->held[R12] = -1;
      mem(g, x->op == J_STW ? STR : STRB, rv, C); forget(g); break;
    case J_LDW2:
      rb = src(g, B, R2); dp(g, 0xEB00, R12, rb, 0); g->held[R12] = -1;
      mem(g, LDR, R2, C); mem(g, LDR, R3, C + 4); g->held[R2] = g->held[R3] = -1; fin(g, A, R2); fin(g, A + 1, R3); break;
    case J_STW2:
      ld(g, R14, B); ld(g, R3, B + 1); rb = src(g, A, R2); dp(g, 0xEB00, R12, rb, 0); g->held[R12] = -1;
      mem(g, STR, R14, C); mem(g, STR, R3, C + 4); forget(g); break;
    case J_LDX: case J_LDXB: case J_IDX: { /* a[i]: an index out of bounds is the interpreter's to report */
      uint32_t size = (uint32_t)x[1].b | (uint32_t)x[1].c << 16, off = x[1].a; int ri = src(g, C, R3), rl = src(g, B + 1, R2);
      cmp(g, ri, rl); bra(g, HS, f, &nf, MAXFIX, (int)(pc | ERR_INDEX));
      rb = src(g, B, R2);
      if (x->op == J_IDX) { /* the element's address, not its value */
        elem(g, rb, ri, size); h2(g, 0xEBB0 | R12, R2 << 8 | 0); /* (r2 = r12 - r0: subs.w r2, r12, r0) */
        g->held[R2] = -1; fin(g, A, R2);
      } else {
        elem(g, rb, ri, size); rd = dst(A); mem(g, x->op == J_LDX ? LDR : LDRB, rd, off); if (rd == R2) g->held[R2] = -1; fin(g, A, rd);
      }
      pc++; lab[pc - s] = g->n; break;
    }
    case J_STX: case J_STXB: {
      uint32_t size = (uint32_t)x[1].b | (uint32_t)x[1].c << 16, off = x[1].a; int ri, rl;
      ri = src(g, B, R3); rl = src(g, A + 1, R2);
      cmp(g, ri, rl); bra(g, HS, f, &nf, MAXFIX, (int)(pc | ERR_INDEX));
      rb = src(g, A, R2); elem(g, rb, ri, size); g->held[R14] = -1; rv = src(g, C, R14); /* (loads don't touch r12) */
      mem(g, x->op == J_STX ? STR : STRB, rv, off); forget(g);
      pc++; lab[pc - s] = g->n; break;
    }
    /* floats, in s0 and s1 */
    case J_FADD: case J_FSUB: case J_FMUL: case J_FDIV: {
      uint32_t op = x->op == J_FMUL ? 0xEE20 : x->op == J_FDIV ? 0xEE80 : 0xEE30;
      rb = src(g, B, R2); rc = src(g, C, R3); s_from(g, 0, rb); s_from(g, 1, rc);
      h2(g, op, 0x0A20 | (x->op == J_FSUB ? 0x40 : 0)); /* vop.f32 s0, s0, s1 */
      rd = dst(A); s_to(g, rd, 0); if (rd == R2) g->held[R2] = -1; fin(g, A, rd); break;
    }
    case J_FNEG: case J_ITOF: case J_FTOI:
      rb = src(g, B, R2); s_from(g, 0, rb);
      if (x->op == J_FNEG) h2(g, 0xEEB1, 0x0A40); else if (x->op == J_ITOF) h2(g, 0xEEB8, 0x0AC0); else h2(g, 0xEEBD, 0x0AC0); /* vneg / vcvt.f32.s32 / vcvt.s32.f32 (toward zero; saturates; NaN: 0, like ftoi) */
      rd = dst(A); s_to(g, rd, 0); if (rd == R2) g->held[R2] = -1; fin(g, A, rd); break;
    case J_FEQ: case J_FNE: case J_FLT: case J_FLE: { /* NaN compares unordered: false, except != */
      int cc = x->op == J_FEQ ? EQ : x->op == J_FNE ? NE : x->op == J_FLT ? MI : LS;
      rb = src(g, B, R2); rc = src(g, C, R3); s_from(g, 0, rb); s_from(g, 1, rc); fcmp01(g);
      rd = dst(A); movi(g, rd, 0); it(g, cc); movi(g, rd, 1); if (rd == R2) g->held[R2] = -1; fin(g, A, rd); break;
    }
    case J_FMIN: case J_FMAX: /* b < c ? b : c, and b > c ? b : c (NaN: c) */
      rb = src(g, B, R2); rc = src(g, C, R3); s_from(g, 0, rb); s_from(g, 1, rc); fcmp01(g);
      mov(g, R14, rc); it(g, x->op == J_FMIN ? MI : GT); h2(g, 0xEA4F, R14 << 8 | (uint32_t)rb); g->held[R14] = -1; fin(g, A, R14); break;
    case J_JMP: case J_JZ: case J_JNZ: case J_JLT: case J_JLE: case J_JEQ: case J_JNE: case J_FORI: case J_EACH:
    case J_JFEQ: case J_JFNE: case J_JFLT: case J_JFLE: case J_JFNLT: case J_JFNLE: {
      int cc = -1, inside = C >= s && C <= e, back = inside && C <= pc, ra = 0, rbb = 0, kind = 0; /* kind: 1 cmp0, 2 cmp, 3 vcmp */
      if (x->op == J_JZ || x->op == J_JNZ) { ra = src(g, A, R2); kind = 1; cc = x->op == J_JZ ? EQ : NE; }
      else if (x->op == J_FORI || x->op == J_EACH) { /* fori: a += 1 while a < b; each: a += b[1] while a < b (unsigned) */
        ra = src(g, A, R2); rd = dst(A);
        if (x->op == J_FORI) h2(g, 0xF100 | (uint32_t)ra, (uint32_t)rd << 8 | 1); /* add.w rd, ra, #1 */
        else { rc = src(g, B + 1, R3); dp(g, 0xEB00, rd, ra, rc); g->held[R3] = -1; }
        if (rd == R2) g->held[R2] = -1;
        fin(g, A, rd); ra = areg(A) >= 0 ? areg(A) : R2;
        rbb = src(g, B, R3); kind = 2; cc = x->op == J_FORI ? LT : LO;
      } else if (x->op >= J_JFEQ && x->op <= J_JFNLE) {
        ra = src(g, A, R2); rbb = src(g, B, R3); kind = 3;
        cc = x->op == J_JFEQ ? EQ : x->op == J_JFNE ? NE : x->op == J_JFLT ? MI : x->op == J_JFLE ? LS : x->op == J_JFNLT ? PL : HI;
      } else if (x->op != J_JMP) { ra = src(g, A, R2); rbb = src(g, B, R3); kind = 2; cc = x->op == J_JLT ? LT : x->op == J_JLE ? LE : x->op == J_JEQ ? EQ : NE; }
      if (back) { /* check for an interrupt on the way back (in a proc: a failure, as there's no going back in) */
        h2(g, 0xF8D1, R12 << 12 | OFF_BRK); cmp0(g, R12); bra(g, NE, f, &nf, MAXFIX, proc ? (int)(pc | ERR_INTR) : (int)head); /* ldr.w r12, [r1, #brk]; cmp; bne */
        g->held[R12] = -1;
      }
      if (kind == 1) cmp0(g, ra); else if (kind == 2) cmp(g, ra, rbb); else if (kind == 3) { s_from(g, 0, ra); s_from(g, 1, rbb); fcmp01(g); }
      bra(g, cc, f, &nf, MAXFIX, inside ? (int)(C | 0x80000000u) : (int)C);
      break;
    }
    default: return -1;
    }
  }
  bra(g, -1, f, &nf, MAXFIX, (int)(e + 1)); /* falling off the end */
  /* jumps inside go to their labels; exits to a stub per bytecode position: the registers go back
     to their slots, and it returns the position */
  for (i = 0; i < nf && !g->bad; i++) {
    uint32_t to = (uint32_t)f[i].to, j, at;
    if (to & 0x80000000u) at = lab[(to & 0x7FFFFFFFu) - s];
    else {
      for (j = 0; j < nx && expc[j] != to; j++) {}
      if (j == nx) {
        int k;
        if (nx >= 64) return -1;
        expc[nx] = to; exat[nx++] = g->n;
        for (k = 0; k < nalloc; k++) if (aslot[k] >= g->kcap) st(g, 4 + k, aslot[k]);
        if (to == PASS) mov(g, 0, R12); else imm(g, 0, to);
        h2(g, 0xE8BD, 0x8FF8); /* r0 = to; pop.w {r3-r11, pc} */
      }
      at = exat[j];
    }
    if (!g->bad && patch(g, f[i].at, f[i].cond, at)) return -1;
  }
  if (g->orig && !g->bad) { /* the copy of the first instruction (and its data word), after the code */
    uint32_t at = g->orig, n, a; RioIns two[2];
    if (g->n & 1) h(g, 0xBF00);
    a = (uint32_t)(uintptr_t)(g->b + g->n); two[0] = code[s]; two[1] = code[s + 1];
    for (n = 0; n < sizeof two / 2; n++) h(g, ((const uint16_t *)(const void *)two)[n]);
    n = g->n; g->n = at; movw(g, R2, a & 0xFFFF); movt(g, R2, a >> 16); g->n = n;
  }
  return g->bad ? -1 : 0;
}

/* the buffer: a table of MAXLOOPS entry points, then each one's bytecode range, then the code */
static int jitnow(Rio *vm) {
  RioIns *code = vm->code; uint32_t pc, cap, k, s, e;
  uint8_t *base = (uint8_t *)vm->jitbuf; RioJitFn *tab = (RioJitFn *)(void *)base;
  uint16_t (*done)[2] = (uint16_t (*)[2])(void *)(base + MAXLOOPS * sizeof(RioJitFn)); Gen g; int count = 0;
  uint32_t head = MAXLOOPS * (uint32_t)sizeof(RioJitFn) + MAXLOOPS * 4;
  g.b = (uint16_t *)(void *)(base + head); g.cap = (vm->jitsize - head) / 2; g.n = vm->jitused;
  if (g.cap > 65535) g.cap = 65535; /* (labels are 16-bit) */
  g.R = (const RioVal *)(const void *)vm->mem; g.kcap = vm->kcap; g.vm = vm;
  vm->jit = tab;
  /* procs, callees first (they're defined before use, but a pass may leave one for the next) */
  for (;;) {
    int more = 0, i;
    for (i = 0; i < vm->nfunc && vm->njit < MAXLOOPS; i++) {
      int m; s = vm->func[i].pc; e = (uint32_t)vm->func[i].end - 1u;
      if (vm->func[i].end <= s || code[e].op != J_RET || e - s + 1 > MAXLEN) continue;
      for (k = 0; k < vm->njit && !(s <= done[k][1] && e >= done[k][0]); k++) {}
      if (k < vm->njit || !compilable(&g, code, s, e, &m) || m) continue; /* (memory ops would keep all its slots out of registers: its loops do better alone) */
      cap = g.n; g.bad = 0;
      if (region(&g, code, s, e, 1)) { g.n = cap; continue; }
      tab[vm->njit] = (RioJitFn)(uintptr_t)((uintptr_t)(g.b + cap) | 1);
      done[vm->njit][0] = (uint16_t)s; done[vm->njit][1] = (uint16_t)e;
      code[s].op = J_JIT; code[s].a = (uint16_t)vm->njit++; count++; more = 1;
    }
    if (!more) break;
  }
  /* biggest loops first: a loop inside one already compiled is part of it */
  while (vm->njit < MAXLOOPS) {
    uint32_t bs = 0, be = 0, best = 0;
    for (pc = 0; pc < vm->pc; pc++) {
      if (!isjump(code[pc].op) || code[pc].c > pc) continue;
      s = code[pc].c; e = pc;
      if (e - s + 1 <= best || e - s + 1 > MAXLEN) continue;
      for (k = 0; k < vm->njit && !(s >= done[k][0] && e <= done[k][1]); k++) {}
      if (k < vm->njit) continue;
      { int m; if (!compilable(&g, code, s, e, &m)) continue; }
      bs = s; be = e; best = e - s + 1;
    }
    if (!best) break;
    cap = g.n; g.bad = 0;
    if (region(&g, code, bs, be, 0)) { g.n = cap; break; } /* out of room: the rest stays bytecode */
    tab[vm->njit] = (RioJitFn)(uintptr_t)((uintptr_t)(g.b + cap) | 1);
    done[vm->njit][0] = (uint16_t)bs; done[vm->njit][1] = (uint16_t)be;
    code[bs].op = J_JIT; code[bs].a = (uint16_t)vm->njit++; count++;
  }
  if (g.n > vm->jitused) {
#if defined(__linux__) || defined(__APPLE__)
    __builtin___clear_cache((char *)(g.b + vm->jitused), (char *)(g.b + g.n));
#else
    __asm volatile("dsb 0xF\n isb 0xF" ::: "memory");
#endif
    vm->jitused = g.n;
  }
  return count;
}
int rio_jit(Rio *vm, void *buf, uint32_t size) {
  uint8_t *b = (uint8_t *)(((uintptr_t)buf + 7) & ~(uintptr_t)7); uint32_t pad = (uint32_t)(b - (uint8_t *)buf);
  if (!buf || size < pad + MAXLOOPS * (sizeof(RioJitFn) + 4) + 256 || OFF_BRK >= 4096 || OFF_TRAP >= 4096) return -1;
  vm->jitbuf = b; vm->jitsize = size - pad; vm->jitused = 0; vm->njit = 0; vm->jitter = jitnow;
  return vm->ok && !vm->repl ? jitnow(vm) : 0;
}
#else
int rio_jit(Rio *vm, void *buf, uint32_t size) { (void)vm; (void)buf; (void)size; return -1; }
#endif
