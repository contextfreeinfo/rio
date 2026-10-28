/* compiler.h - the compiler's state and the functions its files share.

   The compiler is single pass and recursion-free: expressions go through explicit value and operator
   stacks (shunting-yard), blocks through a block stack, and imported files through a resume stack.
   All of its state lives in one scratch buffer (rio_Compiler, then the tables it points into), which
   by default is overlaid on the part of mem that stays zero until the program runs.

   Errors don't jump: rio_fail records the first one and makes the rest of the source read as its end,
   so every loop that reads tokens finishes. rio_FAIL returns from the function at once with a harmless
   value; callers carry on with it, and anything they still do is thrown away. */
#ifndef rio_COMPILER_H
#define rio_COMPILER_H
#include "internal.h"
#ifndef rio_NO_COMPILER

/* ------------------------------------------------------------------ tokens */

typedef enum {
  rio_Tok_eof = 256, rio_Tok_id, rio_Tok_int, rio_Tok_float, rio_Tok_string,
  /* keywords, in the order of the lexer's table */
  rio_Tok_proc, rio_Tok_struct, rio_Tok_if, rio_Tok_else, rio_Tok_for, rio_Tok_in, rio_Tok_end, rio_Tok_return,
  rio_Tok_break, rio_Tok_continue, rio_Tok_import, rio_Tok_include, rio_Tok_enum, rio_Tok_union, rio_Tok_switch,
  rio_Tok_case, rio_Tok_is, rio_Tok_nil, rio_Tok_and, rio_Tok_or, rio_Tok_not, rio_Tok_do,
  /* operators of more than one character (one character: the character itself) */
  rio_Tok_colonColon,  /* :: */
  rio_Tok_colonEq,     /* := */
  rio_Tok_eqEq,        /* == */
  rio_Tok_notEq,       /* != */
  rio_Tok_lessEq,      /* <= */
  rio_Tok_greaterEq,   /* >= */
  rio_Tok_shiftLeft,   /* << */
  rio_Tok_shiftRight,  /* >> */
  rio_Tok_arrow,       /* -> */
  rio_Tok_rangeExcl,   /* ..< */
  rio_Tok_rangeIncl,   /* ..= */
  rio_Tok_opAssign,    /* += -= and so on: op is the operator */
} rio_TokKind;

typedef struct {
  int kind;    /* a rio_TokKind, or a single character */
  int line, col;
  int width;   /* in the source, quotes included */
  int len;     /* of text */
  int op;      /* rio_Tok_opAssign: the operator ('+', rio_Tok_shiftLeft, ...) */
  uint32_t at; /* where it starts in its file (a byte offset) */
  const char *text; /* (a string's: its bytes, escapes done, in the pool, at val.u) */
  rio_Val val; /* a number's value */
  uint8_t newline; /* the first token on its line */
} rio_Token;

/* messages that mean "unfinished" to the REPL, when the source ends */
extern const char rio_msgMissingEnd[], rio_msgUnclosedBracket[], rio_msgExpectedExpr[], rio_msgOptionalLast[];

/* ------------------------------------------------------------------ tables */

/* A type. Some fields mean more than their names, by kind:
     array: an [Enum]T array keeps its enum in firstField
     enum: its values are fields (rio_enumValue: 0, 1, 2 ... as listed); elem is the first constant of
           their names' (address, length) string pairs, for log and print
     union: each type it lists is a field (type: rio_TypeId_void for nil, offset 4); a maybe, ?T, is a
            union of nil and T, with T in elem (0 for the others)
     proc: the parameter types are fields; elem is the result (rio_TypeId_void: none); count, how many of
           the last parameters are optional (name?: T) */
typedef struct {
  uint8_t kind;    /* rio_TypeKind */
  uint8_t hasRefs; /* holds slices or strings somewhere (so it must be global) */
  uint16_t elem, firstField, fieldCount;
  uint32_t count;  /* arrays and builders: the number of elements */
  uint32_t size;   /* bytes */
} rio_Type;

typedef struct {
  uint16_t name, nameLen; /* name: offset in the compiler's names */
  uint16_t type, offset;  /* an enum value's 32 bits are split over these: high, low */
  uint8_t bitAt;          /* a bitfield (bits not 0): Int(bits), from bit bitAt of the word at offset */
  int8_t bits;
} rio_Field;

typedef struct {
  uint16_t name, nameLen;
  uint16_t type;
  uint16_t module;  /* where it's defined (0: built in, visible everywhere) */
  uint8_t kind;     /* rio_SymKind */
  uint8_t exported; /* name* */
  int32_t value;    /* by kind: see rio_SymKind */
} rio_Symbol;

/* A source file: a module, or a part included into one. Index 0 stands for the built-in names and 1 for
   the main source; files start at 2. key, dir and file are offsets in the names. */
enum { rio_ModuleState_compiling = 1, rio_ModuleState_done = 2 };
typedef struct {
  uint16_t key, keyLen; /* the path the loader was asked for */
  uint16_t dir, dirLen; /* where its own local imports and includes start */
  uint16_t file;        /* a 0-terminated name for error messages */
  uint16_t codeStart, codeEnd;
  uint8_t isPackage, state, isInclude;
} rio_Module;

/* a file's text, to read: all of it, in memory (src), or a part at a time (read, into the window) */
typedef struct {
  const char *src;
  uint32_t len;
  rio_SourceFn read;
  void *readData;
} rio_Text;

/* Where to resume the file that imported or included another: for an import, its import statement runs
   again (and just binds names); for an include, compiling continues after it. */
typedef struct {
  rio_Text text;
  uint32_t at, lineAt; /* where in it, and where that line starts (byte offsets) */
  int line, module, file, isInclude;
  int autoImport; /* 1 + which of vm->autoImports this file is (from the loader's source), else 0 */
} rio_Resume;

/* A proc while compiling. Frames: its slots are [frameStart, frameEnd). shared: the frame lives in the
   area procs that never run at the same time share, at [sharedStart, sharedEnd), past the frames of
   everything it calls; depth: its longest chain of calls (see compile.c). */
typedef struct {
  uint16_t entry, end, resultType, firstParam, paramCount, frameStart, frameEnd, sharedStart, sharedEnd, depth;
  uint32_t resultAt;
  uint32_t bigBytes;   /* bytes of its locals stored apart (over 16 words) */
  uint32_t bigTop;     /* its big locals are below this */
  uint8_t defined;     /* its end has been compiled: a call to it isn't recursion */
  uint8_t selfByRef;   /* a method on a struct: self is the receiver's address */
  uint8_t shared;
  uint8_t callsValues; /* it calls through a proc value, itself or through procs it calls (so it can't
                          be a value: that could recurse) */
  uint8_t usedAsValue;
  uint16_t optional;   /* how many of its last parameters are optional (name?: T): left out, they're zero */
} rio_ProcDef;

typedef struct {
  uint16_t type;
  uint16_t refType; /* a &T param's T (type is then the address word) */
  uint32_t addr;
} rio_Param;

typedef struct {
  uint16_t resultType, firstParam, paramCount, optional; /* optional: as declared (name?: T), the last ones */
} rio_FfiDef;

/* ------------------------------------------------------------------ expressions */

/* kinds of expression values; the order matters (values first, then things that aren't) */
typedef enum {
  rio_ExprKind_const,    /* a constant: value is the value */
  rio_ExprKind_static,   /* stored at a fixed address: value (slots too: a slot's address is 4 * it) */
  rio_ExprKind_indirect, /* stored at the address held in slot value, plus offset */
  rio_ExprKind_void,     /* no value */
  rio_ExprKind_proc,     /* a proc (value: which) */
  rio_ExprKind_ffi,
  rio_ExprKind_builtin,
  rio_ExprKind_type,
  rio_ExprKind_sliceEnd, /* the left-out end of xs[a..] (and xs[]) */
  rio_ExprKind_module,
} rio_ExprKind;

/* rio_Expr.access: why a value can't be assigned (0: it can) */
enum {
  rio_Access_readOnly = 1,
  rio_Access_boundMethod = 2, /* a method with its receiver just below on the value stack */
  rio_Access_ref = 4,         /* &place, not yet given to a & parameter or name := &place */
  rio_Access_narrowed = 8,    /* a narrowed union's copy */
};

typedef struct {
  uint8_t kind;       /* rio_ExprKind */
  uint8_t access;
  uint16_t type;
  uint16_t tempStart; /* the first free slot before its temporaries: freeing them goes back there */
  uint16_t localView : 1; /* a view of the current proc's own memory (read through a reference: its place
                             is that memory, or may hold such views) */
  uint16_t fromParam : 1; /* a view that came in through a parameter of the current proc (its caller's;
                             read through a reference: its place came in so) */
  uint16_t bitAt : 5;     /* a bitfield (bits not 0): bits wide, from bit bitAt of the word here */
  uint16_t bits : 5;
  uint16_t bitSigned : 1;
  int32_t value;      /* by kind: see rio_ExprKind */
  int32_t offset;     /* indirect: bytes past the address. (A const 0 with offset 1: xs[..<b]'s left-out start) */
} rio_Expr;

/* the operator stack: operators waiting for their right side, and open brackets */
typedef enum {
  rio_Pending_binary, rio_Pending_unary, rio_Pending_and, rio_Pending_or, /* operators */
  rio_Pending_paren, rio_Pending_call, rio_Pending_index, rio_Pending_literal, /* brackets */
} rio_PendingKind;

/* a, b and c by kind:
     and, or   a: the jump over the right side; b: the result's slot; and: c, argCount: the symbols and
               name bytes before the left side's `x is T`s narrowed x for the right side
     index     a: 1 once a ':' made it a slice
     call      a: the callee's rio_ExprKind (proc, static for a proc value, ffi, builtin);
               b: which (proc, slot holding the proc value, ffi function, rio_Builtin);
               c: a proc value's type; ffi: argument words placed so far (punAt: past the slots kept, with
               a struct argument: rio_ffiRoom); fromInt and has: the enum;
               format: the byte builder's slots; offsetOf and bitOffsetOf: the field path's bit offset + 1
     literal   a: the field being filled in (-1: punned, decided by its value); b: the struct type;
               c: the struct's address. op 1: an array or builder literal instead (b: its type; its
               values go in order, argCount so far, or by name: punAt 2, a the element named) */
typedef struct {
  uint8_t kind, prec;
  int16_t op;           /* binary, unary: the operator token */
  int32_t a, b, c;
  int32_t argCount;     /* call, literal: the arguments done so far */
  int32_t punAt;        /* literal: where a value standing for its field starts, in the source ({x});
                           an array's: 1 its values in order, 2 named (.name = value) */
  uint16_t valueBase;   /* the value stack's height when pushed: a call's arguments start there */
  uint16_t freeAtStart; /* the first free slot when pushed */
  uint64_t fieldsSet;   /* literal: which of its first 64 fields have values; format and print: the
                           slot of the byte builder's old length (and the ok flag after it), and for format,
                           shifted up 32, the slot holding its settings' address */
} rio_PendingOp;

/* ------------------------------------------------------------------ blocks */

/* loops last: break and continue look for kind >= rio_Block_loop */
typedef enum {
  rio_Block_proc, rio_Block_if, rio_Block_else, rio_Block_switch, rio_Block_do, rio_Block_loop, rio_Block_for, rio_Block_each
} rio_BlockKind;

/* a and b by kind:
     proc     a: the jump over its body; b: the proc
     if/else  a: the jump to the next branch, when the condition is false; b: the jumps to the end
     loops    a: the pc of the top; each with an index: b is the index's slot (else rio_NONE)
     switch   a: the slot of the union variable cases narrow (or rio_NONE); b: 1 once a case is seen,
              2 once the else is */
/* what a block knows about reaching its end, for missing returns (docs/spec/declarations.md#procs) */
enum {
  rio_Flow_startUnreachable = 1, /* the code before the block couldn't run */
  rio_Flow_branchesEnd = 2,      /* every branch so far ended where the code can't run */
  rio_Flow_complete = 4,         /* if: it has an else; switch: one of its branches always runs */
  rio_Flow_broken = 8            /* a loop: something breaks out of it */
};
typedef struct {
  uint8_t kind, flow;
  uint16_t symCount, nameBytes, liveSlots; /* the scope to return to at its end */
  uint16_t a, b;
  uint16_t breaks, continues; /* jump lists, to the end and to the next iteration */
  uint16_t condJump; /* while: the condition's jump out; switch: the last case's jump to the next one */
  uint16_t counter, limit; /* for: the slots; each: the element pointer and the end; switch: the
                              subject's slot and type */
  uint16_t caseSymCount, caseNameBytes, caseLiveSlots; /* switch: the scope each case starts from */
  uint16_t switchIndex; /* switch: its row in switchCases */
  uint16_t labelLen;    /* a loop's label (outer: for ...): its length (0: none) */
  char label[24];
} rio_Block;

/* ------------------------------------------------------------------ the compiler */

typedef struct rio_Compiler {
  int failed;         /* a compile error happened: the rest of the source reads as its end */
  int lexingNegative; /* lexing a -D value that had a minus sign */
  /* reading: the current file's text (src to srcEnd, which starts at srcAt in the file), where the
     next token's read from, and where its line starts; the token and the one after it, and where the
     token before ended */
  rio_Text text;
  const char *src, *cursor, *srcEnd;
  uint32_t srcAt, lineAt;
  /* a text read a part at a time: read into the window; the tokens read (tok, and peek after it) copied
     out of it when it moves on; names kept through a statement (rio_keep) too. All three at the top of
     the pool, only when compiling reads a text so (rio_carveWindow) */
  char *window, *tokenCopy[2], *keep;
  uint32_t windowSize, carved;
  int textDone;          /* the text's end has been read */
  int line, prevLine, prevCol, prevWidth;
  rio_Token tok, peek;
  /* names kept through a statement (rio_keep: copies, from a window, since its text moves on), and for
     a value standing for its field ({a.x}): the last name read, and how many tokens read weren't a path's */
  uint32_t keepUsed;
  char lastName[48];
  int lastNameLen, pathBreaks;
  /* the pc -> line table: (pc, line) entries growing down from lineTableTop at the top of the pool */
  uint32_t lineTableTop, lineCount, lastLine, lastLinePc, lineOverride;
  const char *failMessage; /* the last compile error, to tell "unfinished" from "wrong" */
  /* files: curFile is the one being read, curModule the module whose names are in scope (an
     included part shares its module's) */
  rio_Module *modules;
  int moduleCount, curModule, curFile, resumeCount, exporting;
  int afterOneLine; /* the statement before was a one-line case or else: case, else or end comes next */
  int autoSources[rio_MAX_AUTO_IMPORTS], autoSourceCount; /* auto-imports the loader has sources for */
  rio_Resume resume[rio_MAX_IMPORT_DEPTH];
  /* slots: liveSlots hold variables, temporaries start at freeSlot; slotHigh is the most used */
  uint32_t freeSlot, liveSlots, slotHigh;
  uint32_t lastLabel; /* the pc of the last jump target: no fusing instructions across it */
  uint32_t poolUsed, poolCap; /* the string pool */
  uint32_t bigBytes;  /* bytes of all big values */
  uint32_t bigWanted, bigLeft; /* a big value that didn't fit: its bytes, and the room there was (rio_Usage) */
  int curProc;        /* the proc being compiled, or -1 */
  int firstDefine;    /* the symbol of the first -D define */
  int targetType;     /* the type the next rio_parseExpr should produce, or -1 */
  int symCount, typeCount, fieldCount, paramCount, nameBytes, blockCount, valueCount, opCount;
  rio_Limits limits;  /* capacities of the tables below, all carved from scratch */
  rio_Limits high;    /* the most of the shrinking ones used: symbols, nameBytes, blocks, exprDepth (rio_Usage) */
  uint32_t room, source; /* the compiler's room, and the source's bytes when it's in mem (rio_Usage) */
  rio_Symbol *syms;
  rio_Type *types;
  rio_Field *fields;
  rio_ProcDef *procDefs;
  rio_Param *params;
  char *names;
  rio_Block *blocks;
  rio_Expr *values;   /* the expression parser's stacks */
  rio_PendingOp *ops;
  uint8_t *exposed;       /* slots whose address is taken (they can't be C locals in AOT, or shared) */
  uint8_t *constIsString; /* constants holding string pool offsets, relocated at the end */
  uint8_t *constIsAddr;   /* constants holding slot addresses, renumbered when frames are shared */
  uint8_t *localViews;    /* local slots that hold a view of their proc's own memory */
  uint8_t *paramViews;    /* local slots that hold a view that came in through a parameter */
  int storingArgs;        /* storing call arguments: views may go to a callee */
  int rangeStops;         /* a for header's range: ..< and ..= end its first expression (the loop reads them) */
  uint64_t switchCases[16][4]; /* per open switch: which enum values (or union types) have a case */
  int switchCount;
  uint16_t descType[32], descSlot[32]; /* types printed by descriptor (print.h), and where theirs are */
  int descCount;
  struct {
    int32_t resultAt; /* the slot carrying its result (-1 once used by anything but and) */
    int32_t varAt, member; /* x is T: x's address and T's index in the union */
  } isTests[8];           /* the `x is T`s in an if's condition */
  int16_t isTestCount;
  int16_t unreachable;    /* the code here can't run: after a return, break or continue (rio_Flow) */
  uint16_t *slotMap;      /* old slot -> new slot, while sharing frames */
  rio_Instr *scratchCode; /* the code while compiling, when it ends up in mem */
  rio_FfiDef ffi[rio_MAX_FFI];
  uint8_t *pool;          /* string pool: the rest of scratch */
} rio_Compiler;

/* ------------------------------------------------------------------ small helpers */

static inline rio_Type *rio_type(rio_Vm *vm, int t) { return &vm->compiler->types[t]; }
static inline rio_Field *rio_field(rio_Vm *vm, int f) { return &vm->compiler->fields[f]; }
/* the type of a value read from storage of type t (a byte is read as an Int) */
static inline int rio_valueType(int t) { return t == rio_TypeId_byte ? rio_TypeId_int : t; }
static inline int rio_typeWords(rio_Vm *vm, int t) { return (int)((rio_type(vm, t)->size + 3) / 4); }
static inline int32_t rio_enumValue(const rio_Field *f) { return (int32_t)((uint32_t)f->type << 16 | f->offset); }
static inline int rio_isAlpha(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
static inline int rio_isDigit(int c) { return c >= '0' && c <= '9'; }
static inline int rio_isSpace(int c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
/* is the current token the name `word`? */
static inline int rio_tokIs(rio_Vm *vm, const char *word, int len) {
  const rio_Token *t = &vm->compiler->tok;
  return t->kind == rio_Tok_id && t->len == len && !memcmp(t->text, word, (size_t)len);
}
/* at `host proc` or `host struct`, after a declaration's :: (host is a name anywhere else) */
static inline int rio_atHost(rio_Vm *vm) {
  int next = vm->compiler->peek.kind;
  return rio_tokIs(vm, "host", 4) && (next == rio_Tok_proc || next == rio_Tok_struct);
}
static inline int rio_nameIs(rio_Vm *vm, uint16_t name, uint16_t nameLen, const char *s, int len) {
  return nameLen == len && !memcmp(vm->compiler->names + name, s, (size_t)len);
}

static inline rio_Expr rio_exprOf(int kind, int type, int32_t value) {
  rio_Expr e;
  e.kind = (uint8_t)kind;
  e.access = 0;
  e.localView = 0;
  e.fromParam = 0;
  e.bitAt = 0;
  e.bits = 0;
  e.bitSigned = 0;
  e.type = (uint16_t)type;
  e.tempStart = 0;
  e.value = value;
  e.offset = 0;
  return e;
}
static inline rio_Expr rio_voidExpr(void) { return rio_exprOf(rio_ExprKind_void, rio_TypeId_void, 0); }
/* moves a place n bytes on (a field or element) */
static inline void rio_offsetPlace(rio_Expr *e, int32_t n) {
  if (e->kind == rio_ExprKind_static) e->value += n;
  else e->offset += n;
}
/* operands are 16 bits, so they name only the slots below this byte address */
#define rio_SLOTS_END 0x40000

/* ------------------------------------------------------------------ errors (lexer.c) */

static inline int rio_failed(rio_Vm *vm) { return vm->compiler->failed; }
void rio_fail(rio_Vm *vm, const char *message);
void rio_failAtToken(rio_Vm *vm, const char *message);
void rio_failAt(rio_Vm *vm, int line, int col, int width, const char *message);
#define rio_FAIL(result, message) \
  do {                            \
    rio_fail(vm, message);        \
    return result;                \
  } while (0)
#define rio_FAIL_AT_TOKEN(result, message) \
  do {                                     \
    rio_failAtToken(vm, message);          \
    return result;                         \
  } while (0)

/* ------------------------------------------------------------------ lexer.c */

void rio_lex(rio_Vm *vm, rio_Token *t);
void rio_next(rio_Vm *vm);
/* reading text from at (where its line starts at lineAt, and it's line line) */
void rio_readFrom(rio_Vm *vm, uint32_t at, uint32_t lineAt, int line);
/* the text from the current token's start, with enough after it to look ahead by hand (half the
   window, if it's read a part at a time): where it is (end: c->srcEnd) */
const char *rio_lookAhead(rio_Vm *vm);
/* where p, in the text being read, is in its file */
static inline uint32_t rio_at(const rio_Compiler *c, const char *p) { return c->srcAt + (uint32_t)(p - c->src); }
/* a name from a token, kept through the rest of the statement (the token's own text moves on) */
const char *rio_keep(rio_Vm *vm, const char *s, int n);
/* a token's text, at most (a name's or a number's, everywhere alike), and room for names kept */
#define rio_TOKEN_MOST 256
#define rio_KEEP_BYTES 1024
/* room for reading a text a part at a time (the window, and what goes with it), at the top of the
   pool: before anything's in it */
void rio_carveWindow(rio_Vm *vm);
void rio_expect(rio_Vm *vm, int kind, const char *message);
int rio_peekAfterNext(rio_Vm *vm);
int rio_escapeAt(const char *p, const char *e, int *used);
int rio_lexNumber(rio_Vm *vm, const char *text, rio_Token *t);

/* ------------------------------------------------------------------ tables.c */

int rio_addName(rio_Vm *vm, const char *s, int n);
void rio_addSymbol(rio_Vm *vm, const char *s, int n, int kind, int type, int32_t value);
int rio_lookup(rio_Vm *vm, const char *s, int n);
int rio_lookupExport(rio_Vm *vm, int module, const char *s, int n);
int rio_lookupMethod(rio_Vm *vm, int type, const char *s, int n);
int rio_newField(rio_Vm *vm);
int rio_newType(rio_Vm *vm, int kind, int elem, uint32_t count, uint32_t size);
int rio_sliceOf(rio_Vm *vm, int elem);
int rio_arrayOf(rio_Vm *vm, int elem, uint32_t count);
int rio_enumArrayOf(rio_Vm *vm, int elem, int enumType);
int rio_procTypeOf(rio_Vm *vm, const int *params, int n, int result, int optional);
int rio_builderOf(rio_Vm *vm, int elem, uint32_t count);
int rio_maybeOf(rio_Vm *vm, int t);
int rio_builderViewOf(rio_Vm *vm, int elem);
int rio_findField(rio_Vm *vm, const rio_Type *type, const char *s, int n);
/* text written out (out: into it, up to cap), or compared (want: with it, cap bytes) */
typedef struct {
  char *out;
  const char *want;
  uint32_t n, cap;
  int differs;
} rio_HostText;
void rio_hostPut(rio_HostText *o, const char *s, int len);
void rio_hostPutInt(rio_HostText *o, int32_t v);
void rio_layoutOf(rio_Vm *vm, int type, int bits, rio_HostText *o);
int rio_findEnumValue(rio_Vm *vm, int type, const char *s, int n, int32_t *value);
int rio_enumIndex(rio_Vm *vm, int type, int32_t value);
int rio_isBig(const rio_Type *type);
int rio_isAggregate(int kind);
int rio_isViewKind(int kind);

/* ------------------------------------------------------------------ codegen.c */

uint32_t rio_compiledLine(rio_Vm *vm, uint32_t pc);
int rio_emit(rio_Vm *vm, int op, int a, int b, int c);
void rio_emitX(rio_Vm *vm, int op, int x, int a);
void rio_emitData(rio_Vm *vm, int a, uint32_t size);
int rio_label(rio_Vm *vm);
int rio_jumpListAdd(rio_Vm *vm, int list, int jump);
void rio_patchJumps(rio_Vm *vm, int list, int to);
int rio_allocSlots(rio_Vm *vm, int n);
uint32_t rio_allocHigh(rio_Vm *vm, uint32_t bytes);
int rio_findConsts(rio_Vm *vm, const uint32_t *values, int n, int addrBits);
int rio_constSlot(rio_Vm *vm, uint32_t value);
uint32_t rio_expose(rio_Vm *vm, uint32_t addr, uint32_t bytes);
int rio_addrConst(rio_Vm *vm, uint32_t addr, uint32_t bytes);
int rio_retarget(rio_Vm *vm, int from, int to);
int rio_isTemp(rio_Vm *vm, const rio_Expr *e);
int rio_isLocalView(rio_Vm *vm, const rio_Expr *e);
void rio_slotMarks(rio_Vm *vm, rio_Expr *e);
void rio_partMarks(rio_Vm *vm, rio_Expr *e);
void rio_markHeld(rio_Vm *vm, int type, uint32_t addr, int local, int param);
int rio_isLocalMemory(rio_Vm *vm, const rio_Expr *e);
void rio_markLocalView(rio_Vm *vm, uint32_t addr, int words, int on);
void rio_markParamView(rio_Vm *vm, uint32_t addr, int words, int fromParam);
void rio_needValue(rio_Vm *vm, rio_Expr *e);
int rio_descriptor(rio_Vm *vm, int type);
int rio_printOperand(rio_Vm *vm, rio_Expr *e);
void rio_toProcValue(rio_Vm *vm, rio_Expr *e, int type);
void rio_coerce(rio_Vm *vm, rio_Expr *e, int type);
int rio_toSlot(rio_Vm *vm, rio_Expr *e, int reuse);
int rio_toSlot2(rio_Vm *vm, rio_Expr *e);
int rio_toSlot3(rio_Vm *vm, rio_Expr *e);
int rio_toViewSlots(rio_Vm *vm, rio_Expr *e);
int rio_toAddrSlot(rio_Vm *vm, rio_Expr *e);
void rio_store(rio_Vm *vm, rio_Expr *dst, rio_Expr *src);
int rio_invertJump(rio_Instr *in);
int rio_jumpIfFalse(rio_Vm *vm, rio_Expr *e);

/* ------------------------------------------------------------------ operators.c */

void rio_binaryOp(rio_Vm *vm, int op, rio_Expr *l, rio_Expr *r);
void rio_unaryOp(rio_Vm *vm, int op, rio_Expr *e);
void rio_elementwiseOp(rio_Vm *vm, int op, rio_Expr *l, rio_Expr *r, rio_Expr *into);
int rio_numberArrayInfo(rio_Vm *vm, int type, int *count);
int rio_unionMember(rio_Vm *vm, int unionType, const rio_Expr *e);
int rio_storeIntoUnion(rio_Vm *vm, rio_Expr *dst, rio_Expr *src);
void rio_isTestUsed(rio_Vm *vm, const rio_Expr *e, int carry);
void rio_narrowIsTests(rio_Vm *vm, const rio_Expr *cond);
void rio_narrow(rio_Vm *vm, int32_t varAt, int member, int temp);
void rio_narrowRight(rio_Vm *vm, int resultAt);
#define rio_REF_AT 0x40000000 /* (in a varAt: a reference's slot, such as a loop's element, not a variable's address) */
int32_t rio_narrowableAt(rio_Vm *vm, const rio_Expr *e);
int rio_narrowedSymbol(rio_Vm *vm, const rio_Expr *e);

/* ------------------------------------------------------------------ expr.c */

rio_Expr *rio_valueTop(rio_Vm *vm);
rio_Expr rio_popValue(rio_Vm *vm);
void rio_pushResult(rio_Vm *vm, rio_Expr e, int tempStart);
rio_Expr rio_symbolExpr(rio_Vm *vm, int sym);
rio_Expr rio_stringConst(rio_Vm *vm, const char *s, int n);
rio_Expr rio_stringInPool(rio_Vm *vm, uint32_t at, int n);
rio_Expr rio_defineValue(rio_Vm *vm, int define, int type);
rio_Expr rio_parseExpr(rio_Vm *vm);
int32_t rio_parsePositiveConst(rio_Vm *vm);

/* ------------------------------------------------------------------ builtins.c */

/* the builtins, in the order of rio_builtinNames */
typedef enum {
  rio_Builtin_log, rio_Builtin_len, rio_Builtin_min, rio_Builtin_max, rio_Builtin_abs,
  rio_Builtin_sqrt, rio_Builtin_sin, rio_Builtin_cos, rio_Builtin_tan, rio_Builtin_asin, rio_Builtin_acos,
  rio_Builtin_atan, rio_Builtin_exp, rio_Builtin_ln, rio_Builtin_floor, rio_Builtin_ceil, rio_Builtin_round,
  rio_Builtin_atan2, rio_Builtin_pow, rio_Builtin_fmod, /* (from here on, two arguments) */
  rio_Builtin_assert,
  /* builder methods */
  rio_Builtin_push, rio_Builtin_pop, rio_Builtin_clear, rio_Builtin_cap, rio_Builtin_remove,
  rio_Builtin_swapRemove, rio_Builtin_pushAll, rio_Builtin_format, rio_Builtin_print,
  /* byte methods (Blobs, byte builders) */
  rio_Builtin_pack, rio_Builtin_unpack, rio_Builtin_getInt, rio_Builtin_setInt,
  /* conversion methods */
  rio_Builtin_toInt, rio_Builtin_toFloat, rio_Builtin_toBool, rio_Builtin_asString,
  /* Enum.fromInt and Enum.has */
  rio_Builtin_fromInt, rio_Builtin_has,
  rio_Builtin_hexBlob,
  /* a type's layout: sizeOf(T), offsetOf(S, .f), bitOffsetOf(S, .f) */
  rio_Builtin_sizeOf, rio_Builtin_offsetOf, rio_Builtin_bitOffsetOf,
  /* a Range's methods */
  rio_Builtin_rangeLen, rio_Builtin_rangeHas,
  rio_Builtin_count
} rio_Builtin;
extern const char *const rio_builtinNames[rio_Builtin_count];
/* the built-in struct Format (b.format's settings): the first type after the built-in kinds */
#define rio_TypeId_format rio_TypeId_builtinCount
#define rio_TypeId_range (rio_TypeId_builtinCount + 1) /* Range{first, last}: a..<b, a..=b */

int rio_builtinNamed(const char *s, int n, int first, int last);
void rio_logEnumName(rio_Vm *vm, rio_Expr *e, int space);
void rio_logArg(rio_Vm *vm, rio_PendingOp *call);
void rio_formatArg(rio_Vm *vm, rio_PendingOp *call);
void rio_finishBuiltinCall(rio_Vm *vm, rio_PendingOp *call);

/* ------------------------------------------------------------------ decls.c */

int rio_parseType(rio_Vm *vm);
void rio_procDef(rio_Vm *vm, const char *name, int nameLen, int receiver);
int rio_beginProc(rio_Vm *vm, const char *name, int nameLen, int receiver);
void rio_procParam(rio_Vm *vm, int proc, const char *name, int nameLen, int type, int how);
void rio_setProcResult(rio_Vm *vm, int proc, int type);
void rio_beginBody(rio_Vm *vm, int proc);
void rio_methodDef(rio_Vm *vm, int receiver);
void rio_structDef(rio_Vm *vm, const char *name, int nameLen);
void rio_unionDef(rio_Vm *vm, const char *name, int nameLen, int diverse);
void rio_enumDef(rio_Vm *vm, const char *name, int nameLen);
void rio_declareVar(rio_Vm *vm);
int rio_parseNames(rio_Vm *vm, const char **names, int *lens, int *optional);
void rio_destructure(rio_Vm *vm);

/* ------------------------------------------------------------------ statements.c */

rio_Block *rio_pushBlock(rio_Vm *vm, int kind);
void rio_endScope(rio_Vm *vm, const rio_Block *b);
void rio_statement(rio_Vm *vm);
void rio_finishProc(rio_Vm *vm);

/* ------------------------------------------------------------------ modules.c */

void rio_importStatement(rio_Vm *vm);
void rio_includeStatement(rio_Vm *vm);
void rio_endFile(rio_Vm *vm);

/* ------------------------------------------------------------------ host.c */

void rio_hostDecl(rio_Vm *vm, const char *name, int nameLen, int receiver, int line, int col);
int rio_atTypeMethod(rio_Vm *vm);
void rio_typeMethodDecl(rio_Vm *vm);
int rio_hasHostProcs(rio_Vm *vm, const char *key, int keyLen);
int rio_hostModule(rio_Vm *vm, const char *key, int keyLen);
void rio_checkHostModule(rio_Vm *vm, int module);

/* ------------------------------------------------------------------ compile.c and repl.c */

int rio_startCompiler(rio_Vm *vm, void *scratch, uint32_t size, const rio_Limits *lim, int repl, uint32_t below);
void rio_setupCompiler(rio_Vm *vm, int sources);
void rio_openAutoImports(rio_Vm *vm);
void rio_compileAll(rio_Vm *vm);
void rio_fillRuntimeProc(rio_Vm *vm, rio_Proc *out, int proc);
int rio_replLookup(rio_Vm *vm, const char *name, int kind);
const char *rio_replFileOfPc(rio_Vm *vm, uint32_t pc);

#endif /* rio_NO_COMPILER */
#endif
