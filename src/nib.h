/* nib - a tiny statically typed scripting language. C99, no allocation, no recursion. */
#ifndef NIB_H
#define NIB_H
#include <stdint.h>
#include <setjmp.h>

/* compile-time limits; override with -D. NIB_SMALL picks MCU-sized defaults. */
#ifdef NIB_SMALL
#define NIB_DEF(big, small) small
#else
#define NIB_DEF(big, small) big
#endif
#ifndef NIB_MAX_SYMS
#define NIB_MAX_SYMS NIB_DEF(1024, 128)
#endif
#ifndef NIB_MAX_TYPES
#define NIB_MAX_TYPES NIB_DEF(256, 32)
#endif
#ifndef NIB_MAX_FIELDS
#define NIB_MAX_FIELDS NIB_DEF(1024, 64)
#endif
#ifndef NIB_MAX_FUNCS
#define NIB_MAX_FUNCS NIB_DEF(512, 32)
#endif
#ifndef NIB_MAX_PARAMS
#define NIB_MAX_PARAMS NIB_DEF(2048, 128)
#endif
#ifndef NIB_MAX_FFI
#define NIB_MAX_FFI NIB_DEF(128, 16)
#endif
#ifndef NIB_MAX_CONSTS /* words reserved at the bottom of memory for constants */
#define NIB_MAX_CONSTS NIB_DEF(4096, 256)
#endif
#ifndef NIB_MAX_SLOTS /* register-addressable words (frames, globals, small arrays); at most 65536 */
#define NIB_MAX_SLOTS NIB_DEF(65536, 4096)
#endif
#ifndef NIB_NAMES
#define NIB_NAMES NIB_DEF(16384, 1024)
#endif
#ifndef NIB_MAX_BLOCKS
#define NIB_MAX_BLOCKS NIB_DEF(64, 16)
#endif
#ifndef NIB_MAX_EXPR
#define NIB_MAX_EXPR NIB_DEF(256, 48)
#endif
#ifndef NIB_MAX_DEFINES
#define NIB_MAX_DEFINES NIB_DEF(64, 8)
#endif
#ifndef NIB_LOGBUF
#define NIB_LOGBUF NIB_DEF(256, 96)
#endif
#ifndef NIB_ERRBUF
#define NIB_ERRBUF NIB_DEF(192, 96)
#endif

typedef union { int32_t i; float f; uint32_t u; } NibVal;
typedef struct { uint8_t op, x; uint16_t a, b, c; } NibIns;
typedef struct Nib Nib;
/* ffi: args[0..] hold the arguments (strings/blobs take 2 words: addr, len); write the result to args[0] */
typedef void (*NibFn)(Nib *vm, NibVal *args);
typedef void (*NibLogFn)(void *ud, const char *s, int len);

/* mem: all script data (globals, frames, arrays, strings), and by default also the compiler's
   working memory, which is overlaid on space that stays zero until the program runs.
   code: bytecode buffer (max 65535 instructions). */
int nib_init(Nib *vm, void *mem, uint32_t memsize, NibIns *code, uint32_t codecap);
void nib_set_log(Nib *vm, NibLogFn fn, void *ud);
/* sig: params then optional '>' and result. i=i32 f=f32 s=string b=blob. e.g. "ff>f".
   name and sig must stay valid until nib_compile returns. */
int nib_ffi(Nib *vm, const char *name, const char *sig, NibFn fn);
/* like gcc -D: overrides a top-level constant `NAME :: default`, or defines NAME if the script
   doesn't declare it. value is parsed as the declared constant's type (i32, f32, bool or string);
   NULL means true. name and value must stay valid until nib_compile returns. */
int nib_define(Nib *vm, const char *name, const char *value);
int nib_compile(Nib *vm, const char *src, uint32_t len);
/* same, with the compiler's working memory in a separate buffer that stays valid afterwards
   (needed by nib_aot). Returns -1 with "scratch too small" if it can't hold nib_scratch_min(). */
int nib_compile_scratch(Nib *vm, const char *src, uint32_t len, void *scratch, uint32_t size);
uint32_t nib_scratch_min(void);
int nib_run(Nib *vm);                       /* runs top-level code */
int nib_func(Nib *vm, const char *name);    /* -1 if missing */
NibVal *nib_args(Nib *vm, int fn);          /* params, consecutive words in declaration order */
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
/* runtime: per proc entry pc, code end, frame slots [fs, fe), params in [fs, pend), result slot(s) */
typedef struct { uint16_t pc, end, fs, fe, pend, ret; uint8_t retw; } NibFunc;
typedef struct { const char *name, *sig; NibFn fn; uint8_t aw, rw; } NibFfi;
typedef struct { uint32_t name; uint16_t len; uint8_t k, pad; int32_t v; } NibExport; /* lives in mem */

/* compile-time only (lives in scratch) */
typedef struct { int t, line, n, op; const char *s; NibVal v; uint8_t nl; } NibTok;
typedef struct { uint8_t k, ref; uint16_t elem, f0, nf; uint32_t n, size; } NibType;
typedef struct { uint16_t name, len, t, off; } NibField;
typedef struct { uint16_t name, len, t; uint8_t k; int32_t v; } NibSym;
typedef struct { uint16_t pc, end, ret, p0, np, fs, fe; uint32_t retaddr; uint8_t done; } NibCFunc;
typedef struct { uint16_t t; uint32_t addr; } NibParam;
typedef struct { uint16_t ret, p0, np; } NibCFfi;
typedef struct { uint8_t k, ro; uint16_t t, t0; int32_t a, off; } NibEx;
typedef struct { uint8_t k, prec; int16_t op; int32_t a, b, c, n; uint16_t vb, fr0; } NibOp;
typedef struct { uint8_t k; uint16_t nsym, nnames, nact, a, b, brk, cont, cj, i, lim; } NibBlk;
typedef struct NibC {
  jmp_buf jb;
  const char *sp, *se; int line, pline; NibTok tk, nx;
  uint32_t fr, nact, hwm, lastlabel, pool, poolcap; int curfn, def0;
  int nsym, ntype, nfield, nparam, nnames, nblk, nvs, nos;
  NibSym sym[NIB_MAX_SYMS]; NibType type[NIB_MAX_TYPES]; NibField field[NIB_MAX_FIELDS];
  NibCFunc func[NIB_MAX_FUNCS]; NibParam param[NIB_MAX_PARAMS]; NibCFfi ffi[NIB_MAX_FFI];
  char names[NIB_NAMES]; NibBlk blk[NIB_MAX_BLOCKS]; NibEx vs[NIB_MAX_EXPR]; NibOp os[NIB_MAX_EXPR];
  uint8_t exposed[NIB_MAX_SLOTS / 8];    /* slots whose address is taken (for AOT) */
  uint8_t kfix[NIB_MAX_CONSTS / 8];      /* constants holding string-pool offsets, relocated at the end */
  uint8_t *strs;                         /* string pool, right after this struct */
} NibC;

struct Nib {
  uint8_t *mem; uint32_t memsize, hi, nk, csaddr, exports, nexports;
  NibIns *code; uint32_t codecap, pc;
  NibLogFn logfn; void *logud; int logn, trap, ok, nfunc, nffi, ndefs;
  const char *defs[NIB_MAX_DEFINES][2];
  NibC *c; /* compiler state: only during compile, or after nib_compile_scratch */
  NibFunc func[NIB_MAX_FUNCS]; NibFfi ffi[NIB_MAX_FFI];
  char logbuf[NIB_LOGBUF], err[NIB_ERRBUF];
};
#endif
