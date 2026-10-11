/* jit.c - compile simple loops to Thumb-2 machine code (Cortex-M33, and ARMv7 in Thumb mode), ahead of
   running: after compiling or loading, once. A loop qualifies when everything inside it is integer math,
   compares and branches; its first instruction becomes OP_JIT, which calls the native code. That code
   keeps the slots in memory (r0 holds their base, so nothing changes for the interpreter) and returns
   the bytecode position to carry on from: where the loop exits, or the instruction it can't finish
   (dividing by zero), which the interpreter then runs itself and reports as usual. Each backward branch
   checks for rio_interrupt. Elsewhere rio_jit compiles nothing. */
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
  uint16_t *b; uint32_t n, cap; int bad;   /* output, in halfwords */
  int held[16];                            /* register -> slot it holds, or -1 */
  const RioVal *R; uint32_t kcap;          /* constant slots: below kcap, values known */
} Gen;

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
  else { movw(g, R12, s); h2(g, 0xF850, (uint32_t)rt << 12 | 0x20 | R12); } /* ldr.w rt, [r0, r12, lsl #2] */
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

static int supported(int op) {
  switch (op) {
  case J_MOV: case J_ADD: case J_SUB: case J_MUL: case J_DIV: case J_MOD: case J_AND: case J_OR: case J_XOR:
  case J_SHL: case J_SHR: case J_NEG: case J_BNOT: case J_NOT: case J_EQ: case J_NE: case J_LT: case J_LE:
  case J_IABS: case J_IMIN: case J_IMAX:
  case J_JMP: case J_JZ: case J_JNZ: case J_JLT: case J_JLE: case J_JEQ: case J_JNE: case J_FORI: return 1;
  }
  return 0;
}
static int isjump(int op) { return op == J_JMP || op == J_JZ || op == J_JNZ || op == J_JLT || op == J_JLE || op == J_JEQ || op == J_JNE || op == J_FORI; }

#define MAXFIX 256 /* branches in one loop */
#define MAXLEN 1024 /* longest loop compiled, in instructions */
/* native code for bytecode [s, e]; returns 0, or -1 if it doesn't fit */
static int region(Gen *g, const RioIns *code, uint32_t s, uint32_t e) {
  /* static, not on the stack: an MCU's main stack can be just 2 KB (the RP2350's default) */
  static uint32_t lab[MAXLEN], expc[64], exat[64]; static uint8_t tgt[MAXLEN]; static Fix f[MAXFIX];
  int nf = 0, i; uint32_t pc, nx = 0, head = s;
  if (e - s + 1 > MAXLEN) return -1;
  memset(tgt, 0, e - s + 1);
  for (pc = s; pc <= e; pc++) if (isjump(code[pc].op) && code[pc].c >= s && code[pc].c <= e) tgt[code[pc].c - s] = 1;
  forget(g);
  for (pc = s; pc <= e; pc++) {
    const RioIns *x = &code[pc]; uint32_t A = x->a, B = x->b, C = x->c;
    lab[pc - s] = g->n;
    if (tgt[pc - s]) forget(g);
    switch (x->op) {
    case J_MOV: ld(g, R2, B); st(g, R2, A); break;
    case J_ADD: ld(g, R2, B); ld(g, R3, C); dp(g, 0xEB00, R2, R2, R3); st(g, R2, A); break;
    case J_SUB: ld(g, R2, B); ld(g, R3, C); dp(g, 0xEBA0, R2, R2, R3); st(g, R2, A); break;
    case J_AND: ld(g, R2, B); ld(g, R3, C); dp(g, 0xEA00, R2, R2, R3); st(g, R2, A); break;
    case J_OR: ld(g, R2, B); ld(g, R3, C); dp(g, 0xEA40, R2, R2, R3); st(g, R2, A); break;
    case J_XOR: ld(g, R2, B); ld(g, R3, C); dp(g, 0xEA80, R2, R2, R3); st(g, R2, A); break;
    case J_MUL: ld(g, R2, B); ld(g, R3, C); h2(g, 0xFB00 | R2, 0xF000 | R2 << 8 | R3); st(g, R2, A); break;
    case J_DIV: case J_MOD: /* a zero divisor: the interpreter runs this one and reports it */
      ld(g, R3, C); cmp0(g, R3); bra(g, EQ, f, &nf, MAXFIX, (int)pc); ld(g, R2, B);
      h2(g, 0xFB90 | R2, 0xF0F0 | R12 << 8 | R3);                         /* sdiv r12, r2, r3 (-1 and INT_MIN wrap, like the vm) */
      if (x->op == J_MOD) h2(g, 0xFB00 | R12, R2 << 12 | R2 << 8 | 0x10 | R3); /* mls r2, r12, r3, r2 */
      else h2(g, 0xEA4F, R2 << 8 | R12);                                  /* mov.w r2, r12 */
      g->held[R2] = -1; st(g, R2, A); break;
    case J_SHL: case J_SHR:
      ld(g, R2, B); ld(g, R3, C); h2(g, 0xF000 | R3, R3 << 8 | 0x1F); g->held[R3] = -1; /* and.w r3, r3, #31 */
      h2(g, (x->op == J_SHL ? 0xFA00 : 0xFA40) | R2, 0xF000 | R2 << 8 | R3); st(g, R2, A); break;
    case J_NEG: ld(g, R2, B); h2(g, 0xF1C0 | R2, R2 << 8); st(g, R2, A); break;      /* rsb.w r2, r2, #0 */
    case J_BNOT: ld(g, R2, B); h2(g, 0xEA6F, R2 << 8 | R2); st(g, R2, A); break;     /* mvn.w r2, r2 */
    case J_IABS: ld(g, R2, B); cmp0(g, R2); h(g, 0xBF00 | LT << 4 | 8); h(g, 0x4252); g->held[R2] = -1; st(g, R2, A); break; /* it lt; rsblt r2, r2, #0 */
    case J_IMIN: case J_IMAX:
      ld(g, R2, B); ld(g, R3, C); cmp(g, R2, R3); h(g, 0xBF00 | (x->op == J_IMIN ? GT : LT) << 4 | 8); h(g, 0x461A); /* it; mov r2, r3 */
      g->held[R2] = -1; st(g, R2, A); break;
    case J_NOT: ld(g, R2, B); cmp0(g, R2); h2(g, 0xF04F, R2 << 8); h(g, 0xBF00 | EQ << 4 | 8); h(g, 0x2201); /* mov.w r2, #0; it eq; mov r2, #1 */
      g->held[R2] = -1; st(g, R2, A); break;
    case J_EQ: case J_NE: case J_LT: case J_LE: {
      int cc = x->op == J_EQ ? EQ : x->op == J_NE ? NE : x->op == J_LT ? LT : LE;
      ld(g, R2, B); ld(g, R3, C); cmp(g, R2, R3); h2(g, 0xF04F, R2 << 8); h(g, 0xBF00 | (uint32_t)cc << 4 | 8); h(g, 0x2201);
      g->held[R2] = -1; st(g, R2, A); break;
    }
    case J_JMP: case J_JZ: case J_JNZ: case J_JLT: case J_JLE: case J_JEQ: case J_JNE: case J_FORI: {
      int cc = -1, inside = C >= s && C <= e, back = inside && C <= pc;
      if (x->op == J_JZ || x->op == J_JNZ) { ld(g, R2, A); cmp0(g, R2); cc = x->op == J_JZ ? EQ : NE; }
      else if (x->op == J_FORI) { ld(g, R2, A); h2(g, 0xF100 | R2, R2 << 8 | 1); g->held[R2] = -1; st(g, R2, A); ld(g, R3, B); cmp(g, R2, R3); cc = LT; }
      else if (x->op != J_JMP) { ld(g, R2, A); ld(g, R3, B); cmp(g, R2, R3); cc = x->op == J_JLT ? LT : x->op == J_JLE ? LE : x->op == J_JEQ ? EQ : NE; }
      if (!back) { bra(g, cc, f, &nf, MAXFIX, inside ? (int)(C | 0x80000000u) : (int)C); break; }
      { /* backward: skip unless taken, check for an interrupt, then jump */
        uint32_t skip = g->n;
        if (cc >= 0) h2(g, 0, 0);
        h2(g, 0xF8D1, R12 << 12);              /* ldr.w r12, [r1] */
        cmp0(g, R12); bra(g, NE, f, &nf, MAXFIX, (int)head);
        bra(g, -1, f, &nf, MAXFIX, (int)(C | 0x80000000u));
        if (cc >= 0 && patch(g, skip, cc ^ 1, g->n)) return -1;
      }
      break;
    }
    default: return -1;
    }
  }
  bra(g, -1, f, &nf, MAXFIX, (int)(e + 1)); /* falling off the end */
  /* jumps inside go to their labels; exits to a stub per bytecode position, returning it */
  for (i = 0; i < nf && !g->bad; i++) {
    uint32_t to = (uint32_t)f[i].to, j, at;
    if (to & 0x80000000u) at = lab[(to & 0x7FFFFFFFu) - s];
    else {
      for (j = 0; j < nx && expc[j] != to; j++) {}
      if (j == nx) {
        if (nx >= 64) return -1;
        expc[nx] = to; exat[nx++] = g->n;
        movw(g, 0, to); h(g, 0x4770); /* movw r0, #to; bx lr */
      }
      at = exat[j];
    }
    if (!g->bad && patch(g, f[i].at, f[i].cond, at)) return -1;
  }
  return g->bad ? -1 : 0;
}

#define MAXLOOPS 256
/* the buffer: a table of MAXLOOPS entry points, then each one's bytecode range, then the code */
static int jitnow(Rio *vm) {
  RioIns *code = vm->code; uint32_t pc, cap, k, s, e;
  uint8_t *base = (uint8_t *)vm->jitbuf; RioJitFn *tab = (RioJitFn *)(void *)base;
  uint16_t (*done)[2] = (uint16_t (*)[2])(void *)(base + MAXLOOPS * sizeof(RioJitFn)); Gen g; int count = 0;
  uint32_t head = MAXLOOPS * (uint32_t)sizeof(RioJitFn) + MAXLOOPS * 4;
  g.b = (uint16_t *)(void *)(base + head); g.cap = (vm->jitsize - head) / 2; g.n = vm->jitused;
  g.R = (const RioVal *)(const void *)vm->mem; g.kcap = vm->kcap;
  vm->jit = tab;
  /* biggest loops first: a loop inside one already compiled is part of it */
  while (vm->njit < MAXLOOPS) {
    uint32_t bs = 0, be = 0, best = 0;
    for (pc = 0; pc < vm->pc; pc++) {
      if (!isjump(code[pc].op) || code[pc].c > pc) continue;
      s = code[pc].c; e = pc;
      if (e - s + 1 <= best || e - s + 1 > MAXLEN) continue;
      for (k = 0; k < vm->njit && !(s >= done[k][0] && e <= done[k][1]); k++) {}
      if (k < vm->njit) continue;
      for (k = s; k <= e && supported(code[k].op); k++) {}
      if (k <= e) continue;
      bs = s; be = e; best = e - s + 1;
    }
    if (!best) break;
    cap = g.n; g.bad = 0;
    if (region(&g, code, bs, be)) { g.n = cap; break; } /* out of room: the rest stays bytecode */
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
  if (!buf || size < pad + MAXLOOPS * (sizeof(RioJitFn) + 4) + 256) return -1;
  vm->jitbuf = b; vm->jitsize = size - pad; vm->jitused = 0; vm->njit = 0; vm->jitter = jitnow;
  return vm->ok && !vm->repl ? jitnow(vm) : 0;
}
#else
int rio_jit(Rio *vm, void *buf, uint32_t size) { (void)vm; (void)buf; (void)size; return -1; }
#endif
