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

static int rio_runtimeError(rio_Vm *vm, const char *message, uint32_t pc) {
  char copy[rio_ERROR_BUF_SIZE];
  if (message == vm->errorText) { /* a message from rio_trap: rio_setError rewrites errorText */
    memcpy(copy, message, sizeof copy);
    message = copy;
  }
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
  case rio_Op_logi: rio_logPut(vm, text, rio_formatInt(text, R[ip->a].i)); break;
  case rio_Op_logf: rio_logPut(vm, text, rio_formatFloat(text, R[ip->a].f)); break;
  case rio_Op_logs: { /* text, as shown (print.h): control bytes and non-UTF-8 as \xHH */
    rio_Out o;
    o.out = (uint8_t *)vm->logBuf; o.len = (uint32_t)vm->logLen; o.cap = rio_LOG_BUF_SIZE; o.capped = 1; o.full = 0; o.spec = 0;
    rio_outShown(&o, M + R[ip->a].u, R[ip->a + 1].u, 0);
    vm->logLen = (int)o.len;
    break;
  }
  case rio_Op_logb:
    if (R[ip->a].i) rio_logPut(vm, "true", 4);
    else rio_logPut(vm, "false", 5);
    break;
  case rio_Op_logn: { /* an enum value by name (values 0 to n - 1): c.. their (address, length) names */
    uint32_t n = rio_dataSize(ip + 1), i = R[ip->a].u;
    int32_t v = R[ip->a].i;
    if (i < n) rio_logPut(vm, (const char *)M + R[ip->c + 2 * i].i, R[ip->c + 2 * i + 1].i);
    else rio_logPut(vm, text, rio_formatInt(text, v));
    break;
  }
  case rio_Op_logv: { /* a struct, array, builder, union, ...: by its descriptor, as much as the line holds */
    rio_Out o;
    o.out = (uint8_t *)vm->logBuf; o.len = (uint32_t)vm->logLen; o.cap = rio_LOG_BUF_SIZE; o.capped = 1; o.full = 0; o.spec = 0; o.spec = 0;
    rio_printValue(M, &o, ip->b, R[ip->a].u, R[ip->a + 1].u, R[ip->a + 2].u);
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
  if (ip->op == rio_Op_m1) R[ip->a].f = rio_math1[ip->x](R[ip->b].f);
  else if (ip->op == rio_Op_m2) R[ip->a].f = rio_math2[ip->x](R[ip->b].f, R[ip->c].f);
  else rio_logOp(vm, R, ip);
}

/* ------------------------------------------------------------------ the interpreter */

/* the word at byte address p of mem */
#define rio_WORD(p) (*(rio_Val *)(M + (p)))
/* the current pc, for errors */
#define rio_PC ((uint32_t)(ip - code))

rio_HOT static int rio_interpret(rio_Vm *vm, uint32_t pc) {
  const rio_Instr *code = vm->code, *ip = code + pc;
  uint32_t *callStack = (uint32_t *)(void *)(vm->mem + vm->callStackAt);
  uint8_t *M = vm->mem;
  rio_Val *R = (rio_Val *)M;
  int depth = 0;

#ifdef rio_CGOTO
#define rio_LABEL(o) &&rio_L_##o,
#ifdef rio_HOT_SECTION
  rio_HOT_DATA /* the table too: a dispatch reads it every instruction */
#endif
  static const void *const labels[] = {rio_OPS(rio_LABEL)};
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
    if (ip->c <= (uint32_t)(ip - code) && vm->interrupted) goto interrupted; \
    ip = code + ip->c;                                           \
    rio_DISPATCH();                                              \
  }
/* A loop's bottom always jumps back, so it can skip rio_JUMP's direction test. Measured: 5-12% faster
   on the RP2350 (Cortex-M33), but 25% slower on x86 with clang (code layout), so only on ARM. */
#if defined(__arm__) || defined(__thumb__)
#define rio_LOOP()                         \
  {                                        \
    if (vm->interrupted) goto interrupted; \
    ip = code + ip->c;                     \
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

  rio_CASE(mov) R[ip->a] = R[ip->b]; rio_NEXT();
  rio_CASE(add) R[ip->a].u = R[ip->b].u + R[ip->c].u; rio_NEXT();
  rio_CASE(sub) R[ip->a].u = R[ip->b].u - R[ip->c].u; rio_NEXT();
  rio_CASE(mul) R[ip->a].u = R[ip->b].u * R[ip->c].u; rio_NEXT();
  rio_CASE(div) {
    int32_t y = R[ip->c].i;
    if (!y) return rio_runtimeError(vm, "division by zero", rio_PC);
    R[ip->a].i = y == -1 ? (int32_t)(0u - R[ip->b].u) : R[ip->b].i / y; /* (INT_MIN / -1 wraps) */
    rio_NEXT();
  }
  rio_CASE(mod) {
    int32_t y = R[ip->c].i;
    if (!y) return rio_runtimeError(vm, "division by zero", rio_PC);
    R[ip->a].i = y == -1 ? 0 : R[ip->b].i % y;
    rio_NEXT();
  }
  rio_CASE(and) R[ip->a].u = R[ip->b].u & R[ip->c].u; rio_NEXT();
  rio_CASE(or) R[ip->a].u = R[ip->b].u | R[ip->c].u; rio_NEXT();
  rio_CASE(xor) R[ip->a].u = R[ip->b].u ^ R[ip->c].u; rio_NEXT();
  rio_CASE(shl) R[ip->a].u = R[ip->b].u << (R[ip->c].u & 31); rio_NEXT();
  rio_CASE(shr) R[ip->a].i = R[ip->b].i >> (R[ip->c].u & 31); rio_NEXT();
  rio_CASE(neg) R[ip->a].u = 0u - R[ip->b].u; rio_NEXT();
  rio_CASE(bnot) R[ip->a].u = ~R[ip->b].u; rio_NEXT();
  rio_CASE(not) R[ip->a].i = !R[ip->b].i; rio_NEXT();
  rio_CASE(fadd) R[ip->a].f = R[ip->b].f + R[ip->c].f; rio_NEXT();
  rio_CASE(fsub) R[ip->a].f = R[ip->b].f - R[ip->c].f; rio_NEXT();
  rio_CASE(fmul) R[ip->a].f = R[ip->b].f * R[ip->c].f; rio_NEXT();
  rio_CASE(fdiv) R[ip->a].f = R[ip->b].f / R[ip->c].f; rio_NEXT();
  rio_CASE(fneg) R[ip->a].f = -R[ip->b].f; rio_NEXT();
  rio_CASE(eq) R[ip->a].i = R[ip->b].i == R[ip->c].i; rio_NEXT();
  rio_CASE(ne) R[ip->a].i = R[ip->b].i != R[ip->c].i; rio_NEXT();
  rio_CASE(lt) R[ip->a].i = R[ip->b].i < R[ip->c].i; rio_NEXT();
  rio_CASE(le) R[ip->a].i = R[ip->b].i <= R[ip->c].i; rio_NEXT();
  rio_CASE(feq) R[ip->a].i = R[ip->b].f == R[ip->c].f; rio_NEXT();
  rio_CASE(fne) R[ip->a].i = R[ip->b].f != R[ip->c].f; rio_NEXT();
  rio_CASE(flt) R[ip->a].i = R[ip->b].f < R[ip->c].f; rio_NEXT();
  rio_CASE(fle) R[ip->a].i = R[ip->b].f <= R[ip->c].f; rio_NEXT();
  rio_CASE(seq) {
    const rio_Val *s = &R[ip->b], *t = &R[ip->c];
    R[ip->a].i = s[1].i == t[1].i && !memcmp(M + s[0].i, M + t[0].i, (size_t)s[1].i);
    rio_NEXT();
  }
  rio_CASE(sne) {
    const rio_Val *s = &R[ip->b], *t = &R[ip->c];
    R[ip->a].i = !(s[1].i == t[1].i && !memcmp(M + s[0].i, M + t[0].i, (size_t)s[1].i));
    rio_NEXT();
  }
  rio_CASE(itof) R[ip->a].f = (float)R[ip->b].i; rio_NEXT();
  rio_CASE(ftoi) R[ip->a].i = rio_floatToInt(R[ip->b].f); rio_NEXT();
  rio_CASE(ldw) R[ip->a] = rio_WORD(R[ip->b].i + ip->c); rio_NEXT();
  rio_CASE(ldb) R[ip->a].i = M[R[ip->b].i + ip->c]; rio_NEXT();
  rio_CASE(lea) R[ip->a].i = R[ip->b].i + ip->c; rio_NEXT();
  rio_CASE(m1) R[ip->a].f = rio_math1[ip->x](R[ip->b].f); rio_NEXT();
  rio_CASE(m2) R[ip->a].f = rio_math2[ip->x](R[ip->b].f, R[ip->c].f); rio_NEXT();
  rio_CASE(iabs) R[ip->a].u = R[ip->b].i < 0 ? 0u - R[ip->b].u : R[ip->b].u; rio_NEXT();
  rio_CASE(imin) R[ip->a].i = R[ip->b].i < R[ip->c].i ? R[ip->b].i : R[ip->c].i; rio_NEXT();
  rio_CASE(imax) R[ip->a].i = R[ip->b].i > R[ip->c].i ? R[ip->b].i : R[ip->c].i; rio_NEXT();
  rio_CASE(fmin) R[ip->a].f = R[ip->b].f < R[ip->c].f ? R[ip->b].f : R[ip->c].f; rio_NEXT();
  rio_CASE(fmax) R[ip->a].f = R[ip->b].f > R[ip->c].f ? R[ip->b].f : R[ip->c].f; rio_NEXT();

  /* indexing: slice b (address, length) at index c; the data word has the element size, and an
     offset into the element in a */
  rio_CASE(ldx) {
    uint32_t i = R[ip->c].u;
    if (i >= R[ip->b + 1].u) return rio_runtimeError(vm, "index out of bounds", rio_PC);
    R[ip->a] = rio_WORD(R[ip->b].u + i * rio_dataSize(ip + 1) + ip[1].a);
    rio_NEXT2();
  }
  rio_CASE(ldxb) {
    uint32_t i = R[ip->c].u;
    if (i >= R[ip->b + 1].u) return rio_runtimeError(vm, "index out of bounds", rio_PC);
    R[ip->a].u = M[R[ip->b].u + i * rio_dataSize(ip + 1) + ip[1].a];
    rio_NEXT2();
  }
  rio_CASE(mov2) { /* both read first: the pairs may overlap */
    rio_Val x = R[ip->b], y = R[ip->b + 1];
    R[ip->a] = x;
    R[ip->a + 1] = y;
    rio_NEXT();
  }
  rio_CASE(ldw2) {
    int32_t p = R[ip->b].i + ip->c;
    rio_Val x = rio_WORD(p), y = rio_WORD(p + 4);
    R[ip->a] = x;
    R[ip->a + 1] = y;
    rio_NEXT();
  }
  rio_CASE(stw) rio_WORD(R[ip->a].i + ip->c) = R[ip->b]; rio_NEXT();
  rio_CASE(stb) M[R[ip->a].i + ip->c] = (uint8_t)R[ip->b].u; rio_NEXT();
  rio_CASE(stw2) {
    int32_t p = R[ip->a].i + ip->c;
    rio_WORD(p) = R[ip->b];
    rio_WORD(p + 4) = R[ip->b + 1];
    rio_NEXT();
  }
  rio_CASE(stx) {
    uint32_t i = R[ip->b].u;
    if (i >= R[ip->a + 1].u) return rio_runtimeError(vm, "index out of bounds", rio_PC);
    rio_WORD(R[ip->a].u + i * rio_dataSize(ip + 1) + ip[1].a) = R[ip->c];
    rio_NEXT2();
  }
  rio_CASE(stxb) {
    uint32_t i = R[ip->b].u;
    if (i >= R[ip->a + 1].u) return rio_runtimeError(vm, "index out of bounds", rio_PC);
    M[R[ip->a].u + i * rio_dataSize(ip + 1) + ip[1].a] = (uint8_t)R[ip->c].u;
    rio_NEXT2();
  }
  rio_CASE(idx) {
    uint32_t i = R[ip->c].u;
    if (i >= R[ip->b + 1].u) return rio_runtimeError(vm, "index out of bounds", rio_PC);
    R[ip->a].u = R[ip->b].u + i * rio_dataSize(ip + 1);
    rio_NEXT2();
  }
  rio_CASE(slice) {
    int32_t lo = R[ip->c].i, hi = R[ip[1].a].i, n = R[ip->b + 1].i;
    if (lo < 0 || hi < lo || hi > n) return rio_runtimeError(vm, "slice out of bounds", rio_PC);
    R[ip->a].i = R[ip->b].i + lo * (int32_t)rio_dataSize(ip + 1);
    R[ip->a + 1].i = hi - lo;
    rio_NEXT2();
  }
  rio_CASE(copy) memmove(M + R[ip->a].i, M + R[ip->b].i, ip->c); rio_NEXT();
  rio_CASE(movn) memmove(R + ip->a, R + ip->b, ip->c * sizeof(rio_Val)); rio_NEXT();
  rio_CASE(enumname) { /* a, a+1 = the name of value b (values 0 to n - 1): their names at data a */
    uint32_t n = rio_dataSize(ip + 1), i = R[ip->b].u, names = ip[1].a;
    if (i < n) {
      R[ip->a] = R[names + 2 * i];
      R[ip->a + 1] = R[names + 2 * i + 1];
    } else {
      R[ip->a].u = 0;
      R[ip->a + 1].u = 0;
    }
    rio_NEXT2();
  }
  rio_CASE(enumck) { /* is b one of the values 0 to n - 1? x: a runtime error if not, else a = the answer */
    uint32_t n = rio_dataSize(ip + 1), i = R[ip->b].u;
    if (ip->x) {
      if (i >= n) return rio_runtimeError(vm, "not a value of that enum", rio_PC);
    } else R[ip->a].i = i < n;
    rio_NEXT2();
  }
  rio_CASE(zero) memset(M + R[ip->a].i, 0, rio_dataSize(ip)); rio_NEXT();

/* n numbers d[i] = l[i] op r[i]; x: 16 l is a scalar, 32 r is a scalar (the value itself, read once:
   it steps by 0), else l and r are addresses */
#define rio_ELEMENTWISE(body)                                                         \
  {                                                                                   \
    rio_Val lv_ = R[ip->b], rv_ = R[ip->c];                                           \
    rio_Val *d_ = &rio_WORD(R[ip->a].u);                                              \
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
#define rio_LIST_LEN(s) (rio_WORD(R[s].u).u < R[(s) + 2].u ? rio_WORD(R[s].u).u : R[(s) + 2].u)
  rio_CASE(lidx) {
    uint32_t i = R[ip->c].u;
    if (i >= rio_LIST_LEN(ip->b)) return rio_runtimeError(vm, "index out of bounds", rio_PC);
    R[ip->a].u = R[ip->b + 1].u + i * rio_dataSize(ip + 1);
    rio_NEXT2();
  }
  rio_CASE(pusha) {
    uint32_t lenAt = R[ip->b].u, cap = R[ip->b + 2].u, n = rio_LIST_LEN(ip->b);
    if (n >= cap) R[ip->a].u = 0;
    else {
      rio_WORD(lenAt).u = n + 1;
      R[ip->a].u = R[ip->b + 1].u + n * rio_dataSize(ip + 1);
    }
    rio_NEXT2();
  }
  rio_CASE(pushs) {
    uint32_t lenAt = R[ip->b].u, cap = R[ip->b + 2].u, n = rio_LIST_LEN(ip->b);
    uint32_t from = R[ip->c].u, k = R[ip->c + 1].u, size = rio_dataSize(ip + 1);
    if (k > cap - n) R[ip->a].i = 0;
    else {
      memmove(M + R[ip->b + 1].u + n * size, M + from, k * size);
      rio_WORD(lenAt).u = n + k;
      R[ip->a].i = 1;
    }
    rio_NEXT2();
  }
  rio_CASE(pushv) { /* by its descriptor (data), all of it or nothing */
    rio_Out o;
    uint32_t lenAt = R[ip->b].u, n = rio_LIST_LEN(ip->b);
    o.out = M + R[ip->b + 1].u; o.len = n; o.cap = R[ip->b + 2].u; o.capped = 0; o.full = 0;
    o.spec = ip[1].a ? R[ip[1].a].u : 0;
    rio_printValue(M, &o, rio_dataSize(ip + 1), R[ip->c].u, R[ip->c + 1].u, R[ip->c + 2].u);
    if (o.full) R[ip->a].i = 0;
    else {
      rio_WORD(lenAt).u = o.len;
      R[ip->a].i = 1;
    }
    rio_NEXT2();
  }
  rio_CASE(pushf) { /* as its settings say (data: the slot of their address), all of it or nothing */
    rio_Out o;
    uint32_t lenAt = R[ip->b].u, n = rio_LIST_LEN(ip->b);
    o.out = M + R[ip->b + 1].u; o.len = n; o.cap = R[ip->b + 2].u; o.capped = 0; o.full = 0; o.spec = 0;
    rio_outFormatted(M, &o, R[rio_dataSize(ip + 1)].u, ip->x, R[ip->c].u, ip->x == 3 ? R[ip->c + 1].u : 0);
    if (o.full) R[ip->a].i = 0;
    else {
      rio_WORD(lenAt).u = o.len;
      R[ip->a].i = 1;
    }
    rio_NEXT2();
  }
  rio_CASE(pusht) {
    char text[40];
    uint32_t lenAt = R[ip->b].u, cap = R[ip->b + 2].u, n = rio_LIST_LEN(ip->b), k;
    if (ip->x == 1) k = (uint32_t)rio_formatFloat(text, R[ip->c].f);
    else if (ip->x == 2) {
      k = R[ip->c].i ? 4u : 5u;
      memcpy(text, R[ip->c].i ? "true" : "false", k);
    } else k = (uint32_t)rio_formatInt(text, R[ip->c].i);
    if (k > cap - n) R[ip->a].i = 0;
    else {
      memcpy(M + R[ip->b + 1].u + n, text, k);
      rio_WORD(lenAt).u = n + k;
      R[ip->a].i = 1;
    }
    rio_NEXT();
  }
  rio_CASE(popa) {
    uint32_t n = rio_LIST_LEN(ip->b);
    if (!n) return rio_runtimeError(vm, "pop from empty builder", rio_PC);
    rio_WORD(R[ip->b].u).u = --n;
    R[ip->a].u = R[ip->b + 1].u + n * rio_dataSize(ip + 1);
    rio_NEXT2();
  }
  rio_CASE(lrem) {
    uint32_t data = R[ip->a + 1].u, n = rio_LIST_LEN(ip->a), i = R[ip->b].u, size = rio_dataSize(ip + 1);
    if (i >= n) return rio_runtimeError(vm, "index out of bounds", rio_PC);
    memmove(M + data + i * size, M + data + (i + 1) * size, (n - i - 1) * size);
    rio_WORD(R[ip->a].u).u = n - 1;
    rio_NEXT2();
  }
  rio_CASE(lswap) {
    uint32_t data = R[ip->a + 1].u, n = rio_LIST_LEN(ip->a), i = R[ip->b].u, size = rio_dataSize(ip + 1);
    if (i >= n) return rio_runtimeError(vm, "index out of bounds", rio_PC);
    if (i != --n) memmove(M + data + i * size, M + data + n * size, size);
    rio_WORD(R[ip->a].u).u = n;
    rio_NEXT2();
  }
  rio_CASE(lview) {
    uint32_t data = R[ip->b + 1].u, n = rio_LIST_LEN(ip->b);
    R[ip->a].u = data;
    R[ip->a + 1].u = n;
    rio_NEXT();
  }

/* bytes n at byte at of the Blob in slots s, s+1: fail if they're not all in it */
#define rio_BYTES_IN(s, at, n, what) \
  if ((at) < 0 || (uint32_t)(n) > R[(s) + 1].u || (uint32_t)(at) > R[(s) + 1].u - (uint32_t)(n)) return rio_runtimeError(vm, what, rio_PC)
  rio_CASE(bpack) {
    int32_t at = R[ip->b].i;
    uint32_t from = R[ip->c].u, n = R[ip->c + 1].u, to;
    rio_BYTES_IN(ip->a, at, n, ip->x ? "unpack out of bounds" : "pack out of bounds");
    to = R[ip->a].u + (uint32_t)at;
    if (ip->x) memmove(M + from, M + to, n);
    else memmove(M + to, M + from, n);
    rio_NEXT();
  }
  rio_CASE(geti) {
    int32_t at = R[ip->c].i, bits = R[ip->c + 1].i;
    uint32_t n = (uint32_t)(bits < 0 ? -bits : bits) / 8, v = 0, i;
    const uint8_t *p;
    if (n < 1 || n > 4 || bits % 8) return rio_runtimeError(vm, "getInt takes 8, 16, 24 or 32 bits", rio_PC);
    rio_BYTES_IN(ip->b, at, n, "getInt out of bounds");
    p = M + R[ip->b].u + (uint32_t)at;
    for (i = 0; i < n; i++) v |= (uint32_t)p[i] << (8 * i);
    if (bits < 0 && n < 4 && (v >> (8 * n - 1) & 1)) v |= ~0u << (8 * n); /* (signed: the top bit spread) */
    R[ip->a].u = v;
    rio_NEXT();
  }
  rio_CASE(seti) {
    int32_t at = R[ip->c].i, bits = R[ip->c + 1].i;
    uint32_t n = (uint32_t)(bits < 0 ? -bits : bits) / 8, v = R[ip->b].u, i;
    uint8_t *p;
    if (n < 1 || n > 4 || bits % 8) return rio_runtimeError(vm, "setInt takes 8, 16, 24 or 32 bits", rio_PC);
    rio_BYTES_IN(ip->a, at, n, "setInt out of bounds");
    p = M + R[ip->a].u + (uint32_t)at;
    for (i = 0; i < n; i++) p[i] = (uint8_t)(v >> (8 * i));
    rio_NEXT();
  }

  rio_CASE(jmp) rio_JUMP();
  rio_CASE(jz) if (!R[ip->a].i) rio_JUMP(); rio_NEXT();
  rio_CASE(jnz) if (R[ip->a].i) rio_JUMP(); rio_NEXT();
  rio_CASE(jlt) if (R[ip->a].i < R[ip->b].i) rio_JUMP(); rio_NEXT();
  rio_CASE(jle) if (R[ip->a].i <= R[ip->b].i) rio_JUMP(); rio_NEXT();
  rio_CASE(jeq) if (R[ip->a].i == R[ip->b].i) rio_JUMP(); rio_NEXT();
  rio_CASE(jne) if (R[ip->a].i != R[ip->b].i) rio_JUMP(); rio_NEXT();
  rio_CASE(jflt) if (R[ip->a].f < R[ip->b].f) rio_JUMP(); rio_NEXT();
  rio_CASE(jfnlt) if (!(R[ip->a].f < R[ip->b].f)) rio_JUMP(); rio_NEXT();
  rio_CASE(jfnle) if (!(R[ip->a].f <= R[ip->b].f)) rio_JUMP(); rio_NEXT();
  rio_CASE(jfle) if (R[ip->a].f <= R[ip->b].f) rio_JUMP(); rio_NEXT();
  rio_CASE(jfeq) if (R[ip->a].f == R[ip->b].f) rio_JUMP(); rio_NEXT();
  rio_CASE(jfne) if (R[ip->a].f != R[ip->b].f) rio_JUMP(); rio_NEXT();
  rio_CASE(each) if ((R[ip->a].u += R[ip->b + 1].u) < R[ip->b].u) rio_LOOP(); rio_NEXT();
  rio_CASE(fori) if ((R[ip->a].i = (int32_t)(R[ip->a].u + 1u)) < R[ip->b].i) rio_LOOP(); rio_NEXT();
  rio_CASE(forby) { /* (unsigned distances: the counter is between first and last) */
    uint32_t v = R[ip->a].u, end = R[ip->b].u, k = R[ip->b + 1].u;
    if ((int32_t)k > 0 ? end - v >= k : v - end >= 0u - k) {
      R[ip->a].u = v + k;
      rio_LOOP();
    }
    rio_NEXT();
  }

  rio_CASE(call) callStack[depth++] = (uint32_t)(ip + 1 - code); rio_JUMP();
  rio_CASE(ret) {
    if (!depth) return 0;
    ip = code + callStack[--depth];
    rio_DISPATCH();
  }
  rio_CASE(calli) { /* call proc value a: its c argument words, at b, go to its parameters */
    uint32_t proc = R[ip->a].u - 1u;
    if (proc >= (uint32_t)vm->procCount) return rio_runtimeError(vm, "this proc value isn't set", rio_PC);
    if (ip->c) memmove(R + vm->procs[proc].frameStart, R + ip->b, ip->c * sizeof(rio_Val));
    if (vm->interrupted) goto interrupted;
    callStack[depth++] = (uint32_t)(ip + 1 - code);
    ip = code + vm->procs[proc].entry;
    rio_DISPATCH();
  }
  rio_CASE(getret) { /* proc value b's result */
    memmove(R + ip->a, R + vm->procs[R[ip->b].u - 1u].result, ip->c * sizeof(rio_Val));
    rio_NEXT();
  }
  rio_CASE(ffi) {
    vm->ffi[ip->c].fn(vm, R + ip->a);
    if (vm->trapped) {
      vm->trapped = 0;
      return rio_runtimeError(vm, vm->errorText, rio_PC);
    }
    rio_NEXT();
  }
  rio_CASE(logi) rio_CASE(logf) rio_CASE(logs) rio_CASE(logb) rio_CASE(logv) rio_CASE(loge) rio_logOp(vm, R, ip); rio_NEXT();
  rio_CASE(logn) rio_logOp(vm, R, ip); rio_NEXT2();
  rio_CASE(fail) return rio_assertFailed(vm, ip->a, ip->x, rio_PC);
  rio_CASE(halt) return 0;
  rio_CASE(jit) { /* a loop compiled to native code: it says where to carry on, or what failed where */
    uint32_t to;
    if (vm->interrupted) goto interrupted;
    to = vm->jitFns[ip->a](R, vm);
    switch (to >> 16) {
    case 0: break;
    case rio_JitFail_div: return rio_runtimeError(vm, "division by zero", to & 0xFFFF);
    case rio_JitFail_trap: /* an ffi function called rio_trap */
      vm->trapped = 0;
      return rio_runtimeError(vm, vm->errorText, to & 0xFFFF);
    case rio_JitFail_interrupt: /* in a native proc */
      vm->interrupted = 0;
      return rio_runtimeError(vm, "interrupted", to & 0xFFFF);
    default: return rio_runtimeError(vm, "index out of bounds", to & 0xFFFF);
    }
    ip = code + to;
    rio_DISPATCH();
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
#undef rio_PC

/* ------------------------------------------------------------------ entry points */

/* Frames are shared by procs that can't run at the same time: starting rio again from inside an FFI
   function would break that, so it isn't allowed. */
int rio_start(rio_Vm *vm, uint32_t pc) {
  int result;
  if (vm->running) {
    rio_setError(vm, rio_ErrorKind_runtime, 0, 0, 0, "rio is already running (called from an FFI function?)");
    return -1;
  }
  vm->running = 1;
  result = rio_interpret(vm, pc);
  vm->running = 0;
  return result;
}

int rio_run(rio_Vm *vm) {
  if (!vm->running) vm->errorKind = rio_ErrorKind_none;
  vm->interrupted = 0;
  return vm->ready ? rio_start(vm, 0) : -1;
}

int rio_call(rio_Vm *vm, int proc) {
  if (!vm->running) vm->errorKind = rio_ErrorKind_none;
  vm->interrupted = 0;
  return vm->ready && proc >= 0 && proc < vm->procCount ? rio_start(vm, vm->procs[proc].entry) : -1;
}
