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
   scmp                   a = -1, 0 or 1 as string b orders before, with or after string c (bytes, then
                          length: a prefix first)
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
                         (lidx, pusha, pushs, popa, lrem, lswap: the element size is the data's, or
                         slot data.a's when it names one: a generic proc's T)
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
  X(fail) X(halt) X(jit) X(idxv) X(copyv) X(swapv) X(scmp)

#define rio_OP_ENUM(o) rio_Op_##o,
typedef enum {
  rio_OPS(rio_OP_ENUM)
  rio_Op_count,
  rio_Op_data = 255,                /* not an instruction: the data word after one */
  rio_Op_lastScalar = rio_Op_fmax,  /* ops up to here write one scalar to slot a */
} rio_Op;

/* The short forms (rio/plans/smaller-instructions.md): 4 bytes, op, a, b, c, 8 bits each (rio_Short), for
   the ops that matter most, when their operands fit; any other instruction is 8 bytes (rio_Instr: two
   units), then its data word (8 more) if it has one. A short op is 128 or more (rio_Op_s_mov on); the
   string says how each operand reads: S a slot of the running proc's frame (slot - frameStart + 128:
   R + it), I a number under 256, J a jump's target (relative to the
   instruction, in units, -128 to 127), P a proc's number, - unused. (S is a wide operand's bias too, so
   one base, R, serves both: rio_FRAME_BIAS.) More forms of some ops (rio_SHORT_MORE, each after the
   rio_SHORT_OPS: its name, its wide op, its operands) read slots from mem's start, not the frame: K a
   slot under 256 (the constants, low in memory), G any slot (16 bits: the short's b and c, the other
   operand in a). The packer tries an op's first form, then its others in order. */
#define rio_SHORT_OPS(X)                                                                                    X(mov, "SS-") X(add, "SSS") X(sub, "SSS") X(mul, "SSS") X(and, "SSS") X(or, "SSS") X(eq, "SSS")          X(ne, "SSS") X(lt, "SSS") X(le, "SSS") X(imin, "SSS") X(imax, "SSS") X(fadd, "SSS") X(fsub, "SSS")        X(fmul, "SSS") X(flt, "SSS") X(fle, "SSS") X(ldw, "SSI") X(ldb, "SSI") X(ldw2, "SSI") X(lea, "SSI")      X(stw, "SSI") X(stb, "SSI") X(stw2, "SSI") X(movn, "SSI") X(mov2, "SS-") X(jmp, "--J") X(jz, "S-J")       X(jnz, "S-J") X(jeq, "SSJ") X(jne, "SSJ") X(jlt, "SSJ") X(jle, "SSJ") X(fori, "SSJ") X(each, "SSJ")       X(call, "--P") X(ret, "---") X(ffi, "S-I") X(jit, "I--")
#define rio_SHORT_MORE(X)                                                                                     X(movg, mov, "SG-") X(stg, mov, "GS-") X(mov2g, mov2, "SG-") X(st2g, mov2, "GS-")                       X(addk, add, "SSK") X(subk, sub, "SSK") X(mulk, mul, "SSK") X(andk, and, "SSK") X(ork, or, "SSK")       X(eqk, eq, "SSK") X(nek, ne, "SSK") X(ltk, lt, "SSK") X(lek, le, "SSK") X(imink, imin, "SSK")           X(imaxk, imax, "SSK") X(faddk, fadd, "SSK") X(fsubk, fsub, "SSK") X(fmulk, fmul, "SSK")                 X(fltk, flt, "SSK") X(flek, fle, "SSK")
#define rio_SHORT_ENUM(o, k) rio_Op_s_##o,
#define rio_SHORT_ENUM_MORE(o, w, k) rio_Op_s_##o,
enum { rio_Op_s_before = 127, rio_SHORT_OPS(rio_SHORT_ENUM) rio_Op_s_more, rio_Op_s_moreBefore = rio_Op_s_more - 1,
       rio_SHORT_MORE(rio_SHORT_ENUM_MORE) rio_Op_s_end };
typedef struct { uint8_t op, a, b, c; } rio_Short;
#define rio_SHORT_BIAS rio_FRAME_BIAS
extern const uint8_t rio_shortOf[rio_Op_count]; /* a wide op's (first) short op, or 0 */
extern const uint8_t rio_wideOf[rio_Op_s_end - 128]; /* a short op's wide op */
extern const char rio_shortKinds[rio_Op_s_end - 128][4];
int rio_shortField(const char *kinds, int k);

/* the size in an instruction's b and c (a data word's, or zero's own) */
static inline uint32_t rio_dataSize(const rio_Instr *in) { return (uint32_t)in->b | (uint32_t)in->c << 16; }

static inline int rio_isJump(int op) { return op >= rio_Op_jmp && op <= rio_Op_forby; }
/* whether b op c is c op b (so the packer may swap them, to reach a short form) */
static inline int rio_commutes(int op) {
  return op == rio_Op_add || op == rio_Op_mul || op == rio_Op_and || op == rio_Op_or || op == rio_Op_eq || op == rio_Op_ne ||
         op == rio_Op_imin || op == rio_Op_imax || op == rio_Op_fadd || op == rio_Op_fmul;
}

/* which operands of an instruction are slots: bit 1 a, 2 b, 4 c (unused ones are 0, which never moves).
   (A data word's slot, a slice's or a builder op's, isn't among them: the vm reads it from mem's start.) */
static inline int rio_slotOperands(int op) {
  switch (op) {
  case rio_Op_ldw: case rio_Op_ldb: case rio_Op_lea: case rio_Op_ldw2: case rio_Op_stw: case rio_Op_stb: case rio_Op_stw2:
  case rio_Op_copy: case rio_Op_movn: return 3;
  case rio_Op_zero: case rio_Op_ffi: return 1;
  case rio_Op_call: case rio_Op_data: case rio_Op_jit: return 0;
  case rio_Op_calli: case rio_Op_getret: return 3;
  default: return rio_isJump(op) ? 3 : 7;
  }
}
/* The code in two forms. Compiled, as the compiler writes it (and the JIT and the C backend read it):
   pcs count instructions (vm->codeLen of them, a data word one too), operands name slots from mem's
   start, a call names its proc's entry. Run, as the vm runs it: pcs count 4-byte units (an instruction
   is two, for now: see rio/plans/smaller-instructions.md), a proc's code names its slots from its
   frame's start, biased (an operand is slot - frameStart + rio_FRAME_BIAS, as a signed 16-bit number in a
   wide instruction, 8 bits unsigned in a short one: so R moves with the running proc, the vm's R is mem
   + frameStart - rio_FRAME_BIAS, one base for both, and its own slots are small numbers; top-level code
   counts from mem's start the same way, its base 0), and a call
   names its proc by number. Jump targets, the procs' entries and ends, and vm->codeLen go with the form.
   rio_codeFrames(vm, 1) turns the code from vm->framedAt (units) on into the run form (the compiler,
   once frames are placed; the REPL, a piece at a time: procs from vm->framedProcs on are new);
   rio_codeFrames(vm, 0) turns all of it back (and 1 again after). The line table and the modules'
   ranges stay in instructions: rio_unitIndex turns a running pc into one. */
#define rio_FRAME_BIAS 128
void rio_codeFrames(rio_Vm *vm, int run);
/* the units an instruction at pc takes (its data word too): 1 short, 2 wide, 4 with a data word */
static inline uint32_t rio_unitsAt(const uint32_t *units, uint32_t pc, uint32_t len) {
  if ((units[pc] & 255) >= 128) return 1;
  return pc + 2 < len && (units[pc + 2] & 255) == rio_Op_data ? 4 : 2;
}
uint32_t rio_unitIndex(const rio_Vm *vm, uint32_t pc); /* a running pc's instruction (counting data words), for the line table and modules */
/* The code as compiled (rio_codeFrames' compiled form: slots from mem's start, pcs by instruction, calls
   by entry), unpacked into buf (room for vm->codeLen instructions: never more): how many instructions.
   unitOf[i]: instruction i's pc in units (room for vm->codeLen + 1); the procs' entries and ends turn
   into instructions too, theirs in units kept in kept (room for vm->procCount), for rio_procsInUnits to
   put back. For the JIT (in its room) and the C backend (in a buffer of its own). */
uint32_t rio_unpackInto(rio_Vm *vm, rio_Instr *buf, uint16_t *unitOf, uint16_t (*kept)[2]);
void rio_procsInUnits(rio_Vm *vm, const uint16_t (*kept)[2]);

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
  rio_TypeKind_param, /* a generic proc's T (elem: the proc, or rio_NO_PROC for a generic type's; count:
                         which of its type parameters), its size known only while running */
  rio_TypeKind_generic /* a generic type, Less :: (proc[T](...)), filled in where it's named, Less[Point]
                          (elem: the type with its Ts in it; count: how many; size: the first T's type) */
} rio_TypeKind;

#define rio_NO_PROC 0xFFFF

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
int rio_start(rio_Vm *vm, uint32_t pc, uint32_t base); /* (base: the running proc's frame, 0 for top-level code) */
void rio_logOp(rio_Vm *vm, const rio_Val *slots, const rio_Instr *ip);

#endif
