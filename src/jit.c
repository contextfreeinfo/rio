/* jit.c - compiles simple loops to Thumb-2 machine code (Cortex-M33, and ARMv7 in Thumb mode), ahead of
   running: after compiling or loading, once. A loop qualifies when everything inside it is integer or
   float math, compares, branches, memory reads and writes, log, and calls (to ffi functions and native
   procs); its first instruction becomes rio_Op_jit, which calls the native code. That code keeps the
   slots in memory (r0 holds their base, so nothing changes for the interpreter) and returns the bytecode
   position to carry on from, where the loop exits; or, when an instruction fails (a division by zero, an
   index out of bounds), which failure and where, for the vm to report as it would have. Each backward
   branch checks for rio_interrupt.
   Procs whose bodies qualify (and don't touch memory) are compiled whole, first, so loops and other procs
   can call them natively (no recursion: each has one frame, so a call is just a branch). Called from
   bytecode, a proc's first instruction becomes rio_Op_jit too, and its code returns the position of one of
   its rets. Around a call, slots held in registers go back to memory and are loaded again after. Log and
   most math go to rio_jitOp in C; sqrt, abs (and floor and ceil, on ARMv8) are instructions.
   Elsewhere rio_jit compiles nothing. */
#include <stddef.h>
#include "internal.h"

#if defined(__ARM_ARCH_ISA_THUMB) && __ARM_ARCH_ISA_THUMB >= 2 && !defined(rio_NO_JIT)

/* ------------------------------------------------------------------ the output */

/* condition codes */
enum {
  rio_Cond_eq = 0, rio_Cond_ne = 1, rio_Cond_hs = 2, rio_Cond_lo = 3, rio_Cond_mi = 4, rio_Cond_pl = 5,
  rio_Cond_hi = 8, rio_Cond_ls = 9, rio_Cond_ge = 10, rio_Cond_lt = 11, rio_Cond_gt = 12, rio_Cond_le = 13,
  rio_Cond_always = -1
};
/* r0 holds the slots (mem), r1 the vm; r2, r3, r12 and lr are scratch; r4-r11 hold busy slots */
enum { rio_Reg_r0 = 0, rio_Reg_r1 = 1, rio_Reg_r2 = 2, rio_Reg_r3 = 3, rio_Reg_r12 = 12, rio_Reg_lr = 14 };

typedef struct {
  rio_Vm *vm;
  uint32_t copyAt;         /* where code needs the address of a copy of the first instruction (rio_Op_jit
                              takes its place), or 0 */
  uint16_t *out;           /* the code, in halfwords */
  uint32_t len, cap;
  int overflow;
  int held[16];            /* register -> the slot it holds, or -1 */
  const rio_Val *consts;   /* constant slots: below constCap, values known */
  uint32_t constCap;
} rio_Jit;

/* where the native code finds things in rio_Vm (r1 holds the vm) */
#define rio_OFFSET_INTERRUPTED ((uint32_t)offsetof(rio_Vm, interrupted))
#define rio_OFFSET_TRAPPED ((uint32_t)offsetof(rio_Vm, trapped))
#define rio_OFFSET_FFI_FN(i) ((uint32_t)(offsetof(rio_Vm, ffi) + (i) * sizeof(rio_Ffi) + offsetof(rio_Ffi, fn)))

static void rio_put16(rio_Jit *g, uint32_t x) {
  if (g->len < g->cap) g->out[g->len] = (uint16_t)x;
  else g->overflow = 1;
  g->len++;
}

static void rio_put32(rio_Jit *g, uint32_t first, uint32_t second) {
  rio_put16(g, first);
  rio_put16(g, second);
}

static void rio_movw(rio_Jit *g, int rd, uint32_t v) {
  rio_put32(g, 0xF240 | ((v >> 1) & 0x400) | ((v >> 12) & 15), ((v << 4) & 0x7000) | (uint32_t)rd << 8 | (v & 0xFF));
}

static void rio_movt(rio_Jit *g, int rd, uint32_t v) {
  rio_put32(g, 0xF2C0 | ((v >> 1) & 0x400) | ((v >> 12) & 15), ((v << 4) & 0x7000) | (uint32_t)rd << 8 | (v & 0xFF));
}

static void rio_loadImm(rio_Jit *g, int rd, uint32_t v) {
  rio_movw(g, rd, v & 0xFFFF);
  if (v >> 16) rio_movt(g, rd, v >> 16);
}

/* rt = slot s (or its constant value) */
static void rio_loadSlot(rio_Jit *g, int rt, uint32_t s) {
  if (s < g->constCap) {
    rio_loadImm(g, rt, g->consts[s].u);
    g->held[rt] = -1;
    return;
  }
  if (g->held[rt] == (int)s) return;
  if (s < 1024) rio_put32(g, 0xF8D0, (uint32_t)rt << 12 | s * 4); /* ldr.w rt, [r0, #s*4] */
  else {
    rio_movw(g, rt, s);
    rio_put32(g, 0xF850, (uint32_t)rt << 12 | 0x20 | (uint32_t)rt); /* ldr.w rt, [r0, rt, lsl #2] (r12 untouched) */
  }
  g->held[rt] = (int)s;
}

static void rio_storeSlot(rio_Jit *g, int rt, uint32_t s) {
  int r;
  if (s < 1024) rio_put32(g, 0xF8C0, (uint32_t)rt << 12 | s * 4);
  else {
    rio_movw(g, rio_Reg_r12, s);
    rio_put32(g, 0xF840, (uint32_t)rt << 12 | 0x20 | rio_Reg_r12);
  }
  for (r = 0; r < 16; r++)
    if (g->held[r] == (int)s) g->held[r] = -1;
  g->held[rt] = (int)s;
}

static void rio_forgetRegs(rio_Jit *g) {
  int r;
  for (r = 0; r < 16; r++) g->held[r] = -1;
}

/* add.w and friends: rd = rn op rm */
static void rio_dataOp(rio_Jit *g, uint32_t op, int rd, int rn, int rm) { rio_put32(g, op | (uint32_t)rn, (uint32_t)rd << 8 | (uint32_t)rm); }
static void rio_cmp(rio_Jit *g, int rn, int rm) { rio_put32(g, 0xEBB0 | (uint32_t)rn, 0x0F00 | (uint32_t)rm); }
static void rio_cmpZero(rio_Jit *g, int rn) { rio_put32(g, 0xF1B0 | (uint32_t)rn, 0x0F00); }

/* branches are patched once targets are known: each records where it is, and where it goes (to >= 0: a
   bytecode pc; with the high bit, one inside the code being compiled) */
typedef struct {
  uint32_t at;
  int cond, to;
} rio_JitFix;

static void rio_branch(rio_Jit *g, int cond, rio_JitFix *fixes, int *count, int max, int to) {
  if (*count >= max) {
    g->overflow = 1;
    return;
  }
  fixes[*count].at = g->len;
  fixes[*count].cond = cond;
  fixes[*count].to = to;
  (*count)++;
  rio_put32(g, 0, 0);
}

static int rio_patchBranch(rio_Jit *g, uint32_t at, int cond, uint32_t target) {
  int32_t offset = ((int32_t)target - (int32_t)(at + 2)) * 2;
  uint32_t sign = offset < 0;
  if (at + 1 >= g->cap) return 0;
  if (cond >= 0) { /* b<cond>.w */
    if (offset < -(1 << 20) || offset >= (1 << 20)) return -1;
    g->out[at] = (uint16_t)(0xF000 | sign << 10 | (uint32_t)cond << 6 | ((uint32_t)offset >> 12 & 0x3F));
    g->out[at + 1] = (uint16_t)(0x8000 | ((uint32_t)offset >> 18 & 1) << 13 | ((uint32_t)offset >> 19 & 1) << 11 |
                                ((uint32_t)offset >> 1 & 0x7FF));
  } else { /* b.w */
    uint32_t i1 = (uint32_t)offset >> 23 & 1, i2 = (uint32_t)offset >> 22 & 1;
    if (offset < -(1 << 24) || offset >= (1 << 24)) return -1;
    g->out[at] = (uint16_t)(0xF000 | sign << 10 | ((uint32_t)offset >> 12 & 0x3FF));
    g->out[at + 1] = (uint16_t)(0x9000 | ((!i1) ^ sign) << 13 | ((!i2) ^ sign) << 11 | ((uint32_t)offset >> 1 & 0x7FF));
  }
  return 0;
}

/* ------------------------------------------------------------------ what compiles */

/* instructions with a data word after them (element size and offset) */
static int rio_isIndexed(int op) {
  return op == rio_Op_ldx || op == rio_Op_ldxb || op == rio_Op_stx || op == rio_Op_stxb || op == rio_Op_idx;
}
static int rio_hasData(int op) { return rio_isIndexed(op) || op == rio_Op_logn; }
static int rio_isLog(int op) { return op >= rio_Op_logi && op <= rio_Op_loge; }

#define rio_JIT_MAX_NATIVE 256 /* native loops and procs */

/* math done in place: sqrt, abs, and on ARMv8 (which has vrint) floor and ceil */
#if __ARM_ARCH >= 8
#define rio_JIT_HAS_VRINT 1
#else
#define rio_JIT_HAS_VRINT 0
#endif
static int rio_isInlineMath(int which) {
  return which == rio_Math1_sqrt || which == rio_Math1_abs || (rio_JIT_HAS_VRINT && (which == rio_Math1_floor || which == rio_Math1_ceil));
}

/* The buffer: a table of rio_JIT_MAX_NATIVE entry points, then each one's bytecode range [first, last],
   then the code. */
static uint16_t (*rio_nativeRanges(rio_Vm *vm))[2] {
  return (uint16_t(*)[2])(void *)((uint8_t *)vm->jitBuf + rio_JIT_MAX_NATIVE * sizeof(rio_JitFn));
}

/* the proc starting at pc, if it's native code: its entry; else 0 */
static uintptr_t rio_nativeProc(rio_Jit *g, uint32_t pc) {
  rio_Vm *vm = g->vm;
  const rio_Instr *in = &vm->code[pc];
  uint16_t(*ranges)[2];
  if (in->op != rio_Op_jit) return 0;
  ranges = rio_nativeRanges(vm);
  return ranges[in->a][0] == pc && vm->code[ranges[in->a][1]].op == rio_Op_ret ? (uintptr_t)vm->jitFns[in->a] : 0;
}

static int rio_touchesMemory(int op) { /* (an ffi function may write through the pointers it's given) */
  return rio_isIndexed(op) || op == rio_Op_ldw || op == rio_Op_ldb || op == rio_Op_stw || op == rio_Op_stb || op == rio_Op_ldw2 ||
         op == rio_Op_stw2 || op == rio_Op_ffi;
}

static int rio_jitSupports(int op) {
  switch (op) {
  case rio_Op_mov: case rio_Op_add: case rio_Op_sub: case rio_Op_mul: case rio_Op_div: case rio_Op_mod: case rio_Op_and:
  case rio_Op_or: case rio_Op_xor: case rio_Op_shl: case rio_Op_shr: case rio_Op_neg: case rio_Op_bnot: case rio_Op_not:
  case rio_Op_eq: case rio_Op_ne: case rio_Op_lt: case rio_Op_le: case rio_Op_iabs: case rio_Op_imin: case rio_Op_imax:
  case rio_Op_lea: case rio_Op_mov2: case rio_Op_fadd: case rio_Op_fsub: case rio_Op_fmul: case rio_Op_fdiv: case rio_Op_fneg:
  case rio_Op_feq: case rio_Op_fne: case rio_Op_flt: case rio_Op_fle: case rio_Op_itof: case rio_Op_ftoi: case rio_Op_fmin:
  case rio_Op_fmax: case rio_Op_m1: case rio_Op_m2: case rio_Op_call: case rio_Op_ret:
    return 1;
  }
  return rio_isJump(op) || rio_touchesMemory(op) || rio_isLog(op);
}

/* Are [first, last] all instructions this compiles (data words skipped)? *memory: whether any touch
   memory. */
static int rio_jitCompilable(rio_Jit *g, const rio_Instr *code, uint32_t first, uint32_t last, int *memory) {
  uint32_t pc;
  *memory = 0;
  for (pc = first; pc <= last; pc++) {
    int op = code[pc].op;
    if (!rio_jitSupports(op)) return 0;
    if (rio_touchesMemory(op)) *memory = 1;
    if (op == rio_Op_call && !rio_nativeProc(g, code[pc].c)) return 0; /* (calls only to native procs) */
    if ((op == rio_Op_ldw || op == rio_Op_ldb || op == rio_Op_stw || op == rio_Op_stb || op == rio_Op_ldw2 || op == rio_Op_stw2) &&
        code[pc].c >= 4088)
      return 0; /* (offsets this big: left to the vm) */
    if (rio_hasData(op)) {
      if (pc + 1 > last || (rio_isIndexed(op) && code[pc + 1].a >= 4096)) return 0;
      pc++;
    }
    if (op == rio_Op_ffi && rio_OFFSET_FFI_FN(code[pc].c) >= 4096) return 0;
  }
  return pc == last + 1;
}

/* ------------------------------------------------------------------ registers */

#define rio_JIT_MAX_FIXES 128 /* branches in one loop */
#define rio_JIT_MAX_LEN 1024  /* longest loop compiled, in instructions */
#define rio_JIT_REGS 8        /* r4-r11 hold the loop's busiest slots and constants */
#define rio_JIT_PASS 0x7FFFFFFF /* (an exit that returns r12: a called proc's failure) */

/* the slots r4.. hold (static: an MCU's main stack can be just 2 KB, the RP2350's default) */
static uint32_t rio_regSlot[rio_JIT_REGS];
static int rio_regCount;

/* the register allocated to slot s, or -1 */
static int rio_regOf(uint32_t s) {
  int i;
  for (i = 0; i < rio_regCount; i++)
    if (rio_regSlot[i] == s) return 4 + i;
  return -1;
}

/* the register holding slot s: its own, or scratch loaded with it */
static int rio_srcReg(rio_Jit *g, uint32_t s, int scratch) {
  int r = rio_regOf(s);
  if (r >= 0) return r;
  rio_loadSlot(g, scratch, s);
  return scratch;
}

/* where a result for slot s goes first: its own register, or r2 */
static int rio_dstReg(uint32_t s) {
  int r = rio_regOf(s);
  return r >= 0 ? r : rio_Reg_r2;
}

static void rio_movReg(rio_Jit *g, int rd, int rm) {
  if (rd != rm) rio_put32(g, 0xEA4F, (uint32_t)rd << 8 | (uint32_t)rm);
}

/* the result for slot s is in r: into s's register, or memory */
static void rio_finishResult(rio_Jit *g, uint32_t s, int r) {
  int a = rio_regOf(s);
  if (a >= 0) rio_movReg(g, a, r);
  else {
    if (r == rio_Reg_r2 || r == rio_Reg_r3 || r == rio_Reg_lr) g->held[r] = -1;
    rio_storeSlot(g, r, s);
  }
}

static void rio_it(rio_Jit *g, int cond) { rio_put16(g, 0xBF08 | (uint32_t)cond << 4); } /* it <cond>, for the one instruction after */
static void rio_movImm8(rio_Jit *g, int rd, uint32_t v) { rio_put32(g, 0xF04F, (uint32_t)rd << 8 | v); } /* mov.w rd, #v (v < 256) */

/* rd = rn + v (v < 4096; else through r12) */
static void rio_addImm(rio_Jit *g, int rd, int rn, uint32_t v) {
  if (v < 4096) rio_put32(g, 0xF200 | (v >> 11 & 1) << 10 | (uint32_t)rn, (v >> 8 & 7) << 12 | (uint32_t)rd << 8 | (v & 0xFF)); /* addw */
  else {
    rio_loadImm(g, rio_Reg_r12, v);
    rio_dataOp(g, 0xEB00, rd, rn, rio_Reg_r12);
  }
}

/* memory: r12 = M + base + i * size (r12 may hold base or i); then ldr/str rt, [r12, #offset] */
static void rio_elementAddr(rio_Jit *g, int rb, int ri, uint32_t size) {
  uint32_t k = 0;
  while (k < 31 && (1u << k) < size) k++;
  if ((1u << k) == size) /* add.w r12, rb, ri, lsl #k */
    rio_put32(g, 0xEB00 | (uint32_t)rb, (k >> 2) << 12 | rio_Reg_r12 << 8 | (k & 3) << 6 | (uint32_t)ri);
  else {
    rio_loadImm(g, rio_Reg_lr, size);
    rio_put32(g, 0xFB00 | rio_Reg_lr, 0xF000 | rio_Reg_r12 << 8 | (uint32_t)ri); /* mul r12, lr, ri */
    rio_dataOp(g, 0xEB00, rio_Reg_r12, rio_Reg_r12, rb);
    g->held[rio_Reg_lr] = -1;
  }
  rio_dataOp(g, 0xEB00, rio_Reg_r12, rio_Reg_r12, rio_Reg_r0); /* + M (r0) */
  g->held[rio_Reg_r12] = -1;
}

/* ldr / ldrb / str / strb rt, [r12, #offset] (offset < 4096: rio_jitCompilable checks) */
static void rio_memOp(rio_Jit *g, uint32_t op, int rt, uint32_t offset) { rio_put32(g, op | rio_Reg_r12, (uint32_t)rt << 12 | offset); }
#define rio_LDR 0xF8D0
#define rio_LDRB 0xF890
#define rio_STR 0xF8C0
#define rio_STRB 0xF880

/* the VFP: s0 and s1 (free to use), through core registers */
static void rio_vmovToS(rio_Jit *g, int sn, int r) { /* vmov sn, r */
  rio_put32(g, 0xEE00 | (uint32_t)(sn >> 1), (uint32_t)r << 12 | 0x0A10 | (uint32_t)(sn & 1) << 7);
}
static void rio_vmovFromS(rio_Jit *g, int r, int sn) { /* vmov r, sn */
  rio_put32(g, 0xEE10 | (uint32_t)(sn >> 1), (uint32_t)r << 12 | 0x0A10 | (uint32_t)(sn & 1) << 7);
}
static void rio_vcmpS0S1(rio_Jit *g) { /* vcmp.f32 s0, s1; vmrs APSR_nzcv, fpscr */
  rio_put32(g, 0xEEB4, 0x0A60);
  rio_put32(g, 0xEEF1, 0xFA10);
}

/* which slots an instruction reads or writes (bit 0: a, 1: b, 2: c), for choosing registers */
static int rio_slotUses(int op) {
  switch (op) {
  case rio_Op_jmp: case rio_Op_call: case rio_Op_ret: return 0;
  case rio_Op_m1: return 3;
  case rio_Op_jz: case rio_Op_jnz: return 1;
  case rio_Op_jlt: case rio_Op_jle: case rio_Op_jeq: case rio_Op_jne: case rio_Op_fori: case rio_Op_jfeq: case rio_Op_jfne:
  case rio_Op_jflt: case rio_Op_jfle: case rio_Op_jfnlt: case rio_Op_jfnle:
    return 3;
  case rio_Op_mov: case rio_Op_neg: case rio_Op_bnot: case rio_Op_not: case rio_Op_iabs: case rio_Op_fneg: case rio_Op_itof:
  case rio_Op_ftoi: case rio_Op_lea: case rio_Op_each:
    return 3;
  case rio_Op_ldw: case rio_Op_ldb: return 3;
  case rio_Op_stw: case rio_Op_stb: return 3;
  case rio_Op_mov2: case rio_Op_ldw2: case rio_Op_stw2: return 0; /* (pairs: kept in memory) */
  }
  if (rio_isLog(op)) return 0;
  return 7;
}

/* Picks the slots to keep in registers for [first, last]. memory: the code touches memory, which may be
   a slot: then only constants get registers. */
static void rio_allocateRegs(const rio_Instr *code, uint32_t first, uint32_t last, uint32_t constCap, int memory) {
  static uint32_t slots[64], weight[64]; /* (slots past the 64th aren't counted) */
  uint32_t n = 0, pc, i, k, best;
  for (pc = first; pc <= last; pc++) {
    uint32_t operands[3];
    int uses = rio_slotUses(code[pc].op), j;
    operands[0] = code[pc].a;
    operands[1] = code[pc].b;
    operands[2] = code[pc].c;
    for (j = 0; j < 3; j++) {
      if (!(uses >> j & 1) || (memory && operands[j] >= constCap)) continue;
      for (i = 0; i < n && slots[i] != operands[j]; i++) {}
      if (i == n) {
        if (n == 64) continue;
        slots[n] = operands[j];
        weight[n++] = 0;
      }
      /* (a variable saves a load and a store; a constant, a move or two, loaded once before the loop) */
      weight[i] += operands[j] < constCap ? 2 : 4;
    }
    if (rio_hasData(code[pc].op)) pc++;
  }
  for (rio_regCount = 0; rio_regCount < rio_JIT_REGS; rio_regCount++) { /* the heaviest */
    for (k = n, best = 0, i = 0; i < n; i++)
      if (weight[i] > best) {
        best = weight[i];
        k = i;
      }
    if (k == n || best < 2) break;
    rio_regSlot[rio_regCount] = slots[k];
    weight[k] = 0;
  }
}

/* the slots registers hold, to memory and back, around code that reads or writes them there */
static void rio_spillRegs(rio_Jit *g) {
  int k;
  for (k = 0; k < rio_regCount; k++)
    if (rio_regSlot[k] >= g->constCap) rio_storeSlot(g, 4 + k, rio_regSlot[k]);
}

static void rio_reloadRegs(rio_Jit *g) {
  int k;
  rio_forgetRegs(g);
  for (k = 0; k < rio_regCount; k++)
    if (rio_regSlot[k] >= g->constCap) rio_loadSlot(g, 4 + k, rio_regSlot[k]);
  rio_forgetRegs(g);
}

static void rio_spillSlot(rio_Jit *g, uint32_t s) {
  int r = rio_regOf(s);
  if (r >= 0 && s >= g->constCap) rio_storeSlot(g, r, s);
}

/* rio_jitOp(slots, vm, the instruction at pc), with r0 and r1 kept */
static void rio_callJitOp(rio_Jit *g, const rio_Instr *code, uint32_t pc, uint32_t first) {
  rio_put16(g, 0xB403); /* push {r0, r1} */
  if (pc == first) { /* (filled in once the copy has a place) */
    g->copyAt = g->len;
    rio_movw(g, rio_Reg_r2, 0);
    rio_movt(g, rio_Reg_r2, 0);
  } else rio_loadImm(g, rio_Reg_r2, (uint32_t)(uintptr_t)(code + pc));
  rio_loadImm(g, rio_Reg_r12, (uint32_t)(uintptr_t)rio_jitOp);
  rio_put16(g, 0x47E0); /* blx r12 */
  rio_put16(g, 0xBC03); /* pop {r0, r1} */
  rio_forgetRegs(g);
}

/* ------------------------------------------------------------------ compiling a region */

/* Native code for bytecode [first, last] (isProc: a whole proc's body); returns 0, or -1 if it doesn't
   fit. */
static int rio_compileRegion(rio_Jit *g, const rio_Instr *code, uint32_t first, uint32_t last, int isProc) {
  /* static, not on the stack: an MCU's main stack can be just 2 KB (the RP2350's default) */
  static uint16_t labels[rio_JIT_MAX_LEN]; /* (halfwords, up to 128 KB of code) */
  static uint32_t exitPcs[64], exitAts[64];
  static uint8_t isTarget[rio_JIT_MAX_LEN];
  static rio_JitFix fixes[rio_JIT_MAX_FIXES];
  int fixCount = 0, i, memory;
  uint32_t pc, exitCount = 0, head = first;
  if (last - first + 1 > rio_JIT_MAX_LEN || !rio_jitCompilable(g, code, first, last, &memory)) return -1;
  memset(isTarget, 0, last - first + 1);
  for (pc = first; pc <= last; pc++) {
    if (rio_isJump(code[pc].op) && code[pc].c >= first && code[pc].c <= last) isTarget[code[pc].c - first] = 1;
    if (rio_hasData(code[pc].op)) pc++;
  }
  rio_allocateRegs(code, first, last, g->constCap, memory);
  g->copyAt = 0;
  rio_put32(g, 0xE92D, 0x4FF8); /* push.w {r3-r11, lr} (r3 too: the stack stays 8-byte aligned for calls) */
  rio_forgetRegs(g);
  for (i = 0; i < rio_regCount; i++) rio_loadSlot(g, 4 + i, rio_regSlot[i]);
  rio_forgetRegs(g);
  for (pc = first; pc <= last; pc++) {
    const rio_Instr *in = &code[pc];
    uint32_t A = in->a, B = in->b, C = in->c;
    int rb, rc, rd, rv;
    if (isTarget[pc - first]) { /* a jump target: 4-byte aligned (nop), so it's fetched at once */
      rio_forgetRegs(g);
      if (g->len & 1) rio_put16(g, 0xBF00);
    }
    labels[pc - first] = (uint16_t)g->len;
    switch (in->op) {
    case rio_Op_mov:
      rb = rio_srcReg(g, B, rio_Reg_r2);
      rio_finishResult(g, A, rb);
      break;
    case rio_Op_add: case rio_Op_sub: case rio_Op_and: case rio_Op_or: case rio_Op_xor: {
      uint32_t op = in->op == rio_Op_add ? 0xEB00 : in->op == rio_Op_sub ? 0xEBA0 : in->op == rio_Op_and ? 0xEA00 : in->op == rio_Op_or ? 0xEA40 : 0xEA80;
      rb = rio_srcReg(g, B, rio_Reg_r2);
      rc = rio_srcReg(g, C, rio_Reg_r3);
      rd = rio_dstReg(A);
      rio_dataOp(g, op, rd, rb, rc);
      rio_finishResult(g, A, rd);
      break;
    }
    case rio_Op_mul:
      rb = rio_srcReg(g, B, rio_Reg_r2);
      rc = rio_srcReg(g, C, rio_Reg_r3);
      rd = rio_dstReg(A);
      rio_put32(g, 0xFB00 | (uint32_t)rb, 0xF000 | (uint32_t)rd << 8 | (uint32_t)rc);
      rio_finishResult(g, A, rd);
      break;
    case rio_Op_div: case rio_Op_mod: /* a zero divisor: the interpreter runs this one and reports it */
      rc = rio_srcReg(g, C, rio_Reg_r3);
      rio_cmpZero(g, rc);
      rio_branch(g, rio_Cond_eq, fixes, &fixCount, rio_JIT_MAX_FIXES, (int)(pc | rio_JitFail_div << 16));
      rb = rio_srcReg(g, B, rio_Reg_r2);
      rd = rio_dstReg(A);
      rio_put32(g, 0xFB90 | (uint32_t)rb, 0xF0F0 | rio_Reg_r12 << 8 | (uint32_t)rc); /* sdiv r12, rb, rc (-1 and INT_MIN wrap, like the vm) */
      if (in->op == rio_Op_mod) /* mls rd, r12, rc, rb */
        rio_put32(g, 0xFB00 | rio_Reg_r12, (uint32_t)rb << 12 | (uint32_t)rd << 8 | 0x10 | (uint32_t)rc);
      else rio_movReg(g, rd, rio_Reg_r12);
      rio_finishResult(g, A, rd);
      break;
    case rio_Op_shl: case rio_Op_shr:
      rb = rio_srcReg(g, B, rio_Reg_r2);
      rc = rio_srcReg(g, C, rio_Reg_r3);
      rio_put32(g, 0xF000 | (uint32_t)rc, rio_Reg_r3 << 8 | 0x1F); /* and.w r3, rc, #31 */
      g->held[rio_Reg_r3] = -1;
      rd = rio_dstReg(A);
      rio_put32(g, (in->op == rio_Op_shl ? 0xFA00 : 0xFA40) | (uint32_t)rb, 0xF000 | (uint32_t)rd << 8 | rio_Reg_r3);
      rio_finishResult(g, A, rd);
      break;
    case rio_Op_neg: /* rsb.w rd, rb, #0 */
      rb = rio_srcReg(g, B, rio_Reg_r2);
      rd = rio_dstReg(A);
      rio_put32(g, 0xF1C0 | (uint32_t)rb, (uint32_t)rd << 8);
      rio_finishResult(g, A, rd);
      break;
    case rio_Op_bnot: /* mvn.w */
      rb = rio_srcReg(g, B, rio_Reg_r2);
      rd = rio_dstReg(A);
      rio_put32(g, 0xEA6F, (uint32_t)rd << 8 | (uint32_t)rb);
      rio_finishResult(g, A, rd);
      break;
    case rio_Op_iabs:
      rb = rio_srcReg(g, B, rio_Reg_r2);
      rd = rio_dstReg(A);
      rio_cmpZero(g, rb);
      rio_movReg(g, rd, rb);
      rio_it(g, rio_Cond_lt);
      rio_put32(g, 0xF1C0 | (uint32_t)rd, (uint32_t)rd << 8);
      rio_finishResult(g, A, rd);
      break;
    case rio_Op_imin: case rio_Op_imax: /* in r2: rc is never r2 */
      rb = rio_srcReg(g, B, rio_Reg_r2);
      rc = rio_srcReg(g, C, rio_Reg_r3);
      rio_cmp(g, rb, rc);
      rio_movReg(g, rio_Reg_r2, rb);
      rio_it(g, in->op == rio_Op_imin ? rio_Cond_gt : rio_Cond_lt);
      rio_put32(g, 0xEA4F, rio_Reg_r2 << 8 | (uint32_t)rc);
      rio_finishResult(g, A, rio_Reg_r2);
      break;
    case rio_Op_not:
      rb = rio_srcReg(g, B, rio_Reg_r2);
      rd = rio_dstReg(A);
      rio_cmpZero(g, rb);
      rio_movImm8(g, rd, 0);
      rio_it(g, rio_Cond_eq);
      rio_movImm8(g, rd, 1);
      rio_finishResult(g, A, rd);
      break;
    case rio_Op_eq: case rio_Op_ne: case rio_Op_lt: case rio_Op_le: {
      int cond = in->op == rio_Op_eq ? rio_Cond_eq : in->op == rio_Op_ne ? rio_Cond_ne : in->op == rio_Op_lt ? rio_Cond_lt : rio_Cond_le;
      rb = rio_srcReg(g, B, rio_Reg_r2);
      rc = rio_srcReg(g, C, rio_Reg_r3);
      rd = rio_dstReg(A);
      rio_cmp(g, rb, rc);
      rio_movImm8(g, rd, 0);
      rio_it(g, cond);
      rio_movImm8(g, rd, 1);
      rio_finishResult(g, A, rd);
      break;
    }
    case rio_Op_lea:
      rb = rio_srcReg(g, B, rio_Reg_r2);
      rd = rio_dstReg(A);
      rio_addImm(g, rd, rb, C);
      rio_finishResult(g, A, rd);
      break;
    case rio_Op_ffi: /* fn(vm, slots + a), then a trap is an error here; r0 and r1 kept around the call */
      rio_put16(g, 0xB403); /* push {r0, r1} */
      rio_movReg(g, rio_Reg_r2, rio_Reg_r1); /* r2 = vm */
      rio_put32(g, 0xF8D0 | rio_Reg_r2, rio_Reg_r12 << 12 | rio_OFFSET_FFI_FN(C)); /* r12 = vm->ffi[c].fn */
      if (A * 4 < 4096) rio_addImm(g, rio_Reg_r1, rio_Reg_r0, A * 4);
      else {
        rio_loadImm(g, rio_Reg_r3, A * 4);
        rio_dataOp(g, 0xEB00, rio_Reg_r1, rio_Reg_r0, rio_Reg_r3);
      }
      rio_movReg(g, rio_Reg_r0, rio_Reg_r2);
      rio_put16(g, 0x47E0); /* blx r12 */
      rio_put16(g, 0xBC03); /* pop {r0, r1} (measured: keeping the slots in memory beats spilling registers
                               around each call, for pset loops) */
      rio_forgetRegs(g);
      rio_put32(g, 0xF8D1, rio_Reg_r12 << 12 | rio_OFFSET_TRAPPED);
      rio_cmpZero(g, rio_Reg_r12);
      rio_branch(g, rio_Cond_ne, fixes, &fixCount, rio_JIT_MAX_FIXES, (int)(pc | rio_JitFail_trap << 16));
      break;
    case rio_Op_call: /* a native proc: it returns where it returned, or a failure (kind << 16 | pc) to pass on */
      rio_spillRegs(g);
      rio_put16(g, 0xB403); /* push {r0, r1} */
      rio_loadImm(g, rio_Reg_r12, (uint32_t)rio_nativeProc(g, C));
      rio_put16(g, 0x47E0); /* blx r12 */
      rio_movReg(g, rio_Reg_r12, rio_Reg_r0);
      rio_put16(g, 0xBC03); /* pop {r0, r1} */
      rio_reloadRegs(g);
      rio_put32(g, 0xF5B0 | rio_Reg_r12, 0x3F80); /* cmp.w r12, #0x10000 */
      rio_branch(g, rio_Cond_hs, fixes, &fixCount, rio_JIT_MAX_FIXES, (int)rio_JIT_PASS);
      break;
    case rio_Op_ret: /* (out to it: the vm returns, or a native caller carries on) */
      rio_branch(g, rio_Cond_always, fixes, &fixCount, rio_JIT_MAX_FIXES, (int)pc);
      break;
    case rio_Op_m1:
      if (rio_isInlineMath(in->x)) {
        rb = rio_srcReg(g, B, rio_Reg_r2);
        rio_vmovToS(g, 0, rb);
        if (in->x == rio_Math1_sqrt) rio_put32(g, 0xEEB1, 0x0AC0); /* vsqrt */
        else if (in->x == rio_Math1_abs) rio_put32(g, 0xEEB0, 0x0AC0); /* vabs */
        else rio_put32(g, in->x == rio_Math1_floor ? 0xFEBB : 0xFEBA, 0x0A40); /* vrintm / vrintp: floor / ceil */
        rd = rio_dstReg(A);
        rio_vmovFromS(g, rd, 0);
        if (rd == rio_Reg_r2) g->held[rio_Reg_r2] = -1;
        rio_finishResult(g, A, rd);
        break;
      }
      /* fall through */
    case rio_Op_m2: /* the operands to memory, the call, and the result back */
      rio_spillSlot(g, B);
      if (in->op == rio_Op_m2) rio_spillSlot(g, C);
      rio_callJitOp(g, code, pc, first);
      if (rio_regOf(A) >= 0) rio_loadSlot(g, rio_regOf(A), A);
      rio_forgetRegs(g);
      break;
    case rio_Op_logi: case rio_Op_logf: case rio_Op_logs: case rio_Op_logb: case rio_Op_logn: case rio_Op_logv: case rio_Op_loge:
      rio_spillRegs(g);
      rio_callJitOp(g, code, pc, first);
      if (in->op == rio_Op_logn) {
        pc++;
        labels[pc - first] = (uint16_t)g->len;
      }
      break;
    case rio_Op_mov2: /* both read before either is written: they may overlap */
      rb = rio_srcReg(g, B, rio_Reg_r2);
      rio_movReg(g, rio_Reg_r2, rb);
      rc = rio_srcReg(g, B + 1, rio_Reg_r3);
      rio_movReg(g, rio_Reg_r3, rc);
      g->held[rio_Reg_r2] = g->held[rio_Reg_r3] = -1;
      rio_finishResult(g, A, rio_Reg_r2);
      rio_finishResult(g, A + 1, rio_Reg_r3);
      break;
    /* memory (bytes of M, whose start is r0): a[b + c] and the like */
    case rio_Op_ldw: case rio_Op_ldb:
      rb = rio_srcReg(g, B, rio_Reg_r2);
      rio_dataOp(g, 0xEB00, rio_Reg_r12, rb, rio_Reg_r0); /* add.w r12, rb, r0 */
      g->held[rio_Reg_r12] = -1;
      rd = rio_dstReg(A);
      rio_memOp(g, in->op == rio_Op_ldw ? rio_LDR : rio_LDRB, rd, C);
      if (rd == rio_Reg_r2) g->held[rio_Reg_r2] = -1;
      rio_finishResult(g, A, rd);
      break;
    case rio_Op_stw: case rio_Op_stb:
      rv = rio_srcReg(g, B, rio_Reg_lr);
      rb = rio_srcReg(g, A, rio_Reg_r2);
      rio_dataOp(g, 0xEB00, rio_Reg_r12, rb, rio_Reg_r0);
      g->held[rio_Reg_r12] = -1;
      rio_memOp(g, in->op == rio_Op_stw ? rio_STR : rio_STRB, rv, C);
      rio_forgetRegs(g);
      break;
    case rio_Op_ldw2:
      rb = rio_srcReg(g, B, rio_Reg_r2);
      rio_dataOp(g, 0xEB00, rio_Reg_r12, rb, rio_Reg_r0);
      g->held[rio_Reg_r12] = -1;
      rio_memOp(g, rio_LDR, rio_Reg_r2, C);
      rio_memOp(g, rio_LDR, rio_Reg_r3, C + 4);
      g->held[rio_Reg_r2] = g->held[rio_Reg_r3] = -1;
      rio_finishResult(g, A, rio_Reg_r2);
      rio_finishResult(g, A + 1, rio_Reg_r3);
      break;
    case rio_Op_stw2:
      rio_loadSlot(g, rio_Reg_lr, B);
      rio_loadSlot(g, rio_Reg_r3, B + 1);
      rb = rio_srcReg(g, A, rio_Reg_r2);
      rio_dataOp(g, 0xEB00, rio_Reg_r12, rb, rio_Reg_r0);
      g->held[rio_Reg_r12] = -1;
      rio_memOp(g, rio_STR, rio_Reg_lr, C);
      rio_memOp(g, rio_STR, rio_Reg_r3, C + 4);
      rio_forgetRegs(g);
      break;
    case rio_Op_ldx: case rio_Op_ldxb: case rio_Op_idx: { /* a[i]: an index out of bounds is the interpreter's to report */
      uint32_t size = rio_dataSize(in + 1), offset = in[1].a;
      int ri = rio_srcReg(g, C, rio_Reg_r3), rl = rio_srcReg(g, B + 1, rio_Reg_r2);
      rio_cmp(g, ri, rl);
      rio_branch(g, rio_Cond_hs, fixes, &fixCount, rio_JIT_MAX_FIXES, (int)(pc | rio_JitFail_index << 16));
      rb = rio_srcReg(g, B, rio_Reg_r2);
      if (in->op == rio_Op_idx) { /* the element's address, not its value */
        rio_elementAddr(g, rb, ri, size);
        rio_put32(g, 0xEBB0 | rio_Reg_r12, rio_Reg_r2 << 8 | rio_Reg_r0); /* (r2 = r12 - r0: subs.w r2, r12, r0) */
        g->held[rio_Reg_r2] = -1;
        rio_finishResult(g, A, rio_Reg_r2);
      } else {
        rio_elementAddr(g, rb, ri, size);
        rd = rio_dstReg(A);
        rio_memOp(g, in->op == rio_Op_ldx ? rio_LDR : rio_LDRB, rd, offset);
        if (rd == rio_Reg_r2) g->held[rio_Reg_r2] = -1;
        rio_finishResult(g, A, rd);
      }
      pc++;
      labels[pc - first] = (uint16_t)g->len;
      break;
    }
    case rio_Op_stx: case rio_Op_stxb: {
      uint32_t size = rio_dataSize(in + 1), offset = in[1].a;
      int ri = rio_srcReg(g, B, rio_Reg_r3), rl = rio_srcReg(g, A + 1, rio_Reg_r2);
      rio_cmp(g, ri, rl);
      rio_branch(g, rio_Cond_hs, fixes, &fixCount, rio_JIT_MAX_FIXES, (int)(pc | rio_JitFail_index << 16));
      rb = rio_srcReg(g, A, rio_Reg_r2);
      rio_elementAddr(g, rb, ri, size);
      g->held[rio_Reg_lr] = -1;
      rv = rio_srcReg(g, C, rio_Reg_lr); /* (loads don't touch r12) */
      rio_memOp(g, in->op == rio_Op_stx ? rio_STR : rio_STRB, rv, offset);
      rio_forgetRegs(g);
      pc++;
      labels[pc - first] = (uint16_t)g->len;
      break;
    }
    /* floats, in s0 and s1 */
    case rio_Op_fadd: case rio_Op_fsub: case rio_Op_fmul: case rio_Op_fdiv: {
      uint32_t op = in->op == rio_Op_fmul ? 0xEE20 : in->op == rio_Op_fdiv ? 0xEE80 : 0xEE30;
      rb = rio_srcReg(g, B, rio_Reg_r2);
      rc = rio_srcReg(g, C, rio_Reg_r3);
      rio_vmovToS(g, 0, rb);
      rio_vmovToS(g, 1, rc);
      rio_put32(g, op, 0x0A20 | (in->op == rio_Op_fsub ? 0x40 : 0)); /* vop.f32 s0, s0, s1 */
      rd = rio_dstReg(A);
      rio_vmovFromS(g, rd, 0);
      if (rd == rio_Reg_r2) g->held[rio_Reg_r2] = -1;
      rio_finishResult(g, A, rd);
      break;
    }
    case rio_Op_fneg: case rio_Op_itof: case rio_Op_ftoi:
      rb = rio_srcReg(g, B, rio_Reg_r2);
      rio_vmovToS(g, 0, rb);
      if (in->op == rio_Op_fneg) rio_put32(g, 0xEEB1, 0x0A40); /* vneg */
      else if (in->op == rio_Op_itof) rio_put32(g, 0xEEB8, 0x0AC0); /* vcvt.f32.s32 */
      else rio_put32(g, 0xEEBD, 0x0AC0); /* vcvt.s32.f32 (toward zero; saturates; NaN: 0, like rio_floatToInt) */
      rd = rio_dstReg(A);
      rio_vmovFromS(g, rd, 0);
      if (rd == rio_Reg_r2) g->held[rio_Reg_r2] = -1;
      rio_finishResult(g, A, rd);
      break;
    case rio_Op_feq: case rio_Op_fne: case rio_Op_flt: case rio_Op_fle: { /* NaN compares unordered: false, except != */
      int cond = in->op == rio_Op_feq ? rio_Cond_eq : in->op == rio_Op_fne ? rio_Cond_ne : in->op == rio_Op_flt ? rio_Cond_mi : rio_Cond_ls;
      rb = rio_srcReg(g, B, rio_Reg_r2);
      rc = rio_srcReg(g, C, rio_Reg_r3);
      rio_vmovToS(g, 0, rb);
      rio_vmovToS(g, 1, rc);
      rio_vcmpS0S1(g);
      rd = rio_dstReg(A);
      rio_movImm8(g, rd, 0);
      rio_it(g, cond);
      rio_movImm8(g, rd, 1);
      if (rd == rio_Reg_r2) g->held[rio_Reg_r2] = -1;
      rio_finishResult(g, A, rd);
      break;
    }
    case rio_Op_fmin: case rio_Op_fmax: /* b < c ? b : c, and b > c ? b : c (NaN: c) */
      rb = rio_srcReg(g, B, rio_Reg_r2);
      rc = rio_srcReg(g, C, rio_Reg_r3);
      rio_vmovToS(g, 0, rb);
      rio_vmovToS(g, 1, rc);
      rio_vcmpS0S1(g);
      rio_movReg(g, rio_Reg_lr, rc);
      rio_it(g, in->op == rio_Op_fmin ? rio_Cond_mi : rio_Cond_gt);
      rio_put32(g, 0xEA4F, rio_Reg_lr << 8 | (uint32_t)rb);
      g->held[rio_Reg_lr] = -1;
      rio_finishResult(g, A, rio_Reg_lr);
      break;
    case rio_Op_jmp: case rio_Op_jz: case rio_Op_jnz: case rio_Op_jlt: case rio_Op_jle: case rio_Op_jeq: case rio_Op_jne:
    case rio_Op_fori: case rio_Op_each: case rio_Op_jfeq: case rio_Op_jfne: case rio_Op_jflt: case rio_Op_jfle: case rio_Op_jfnlt:
    case rio_Op_jfnle: {
      /* compare: 1 with zero, 2 two registers, 3 two floats */
      int cond = rio_Cond_always, inside = C >= first && C <= last, back = inside && C <= pc, ra = 0, rbb = 0, compare = 0;
      if (in->op == rio_Op_jz || in->op == rio_Op_jnz) {
        ra = rio_srcReg(g, A, rio_Reg_r2);
        compare = 1;
        cond = in->op == rio_Op_jz ? rio_Cond_eq : rio_Cond_ne;
      } else if (in->op == rio_Op_fori || in->op == rio_Op_each) { /* fori: a += 1 while a < b; each: a += b[1] while a < b (unsigned) */
        ra = rio_srcReg(g, A, rio_Reg_r2);
        rd = rio_dstReg(A);
        if (in->op == rio_Op_fori) rio_put32(g, 0xF100 | (uint32_t)ra, (uint32_t)rd << 8 | 1); /* add.w rd, ra, #1 */
        else {
          rc = rio_srcReg(g, B + 1, rio_Reg_r3);
          rio_dataOp(g, 0xEB00, rd, ra, rc);
          g->held[rio_Reg_r3] = -1;
        }
        if (rd == rio_Reg_r2) g->held[rio_Reg_r2] = -1;
        rio_finishResult(g, A, rd);
        ra = rio_regOf(A) >= 0 ? rio_regOf(A) : rio_Reg_r2;
        rbb = rio_srcReg(g, B, rio_Reg_r3);
        compare = 2;
        cond = in->op == rio_Op_fori ? rio_Cond_lt : rio_Cond_lo;
      } else if (in->op >= rio_Op_jfeq && in->op <= rio_Op_jfnle) {
        ra = rio_srcReg(g, A, rio_Reg_r2);
        rbb = rio_srcReg(g, B, rio_Reg_r3);
        compare = 3;
        cond = in->op == rio_Op_jfeq ? rio_Cond_eq : in->op == rio_Op_jfne ? rio_Cond_ne : in->op == rio_Op_jflt ? rio_Cond_mi
               : in->op == rio_Op_jfle ? rio_Cond_ls : in->op == rio_Op_jfnlt ? rio_Cond_pl : rio_Cond_hi;
      } else if (in->op != rio_Op_jmp) {
        ra = rio_srcReg(g, A, rio_Reg_r2);
        rbb = rio_srcReg(g, B, rio_Reg_r3);
        compare = 2;
        cond = in->op == rio_Op_jlt ? rio_Cond_lt : in->op == rio_Op_jle ? rio_Cond_le : in->op == rio_Op_jeq ? rio_Cond_eq : rio_Cond_ne;
      }
      if (back) { /* check for an interrupt on the way back (in a proc: a failure, as there's no going back in) */
        rio_put32(g, 0xF8D1, rio_Reg_r12 << 12 | rio_OFFSET_INTERRUPTED); /* ldr.w r12, [r1, #interrupted] */
        rio_cmpZero(g, rio_Reg_r12);
        rio_branch(g, rio_Cond_ne, fixes, &fixCount, rio_JIT_MAX_FIXES, isProc ? (int)(pc | rio_JitFail_interrupt << 16) : (int)head);
        g->held[rio_Reg_r12] = -1;
      }
      if (compare == 1) rio_cmpZero(g, ra);
      else if (compare == 2) rio_cmp(g, ra, rbb);
      else if (compare == 3) {
        rio_vmovToS(g, 0, ra);
        rio_vmovToS(g, 1, rbb);
        rio_vcmpS0S1(g);
      }
      rio_branch(g, cond, fixes, &fixCount, rio_JIT_MAX_FIXES, inside ? (int)(C | 0x80000000u) : (int)C);
      break;
    }
    default: return -1;
    }
  }
  rio_branch(g, rio_Cond_always, fixes, &fixCount, rio_JIT_MAX_FIXES, (int)(last + 1)); /* falling off the end */
  /* jumps inside go to their labels; exits to a stub per bytecode position: the registers go back to
     their slots, and it returns the position */
  for (i = 0; i < fixCount && !g->overflow; i++) {
    uint32_t to = (uint32_t)fixes[i].to, j, at;
    if (to & 0x80000000u) at = labels[(to & 0x7FFFFFFFu) - first];
    else {
      for (j = 0; j < exitCount && exitPcs[j] != to; j++) {}
      if (j == exitCount) {
        int k;
        if (exitCount >= 64) return -1;
        exitPcs[exitCount] = to;
        exitAts[exitCount++] = g->len;
        for (k = 0; k < rio_regCount; k++)
          if (rio_regSlot[k] >= g->constCap) rio_storeSlot(g, 4 + k, rio_regSlot[k]);
        if (to == rio_JIT_PASS) rio_movReg(g, rio_Reg_r0, rio_Reg_r12);
        else rio_loadImm(g, rio_Reg_r0, to);
        rio_put32(g, 0xE8BD, 0x8FF8); /* r0 = to; pop.w {r3-r11, pc} */
      }
      at = exitAts[j];
    }
    if (!g->overflow && rio_patchBranch(g, fixes[i].at, fixes[i].cond, at)) return -1;
  }
  if (g->copyAt && !g->overflow) { /* the copy of the first instruction (and its data word), after the code */
    uint32_t at = g->copyAt, n, addr;
    rio_Instr two[2];
    if (g->len & 1) rio_put16(g, 0xBF00);
    addr = (uint32_t)(uintptr_t)(g->out + g->len);
    two[0] = code[first];
    two[1] = code[first + 1];
    for (n = 0; n < sizeof two / 2; n++) rio_put16(g, ((const uint16_t *)(const void *)two)[n]);
    n = g->len; /* the movw/movt that loads its address */
    g->len = at;
    rio_movw(g, rio_Reg_r2, addr & 0xFFFF);
    rio_movt(g, rio_Reg_r2, addr >> 16);
    g->len = n;
  }
  return g->overflow ? -1 : 0;
}

/* ------------------------------------------------------------------ compiling what's there */

/* compiles [first, last] as native entry `count` (the next); 0 if it did */
static int rio_addNative(rio_Jit *g, uint32_t first, uint32_t last, int isProc) {
  rio_Vm *vm = g->vm;
  uint32_t start = g->len;
  g->overflow = 0;
  if (rio_compileRegion(g, vm->code, first, last, isProc)) {
    g->len = start;
    return -1;
  }
  vm->jitFns[vm->jitCount] = (rio_JitFn)(uintptr_t)((uintptr_t)(g->out + start) | 1); /* (| 1: Thumb) */
  rio_nativeRanges(vm)[vm->jitCount][0] = (uint16_t)first;
  rio_nativeRanges(vm)[vm->jitCount][1] = (uint16_t)last;
  vm->code[first].op = rio_Op_jit;
  vm->code[first].a = (uint16_t)vm->jitCount++;
  return 0;
}

/* compiles what qualifies in the code, from where earlier compiles stopped; returns how many */
static int rio_jitAll(rio_Vm *vm) {
  rio_Instr *code = vm->code;
  uint8_t *base = (uint8_t *)vm->jitBuf;
  uint16_t(*ranges)[2] = rio_nativeRanges(vm);
  uint32_t head = rio_JIT_MAX_NATIVE * (uint32_t)sizeof(rio_JitFn) + rio_JIT_MAX_NATIVE * 4, pc, k, first, last;
  rio_Jit g;
  int count = 0;
  g.out = (uint16_t *)(void *)(base + head);
  g.cap = (vm->jitSize - head) / 2;
  g.len = vm->jitUsed;
  if (g.cap > 65535) g.cap = 65535; /* (labels are 16-bit) */
  g.consts = (const rio_Val *)(const void *)vm->mem;
  g.constCap = vm->constCap;
  g.vm = vm;
  vm->jitFns = (rio_JitFn *)(void *)base;
  /* procs, callees first (they're defined before use, but a pass may leave one for the next) */
  for (;;) {
    int more = 0, i;
    for (i = 0; i < vm->procCount && vm->jitCount < rio_JIT_MAX_NATIVE; i++) {
      int memory;
      first = vm->procs[i].entry;
      last = (uint32_t)vm->procs[i].end - 1u;
      if (vm->procs[i].end <= first || code[last].op != rio_Op_ret || last - first + 1 > rio_JIT_MAX_LEN) continue;
      for (k = 0; k < vm->jitCount && !(first <= ranges[k][1] && last >= ranges[k][0]); k++) {} /* overlaps one done? */
      if (k < vm->jitCount || !rio_jitCompilable(&g, code, first, last, &memory) || memory)
        continue; /* (memory ops would keep all its slots out of registers: its loops do better alone) */
      if (rio_addNative(&g, first, last, 1)) continue;
      count++;
      more = 1;
    }
    if (!more) break;
  }
  /* loops, biggest first: a loop inside one already compiled is part of it */
  while (vm->jitCount < rio_JIT_MAX_NATIVE) {
    uint32_t bestFirst = 0, bestLast = 0, best = 0;
    for (pc = 0; pc < vm->codeLen; pc++) {
      int memory;
      if (!rio_isJump(code[pc].op) || code[pc].c > pc) continue; /* a backward jump closes a loop */
      first = code[pc].c;
      last = pc;
      if (last - first + 1 <= best || last - first + 1 > rio_JIT_MAX_LEN) continue;
      for (k = 0; k < vm->jitCount && !(first >= ranges[k][0] && last <= ranges[k][1]); k++) {} /* inside one done? */
      if (k < vm->jitCount) continue;
      if (!rio_jitCompilable(&g, code, first, last, &memory)) continue;
      bestFirst = first;
      bestLast = last;
      best = last - first + 1;
    }
    if (!best) break;
    if (rio_addNative(&g, bestFirst, bestLast, 0)) break; /* out of room: the rest stays bytecode */
    count++;
  }
  if (g.len > vm->jitUsed) {
#if defined(__linux__) || defined(__APPLE__)
    __builtin___clear_cache((char *)(g.out + vm->jitUsed), (char *)(g.out + g.len));
#else
    __asm volatile("dsb 0xF\n isb 0xF" ::: "memory");
#endif
    vm->jitUsed = g.len;
  }
  return count;
}

int rio_jit(rio_Vm *vm, void *buf, uint32_t size) {
  uint8_t *b = (uint8_t *)(((uintptr_t)buf + 7) & ~(uintptr_t)7);
  uint32_t pad = (uint32_t)(b - (uint8_t *)buf);
  if (!buf || size < pad + rio_JIT_MAX_NATIVE * (sizeof(rio_JitFn) + 4) + 256 || rio_OFFSET_INTERRUPTED >= 4096 ||
      rio_OFFSET_TRAPPED >= 4096)
    return -1;
  vm->jitBuf = b;
  vm->jitSize = size - pad;
  vm->jitUsed = 0;
  vm->jitCount = 0;
  vm->jitHook = rio_jitAll;
  return vm->ready && !vm->inRepl ? rio_jitAll(vm) : 0;
}

#else
int rio_jit(rio_Vm *vm, void *buf, uint32_t size) {
  (void)vm;
  (void)buf;
  (void)size;
  return -1;
}
#endif
