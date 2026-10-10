/* rio - a tiny statically typed scripting language. C99, no allocation, no recursion. */
#ifndef RIO_H
#define RIO_H
#include <stdint.h>
#include <setjmp.h>

/* fixed caps for things the host registers before compiling; override with -D.
   RIO_SMALL picks MCU-sized values. Compiler table sizes are chosen per compile: see RioLimits.
   The host can look up (rio_func, rio_global) main and the main file's names marked name*; build with
   RIO_EXPORTS_ALL to make every top-level proc and global findable (a 12-byte entry plus the name each). */
#ifdef RIO_SMALL
#define RIO_DEF(big, small) small
#else
#define RIO_DEF(big, small) big
#endif
#ifndef RIO_MAX_FFI
#define RIO_MAX_FFI RIO_DEF(128, 16)
#endif
#ifndef RIO_MAX_DEFINES
#define RIO_MAX_DEFINES RIO_DEF(64, 8)
#endif
#ifndef RIO_MAX_IMPORT_DEPTH
#define RIO_MAX_IMPORT_DEPTH 16
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
/* modules and includes come from the host. kind is a combination of:
     RIO_LOAD_PKG   the path is in the library paths (otherwise relative to the main file's directory)
     RIO_LOAD_FILE  the path names an exact file, from `include "x.rio"`; without it, the path names
                    a module: try "path.rio", then the directory's entry file "path/<last>.rio"
   `import .ui.button` asks for module "ui/button"; `import tween` for package module "tween";
   `include "parts/a.rio"` inside shapes/shapes.rio for file "shapes/parts/a.rio".
   Return 0 and fill *out (src must stay valid until compiling ends; isdir: a module path named a
   directory and its entry file was used; name: optional text for error messages), 1 if not
   found, 2 if found in more than one place. */
enum { RIO_LOAD_PKG = 1, RIO_LOAD_FILE = 2 };
typedef struct { const char *src; uint32_t len; int isdir; const char *name; } RioSource;
typedef int (*RioLoadFn)(void *ud, const char *path, int kind, RioSource *out);
void rio_set_loader(Rio *vm, RioLoadFn fn, void *ud);
/* compiler capacities. consts is also how many words of mem are kept for constants, and slots
   (at most 65536, counting the constants) caps the register-addressable part of mem. */
typedef struct { uint32_t syms, names, types, fields, procs, params, consts, slots, blocks, expr, modules; } RioLimits;
RioLimits rio_limits_for(uint32_t bytes);     /* limits whose tables fill about 60% of `bytes` */
uint32_t rio_limits_size(const RioLimits *l); /* scratch bytes those tables take; strings need more */
/* compile with the compiler's working memory overlaid on the part of mem that stays zero until
   the program runs; limits are scaled to mem */
int rio_compile(Rio *vm, const char *src, uint32_t len);
/* same, with the compiler's working memory in a separate buffer that stays valid afterwards
   (rio_aot needs it); limits are scaled to the buffer */
int rio_compile_scratch(Rio *vm, const char *src, uint32_t len, void *scratch, uint32_t size);
/* either of the above with explicit limits: scratch NULL overlays mem; lim NULL scales */
int rio_compile_ex(Rio *vm, const char *src, uint32_t len, void *scratch, uint32_t size, const RioLimits *lim);
int rio_run(Rio *vm);                       /* runs top-level code */
int rio_func(Rio *vm, const char *name);    /* -1 if missing */
RioVal *rio_args(Rio *vm, int fn);          /* params, consecutive words in declaration order */
RioVal *rio_ret(Rio *vm, int fn);
int rio_call(Rio *vm, int fn);
void *rio_global(Rio *vm, const char *name); /* address of a global variable */
void *rio_ptr(Rio *vm, const RioVal *slice); /* bytes of a string/blob/slice value */
void rio_trap(Rio *vm, const char *msg);     /* call from ffi to abort with an error */
/* break: safe from a signal handler, an interrupt or another core. A running script stops at its
   next loop iteration with the runtime error "interrupted". */
void rio_interrupt(Rio *vm);
/* REPL: compile and run code a piece at a time; globals, procs, types and methods carry over.
   The compiler keeps its state in scratch for the whole session. A bare expression prints its value. */
enum { RIO_MORE = 1 };
int rio_repl_begin(Rio *vm, void *scratch, uint32_t size, const RioLimits *lim);
/* 0: ran. -1: error (see rio_error; a compile error leaves the session unchanged).
   RIO_MORE: the input is unfinished (an open block, bracket or string): call again with the
   same text plus more appended. */
int rio_repl_eval(Rio *vm, const char *src, uint32_t len);
const char *rio_error(Rio *vm);           /* "line:col: message", or "line: runtime error: message" */
/* structured error for editors: line/col are 1-based (0 = unknown); len is the width of the
   offending token (0 = unknown); msg is the message without the position prefix */
enum { RIO_ENONE, RIO_ECOMPILE, RIO_ERUNTIME };
typedef struct { int kind, line, col, len; const char *msg, *file; } RioError; /* file: NULL for the main source */
RioError rio_error_info(Rio *vm);
int rio_pc_line(Rio *vm, uint32_t pc);      /* source line of a bytecode position (0 = unknown) */

/* ---- private ---- */
enum { RIO_S_VAR, RIO_S_CONST, RIO_S_TYPE, RIO_S_FN, RIO_S_FFI, RIO_S_BI, RIO_S_METH, RIO_S_SELF, RIO_S_MOD };
#define RIO_OPS(X) X(MOV) X(ADD) X(SUB) X(MUL) X(DIV) X(MOD) X(AND) X(OR) X(XOR) X(SHL) X(SHR) \
  X(NEG) X(BNOT) X(NOT) X(FADD) X(FSUB) X(FMUL) X(FDIV) X(FNEG) \
  X(EQ) X(NE) X(LT) X(LE) X(FEQ) X(FNE) X(FLT) X(FLE) X(SEQ) X(SNE) \
  X(ITOF) X(FTOI) X(LDW) X(LDB) X(LEA) X(M1) X(M2) X(IABS) X(IMIN) X(IMAX) X(FMIN) X(FMAX) X(LDX) X(LDXB) \
  X(MOV2) X(LDW2) X(STW) X(STB) X(STW2) X(STX) X(STXB) X(IDX) X(SLICE) X(COPY) X(MOVN) X(ZERO) \
  X(VADD) X(VSUB) X(VMUL) X(VDIV) X(VMOD) X(FVADD) X(FVSUB) X(FVMUL) X(FVDIV) \
  X(LIDX) X(PUSHA) X(PUSHS) X(PUSHT) X(POPA) X(LREM) X(LSWAP) X(LVIEW) \
  X(JMP) X(JZ) X(JNZ) X(JEQ) X(JNE) X(JLT) X(JLE) X(JFEQ) X(JFNE) X(JFLT) X(JFLE) X(JFNLT) X(JFNLE) X(EACH) X(FORI) \
  X(CALL) X(RET) X(FFI) X(LOGI) X(LOGF) X(LOGS) X(LOGB) X(LOGE) X(HALT)
/* runtime: per proc entry pc, code end, frame slots [fs, fe), params in [fs, pend), result slot(s) */
typedef struct { uint16_t pc, end, fs, fe, pend, ret; uint8_t retw; } RioFunc;
typedef struct { const char *name, *sig; RioFn fn; uint8_t aw, rw; } RioFfi;
typedef struct { uint32_t name; uint16_t len; uint8_t k, pad; int32_t v; } RioExport; /* lives in mem */
typedef struct { uint16_t pc0, pc1; uint32_t file; } RioModRt; /* lives in mem: code range of a module */

/* compile-time only (lives in scratch) */
typedef struct { int t, line, col, w, n, op; const char *s; RioVal v; uint8_t nl; } RioTok;
typedef struct { uint8_t k, ref; uint16_t elem, f0, nf; uint32_t n, size; } RioType;
typedef struct { uint16_t name, len, t, off; } RioField;
typedef struct { uint16_t name, len, t, mod; uint8_t k, ex; int32_t v; } RioSym; /* mod: defining module; ex: exported */
/* a source file: a module, or a part included into one. state 1 compiling, 2 done */
typedef struct { uint16_t key, keylen, dir, dirlen, file, pc0, pc1; uint8_t pkg, state, inc; } RioMod;
/* where to resume the file that imported or included another: for an import, its import statement
   runs again (and just binds names); for an include, compiling continues after it */
typedef struct { const char *src, *se, *ls, *pos; int line, mod, f, inc; } RioImp;
/* big: bytes of its locals stored apart (over 16 words). shared: its frame lives in the shared area at
   offset ob; oe is where the shared frames of it and everything it calls end; dep: its longest call chain */
typedef struct { uint16_t pc, end, ret, p0, np, fs, fe, ob, oe, dep; uint32_t retaddr, big, hi0; uint8_t done, selfref, shared; } RioCFunc; /* hi0: its big locals are below this */
typedef struct { uint16_t t, ref; uint32_t addr; } RioParam; /* ref: a &T param's T (t is then the address word) */
typedef struct { uint16_t ret, p0, np; } RioCFfi;
typedef struct { uint8_t k, ro; uint16_t t, t0; uint8_t lv; int32_t a, off; } RioEx; /* lv: a view of the current proc's own memory */
typedef struct { uint8_t k, prec; int16_t op; int32_t a, b, c, n, pun; uint16_t vb, fr0; uint64_t set; } RioOp;
typedef struct { uint8_t k; uint16_t nsym, nnames, nact, a, b, brk, cont, cj, i, lim; } RioBlk;
typedef struct RioC {
  jmp_buf jb;
  const char *src, *sp, *se, *ls; int line, pline, pcol, pw; RioTok tk, nx;
  uint32_t linetop, nline, lastline, lastlinepc, lineovr; /* pc->line table, growing down from the top of the pool */
  const char *failmsg; /* the last compile error, to tell "unfinished" from "wrong" */
  RioMod *mods; int nmod, curmod, curf, nis, exporting; RioImp is[RIO_MAX_IMPORT_DEPTH]; /* curf: the file being read */
  uint32_t fr, nact, hwm, lastlabel, pool, poolcap, big; int curfn, def0, target; /* target: type the next expr() should produce, or -1; big: bytes of all big values */
  int nsym, ntype, nfield, nparam, nnames, nblk, nvs, nos;
  RioLimits lim;                         /* capacities of the tables below, all carved from scratch */
  RioSym *sym; RioType *type; RioField *field; RioCFunc *func; RioParam *param;
  char *names; RioBlk *blk; RioEx *vs; RioOp *os;
  uint8_t *exposed;                      /* slots whose address is taken (for AOT) */
  uint8_t *kfix;                         /* constants holding string-pool offsets, relocated at the end */
  uint8_t *kadr;                         /* constants holding slot addresses, renumbered when frames are shared */
  uint8_t *lvs;                          /* local slots that hold a view of their proc's own memory */
  int argstore;                          /* storing call arguments: views may go to a callee */
  uint16_t *smap;                        /* old slot -> new slot, while sharing frames */
  RioCFfi ffi[RIO_MAX_FFI];
  uint8_t *strs;                         /* string pool: the rest of scratch */
} RioC;

struct Rio {
  uint8_t *mem; uint32_t memsize, hi, nk, kcap, csaddr, exports, nexports, lines, nlines; /* nlines: bytes of the packed table */
  uint32_t lcpos, lcpc, lcline; /* where the last line lookup stopped, so lookups in pc order are one pass */
  int ekind, eline, ecol, elen, emsg; const char *efile;
  RioLoadFn loader; void *loadud; uint32_t mods, nmods; /* runtime table of module code ranges, in mem */
  RioIns *code; uint32_t codecap, pc;
  RioLogFn logfn; void *logud; int logn, trap, ok, nfunc, nffi, ndefs, repl, running;
  volatile int brk; /* set by rio_interrupt */
  const char *defs[RIO_MAX_DEFINES][2];
  RioC *c; /* compiler state: only during compile, or after rio_compile_scratch */
  RioFunc *func; /* per proc, in mem next to the strings */
  RioFfi ffi[RIO_MAX_FFI];
  char logbuf[RIO_LOGBUF], err[RIO_ERRBUF];
};
#endif
