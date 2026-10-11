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
#define NREG 8      /* r4-r11 hold the loop's busiest slots and constants */
static uint32_t aslot[NREG]; static int nalloc;
static int areg(uint32_t s) { int i; for (i = 0; i < nalloc; i++) if (aslot[i] == s) return 4 + i; return -1; }
/* the register holding slot s: its own, or scratch loaded with it */
static int src(Gen *g, uint32_t s, int scratch) { int r = areg(s); if (r >= 0) return r; ld(g, scratch, s); return scratch; }
static int dst(uint32_t s) { int r = areg(s); return r >= 0 ? r : R2; }
static void mov(Gen *g, int rd, int rm) { if (rd != rm) h2(g, 0xEA4F, (uint32_t)rd << 8 | (uint32_t)rm); }
/* the result for slot s is in r: into s's register, or memory */
static void fin(Gen *g, uint32_t s, int r) {
  int a = areg(s);
  if (a >= 0) mov(g, a, r); else { if (r == R2 || r == R3) g->held[r] = -1; st(g, r, s); }
}
static void it(Gen *g, int cc) { h(g, 0xBF08 | (uint32_t)cc << 4); } /* it <cc>, for the one instruction after */
static void movi(Gen *g, int rd, uint32_t v) { h2(g, 0xF04F, (uint32_t)rd << 8 | v); } /* mov.w rd, #v (v < 256) */
/* which slots an instruction reads or writes (bit 0: a, 1: b, 2: c) */
static int uses(int op) {
  switch (op) {
  case J_JMP: return 0;
  case J_JZ: case J_JNZ: return 1;
  case J_JLT: case J_JLE: case J_JEQ: case J_JNE: case J_FORI: return 3;
  case J_MOV: case J_NEG: case J_BNOT: case J_NOT: case J_IABS: return 3;
  }
  return 7;
}
static void allocate(const RioIns *code, uint32_t s, uint32_t e, uint32_t kcap) {
  static uint32_t sl[64], cnt[64]; uint32_t n = 0, pc, i, k, best; /* (slots past the 64th aren't counted) */
  for (pc = s; pc <= e; pc++) {
    uint32_t v[3]; int u = uses(code[pc].op), j;
    v[0] = code[pc].a; v[1] = code[pc].b; v[2] = code[pc].c;
    for (j = 0; j < 3; j++) {
      if (!(u >> j & 1)) continue;
      for (i = 0; i < n && sl[i] != v[j]; i++) {}
      if (i == n) { if (n == 64) continue; sl[n] = v[j]; cnt[n++] = 0; }
      cnt[i] += v[j] < kcap ? 2 : 4; /* (a variable saves a load and a store; a constant, a move or two, loaded once before the loop) */
    }
  }
  for (nalloc = 0; nalloc < NREG; nalloc++) {
    for (k = n, best = 0, i = 0; i < n; i++) if (cnt[i] > best) { best = cnt[i]; k = i; }
    if (k == n || best < 2) break;
    aslot[nalloc] = sl[k]; cnt[k] = 0;
  }
}
/* native code for bytecode [s, e]; returns 0, or -1 if it doesn't fit */
static int region(Gen *g, const RioIns *code, uint32_t s, uint32_t e) {
  /* static, not on the stack: an MCU's main stack can be just 2 KB (the RP2350's default) */
  static uint32_t lab[MAXLEN], expc[64], exat[64]; static uint8_t tgt[MAXLEN]; static Fix f[MAXFIX];
  int nf = 0, i; uint32_t pc, nx = 0, head = s;
  if (e - s + 1 > MAXLEN) return -1;
  memset(tgt, 0, e - s + 1);
  for (pc = s; pc <= e; pc++) if (isjump(code[pc].op) && code[pc].c >= s && code[pc].c <= e) tgt[code[pc].c - s] = 1;
  allocate(code, s, e, g->kcap);
  h2(g, 0xE92D, 0x4FF0); /* push.w {r4-r11, lr} */
  forget(g);
  for (i = 0; i < nalloc; i++) ld(g, 4 + i, aslot[i]);
  forget(g);
  for (pc = s; pc <= e; pc++) {
    const RioIns *x = &code[pc]; uint32_t A = x->a, B = x->b, C = x->c; int rb, rc, rd;
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
      rc = src(g, C, R3); cmp0(g, rc); bra(g, EQ, f, &nf, MAXFIX, (int)pc); rb = src(g, B, R2); rd = dst(A);
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
    case J_JMP: case J_JZ: case J_JNZ: case J_JLT: case J_JLE: case J_JEQ: case J_JNE: case J_FORI: {
      int cc = -1, inside = C >= s && C <= e, back = inside && C <= pc, ra = 0, rbb = 0, z = 0;
      if (x->op == J_JZ || x->op == J_JNZ) { ra = src(g, A, R2); z = 1; cc = x->op == J_JZ ? EQ : NE; }
      else if (x->op == J_FORI) {
        ra = src(g, A, R2); rd = dst(A); h2(g, 0xF100 | (uint32_t)ra, (uint32_t)rd << 8 | 1); fin(g, A, rd); ra = areg(A) >= 0 ? areg(A) : R2; /* add.w rd, ra, #1 */
        rbb = src(g, B, R3); cc = LT;
      } else if (x->op != J_JMP) { ra = src(g, A, R2); rbb = src(g, B, R3); cc = x->op == J_JLT ? LT : x->op == J_JLE ? LE : x->op == J_JEQ ? EQ : NE; }
      if (back) { /* check for an interrupt on the way back */
        h2(g, 0xF8D1, R12 << 12); cmp0(g, R12); bra(g, NE, f, &nf, MAXFIX, (int)head); /* ldr.w r12, [r1]; cmp; bne */
      }
      if (cc >= 0) { if (z) cmp0(g, ra); else cmp(g, ra, rbb); }
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
        movw(g, 0, to); h2(g, 0xE8BD, 0x8FF0); /* movw r0, #to; pop.w {r4-r11, pc} */
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
