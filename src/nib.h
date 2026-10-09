/* nib - a tiny statically typed scripting language. C99, no allocation, no recursion. */
#ifndef NIB_H
#define NIB_H
#include <stdint.h>
#include <setjmp.h>

/* compile-time limits; override with -D */
#ifndef NIB_MAX_SYMS
#define NIB_MAX_SYMS 1024
#endif
#ifndef NIB_MAX_TYPES
#define NIB_MAX_TYPES 256
#endif
#ifndef NIB_MAX_FIELDS
#define NIB_MAX_FIELDS 1024
#endif
#ifndef NIB_MAX_FUNCS
#define NIB_MAX_FUNCS 512
#endif
#ifndef NIB_MAX_PARAMS
#define NIB_MAX_PARAMS 2048
#endif
#ifndef NIB_MAX_FFI
#define NIB_MAX_FFI 128
#endif
#ifndef NIB_MAX_CONSTS /* words reserved at the bottom of memory for constants */
#define NIB_MAX_CONSTS 4096
#endif
#ifndef NIB_NAMES
#define NIB_NAMES 16384
#endif
#ifndef NIB_MAX_BLOCKS
#define NIB_MAX_BLOCKS 64
#endif
#ifndef NIB_MAX_EXPR
#define NIB_MAX_EXPR 256
#endif
#define NIB_LOGBUF 256

typedef union { int32_t i; float f; uint32_t u; } NibVal;
typedef struct { uint8_t op, x; uint16_t a, b, c; } NibIns;
typedef struct Nib Nib;
/* ffi: args[0..] hold the arguments (strings/blobs take 2 words: addr, len); write the result to args[0] */
typedef void (*NibFn)(Nib *vm, NibVal *args);
typedef void (*NibLogFn)(void *ud, const char *s, int len);

/* mem: all script data (globals, frames, arrays, strings). code: bytecode buffer (max 65535). */
int nib_init(Nib *vm, void *mem, uint32_t memsize, NibIns *code, uint32_t codecap);
void nib_set_log(Nib *vm, NibLogFn fn, void *ud);
/* sig: params then optional '>' and result. i=i32 f=f32 s=string b=blob. e.g. "ff>f" */
int nib_ffi(Nib *vm, const char *name, const char *sig, NibFn fn);
int nib_compile(Nib *vm, const char *src, uint32_t len);
int nib_run(Nib *vm);                       /* runs top-level code */
int nib_func(Nib *vm, const char *name);    /* -1 if missing */
NibVal *nib_arg(Nib *vm, int fn, int i);    /* write args here before nib_call */
NibVal *nib_ret(Nib *vm, int fn);
int nib_call(Nib *vm, int fn);
void *nib_global(Nib *vm, const char *name); /* address of a global variable */
void *nib_ptr(Nib *vm, const NibVal *slice); /* bytes of a string/blob/slice value */
void nib_trap(Nib *vm, const char *msg);     /* call from ffi to abort with an error */
const char *nib_error(Nib *vm);

/* ---- private ---- */
enum { NIB_S_VAR, NIB_S_CONST, NIB_S_TYPE, NIB_S_FN, NIB_S_FFI, NIB_S_BI };
#define NIB_OPS(X) X(MOV) X(ADD) X(SUB) X(MUL) X(DIV) X(MOD) X(AND) X(OR) X(XOR) X(SHL) X(SHR) \
  X(NEG) X(BNOT) X(NOT) X(FADD) X(FSUB) X(FMUL) X(FDIV) X(FNEG) \
  X(EQ) X(NE) X(LT) X(LE) X(FEQ) X(FNE) X(FLT) X(FLE) X(SEQ) X(SNE) \
  X(ITOF) X(FTOI) X(LDW) X(LDB) X(LEA) X(M1) X(M2) X(IABS) X(IMIN) X(IMAX) X(FMIN) X(FMAX) X(LDX) X(LDXB) \
  X(MOV2) X(LDW2) X(STW) X(STB) X(STW2) X(STX) X(STXB) X(IDX) X(SLICE) X(COPY) X(ZERO) \
  X(JMP) X(JZ) X(JNZ) X(JEQ) X(JNE) X(JLT) X(JLE) X(JFEQ) X(JFNE) X(JFLT) X(JFLE) X(JFNLT) X(JFNLE) X(FORI) \
  X(CALL) X(RET) X(FFI) X(LOGI) X(LOGF) X(LOGS) X(LOGB) X(LOGE) X(HALT)
typedef struct { int t, line, n, op; const char *s; NibVal v; uint8_t nl; } NibTok;
typedef struct { uint8_t k, ref; uint16_t elem, f0, nf; uint32_t n, size; } NibType;
typedef struct { uint16_t name, len, t, off; } NibField;
typedef struct { uint16_t name, len, t; uint8_t k; int32_t v; } NibSym;
typedef struct { uint16_t pc, end, ret, p0, np, fs, fe; uint32_t retaddr; uint8_t done; } NibFunc;
typedef struct { uint16_t t; uint32_t addr; } NibParam;
typedef struct { NibFn fn; uint16_t ret, p0, np; } NibFfi;
typedef struct { uint8_t k, ro; uint16_t t, t0; int32_t a, off; } NibEx;
typedef struct { uint8_t k, prec; int16_t op; int32_t a, b, c, n; uint16_t vb, fr0; } NibOp;
typedef struct { uint8_t k; uint16_t nsym, nnames, nact, a, b, brk, cont, cj, i, lim; } NibBlk;

struct Nib {
  uint8_t *mem; uint32_t memsize, hi;
  NibIns *code; uint32_t codecap, pc, lastlabel;
  NibLogFn logfn; void *logud; int logn, trap; char logbuf[NIB_LOGBUF];
  char err[192];
  const char *sp, *se; int line, pline; NibTok tk, nx; jmp_buf jb;
  uint32_t nk, fr, nact, hwm; int curfn, ok;
  int nsym, ntype, nfield, nfunc, nparam, nffi, nnames, nblk, nvs, nos;
  NibSym sym[NIB_MAX_SYMS]; NibType type[NIB_MAX_TYPES]; NibField field[NIB_MAX_FIELDS];
  NibFunc func[NIB_MAX_FUNCS]; NibParam param[NIB_MAX_PARAMS]; NibFfi ffi[NIB_MAX_FFI];
  char names[NIB_NAMES]; NibBlk blk[NIB_MAX_BLOCKS]; NibEx vs[NIB_MAX_EXPR]; NibOp os[NIB_MAX_EXPR];
  uint8_t exposed[8192]; /* slots whose address is taken */
};
#endif
