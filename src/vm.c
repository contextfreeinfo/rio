/* vm.c - the interpreter. Bytecode operands are absolute slots (word indexes into mem): with no
   recursion, each proc has one static frame, so there's no stack of frames and no frame pointer. Calls
   push just a return address, on a small return stack in mem. */
#include "internal.h"
#ifndef rio_NO_COMPILER
#include "compiler.h"
#endif

/* ------------------------------------------------------------------ errors and logging */

/* the file of the module whose code holds pc (the innermost one), or NULL for the main source */
static const char *rio_fileOfPc(rio_Vm *vm, uint32_t pc) {
  const char *file = 0;
  uint32_t best = 0x10000, k;
#ifndef rio_NO_COMPILER
  if (vm->inRepl && vm->compiler) return rio_replFileOfPc(vm, pc);
#endif
  for (k = 0; k < vm->moduleCount; k++) {
    rio_ModuleCode *m = (rio_ModuleCode *)(void *)(vm->mem + vm->modulesAt) + k;
    if (pc >= m->start && pc < m->end && (uint32_t)(m->end - m->start) < best) {
      best = m->end - m->start;
      file = (const char *)vm->mem + m->fileAt;
    }
  }
  return file;
}

/* a host proc called by an image made when it had fewer parameters: the words it had, then zeros, in a
   copy; its result back */
static rio_NOINLINE void rio_ffiPadded(rio_Vm *vm, const rio_Ffi *f, rio_Val *a) {
  rio_Val w[rio_FFI_PAD_WORDS];
  int given = f->argWords - f->pad, i;
  for (i = 0; i < given; i++) w[i] = a[i];
  for (; i < f->argWords; i++) w[i].u = 0;
  f->fn(vm, w);
  for (i = 0; i < f->resultWords; i++) a[i] = w[i];
}

static int rio_runtimeError(rio_Vm *vm, const char *message, uint32_t pc) {
  char copy[rio_ERROR_BUF_SIZE];
  if (message == vm->errorText) { /* a message from rio_trap: rio_setError rewrites errorText */
    memcpy(copy, message, sizeof copy);
    message = copy;
  }
  pc = rio_unitIndex(vm, pc); /* (the line table and the modules count instructions) */
  rio_setError(vm, rio_ErrorKind_runtime, rio_pcLine(vm, pc), 0, 0, message);
  vm->errorFile = rio_fileOfPc(vm, pc);
  return -1;
}

/* out of the interpreter loop: this is rare, and building the message there tripped an LLVM 20
   optimizer crash */
static rio_NOINLINE int rio_assertFailed(rio_Vm *vm, uint32_t messageSlot, int plain, uint32_t pc) {
  char text[rio_ERROR_BUF_SIZE];
  int n = plain ? 0 : rio_appendText(text, 0, "assertion failed");
  if (messageSlot) {
    rio_Val *R = (rio_Val *)vm->mem;
    uint32_t len = R[messageSlot + 1].u;
    if (!plain) n = rio_appendText(text, n, ": ");
    if (len > (uint32_t)(rio_ERROR_BUF_SIZE - 1 - n)) len = (uint32_t)(rio_ERROR_BUF_SIZE - 1 - n);
    memcpy(text + n, vm->mem + R[messageSlot].u, len);
    n += (int)len;
  }
  text[n] = 0;
  return rio_runtimeError(vm, text, pc);
}

/* values by their descriptors (print.h), for logv and pushv */
rio_PRINTER(rio_PRINT_CODE)

static void rio_logPut(rio_Vm *vm, const char *s, int n) {
  while (n-- > 0 && vm->logLen < rio_LOG_BUF_SIZE) vm->logBuf[vm->logLen++] = *s++;
}

void rio_logOp(rio_Vm *vm, const rio_Val *R, const rio_Instr *ip) {
  const uint8_t *M = vm->mem;
  char text[32];
  if (ip->op != rio_Op_loge && ip->x) rio_logPut(vm, " ", 1);
  switch (ip->op) {
  case rio_Op_logi: rio_logPut(vm, text, rio_formatInt(text, R[(int16_t)ip->a].i)); break;
  case rio_Op_logf: rio_logPut(vm, text, rio_formatFloat(text, R[(int16_t)ip->a].f)); break;
  case rio_Op_logs: { /* text, as shown (print.h): control bytes and non-UTF-8 as \xHH */
    rio_Out o;
    o.out = (uint8_t *)vm->logBuf; o.len = (uint32_t)vm->logLen; o.cap = rio_LOG_BUF_SIZE; o.capped = 1; o.full = 0; o.spec = 0;
    rio_outShown(&o, M + R[(int16_t)ip->a].u, R[(int16_t)ip->a + 1].u, 0);
    vm->logLen = (int)o.len;
    break;
  }
  case rio_Op_logb:
    if (R[(int16_t)ip->a].i) rio_logPut(vm, "true", 4);
    else rio_logPut(vm, "false", 5);
    break;
  case rio_Op_logn: { /* an enum value by name (values 0 to n - 1): c.. their (address, length) names */
    uint32_t n = rio_dataSize(ip + 1), i = R[(int16_t)ip->a].u;
    int32_t v = R[(int16_t)ip->a].i;
    if (i < n) rio_logPut(vm, (const char *)M + R[(int16_t)ip->c + 2 * i].i, R[(int16_t)ip->c + 2 * i + 1].i);
    else rio_logPut(vm, text, rio_formatInt(text, v));
    break;
  }
  case rio_Op_logv: { /* a struct, array, builder, union, ...: by its descriptor, as much as the line holds */
    rio_Out o;
    o.out = (uint8_t *)vm->logBuf; o.len = (uint32_t)vm->logLen; o.cap = rio_LOG_BUF_SIZE; o.capped = 1; o.full = 0; o.spec = 0;
    rio_printValue(M, &o, (uint32_t)(R + (int16_t)ip->b - (const rio_Val *)M), R[(int16_t)ip->a].u, R[(int16_t)ip->a + 1].u, R[(int16_t)ip->a + 2].u); /* (its descriptor's slot, from mem's start) */
    vm->logLen = (int)o.len;
    break;
  }
  case rio_Op_loge:
    if (vm->logFn) vm->logFn(vm->logData, vm->logBuf, vm->logLen);
    vm->logLen = 0;
    break;
  }
}

/* for native code (jit.c): an instruction it leaves to C (log, and most math) */
void rio_jitOp(rio_Val *R, rio_Vm *vm, const rio_Instr *ip) {
  if (ip->op == rio_Op_m1) R[(int16_t)ip->a].f = rio_math1[ip->x](R[(int16_t)ip->b].f);
  else if (ip->op == rio_Op_m2) R[(int16_t)ip->a].f = rio_math2[ip->x](R[(int16_t)ip->b].f, R[(int16_t)ip->c].f);
  else rio_logOp(vm, R, ip);
}

/* what native code says failed (kind << 16 | pc), as a runtime error */
static rio_NOINLINE int rio_jitFailed(rio_Vm *vm, uint32_t to) {
  switch (to >> 16) {
  case rio_JitFail_div: return rio_runtimeError(vm, "division by zero", to & 0xFFFF);
  case rio_JitFail_trap: /* an ffi function called rio_trap */
    vm->trapped = 0;
    return rio_runtimeError(vm, vm->errorText, to & 0xFFFF);
  case rio_JitFail_interrupt: /* in a native proc */
    vm->interrupted = 0;
    return rio_runtimeError(vm, "interrupted", to & 0xFFFF);
  default: return rio_runtimeError(vm, "index out of bounds", to & 0xFFFF);
  }
}

/* ------------------------------------------------------------------ the interpreter */

/* the word at byte address p of mem */
#define rio_WORD(p) (*(rio_Val *)(M + (p)))
/* slot s counted from mem's start (a data word's: they don't move with the frame) */
#define rio_ABS(s) (((rio_Val *)(void *)M)[s])
/* the instruction at pc, and the pc of one (pcs count 4-byte units: rio_codeFrames) */
#define rio_AT(pc) ((const rio_Instr *)(const void *)(units + (pc)))
#define rio_PC_OF(p) ((uint32_t)((const uint32_t *)(const void *)(p) - units))
/* the current pc, for errors */
#define rio_PC rio_PC_OF(ip)

/* from pc, in the frame starting at slot frame (R: the running proc's slots, an instruction's operands
   counted from there: rio_codeFrames) */
/* (GCC's global common subexpressions would merge every handler's computed goto into one, a branch more
   an instruction: GCC's own advice for interpreters is to turn it off) */
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((optimize("no-gcse")))
#endif
rio_HOT static int rio_interpret(rio_Vm *vm, uint32_t pc, uint32_t frame) {
  const uint32_t *units = (const uint32_t *)(const void *)vm->code;
  const rio_Instr *ip = rio_AT(pc);
  uint32_t *callStack = (uint32_t *)(void *)(vm->mem + vm->callStackAt); /* (each call: where it returns, and its caller's frame) */
  uint8_t *M = vm->mem;
  rio_Val *R = (rio_Val *)M + frame - rio_FRAME_BIAS;
  int depth = 0;

#ifdef rio_CGOTO
#define rio_LABEL(o) &&rio_L_##o,
#ifdef rio_HOT_SECTION
  rio_HOT_DATA /* the table too: a dispatch reads it every instruction */
#endif
#define rio_SLABEL(o, k) &&rio_L_s_##o,
#define rio_SLABEL_MORE(o, w, k) &&rio_L_s_##o,
  static const void *const labels[256] = {rio_OPS(rio_LABEL)[128] = rio_SHORT_OPS(rio_SLABEL) rio_SHORT_MORE(rio_SLABEL_MORE)};
#define rio_CASE(o) rio_L_##o:
#define rio_DISPATCH() goto *labels[ip->op]
#define rio_NEXT() \
  do {             \
    ip++;          \
    rio_DISPATCH(); \
  } while (0)
  rio_DISPATCH();
#else
#define rio_CASE(o) case rio_Op_##o:
#define rio_DISPATCH() continue
#define rio_NEXT() \
  {                \
    ip++;          \
    continue;      \
  }
  for (;;) switch (ip->op) {
#endif

/* to pc c; a backward jump checks for rio_interrupt */
#define rio_JUMP()                                               \
  {                                                              \
    if (ip->c <= rio_PC && vm->interrupted) goto interrupted;    \
    ip = rio_AT(ip->c);                                          \
    rio_DISPATCH();                                              \
  }
/* A loop's bottom always jumps back, so it can skip rio_JUMP's direction test. Measured: 5-12% faster
   on the RP2350 (Cortex-M33), but 25% slower on x86 with clang (code layout), so only on ARM. */
#if defined(__arm__) || defined(__thumb__)
#define rio_LOOP()                         \
  {                                        \
    if (vm->interrupted) goto interrupted; \
    ip = rio_AT(ip->c);                    \
    rio_DISPATCH();                        \
  }
#else
#define rio_LOOP() rio_JUMP()
#endif
/* an instruction followed by a data word */
#define rio_NEXT2() \
  {                 \
    ip += 2;        \
    rio_DISPATCH(); \
  }

  rio_CASE(mov) R[(int16_t)ip->a] = R[(int16_t)ip->b]; rio_NEXT();
  rio_CASE(add) R[(int16_t)ip->a].u = R[(int16_t)ip->b].u + R[(int16_t)ip->c].u; rio_NEXT();
  rio_CASE(sub) R[(int16_t)ip->a].u = R[(int16_t)ip->b].u - R[(int16_t)ip->c].u; rio_NEXT();
  rio_CASE(mul) R[(int16_t)ip->a].u = R[(int16_t)ip->b].u * R[(int16_t)ip->c].u; rio_NEXT();
  rio_CASE(div) {
    int32_t y = R[(int16_t)ip->c].i;
    if (!y) return rio_runtimeError(vm, "division by zero", rio_PC);
    R[(int16_t)ip->a].i = y == -1 ? (int32_t)(0u - R[(int16_t)ip->b].u) : R[(int16_t)ip->b].i / y; /* (INT_MIN / -1 wraps) */
    rio_NEXT();
  }
  rio_CASE(mod) {
    int32_t y = R[(int16_t)ip->c].i;
    if (!y) return rio_runtimeError(vm, "division by zero", rio_PC);
    R[(int16_t)ip->a].i = y == -1 ? 0 : R[(int16_t)ip->b].i % y;
    rio_NEXT();
  }
  rio_CASE(and) R[(int16_t)ip->a].u = R[(int16_t)ip->b].u & R[(int16_t)ip->c].u; rio_NEXT();
  rio_CASE(or) R[(int16_t)ip->a].u = R[(int16_t)ip->b].u | R[(int16_t)ip->c].u; rio_NEXT();
  rio_CASE(xor) R[(int16_t)ip->a].u = R[(int16_t)ip->b].u ^ R[(int16_t)ip->c].u; rio_NEXT();
  rio_CASE(shl) R[(int16_t)ip->a].u = R[(int16_t)ip->b].u << (R[(int16_t)ip->c].u & 31); rio_NEXT();
  rio_CASE(shr) R[(int16_t)ip->a].i = R[(int16_t)ip->b].i >> (R[(int16_t)ip->c].u & 31); rio_NEXT();
  rio_CASE(ushr) R[(int16_t)ip->a].u = R[(int16_t)ip->b].u >> (R[(int16_t)ip->c].u & 31); rio_NEXT();
  rio_CASE(neg) R[(int16_t)ip->a].u = 0u - R[(int16_t)ip->b].u; rio_NEXT();
  rio_CASE(bnot) R[(int16_t)ip->a].u = ~R[(int16_t)ip->b].u; rio_NEXT();
  rio_CASE(not) R[(int16_t)ip->a].i = !R[(int16_t)ip->b].i; rio_NEXT();
  rio_CASE(fadd) R[(int16_t)ip->a].f = R[(int16_t)ip->b].f + R[(int16_t)ip->c].f; rio_NEXT();
  rio_CASE(fsub) R[(int16_t)ip->a].f = R[(int16_t)ip->b].f - R[(int16_t)ip->c].f; rio_NEXT();
  rio_CASE(fmul) R[(int16_t)ip->a].f = R[(int16_t)ip->b].f * R[(int16_t)ip->c].f; rio_NEXT();
  rio_CASE(fdiv) R[(int16_t)ip->a].f = R[(int16_t)ip->b].f / R[(int16_t)ip->c].f; rio_NEXT();
  rio_CASE(fneg) R[(int16_t)ip->a].f = -R[(int16_t)ip->b].f; rio_NEXT();
  rio_CASE(eq) R[(int16_t)ip->a].i = R[(int16_t)ip->b].i == R[(int16_t)ip->c].i; rio_NEXT();
  rio_CASE(ne) R[(int16_t)ip->a].i = R[(int16_t)ip->b].i != R[(int16_t)ip->c].i; rio_NEXT();
  rio_CASE(lt) R[(int16_t)ip->a].i = R[(int16_t)ip->b].i < R[(int16_t)ip->c].i; rio_NEXT();
  rio_CASE(le) R[(int16_t)ip->a].i = R[(int16_t)ip->b].i <= R[(int16_t)ip->c].i; rio_NEXT();
  rio_CASE(feq) R[(int16_t)ip->a].i = R[(int16_t)ip->b].f == R[(int16_t)ip->c].f; rio_NEXT();
  rio_CASE(fne) R[(int16_t)ip->a].i = R[(int16_t)ip->b].f != R[(int16_t)ip->c].f; rio_NEXT();
  rio_CASE(flt) R[(int16_t)ip->a].i = R[(int16_t)ip->b].f < R[(int16_t)ip->c].f; rio_NEXT();
  rio_CASE(fle) R[(int16_t)ip->a].i = R[(int16_t)ip->b].f <= R[(int16_t)ip->c].f; rio_NEXT();
  rio_CASE(scmp) {
    uint32_t m = R[(int16_t)ip->b + 1].u, n = R[(int16_t)ip->c + 1].u;
    int k = memcmp(M + R[(int16_t)ip->b].u, M + R[(int16_t)ip->c].u, m < n ? m : n);
    R[(int16_t)ip->a].i = k ? (k < 0 ? -1 : 1) : (m < n ? -1 : m > n);
    rio_NEXT();
  }
  rio_CASE(seq) {
    const rio_Val *s = &R[(int16_t)ip->b], *t = &R[(int16_t)ip->c];
    R[(int16_t)ip->a].i = s[1].i == t[1].i && !memcmp(M + s[0].i, M + t[0].i, (size_t)s[1].i);
    rio_NEXT();
  }
  rio_CASE(sne) {
    const rio_Val *s = &R[(int16_t)ip->b], *t = &R[(int16_t)ip->c];
    R[(int16_t)ip->a].i = !(s[1].i == t[1].i && !memcmp(M + s[0].i, M + t[0].i, (size_t)s[1].i));
    rio_NEXT();
  }
  rio_CASE(itof) R[(int16_t)ip->a].f = (float)R[(int16_t)ip->b].i; rio_NEXT();
  rio_CASE(ftoi) R[(int16_t)ip->a].i = rio_floatToInt(R[(int16_t)ip->b].f); rio_NEXT();
  rio_CASE(ldw) R[(int16_t)ip->a] = rio_WORD(R[(int16_t)ip->b].i + ip->c); rio_NEXT();
  rio_CASE(ldb) R[(int16_t)ip->a].i = M[R[(int16_t)ip->b].i + ip->c]; rio_NEXT();
  rio_CASE(lea) R[(int16_t)ip->a].i = R[(int16_t)ip->b].i + ip->c; rio_NEXT();
  rio_CASE(m1) R[(int16_t)ip->a].f = rio_math1[ip->x](R[(int16_t)ip->b].f); rio_NEXT();
  rio_CASE(m2) R[(int16_t)ip->a].f = rio_math2[ip->x](R[(int16_t)ip->b].f, R[(int16_t)ip->c].f); rio_NEXT();
  rio_CASE(iabs) R[(int16_t)ip->a].u = R[(int16_t)ip->b].i < 0 ? 0u - R[(int16_t)ip->b].u : R[(int16_t)ip->b].u; rio_NEXT();
  rio_CASE(imin) R[(int16_t)ip->a].i = R[(int16_t)ip->b].i < R[(int16_t)ip->c].i ? R[(int16_t)ip->b].i : R[(int16_t)ip->c].i; rio_NEXT();
  rio_CASE(imax) R[(int16_t)ip->a].i = R[(int16_t)ip->b].i > R[(int16_t)ip->c].i ? R[(int16_t)ip->b].i : R[(int16_t)ip->c].i; rio_NEXT();
  rio_CASE(fmin) R[(int16_t)ip->a].f = R[(int16_t)ip->b].f < R[(int16_t)ip->c].f ? R[(int16_t)ip->b].f : R[(int16_t)ip->c].f; rio_NEXT();
  rio_CASE(fmax) R[(int16_t)ip->a].f = R[(int16_t)ip->b].f > R[(int16_t)ip->c].f ? R[(int16_t)ip->b].f : R[(int16_t)ip->c].f; rio_NEXT();

  /* indexing: slice b (address, length) at index c; the data word has the element size, and an
     offset into the element in a */
  rio_CASE(ldx) {
    uint32_t i = R[(int16_t)ip->c].u;
    if (i >= R[(int16_t)ip->b + 1].u) return rio_runtimeError(vm, "index out of bounds", rio_PC);
    R[(int16_t)ip->a] = rio_WORD(R[(int16_t)ip->b].u + i * rio_dataSize(ip + 1) + ip[1].a);
    rio_NEXT2();
  }
  rio_CASE(ldxb) {
    uint32_t i = R[(int16_t)ip->c].u;
    if (i >= R[(int16_t)ip->b + 1].u) return rio_runtimeError(vm, "index out of bounds", rio_PC);
    R[(int16_t)ip->a].u = M[R[(int16_t)ip->b].u + i * rio_dataSize(ip + 1) + ip[1].a];
    rio_NEXT2();
  }
  rio_CASE(mov2) { /* both read first: the pairs may overlap */
    rio_Val x = R[(int16_t)ip->b], y = R[(int16_t)ip->b + 1];
    R[(int16_t)ip->a] = x;
    R[(int16_t)ip->a + 1] = y;
    rio_NEXT();
  }
  rio_CASE(ldw2) {
    int32_t p = R[(int16_t)ip->b].i + ip->c;
    rio_Val x = rio_WORD(p), y = rio_WORD(p + 4);
    R[(int16_t)ip->a] = x;
    R[(int16_t)ip->a + 1] = y;
    rio_NEXT();
  }
  rio_CASE(stw) rio_WORD(R[(int16_t)ip->a].i + ip->c) = R[(int16_t)ip->b]; rio_NEXT();
  rio_CASE(stb) M[R[(int16_t)ip->a].i + ip->c] = (uint8_t)R[(int16_t)ip->b].u; rio_NEXT();
  rio_CASE(stw2) {
    int32_t p = R[(int16_t)ip->a].i + ip->c;
    rio_WORD(p) = R[(int16_t)ip->b];
    rio_WORD(p + 4) = R[(int16_t)ip->b + 1];
    rio_NEXT();
  }
  rio_CASE(stx) {
    uint32_t i = R[(int16_t)ip->b].u;
    if (i >= R[(int16_t)ip->a + 1].u) return rio_runtimeError(vm, "index out of bounds", rio_PC);
    rio_WORD(R[(int16_t)ip->a].u + i * rio_dataSize(ip + 1) + ip[1].a) = R[(int16_t)ip->c];
    rio_NEXT2();
  }
  rio_CASE(stxb) {
    uint32_t i = R[(int16_t)ip->b].u;
    if (i >= R[(int16_t)ip->a + 1].u) return rio_runtimeError(vm, "index out of bounds", rio_PC);
    M[R[(int16_t)ip->a].u + i * rio_dataSize(ip + 1) + ip[1].a] = (uint8_t)R[(int16_t)ip->c].u;
    rio_NEXT2();
  }
  rio_CASE(idx) {
    uint32_t i = R[(int16_t)ip->c].u;
    if (i >= R[(int16_t)ip->b + 1].u) return rio_runtimeError(vm, "index out of bounds", rio_PC);
    R[(int16_t)ip->a].u = R[(int16_t)ip->b].u + i * rio_dataSize(ip + 1);
    rio_NEXT2();
  }
  rio_CASE(slice) {
    int32_t lo = R[(int16_t)ip->c].i, hi = rio_ABS(ip[1].a).i, n = R[(int16_t)ip->b + 1].i;
    if (lo < 0 || hi < lo || hi > n) return rio_runtimeError(vm, "slice out of bounds", rio_PC);
    R[(int16_t)ip->a].i = R[(int16_t)ip->b].i + lo * (int32_t)rio_dataSize(ip + 1);
    R[(int16_t)ip->a + 1].i = hi - lo;
    rio_NEXT2();
  }
  rio_CASE(copy) memmove(M + R[(int16_t)ip->a].i, M + R[(int16_t)ip->b].i, ip->c); rio_NEXT();
  rio_CASE(copyv) memmove(M + R[(int16_t)ip->a].u, M + R[(int16_t)ip->b].u, R[(int16_t)ip->c].u); rio_NEXT();
  rio_CASE(idxv) {
    uint32_t i = R[(int16_t)ip->c].u;
    if (i >= R[(int16_t)ip->b + 1].u) return rio_runtimeError(vm, "index out of bounds", rio_PC);
    R[(int16_t)ip->a].u = R[(int16_t)ip->b].u + i * R[(int16_t)ip->a].u;
    rio_NEXT();
  }
  rio_CASE(swapv) {
    uint8_t *x = M + R[(int16_t)ip->a].u, *y = M + R[(int16_t)ip->b].u, t;
    uint32_t n = R[(int16_t)ip->c].u;
    while (n--) t = *x, *x++ = *y, *y++ = t;
    rio_NEXT();
  }
  rio_CASE(movn) memmove(R + (int16_t)ip->a, R + (int16_t)ip->b, ip->c * sizeof(rio_Val)); rio_NEXT();
  rio_CASE(enumname) { /* a, a+1 = the name of value b (values 0 to n - 1): their names at data a */
    uint32_t n = rio_dataSize(ip + 1), i = R[(int16_t)ip->b].u, names = ip[1].a;
    if (i < n) {
      R[(int16_t)ip->a] = rio_ABS(names + 2 * i); /* (a data word's slots: from mem's start) */
      R[(int16_t)ip->a + 1] = rio_ABS(names + 2 * i + 1);
    } else {
      R[(int16_t)ip->a].u = 0;
      R[(int16_t)ip->a + 1].u = 0;
    }
    rio_NEXT2();
  }
  rio_CASE(enumck) { /* is b one of the values 0 to n - 1? x: a runtime error if not, else a = the answer */
    uint32_t n = rio_dataSize(ip + 1), i = R[(int16_t)ip->b].u;
    if (ip->x) {
      if (i >= n) return rio_runtimeError(vm, "not a value of that enum", rio_PC);
    } else R[(int16_t)ip->a].i = i < n;
    rio_NEXT2();
  }
  rio_CASE(zero) memset(M + R[(int16_t)ip->a].i, 0, rio_dataSize(ip)); rio_NEXT();

/* n numbers d[i] = l[i] op r[i]; x: 16 l is a scalar, 32 r is a scalar (the value itself, read once:
   it steps by 0), else l and r are addresses */
#define rio_ELEMENTWISE(body)                                                         \
  {                                                                                   \
    rio_Val lv_ = R[(int16_t)ip->b], rv_ = R[(int16_t)ip->c];                                           \
    rio_Val *d_ = &rio_WORD(R[(int16_t)ip->a].u);                                              \
    rio_Val *l_ = ip->x & 16 ? &lv_ : &rio_WORD(lv_.u), *r_ = ip->x & 32 ? &rv_ : &rio_WORD(rv_.u); \
    uint32_t n_ = rio_dataSize(ip + 1), i_, lStep_ = ip->x & 16 ? 0 : 1, rStep_ = ip->x & 32 ? 0 : 1; \
    for (i_ = 0; i_ < n_; i_++) body;                                                 \
    rio_NEXT2();                                                                      \
  }
#define rio_L l_[i_ * lStep_]
#define rio_R r_[i_ * rStep_]
  rio_CASE(vadd) rio_ELEMENTWISE(d_[i_].u = rio_L.u + rio_R.u)
  rio_CASE(vsub) rio_ELEMENTWISE(d_[i_].u = rio_L.u - rio_R.u)
  rio_CASE(vmul) rio_ELEMENTWISE(d_[i_].u = rio_L.u * rio_R.u)
  rio_CASE(vdiv) rio_ELEMENTWISE({
    int32_t y = rio_R.i;
    if (!y) return rio_runtimeError(vm, "division by zero", rio_PC);
    d_[i_].i = y == -1 ? (int32_t)(0u - rio_L.u) : rio_L.i / y;
  })
  rio_CASE(vmod) rio_ELEMENTWISE({
    int32_t y = rio_R.i;
    if (!y) return rio_runtimeError(vm, "division by zero", rio_PC);
    d_[i_].i = y == -1 ? 0 : rio_L.i % y;
  })
  rio_CASE(fvadd) rio_ELEMENTWISE(d_[i_].f = rio_L.f + rio_R.f)
  rio_CASE(fvsub) rio_ELEMENTWISE(d_[i_].f = rio_L.f - rio_R.f)
  rio_CASE(fvmul) rio_ELEMENTWISE(d_[i_].f = rio_L.f * rio_R.f)
  rio_CASE(fvdiv) rio_ELEMENTWISE(d_[i_].f = rio_L.f / rio_R.f)

/* a builder view in slots s..s+2: (address of the length word, address of the data, capacity); its length,
   never above its capacity */
#define rio_LIST_LEN(s) (rio_WORD(R[(int16_t)(s)].u).u < R[(int16_t)(s) + 2].u ? rio_WORD(R[(int16_t)(s)].u).u : R[(int16_t)(s) + 2].u)
  /* (a builder's element size: its data's, or a slot's when the data names one, for a generic proc's T) */
#define rio_ELEM_SIZE(ip) ((ip)[1].a ? rio_ABS((ip)[1].a).u : rio_dataSize((ip) + 1))
  rio_CASE(lidx) {
    uint32_t i = R[(int16_t)ip->c].u;
    if (i >= rio_LIST_LEN(ip->b)) return rio_runtimeError(vm, "index out of bounds", rio_PC);
    R[(int16_t)ip->a].u = R[(int16_t)ip->b + 1].u + i * rio_ELEM_SIZE(ip);
    rio_NEXT2();
  }
  rio_CASE(pusha) {
    uint32_t lenAt = R[(int16_t)ip->b].u, cap = R[(int16_t)ip->b + 2].u, n = rio_LIST_LEN(ip->b);
    if (n >= cap) R[(int16_t)ip->a].u = 0;
    else {
      rio_WORD(lenAt).u = n + 1;
      R[(int16_t)ip->a].u = R[(int16_t)ip->b + 1].u + n * rio_ELEM_SIZE(ip);
    }
    rio_NEXT2();
  }
  rio_CASE(pushs) {
    uint32_t lenAt = R[(int16_t)ip->b].u, cap = R[(int16_t)ip->b + 2].u, n = rio_LIST_LEN(ip->b);
    uint32_t from = R[(int16_t)ip->c].u, k = R[(int16_t)ip->c + 1].u, size = rio_ELEM_SIZE(ip);
    if (k > cap - n) R[(int16_t)ip->a].i = 0;
    else {
      memmove(M + R[(int16_t)ip->b + 1].u + n * size, M + from, k * size);
      rio_WORD(lenAt).u = n + k;
      R[(int16_t)ip->a].i = 1;
    }
    rio_NEXT2();
  }
  rio_CASE(pushv) { /* by its descriptor (data), all of it or nothing */
    rio_Out o;
    uint32_t lenAt = R[(int16_t)ip->b].u, n = rio_LIST_LEN(ip->b);
    o.out = M + R[(int16_t)ip->b + 1].u; o.len = n; o.cap = R[(int16_t)ip->b + 2].u; o.capped = 0; o.full = 0;
    o.spec = ip[1].a ? rio_ABS(ip[1].a).u : 0;
    rio_printValue(M, &o, rio_dataSize(ip + 1), R[(int16_t)ip->c].u, R[(int16_t)ip->c + 1].u, R[(int16_t)ip->c + 2].u);
    if (o.full) R[(int16_t)ip->a].i = 0;
    else {
      rio_WORD(lenAt).u = o.len;
      R[(int16_t)ip->a].i = 1;
    }
    rio_NEXT2();
  }
  rio_CASE(pushf) { /* as its settings say (data: the slot of their address), all of it or nothing */
    rio_Out o;
    uint32_t lenAt = R[(int16_t)ip->b].u, n = rio_LIST_LEN(ip->b);
    o.out = M + R[(int16_t)ip->b + 1].u; o.len = n; o.cap = R[(int16_t)ip->b + 2].u; o.capped = 0; o.full = 0; o.spec = 0;
    rio_outFormatted(M, &o, rio_ABS(rio_dataSize(ip + 1)).u, ip->x, R[(int16_t)ip->c].u, ip->x == 3 ? R[(int16_t)ip->c + 1].u : 0);
    if (o.full) R[(int16_t)ip->a].i = 0;
    else {
      rio_WORD(lenAt).u = o.len;
      R[(int16_t)ip->a].i = 1;
    }
    rio_NEXT2();
  }
  rio_CASE(pusht) {
    char text[40];
    uint32_t lenAt = R[(int16_t)ip->b].u, cap = R[(int16_t)ip->b + 2].u, n = rio_LIST_LEN(ip->b), k;
    if (ip->x == 1) k = (uint32_t)rio_formatFloat(text, R[(int16_t)ip->c].f);
    else if (ip->x == 2) {
      k = R[(int16_t)ip->c].i ? 4u : 5u;
      memcpy(text, R[(int16_t)ip->c].i ? "true" : "false", k);
    } else k = (uint32_t)rio_formatInt(text, R[(int16_t)ip->c].i);
    if (k > cap - n) R[(int16_t)ip->a].i = 0;
    else {
      memcpy(M + R[(int16_t)ip->b + 1].u + n, text, k);
      rio_WORD(lenAt).u = n + k;
      R[(int16_t)ip->a].i = 1;
    }
    rio_NEXT();
  }
  rio_CASE(popa) {
    uint32_t n = rio_LIST_LEN(ip->b);
    if (!n) return rio_runtimeError(vm, "pop from empty builder", rio_PC);
    rio_WORD(R[(int16_t)ip->b].u).u = --n;
    R[(int16_t)ip->a].u = R[(int16_t)ip->b + 1].u + n * rio_ELEM_SIZE(ip);
    rio_NEXT2();
  }
  rio_CASE(lrem) {
    uint32_t data = R[(int16_t)ip->a + 1].u, n = rio_LIST_LEN(ip->a), i = R[(int16_t)ip->b].u, size = rio_ELEM_SIZE(ip);
    if (i >= n) return rio_runtimeError(vm, "index out of bounds", rio_PC);
    memmove(M + data + i * size, M + data + (i + 1) * size, (n - i - 1) * size);
    rio_WORD(R[(int16_t)ip->a].u).u = n - 1;
    rio_NEXT2();
  }
  rio_CASE(lswap) {
    uint32_t data = R[(int16_t)ip->a + 1].u, n = rio_LIST_LEN(ip->a), i = R[(int16_t)ip->b].u, size = rio_ELEM_SIZE(ip);
    if (i >= n) return rio_runtimeError(vm, "index out of bounds", rio_PC);
    if (i != --n) memmove(M + data + i * size, M + data + n * size, size);
    rio_WORD(R[(int16_t)ip->a].u).u = n;
    rio_NEXT2();
  }
  rio_CASE(lview) {
    uint32_t data = R[(int16_t)ip->b + 1].u, n = rio_LIST_LEN(ip->b);
    R[(int16_t)ip->a].u = data;
    R[(int16_t)ip->a + 1].u = n;
    rio_NEXT();
  }

/* bytes n at byte at of the Blob in slots s, s+1: fail if they're not all in it */
#define rio_BYTES_IN(s, at, n, what) \
  if ((at) < 0 || (uint32_t)(n) > R[(int16_t)(s) + 1].u || (uint32_t)(at) > R[(int16_t)(s) + 1].u - (uint32_t)(n)) return rio_runtimeError(vm, what, rio_PC)
  rio_CASE(bpack) {
    int32_t at = R[(int16_t)ip->b].i;
    uint32_t from = R[(int16_t)ip->c].u, n = R[(int16_t)ip->c + 1].u, to;
    rio_BYTES_IN(ip->a, at, n, ip->x ? "unpack out of bounds" : "pack out of bounds");
    to = R[(int16_t)ip->a].u + (uint32_t)at;
    if (ip->x) memmove(M + from, M + to, n);
    else memmove(M + to, M + from, n);
    rio_NEXT();
  }
  rio_CASE(bchk) {
    uint32_t at = R[(int16_t)ip->a].u, n = R[(int16_t)ip->a + 1].u, size = R[(int16_t)ip->c].u, i;
    for (i = 0; size && i + size <= n; i += size)
      if (!rio_holdsOk(M, (uint32_t)(R + (int16_t)ip->b - (rio_Val *)M), at + i)) return rio_runtimeError(vm, "unpacked a value its type doesn't have", rio_PC);
    rio_NEXT();
  }
  rio_CASE(geti) {
    int32_t at = R[(int16_t)ip->c].i, bits = R[(int16_t)ip->c + 1].i;
    uint32_t n = (uint32_t)(bits < 0 ? -bits : bits) / 8, v = 0, i;
    const uint8_t *p;
    if (n < 1 || n > 4 || bits % 8) return rio_runtimeError(vm, "getInt takes 8, 16, 24 or 32 bits", rio_PC);
    rio_BYTES_IN(ip->b, at, n, "getInt out of bounds");
    p = M + R[(int16_t)ip->b].u + (uint32_t)at;
    for (i = 0; i < n; i++) v |= (uint32_t)p[i] << (8 * i);
    if (bits < 0 && n < 4 && (v >> (8 * n - 1) & 1)) v |= ~0u << (8 * n); /* (signed: the top bit spread) */
    R[(int16_t)ip->a].u = v;
    rio_NEXT();
  }
  rio_CASE(seti) {
    int32_t at = R[(int16_t)ip->c].i, bits = R[(int16_t)ip->c + 1].i;
    uint32_t n = (uint32_t)(bits < 0 ? -bits : bits) / 8, v = R[(int16_t)ip->b].u, i;
    uint8_t *p;
    if (n < 1 || n > 4 || bits % 8) return rio_runtimeError(vm, "setInt takes 8, 16, 24 or 32 bits", rio_PC);
    rio_BYTES_IN(ip->a, at, n, "setInt out of bounds");
    p = M + R[(int16_t)ip->a].u + (uint32_t)at;
    for (i = 0; i < n; i++) p[i] = (uint8_t)(v >> (8 * i));
    rio_NEXT();
  }
  rio_CASE(pnum) {
    int32_t base = ip->x ? 0 : R[(int16_t)ip->c].i ? R[(int16_t)ip->c].i : 10;
    uint32_t v = 0;
    int has;
    if (!ip->x && (base < 2 || base > 36)) return rio_runtimeError(vm, "toInt's base is 2 to 36", rio_PC);
    has = rio_parseNumber(M + R[(int16_t)ip->b].u, R[(int16_t)ip->b + 1].u, base, &v);
    R[(int16_t)ip->a].i = has;
    R[(int16_t)ip->a + 1].u = has ? v : 0;
    rio_NEXT();
  }

  rio_CASE(jmp) rio_JUMP();
  rio_CASE(jz) if (!R[(int16_t)ip->a].i) rio_JUMP(); rio_NEXT();
  rio_CASE(jnz) if (R[(int16_t)ip->a].i) rio_JUMP(); rio_NEXT();
  rio_CASE(jlt) if (R[(int16_t)ip->a].i < R[(int16_t)ip->b].i) rio_JUMP(); rio_NEXT();
  rio_CASE(jle) if (R[(int16_t)ip->a].i <= R[(int16_t)ip->b].i) rio_JUMP(); rio_NEXT();
  rio_CASE(jeq) if (R[(int16_t)ip->a].i == R[(int16_t)ip->b].i) rio_JUMP(); rio_NEXT();
  rio_CASE(jne) if (R[(int16_t)ip->a].i != R[(int16_t)ip->b].i) rio_JUMP(); rio_NEXT();
  rio_CASE(jflt) if (R[(int16_t)ip->a].f < R[(int16_t)ip->b].f) rio_JUMP(); rio_NEXT();
  rio_CASE(jfnlt) if (!(R[(int16_t)ip->a].f < R[(int16_t)ip->b].f)) rio_JUMP(); rio_NEXT();
  rio_CASE(jfnle) if (!(R[(int16_t)ip->a].f <= R[(int16_t)ip->b].f)) rio_JUMP(); rio_NEXT();
  rio_CASE(jfle) if (R[(int16_t)ip->a].f <= R[(int16_t)ip->b].f) rio_JUMP(); rio_NEXT();
  rio_CASE(jfeq) if (R[(int16_t)ip->a].f == R[(int16_t)ip->b].f) rio_JUMP(); rio_NEXT();
  rio_CASE(jfne) if (R[(int16_t)ip->a].f != R[(int16_t)ip->b].f) rio_JUMP(); rio_NEXT();
  rio_CASE(each) if ((R[(int16_t)ip->a].u += R[(int16_t)ip->b + 1].u) < R[(int16_t)ip->b].u) rio_LOOP(); rio_NEXT();
  rio_CASE(fori) if ((R[(int16_t)ip->a].i = (int32_t)(R[(int16_t)ip->a].u + 1u)) < R[(int16_t)ip->b].i) rio_LOOP(); rio_NEXT();
  rio_CASE(forby) { /* (unsigned distances: the counter is between first and last) */
    uint32_t v = R[(int16_t)ip->a].u, end = R[(int16_t)ip->b].u, k = R[(int16_t)ip->b + 1].u;
    if ((int32_t)k > 0 ? end - v >= k : v - end >= 0u - k) {
      R[(int16_t)ip->a].u = v + k;
      rio_LOOP();
    }
    rio_NEXT();
  }

  rio_CASE(call) { /* proc c */
    const rio_Proc *p = &vm->procs[ip->c];
    callStack[depth++] = rio_PC_OF(ip + 1);
    callStack[depth++] = (uint32_t)(R - (rio_Val *)M);
    if (vm->interrupted) goto interrupted;
    R = (rio_Val *)M + p->frameStart - rio_FRAME_BIAS;
    ip = rio_AT(p->entry);
    rio_DISPATCH();
  }
  rio_CASE(ret) {
    if (!depth) return 0;
    R = (rio_Val *)M + (int32_t)callStack[--depth];
    ip = rio_AT(callStack[--depth]);
    rio_DISPATCH();
  }
  rio_CASE(calli) { /* call proc value a: its c argument words, at b, go to its parameters */
    uint32_t proc = R[(int16_t)ip->a].u - 1u;
    if (proc >= (uint32_t)vm->procCount) return rio_runtimeError(vm, "this proc value isn't set", rio_PC);
    if (ip->c) memmove((rio_Val *)M + vm->procs[proc].frameStart, R + (int16_t)ip->b, ip->c * sizeof(rio_Val));
    if (vm->interrupted) goto interrupted;
    callStack[depth++] = rio_PC_OF(ip + 1);
    callStack[depth++] = (uint32_t)(R - (rio_Val *)M);
    R = (rio_Val *)M + vm->procs[proc].frameStart - rio_FRAME_BIAS;
    ip = rio_AT(vm->procs[proc].entry);
    rio_DISPATCH();
  }
  rio_CASE(getret) { /* proc value b's result */
    memmove(R + (int16_t)ip->a, (rio_Val *)M + vm->procs[R[(int16_t)ip->b].u - 1u].result, ip->c * sizeof(rio_Val));
    rio_NEXT();
  }
  rio_CASE(ffi) {
    if (vm->ffi[ip->c].pad) rio_ffiPadded(vm, &vm->ffi[ip->c], R + (int16_t)ip->a);
    else vm->ffi[ip->c].fn(vm, R + (int16_t)ip->a);
    if (vm->trapped) {
      vm->trapped = 0;
      return rio_runtimeError(vm, vm->errorText, rio_PC);
    }
    rio_NEXT();
  }
  rio_CASE(logi) rio_CASE(logf) rio_CASE(logs) rio_CASE(logb) rio_CASE(logv) rio_CASE(loge) rio_logOp(vm, R, ip); rio_NEXT();
  rio_CASE(logn) rio_logOp(vm, R, ip); rio_NEXT2();
  rio_CASE(fail) return rio_assertFailed(vm, (uint32_t)(R + (int16_t)ip->a - (rio_Val *)M), ip->x, rio_PC); /* (no message: slot 0, a constant 0) */
  rio_CASE(halt) return 0;
  rio_CASE(jit) { /* a loop compiled to native code: it says where to carry on, or what failed where */
    uint32_t to;
    if (vm->interrupted) goto interrupted;
    to = vm->jitFns[ip->a]((rio_Val *)M, vm); /* (native code names slots from mem's start) */
    if (to >> 16) return rio_jitFailed(vm, to);
    ip = rio_AT(to); /* (native code's pcs count units, as the vm's do) */
    rio_DISPATCH();
  }

  /* the short forms (internal.h: rio_Short): operands of 8 bits, slots from R as a wide one's are */
#define rio_SH ((const rio_Short *)(const void *)ip)
#define rio_SA R[rio_SH->a]
#define rio_SB R[rio_SH->b]
#define rio_SC R[rio_SH->c]
#define rio_SNEXT() \
  {                 \
    ip = (const rio_Instr *)(const void *)((const uint32_t *)(const void *)ip + 1); \
    rio_DISPATCH(); \
  }
#define rio_SJUMP() \
  {                 \
    int32_t d_ = (int8_t)rio_SH->c; \
    if (d_ <= 0 && vm->interrupted) goto interrupted; \
    ip = (const rio_Instr *)(const void *)((const uint32_t *)(const void *)ip + d_); \
    rio_DISPATCH(); \
  }
  rio_CASE(s_mov) rio_SA = rio_SB; rio_SNEXT();
  /* K: a slot under 256 from mem's start (a constant); G: any, 16 bits in b and c, the frame's in a */
#define rio_SK rio_ABS(rio_SH->c)
#define rio_SG (rio_SH->b | (uint32_t)rio_SH->c << 8)
  rio_CASE(s_movg) rio_SA = rio_ABS(rio_SG); rio_SNEXT();
  rio_CASE(s_stg) rio_ABS(rio_SG) = rio_SA; rio_SNEXT();
  rio_CASE(s_mov2g) {
    rio_Val x = rio_ABS(rio_SG), y = rio_ABS(rio_SG + 1);
    R[rio_SH->a] = x;
    R[rio_SH->a + 1] = y;
    rio_SNEXT();
  }
  rio_CASE(s_st2g) {
    rio_Val x = R[rio_SH->a], y = R[rio_SH->a + 1];
    rio_ABS(rio_SG) = x;
    rio_ABS(rio_SG + 1) = y;
    rio_SNEXT();
  }
  rio_CASE(s_addk) rio_SA.u = rio_SB.u + rio_SK.u; rio_SNEXT();
  rio_CASE(s_subk) rio_SA.u = rio_SB.u - rio_SK.u; rio_SNEXT();
  rio_CASE(s_mulk) rio_SA.u = rio_SB.u * rio_SK.u; rio_SNEXT();
  rio_CASE(s_andk) rio_SA.u = rio_SB.u & rio_SK.u; rio_SNEXT();
  rio_CASE(s_ork) rio_SA.u = rio_SB.u | rio_SK.u; rio_SNEXT();
  rio_CASE(s_eqk) rio_SA.i = rio_SB.i == rio_SK.i; rio_SNEXT();
  rio_CASE(s_nek) rio_SA.i = rio_SB.i != rio_SK.i; rio_SNEXT();
  rio_CASE(s_ltk) rio_SA.i = rio_SB.i < rio_SK.i; rio_SNEXT();
  rio_CASE(s_lek) rio_SA.i = rio_SB.i <= rio_SK.i; rio_SNEXT();
  rio_CASE(s_imink) { int32_t k = rio_SK.i; rio_SA.i = rio_SB.i < k ? rio_SB.i : k; rio_SNEXT(); }
  rio_CASE(s_imaxk) { int32_t k = rio_SK.i; rio_SA.i = rio_SB.i > k ? rio_SB.i : k; rio_SNEXT(); }
  rio_CASE(s_faddk) rio_SA.f = rio_SB.f + rio_SK.f; rio_SNEXT();
  rio_CASE(s_fsubk) rio_SA.f = rio_SB.f - rio_SK.f; rio_SNEXT();
  rio_CASE(s_fmulk) rio_SA.f = rio_SB.f * rio_SK.f; rio_SNEXT();
  rio_CASE(s_fltk) rio_SA.i = rio_SB.f < rio_SK.f; rio_SNEXT();
  rio_CASE(s_flek) rio_SA.i = rio_SB.f <= rio_SK.f; rio_SNEXT();
  rio_CASE(s_add) rio_SA.u = rio_SB.u + rio_SC.u; rio_SNEXT();
  rio_CASE(s_sub) rio_SA.u = rio_SB.u - rio_SC.u; rio_SNEXT();
  rio_CASE(s_mul) rio_SA.u = rio_SB.u * rio_SC.u; rio_SNEXT();
  rio_CASE(s_and) rio_SA.u = rio_SB.u & rio_SC.u; rio_SNEXT();
  rio_CASE(s_or) rio_SA.u = rio_SB.u | rio_SC.u; rio_SNEXT();
  rio_CASE(s_eq) rio_SA.i = rio_SB.i == rio_SC.i; rio_SNEXT();
  rio_CASE(s_ne) rio_SA.i = rio_SB.i != rio_SC.i; rio_SNEXT();
  rio_CASE(s_lt) rio_SA.i = rio_SB.i < rio_SC.i; rio_SNEXT();
  rio_CASE(s_le) rio_SA.i = rio_SB.i <= rio_SC.i; rio_SNEXT();
  rio_CASE(s_imin) rio_SA.i = rio_SB.i < rio_SC.i ? rio_SB.i : rio_SC.i; rio_SNEXT();
  rio_CASE(s_imax) rio_SA.i = rio_SB.i > rio_SC.i ? rio_SB.i : rio_SC.i; rio_SNEXT();
  rio_CASE(s_fadd) rio_SA.f = rio_SB.f + rio_SC.f; rio_SNEXT();
  rio_CASE(s_fsub) rio_SA.f = rio_SB.f - rio_SC.f; rio_SNEXT();
  rio_CASE(s_fmul) rio_SA.f = rio_SB.f * rio_SC.f; rio_SNEXT();
  rio_CASE(s_flt) rio_SA.i = rio_SB.f < rio_SC.f; rio_SNEXT();
  rio_CASE(s_fle) rio_SA.i = rio_SB.f <= rio_SC.f; rio_SNEXT();
  rio_CASE(s_ldw) rio_SA = rio_WORD(rio_SB.i + rio_SH->c); rio_SNEXT();
  rio_CASE(s_ldb) rio_SA.i = M[rio_SB.i + rio_SH->c]; rio_SNEXT();
  rio_CASE(s_lea) rio_SA.i = rio_SB.i + rio_SH->c; rio_SNEXT();
  rio_CASE(s_ldw2) {
    int32_t p = rio_SB.i + rio_SH->c;
    rio_Val x = rio_WORD(p), y = rio_WORD(p + 4);
    R[rio_SH->a] = x;
    R[rio_SH->a + 1] = y;
    rio_SNEXT();
  }
  rio_CASE(s_stw) rio_WORD(rio_SA.i + rio_SH->c) = rio_SB; rio_SNEXT();
  rio_CASE(s_stb) M[rio_SA.i + rio_SH->c] = (uint8_t)rio_SB.u; rio_SNEXT();
  rio_CASE(s_stw2) {
    int32_t p = rio_SA.i + rio_SH->c;
    rio_WORD(p) = R[rio_SH->b];
    rio_WORD(p + 4) = R[rio_SH->b + 1];
    rio_SNEXT();
  }
  rio_CASE(s_movn) memmove(&rio_SA, &rio_SB, rio_SH->c * sizeof(rio_Val)); rio_SNEXT();
  rio_CASE(s_mov2) { /* both read first: the pairs may overlap */
    rio_Val x = R[rio_SH->b], y = R[rio_SH->b + 1];
    R[rio_SH->a] = x;
    R[rio_SH->a + 1] = y;
    rio_SNEXT();
  }
  rio_CASE(s_jmp) rio_SJUMP();
  rio_CASE(s_jz) if (!rio_SA.i) rio_SJUMP(); rio_SNEXT();
  rio_CASE(s_jnz) if (rio_SA.i) rio_SJUMP(); rio_SNEXT();
  rio_CASE(s_jeq) if (rio_SA.i == rio_SB.i) rio_SJUMP(); rio_SNEXT();
  rio_CASE(s_jne) if (rio_SA.i != rio_SB.i) rio_SJUMP(); rio_SNEXT();
  rio_CASE(s_jlt) if (rio_SA.i < rio_SB.i) rio_SJUMP(); rio_SNEXT();
  rio_CASE(s_jle) if (rio_SA.i <= rio_SB.i) rio_SJUMP(); rio_SNEXT();
  rio_CASE(s_fori) if ((rio_SA.i = (int32_t)(rio_SA.u + 1u)) < rio_SB.i) rio_SJUMP(); rio_SNEXT();
  rio_CASE(s_each) if ((rio_SA.u += R[rio_SH->b + 1].u) < rio_SB.u) rio_SJUMP(); rio_SNEXT();
  rio_CASE(s_call) { /* proc c */
    const rio_Proc *p = &vm->procs[rio_SH->c];
    callStack[depth++] = rio_PC + 1;
    callStack[depth++] = (uint32_t)(R - (rio_Val *)M);
    if (vm->interrupted) goto interrupted;
    R = (rio_Val *)M + p->frameStart - rio_FRAME_BIAS;
    ip = rio_AT(p->entry);
    rio_DISPATCH();
  }
  rio_CASE(s_ret) {
    if (!depth) return 0;
    R = (rio_Val *)M + (int32_t)callStack[--depth];
    ip = rio_AT(callStack[--depth]);
    rio_DISPATCH();
  }
  rio_CASE(s_jit) { /* a loop compiled to native code (the jit op's short form) */
    uint32_t to;
    if (vm->interrupted) goto interrupted;
    to = vm->jitFns[rio_SH->a]((rio_Val *)M, vm);
    if (to >> 16) return rio_jitFailed(vm, to);
    ip = rio_AT(to);
    rio_DISPATCH();
  }
  rio_CASE(s_ffi) {
    if (vm->ffi[rio_SH->c].pad) rio_ffiPadded(vm, &vm->ffi[rio_SH->c], &rio_SA);
    else vm->ffi[rio_SH->c].fn(vm, &rio_SA);
    if (vm->trapped) {
      vm->trapped = 0;
      return rio_runtimeError(vm, vm->errorText, rio_PC);
    }
    rio_SNEXT();
  }
#ifndef rio_CGOTO
  default: return rio_runtimeError(vm, "bad opcode", rio_PC);
  }
#endif

interrupted:
  vm->interrupted = 0;
  return rio_runtimeError(vm, "interrupted", rio_PC);
}
#undef rio_LABEL
#undef rio_CASE
#undef rio_DISPATCH
#undef rio_NEXT
#undef rio_NEXT2
#undef rio_JUMP
#undef rio_LOOP
#undef rio_ELEMENTWISE
#undef rio_L
#undef rio_R
#undef rio_LIST_LEN
#undef rio_WORD
#undef rio_ABS
#undef rio_PC
#undef rio_SLABEL
#undef rio_SJUMP
#undef rio_SNEXT
#undef rio_SC
#undef rio_SK
#undef rio_SG
#undef rio_SLABEL_MORE
#undef rio_SB
#undef rio_SA
#undef rio_SH

/* ------------------------------------------------------------------ entry points */

/* Frames are shared by procs that can't run at the same time: starting rio again from inside an FFI
   function would break that, so it isn't allowed. */
int rio_start(rio_Vm *vm, uint32_t pc, uint32_t base) {
  int result;
  if (vm->running) {
    rio_setError(vm, rio_ErrorKind_runtime, 0, 0, 0, "rio is already running (called from an FFI function?)");
    return -1;
  }
  vm->running = 1;
  result = rio_interpret(vm, pc, base);
  vm->running = 0;
  return result;
}

int rio_run(rio_Vm *vm) {
  if (!vm->running) vm->errorKind = rio_ErrorKind_none;
  vm->interrupted = 0;
  return vm->ready ? rio_start(vm, 0, 0) : -1;
}

int rio_call(rio_Vm *vm, int proc) {
  if (!vm->running) vm->errorKind = rio_ErrorKind_none;
  vm->interrupted = 0;
  return vm->ready && proc >= 0 && proc < vm->procCount ? rio_start(vm, vm->procs[proc].entry, vm->procs[proc].frameStart) : -1;
}
