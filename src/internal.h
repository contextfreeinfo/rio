/* internal.h - what rio's own files share, beyond the public API: the instruction set, the kinds of
   symbols and types, the runtime tables in mem, and small helpers. Not for hosts. */
#ifndef rio_INTERNAL_H
#define rio_INTERNAL_H
#include <string.h>
#include "rio.h"
#include "print.h"
rio_PRINT_KINDS /* (the kinds of value print.h prints: descriptors start with one) */

#if defined(__GNUC__) || defined(__clang__)
#ifndef rio_NO_CGOTO
#define rio_CGOTO 1 /* dispatch by computed goto (otherwise a switch) */
#endif
#define rio_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define rio_NOINLINE __declspec(noinline)
#else
#define rio_NOINLINE
#endif

/* rio_HOT_SECTION: a linker section for the interpreter loop, so a host can place it in fast memory
   (with the Pico SDK, ".time_critical.rio" puts it in SRAM instead of flash) */
#if defined(rio_HOT_SECTION) && (defined(__GNUC__) || defined(__clang__))
#define rio_HOT __attribute__((section(rio_HOT_SECTION)))
#define rio_HOT_DATA __attribute__((section(rio_HOT_SECTION "_data")))
#else
#define rio_HOT
#define rio_HOT_DATA
#endif

#define rio_FFI_PAD_WORDS 64 /* a host proc called with zeros for parameters it gained (rio_Ffi pad) takes at most this many words */

/* ------------------------------------------------------------------ the instruction set */

/* Operands a, b and c are slots (word indexes into mem) unless noted; constants are slots too, below
   rio_Vm.constCap. "data" is the word after an instruction (op rio_Op_data): a size spread over its
   b and c (rio_dataSize), and sometimes a slot or offset in its a. The order matters: images store
   these numbers, compares map onto compare-and-branches, and ranges of them are tested.
     mov                    a = b
     add sub mul div mod    a = b op c (Int; div and mod fail on 0, and wrap on -1)
     and or xor shl shr     a = b op c (Int)
     ushr                   a = b >>> c: b's bits shifted right, zeros in from the top
     neg bnot not           a = op b
     fadd fsub fmul fdiv    a = b op c (Float)
     fneg                   a = -b
     eq ne lt le            a = b op c (Int)
     feq fne flt fle        a = b op c (Float)
     seq sne                a = string b ==/!= string c (2 slots each: address, length)
     itof ftoi              a = conversion of b
     ldw ldb                a = word / byte at address b + c (c a constant)
     lea                    a = b + c (c a constant)
     m1 m2                  a = math function x of b (and c): rio_Math1 / rio_Math2
     iabs imin imax fmin fmax
     ldx ldxb               a = word / byte at element c of slice b, data a bytes in (bounds-checked)
   Everything up to here writes one scalar to slot a: see rio_Op_lastScalar.
     mov2                   a, a+1 = b, b+1
     ldw2                   a, a+1 = the two words at address b + c
     stw stb                word / byte at address a + c = b
     stw2                   the two words at address a + c = b, b+1
     stx stxb               element b of slice a, data a bytes in = c (bounds-checked)
     idx                    a = address of element c of slice b (bounds-checked)
     slice                  a, a+1 = slice b from index c to the index in slot data a (bounds-checked)
     copy                   c bytes from address b to address a
     movn                   c words from slot b to slot a
     zero                   zero b | c << 16 bytes at address a
     enumck                 is b one of the data-size values in the slots from c? x: fail if not; else
                            a = the answer
     enumname               a, a+1 = name of enum value b (values at c, names at data a)
     vadd vsub vmul vdiv vmod       element by element: address a = b op c, data-size numbers;
     fvadd fvsub fvmul fvdiv        x & 16: b is a scalar, x & 32: c is a scalar
   Builders: a builder view is 3 slots (address of the length word, address of the data, capacity).
     lidx                   a = address of element c of builder view b (checked against its length)
     pusha                  a = address of a new last element of builder view b, or 0 if full
     pushs                  a = append slice c to builder view b (0 if it doesn't fit)
     pusht                  a = append the text of c to Blob builder view b; x: 0 Int, 1 Float, 2 Bool
     pushv                  a = append the text of value c to Blob builder view b, by the descriptor in
                            constant data size (print.h: c is a slice's or builder view's slots, else holds
                            an address), as the Format at the address in slot data a says (0: none)
     pushf                  a = append the text of c to Blob builder view b, as the Format at the address in
                            slot data a says; x: 0 Int, 1 Float, 2 Bool, 3 text (c, c+1: a slice)
     popa                   a = address of the removed last element of builder view b
     lrem lswap             remove element b of builder view a, keeping order / moving the last into it
     lview                  a, a+1 = slice of builder view b's live elements
   Bytes: a Blob is 2 slots (address, length), as a slice; at is a byte offset in it (bounds-checked).
     bpack                  x 0: copy bytes c (address, how many) into Blob a at b; x 1: from it
     geti                   a = the int in Blob b at c, of c+1 bits (8, 16, 24 or 32; negative: signed),
                            little-endian
     seti                   the low c+1 bits of b into Blob a at c, little-endian
     bchk                   fail unless each value at a (address, bytes: c bytes each) holds only what
                            its type can, by the descriptor in constant b (print.h: enums, Bools,
                            union tags), as unpack leaves them
     pnum                   a, a+1 = the text b (address, length) as a number, a maybe (rio_parseNumber):
                            x 0, an Int in base c (0: 10; fail unless 2 to 36); x 1, a Float
   Jumps go to pc c.
     jmp jz jnz             always / if a == 0 / if a != 0
     jeq jne jlt jle        if a op b (Int)
     jfeq jfne jflt jfle    if a op b (Float)
     jfnlt jfnle            if not (a < b) / not (a <= b) (Float: differs from >= on NaN)
     each                   a += b+1 (an element size); loop while a < b
     fori                   a += 1; loop while a < b
     forby                  a += b+1 (the step), and loop, unless that passes b (the end; a stepped
                            loop's counter stays between its first and last, so this never wraps)
     call                   call the proc at pc c
     ret                    return (from top-level code: stop)
     calli                  call proc value a, copying its c argument words from slot b
     getret                 a... = the c result words of proc value b
     ffi                    call ffi function c with its arguments at slot a
     logi logf logs logb    add a value to the log line (x: a space first)
     logn                   add enum value a by name (values at b, names at c)
     logv                   add value a by the descriptor in constant b (print.h; a as pushv's c)
     loge                   end the log line
     fail                   a failed assert: a = its message (0: none); x 1: a runtime error, a its message
     halt                   stop
     jit                    run native loop a (rio_jit), then carry on where it says
   A size known only while running (a generic proc's T), in a slot (idxv finds it in a, then writes a).
     idxv                   a = address of element c of slice b, each a bytes (bounds-checked)
     copyv                  c bytes from address b to address a, c a slot
     swapv                  swap c bytes at address a with those at address b, c a slot */
#define rio_OPS(X)                                                                                   \
  X(mov) X(add) X(sub) X(mul) X(div) X(mod) X(and) X(or) X(xor) X(shl) X(shr) X(ushr)               \
  X(neg) X(bnot) X(not) X(fadd) X(fsub) X(fmul) X(fdiv) X(fneg)                                      \
  X(eq) X(ne) X(lt) X(le) X(feq) X(fne) X(flt) X(fle) X(seq) X(sne)                                  \
  X(itof) X(ftoi) X(ldw) X(ldb) X(lea) X(m1) X(m2) X(iabs) X(imin) X(imax) X(fmin) X(fmax)           \
  X(ldx) X(ldxb)                                                                                     \
  X(mov2) X(ldw2) X(stw) X(stb) X(stw2) X(stx) X(stxb) X(idx) X(slice) X(copy) X(movn) X(zero)       \
  X(enumck) X(enumname)                                                                              \
  X(vadd) X(vsub) X(vmul) X(vdiv) X(vmod) X(fvadd) X(fvsub) X(fvmul) X(fvdiv)                        \
  X(lidx) X(pusha) X(pushs) X(pusht) X(pushv) X(pushf) X(popa) X(lrem) X(lswap) X(lview) X(bpack) X(geti) X(seti) X(bchk) X(pnum) \
  X(jmp) X(jz) X(jnz) X(jeq) X(jne) X(jlt) X(jle) X(jfeq) X(jfne) X(jflt) X(jfle) X(jfnlt) X(jfnle)  \
  X(each) X(fori) X(forby)                                                                                    \
  X(call) X(ret) X(calli) X(getret) X(ffi) X(logi) X(logf) X(logs) X(logb) X(logn) X(logv) X(loge)   \
  X(fail) X(halt) X(jit) X(idxv) X(copyv) X(swapv)

#define rio_OP_ENUM(o) rio_Op_##o,
typedef enum {
  rio_OPS(rio_OP_ENUM)
  rio_Op_count,
  rio_Op_data = 255,                /* not an instruction: the data word after one */
  rio_Op_lastScalar = rio_Op_fmax,  /* ops up to here write one scalar to slot a */
} rio_Op;

/* the size in an instruction's b and c (a data word's, or zero's own) */
static inline uint32_t rio_dataSize(const rio_Instr *in) { return (uint32_t)in->b | (uint32_t)in->c << 16; }

static inline int rio_isJump(int op) { return op >= rio_Op_jmp && op <= rio_Op_forby; }

/* math builtins, by the index in rio_Op_m1 / rio_Op_m2's x */
typedef enum {
  rio_Math1_sqrt, rio_Math1_sin, rio_Math1_cos, rio_Math1_tan, rio_Math1_asin, rio_Math1_acos, rio_Math1_atan,
  rio_Math1_exp, rio_Math1_ln, rio_Math1_floor, rio_Math1_ceil, rio_Math1_round, rio_Math1_abs, rio_Math1_count
} rio_Math1;
typedef enum { rio_Math2_atan2, rio_Math2_pow, rio_Math2_fmod, rio_Math2_count } rio_Math2;
extern float (*const rio_math1[rio_Math1_count])(float);
extern float (*const rio_math2[rio_Math2_count])(float, float);

/* ------------------------------------------------------------------ symbols and types */

/* kinds of symbols (rio_Symbol.kind); exports in mem use the same numbers */
typedef enum {
  rio_SymKind_var,         /* value: its address */
  rio_SymKind_const,       /* value: the constant (a String: its constant's address) */
  rio_SymKind_type,
  rio_SymKind_proc,        /* value: the proc's index */
  rio_SymKind_ffi,
  rio_SymKind_builtin,     /* value: a rio_Builtin */
  rio_SymKind_method,      /* a proc declared on its type (type) */
  rio_SymKind_ref,         /* self, a & parameter, or a for-each element: value is the slot holding
                              its address */
  rio_SymKind_module,      /* value: the module's index */
  rio_SymKind_readOnlyRef, /* a ref that can't be assigned through (a String's elements) */
  rio_SymKind_narrowed,    /* a union variable narrowed to one of its types: a read-only copy */
} rio_SymKind;

/* kinds of types (rio_Type.kind) */
typedef enum {
  rio_TypeKind_void, rio_TypeKind_int, rio_TypeKind_float, rio_TypeKind_byte, rio_TypeKind_slice,
  rio_TypeKind_array, rio_TypeKind_struct, rio_TypeKind_bool, rio_TypeKind_builder, rio_TypeKind_builderView,
  rio_TypeKind_enum, rio_TypeKind_union, rio_TypeKind_proc,
  rio_TypeKind_param /* a generic proc's T (elem: the proc; count: which of its type parameters), its size
                        known only while running */
} rio_TypeKind;

/* the built-in types, first in every type table */
typedef enum {
  rio_TypeId_void, rio_TypeId_int, rio_TypeId_float, rio_TypeId_byte, rio_TypeId_string, rio_TypeId_blob,
  rio_TypeId_bool, rio_TypeId_builtinCount
} rio_TypeId;

/* up to this many words, an array or struct lives in slots and is copied without being asked (x := v,
   by-value params and returns, array arithmetic temporaries); bigger ones need &T, []T or a written
   type */
#define rio_SMALL_WORDS 16

/* "none" for 16-bit jump lists, slots and pcs */
#define rio_NONE 0xFFFF

/* ------------------------------------------------------------------ runtime tables in mem */

/* a name the host can look up */
typedef struct {
  uint32_t nameAt;
  uint16_t nameLen;
  uint8_t kind, pad; /* kind: rio_SymKind_proc or rio_SymKind_var */
  int32_t value;     /* the proc's index, or the global's address */
} rio_Export;

/* the code of a module (or an included part), to name its file in runtime errors */
typedef struct {
  uint16_t start, end;
  uint32_t fileAt;
} rio_ModuleCode;

/* ------------------------------------------------------------------ helpers */

static inline uint32_t rio_align4(uint32_t n) { return (n + 3) & ~3u; }

/* bit sets of slots or constants */
static inline int rio_bitGet(const uint8_t *bits, uint32_t i) { return bits[i >> 3] >> (i & 7) & 1; }
static inline void rio_bitSet(uint8_t *bits, uint32_t i) { bits[i >> 3] |= (uint8_t)(1u << (i & 7)); }
static inline void rio_bitClear(uint8_t *bits, uint32_t i) { bits[i >> 3] &= (uint8_t)~(1u << (i & 7)); }

/* Float to Int, truncating toward zero, saturating, and 0 for NaN */
static inline int32_t rio_floatToInt(float f) {
  if (f != f) return 0;
  if (f >= 2147483648.f) return 0x7FFFFFFF;
  if (f <= -2147483648.f) return (int32_t)0x80000000u;
  return (int32_t)f;
}

/* text into out (no terminator); each returns the length */
int rio_formatInt(char *out, int32_t v);
int rio_formatFloat(char *out, float f);
/* appends text to an error message being built in out, which already has len characters, stopping
   short of rio_ERROR_BUF_SIZE; returns the new length (not terminated) */
int rio_appendText(char *out, int len, const char *text);

/* records an error: "line:col: message" (position parts left out when 0), remembering the parts for
   rio_errorInfo; returns the text's length */
int rio_setError(rio_Vm *vm, int kind, int line, int col, int len, const char *message);

/* Native code (jit.c) returns the pc to carry on from, or a failure for the vm to report as it would
   have: (failure << 16) | the pc where it happened. */
typedef enum { rio_JitFail_div = 1, rio_JitFail_index = 2, rio_JitFail_trap = 3, rio_JitFail_interrupt = 4 } rio_JitFail;

/* vm.c: runs from pc, unless rio is already running; and a log instruction (the JIT shares it) */
int rio_start(rio_Vm *vm, uint32_t pc);
void rio_logOp(rio_Vm *vm, const rio_Val *slots, const rio_Instr *ip);

#endif
