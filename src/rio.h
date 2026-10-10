/* rio - a tiny statically typed scripting language. C99, no allocation, no recursion. */
#ifndef RIO_H
#define RIO_H
#include <stdint.h>
#include <setjmp.h>

/* compile-time limits; override with -D. RIO_SMALL picks MCU-sized defaults. */
#ifdef RIO_SMALL
#define RIO_DEF(big, small) small
#else
#define RIO_DEF(big, small) big
#endif
#ifndef RIO_MAX_SYMS
#define RIO_MAX_SYMS RIO_DEF(1024, 128)
#endif
#ifndef RIO_MAX_TYPES
#define RIO_MAX_TYPES RIO_DEF(256, 32)
#endif
#ifndef RIO_MAX_FIELDS
#define RIO_MAX_FIELDS RIO_DEF(1024, 64)
#endif
#ifndef RIO_MAX_FUNCS
#define RIO_MAX_FUNCS RIO_DEF(512, 32)
#endif
#ifndef RIO_MAX_PARAMS
#define RIO_MAX_PARAMS RIO_DEF(2048, 128)
#endif
#ifndef RIO_MAX_FFI
#define RIO_MAX_FFI RIO_DEF(128, 16)
#endif
#ifndef RIO_MAX_CONSTS /* words reserved at the bottom of memory for constants */
#define RIO_MAX_CONSTS RIO_DEF(4096, 256)
#endif
#ifndef RIO_MAX_SLOTS /* register-addressable words (frames, globals, small arrays); at most 65536 */
#define RIO_MAX_SLOTS RIO_DEF(65536, 4096)
#endif
#ifndef RIO_NAMES
#define RIO_NAMES RIO_DEF(16384, 1024)
#endif
#ifndef RIO_MAX_BLOCKS
#define RIO_MAX_BLOCKS RIO_DEF(64, 16)
#endif
#ifndef RIO_MAX_EXPR
#define RIO_MAX_EXPR RIO_DEF(256, 48)
#endif
#ifndef RIO_MAX_DEFINES
#define RIO_MAX_DEFINES RIO_DEF(64, 8)
#endif
#ifndef RIO_LOGBUF
#define RIO_LOGBUF RIO_DEF(256, 96)
#endif
#ifndef RIO_ERRBUF
#define RIO_ERRBUF RIO_DEF(192, 96)
#endif

typedef union { int32_t i; float f; uint32_t u; } RioVal;
typedef struct { uint8_t op, x; uint16_t a, b, c; } RioIns;
typedef struct Rio Rio;
/* ffi: args[0..] hold the arguments (strings/blobs take 2 words: addr, len); write the result to args[0] */
typedef void (*RioFn)(Rio *vm, RioVal *args);
typedef void (*RioLogFn)(void *ud, const char *s, int len);

/* mem: all script data (globals, frames, arrays, strings), and by default also the compiler's
   working memory, which is overlaid on space that stays zero until the program runs.
   code: bytecode buffer (max 65535 instructions). */
int rio_init(Rio *vm, void *mem, uint32_t memsize, RioIns *code, uint32_t codecap);
void rio_set_log(Rio *vm, RioLogFn fn, void *ud);
/* sig: params then optional '>' and result. i=i32 f=f32 s=string b=blob. e.g. "ff>f".
   name and sig must stay valid until rio_compile returns. */
int rio_ffi(Rio *vm, const char *name, const char *sig, RioFn fn);
/* like gcc -D: overrides a top-level constant `NAME :: default`, or defines NAME if the script
   doesn't declare it. value is parsed as the declared constant's type (i32, f32, bool or string);
   NULL means true. name and value must stay valid until rio_compile returns. */
int rio_define(Rio *vm, const char *name, const char *value);
int rio_compile(Rio *vm, const char *src, uint32_t len);
/* same, with the compiler's working memory in a separate buffer that stays valid afterwards
   (needed by rio_aot). Returns -1 with "scratch too small" if it can't hold rio_scratch_min(). */
int rio_compile_scratch(Rio *vm, const char *src, uint32_t len, void *scratch, uint32_t size);
uint32_t rio_scratch_min(void);
int rio_run(Rio *vm);                       /* runs top-level code */
int rio_func(Rio *vm, const char *name);    /* -1 if missing */
RioVal *rio_args(Rio *vm, int fn);          /* params, consecutive words in declaration order */
RioVal *rio_ret(Rio *vm, int fn);
int rio_call(Rio *vm, int fn);
void *rio_global(Rio *vm, const char *name); /* address of a global variable */
void *rio_ptr(Rio *vm, const RioVal *slice); /* bytes of a string/blob/slice value */
void rio_trap(Rio *vm, const char *msg);     /* call from ffi to abort with an error */
const char *rio_error(Rio *vm);

/* ---- private ---- */
enum { RIO_S_VAR, RIO_S_CONST, RIO_S_TYPE, RIO_S_FN, RIO_S_FFI, RIO_S_BI };
#define RIO_OPS(X) X(MOV) X(ADD) X(SUB) X(MUL) X(DIV) X(MOD) X(AND) X(OR) X(XOR) X(SHL) X(SHR) \
  X(NEG) X(BNOT) X(NOT) X(FADD) X(FSUB) X(FMUL) X(FDIV) X(FNEG) \
  X(EQ) X(NE) X(LT) X(LE) X(FEQ) X(FNE) X(FLT) X(FLE) X(SEQ) X(SNE) \
  X(ITOF) X(FTOI) X(LDW) X(LDB) X(LEA) X(M1) X(M2) X(IABS) X(IMIN) X(IMAX) X(FMIN) X(FMAX) X(LDX) X(LDXB) \
  X(MOV2) X(LDW2) X(STW) X(STB) X(STW2) X(STX) X(STXB) X(IDX) X(SLICE) X(COPY) X(ZERO) \
  X(JMP) X(JZ) X(JNZ) X(JEQ) X(JNE) X(JLT) X(JLE) X(JFEQ) X(JFNE) X(JFLT) X(JFLE) X(JFNLT) X(JFNLE) X(FORI) \
  X(CALL) X(RET) X(FFI) X(LOGI) X(LOGF) X(LOGS) X(LOGB) X(LOGE) X(HALT)
/* runtime: per proc entry pc, code end, frame slots [fs, fe), params in [fs, pend), result slot(s) */
typedef struct { uint16_t pc, end, fs, fe, pend, ret; uint8_t retw; } RioFunc;
typedef struct { const char *name, *sig; RioFn fn; uint8_t aw, rw; } RioFfi;
typedef struct { uint32_t name; uint16_t len; uint8_t k, pad; int32_t v; } RioExport; /* lives in mem */

/* compile-time only (lives in scratch) */
typedef struct { int t, line, n, op; const char *s; RioVal v; uint8_t nl; } RioTok;
typedef struct { uint8_t k, ref; uint16_t elem, f0, nf; uint32_t n, size; } RioType;
typedef struct { uint16_t name, len, t, off; } RioField;
typedef struct { uint16_t name, len, t; uint8_t k; int32_t v; } RioSym;
typedef struct { uint16_t pc, end, ret, p0, np, fs, fe; uint32_t retaddr; uint8_t done; } RioCFunc;
typedef struct { uint16_t t; uint32_t addr; } RioParam;
typedef struct { uint16_t ret, p0, np; } RioCFfi;
typedef struct { uint8_t k, ro; uint16_t t, t0; int32_t a, off; } RioEx;
typedef struct { uint8_t k, prec; int16_t op; int32_t a, b, c, n; uint16_t vb, fr0; uint64_t set; } RioOp;
typedef struct { uint8_t k; uint16_t nsym, nnames, nact, a, b, brk, cont, cj, i, lim; } RioBlk;
typedef struct RioC {
  jmp_buf jb;
  const char *sp, *se; int line, pline; RioTok tk, nx;
  uint32_t fr, nact, hwm, lastlabel, pool, poolcap; int curfn, def0;
  int nsym, ntype, nfield, nparam, nnames, nblk, nvs, nos;
  RioSym sym[RIO_MAX_SYMS]; RioType type[RIO_MAX_TYPES]; RioField field[RIO_MAX_FIELDS];
  RioCFunc func[RIO_MAX_FUNCS]; RioParam param[RIO_MAX_PARAMS]; RioCFfi ffi[RIO_MAX_FFI];
  char names[RIO_NAMES]; RioBlk blk[RIO_MAX_BLOCKS]; RioEx vs[RIO_MAX_EXPR]; RioOp os[RIO_MAX_EXPR];
  uint8_t exposed[RIO_MAX_SLOTS / 8];    /* slots whose address is taken (for AOT) */
  uint8_t kfix[RIO_MAX_CONSTS / 8];      /* constants holding string-pool offsets, relocated at the end */
  uint8_t *strs;                         /* string pool, right after this struct */
} RioC;

struct Rio {
  uint8_t *mem; uint32_t memsize, hi, nk, csaddr, exports, nexports;
  RioIns *code; uint32_t codecap, pc;
  RioLogFn logfn; void *logud; int logn, trap, ok, nfunc, nffi, ndefs;
  const char *defs[RIO_MAX_DEFINES][2];
  RioC *c; /* compiler state: only during compile, or after rio_compile_scratch */
  RioFunc func[RIO_MAX_FUNCS]; RioFfi ffi[RIO_MAX_FFI];
  char logbuf[RIO_LOGBUF], err[RIO_ERRBUF];
};
#endif
