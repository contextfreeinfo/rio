/* rio - a tiny statically typed scripting language. C99, no allocation, no recursion.

   Naming: everything rio defines starts with rio_. Functions are rio_camelCase, types rio_PascalCase,
   enum values rio_EnumType_value, and macros rio_UPPER_CASE. */
#ifndef rio_H
#define rio_H
#include <stdint.h>

/* ------------------------------------------------------------------ build-time configuration */

/* Fixed caps for things the host registers before compiling; override any of them with -D.
   rio_SMALL picks MCU-sized values. Compiler table sizes are chosen per compile instead: see
   rio_Limits.
   The host can look up (rio_findProc, rio_findGlobal) main and the main file's names marked name*.
   Build with rio_EXPORTS_ALL to make every top-level proc and global findable (each costs a 12-byte
   entry plus its name). */
#ifdef rio_SMALL
#define rio_PICK(big, small) small
#else
#define rio_PICK(big, small) big
#endif
#ifndef rio_MAX_FFI
#define rio_MAX_FFI rio_PICK(512, 16)
#endif
#ifndef rio_MAX_DEFINES
#define rio_MAX_DEFINES rio_PICK(64, 8)
#endif
#ifndef rio_MAX_IMPORT_DEPTH
#define rio_MAX_IMPORT_DEPTH 16
#endif
#ifndef rio_LOG_BUF_SIZE
#define rio_LOG_BUF_SIZE rio_PICK(256, 96)
#endif
#ifndef rio_MAX_SOURCES
#define rio_MAX_SOURCES rio_PICK(8, 4)
#endif
#ifndef rio_MAX_AUTO_IMPORTS
#define rio_MAX_AUTO_IMPORTS rio_PICK(8, 2)
#endif
#ifndef rio_ERROR_BUF_SIZE
#define rio_ERROR_BUF_SIZE rio_PICK(192, 96)
#endif

/* ------------------------------------------------------------------ basic types */

/* one word of script memory */
typedef union {
  int32_t i;
  float f;
  uint32_t u;
} rio_Val;

/* one bytecode instruction: an opcode, a small extra operand, and three 16-bit operands */
typedef struct {
  uint8_t op, x;
  uint16_t a, b, c;
} rio_Instr;

typedef struct rio_Vm rio_Vm;

/* ffi: args[0..] hold the arguments (strings and blobs take 2 words: address, length); write the
   result to args[0] */
typedef void (*rio_FfiFn)(rio_Vm *vm, rio_Val *args);
typedef void (*rio_LogFn)(void *userData, const char *text, int len);

/* ------------------------------------------------------------------ setup */

/* mem: all script data (globals, frames, arrays, strings), and by default also the compiler's
   working memory, which is overlaid on space that stays zero until the program runs.
   code: bytecode buffer (max 65535 instructions), or NULL to keep the code in mem too: then it takes
   just its own size there, below the strings (codeCap is ignored; see rio_Limits.code). mem is used from
   its first 16-byte boundary, in whole 16 bytes: a buffer that isn't aligned loses up to 15 bytes, so to
   load an image made with as much memory, give it 16 bytes more (or align it). */
int rio_init(rio_Vm *vm, void *mem, uint32_t memSize, rio_Instr *code, uint32_t codeCap);
void rio_setLog(rio_Vm *vm, rio_LogFn fn, void *userData);

/* A host proc. sig: params then optional '>' and result. i=i32 f=f32 s=string b=blob, [x] a slice of
   x (one of those four: [s] is a []String, 2 words, where its elements start and how many), a struct
   parameter as its layout in braces, {x:Int,y:Int} (rio_hostHeader writes them), and & before i, f or
   a struct for a place the host may change; e.g. "ff>f".
   name and sig must stay valid until rio_compile (or rio_loadImage) returns.
   A plain name is global. A dotted one, "pc.cls", is in module pc instead: `import pc` (or
   rio_autoImport) gives pc.cls. If the loader has a source for pc, that's the module, and it must
   declare each of pc's procs: `cls* :: host proc(c: Int)`, checked against the sig. Otherwise the
   module is made from the registrations, with nothing to parse. */
int rio_addFfi(rio_Vm *vm, const char *name, const char *sig, rio_FfiFn fn);
/* the source of host module `module` (its host procs declared, its types and constants): compiling
   takes it in place of asking the loader, so declarations go with the host they declare. src must stay
   valid as long as the vm. Images keep its name and hash (rio_hashText: FNV-1a, with \r skipped, so
   line ends don't change it), for messages when one doesn't fit.
   rio_hostSource gives a registered source back (for help or an editor), or NULL; *len its length */
int rio_addSource(rio_Vm *vm, const char *module, const char *src, uint32_t len);
const char *rio_hostSource(rio_Vm *vm, const char *module, uint32_t *len);
uint32_t rio_hashText(const char *s, uint32_t n);
/* makes a module of host procs (see rio_addFfi) visible everywhere without an import, as `name.proc`.
   If the loader has a source for it (a package, `name`), rio_compile compiles that first, so it can
   hold types and constants too (an enum may stand for an Int in its host procs), and a REPL's
   rio_replBegin compiles it before any input; otherwise it's made from the registrations */
int rio_autoImport(rio_Vm *vm, const char *module);

/* ------------------------------------------------------------------ images */

/* A compiled program, ready to run, for loading into a vm (even one built with rio_NO_COMPILER,
   which leaves the compiler out). Save right after compiling, before running.
   The image holds absolute addresses, so the loading vm needs at least the memory it was compiled
   with (and room for its code); register the ffi functions it calls (by name, in any order) before
   loading, each with the sig it had, or one with more parameters at its end (an image passes zeros for
   those) and the same result. Little-endian hosts only. rio_saveImage returns the image's size (out NULL: just
   the size), or 0 if it doesn't fit or there's nothing to save. */
#ifndef rio_NO_COMPILER
uint32_t rio_saveImage(rio_Vm *vm, void *out, uint32_t cap);
/* C for a host to write its host procs against (src/header.c, which the rio CLI has): compiles src, a module that declares them (host procs
   this vm lacks are fine here), with the compiler in scratch, and writes into out (cap bytes): for each
   struct a host proc takes, and each inside one, a C struct of the same layout, accessors for its
   bitfields and name_LAYOUT; and for each host proc, name_SIG, its sig. Each name starts with prefix
   (pc: pc_Text, pc_text_SIG). Returns the length written (0-terminated), or 0: an error (rio_error), or
   too little room. */
uint32_t rio_hostHeader(rio_Vm *vm, const char *src, uint32_t len, void *scratch, uint32_t scratchSize, const char *prefix,
                        char *out, uint32_t cap);
/* src (len bytes of Rio code), laid out the one way (src/fmt.c, rio fmt), into out (cap bytes): only
   spaces change, never a token or a line break, and it checks that before giving it back. Needs no vm,
   and the code needn't compile. Returns the length written (0-terminated), or -1: too little room (the
   same lines, with their indents, may need more than len), or -2: it couldn't keep the tokens the same
   (a bug in rio_format: keep src as it is). */
int32_t rio_format(const char *src, uint32_t len, char *out, uint32_t cap);
/* The kinds of Rio's tokens, for colors (src/fmt.c): an editor's, a terminal's (the REPL's), and a
   language server's semantic tokens (keyword, type, function, number, string, comment, operator). As
   rinc's theme roles have them */
enum { rio_Hl_code, rio_Hl_keyword, rio_Hl_type, rio_Hl_call, rio_Hl_number, rio_Hl_string, rio_Hl_comment, rio_Hl_punct };
typedef void (*rio_HighlightFn)(void *userData, uint32_t at, uint32_t len, int kind);
/* each of src's tokens (len bytes of Rio code, compiling or not), in order, through fn: where it is in
   src, how long, and its kind (rio_Hl_*): a word of Rio's, a type (a name with a capital), a call (a name
   right before its "("), a number, a string or character, a comment, punctuation, else code. Needs no
   vm. Returns how many */
int rio_highlight(const char *src, uint32_t len, rio_HighlightFn fn, void *userData);
#endif
int rio_loadImage(rio_Vm *vm, const void *image, uint32_t len);
/* The same, a part at a time, for a host without room for a whole image beside the vm (such as one
   saving to a file and loading from it): rio_writeImage hands write the image's bytes in order (each
   write returns 0, or nonzero to stop), and returns its size, or 0. rio_readImage asks read for an
   image of len bytes in order (exactly n each time: 0, or nonzero if it can't), and puts each part
   straight in place. A failed rio_readImage leaves the vm with nothing loaded. */
typedef int (*rio_WriteFn)(void *userData, const void *bytes, uint32_t n);
typedef int (*rio_ReadFn)(void *userData, void *bytes, uint32_t n);
#ifndef rio_NO_COMPILER
uint32_t rio_writeImage(rio_Vm *vm, rio_WriteFn write, void *userData);
#endif
int rio_readImage(rio_Vm *vm, rio_ReadFn read, void *userData, uint32_t len);

/* ------------------------------------------------------------------ compiling */

#ifndef rio_NO_COMPILER
/* like gcc -D: overrides a top-level constant `NAME :: default`, or defines NAME if the script
   doesn't declare it. value is parsed as the declared constant's type (i32, f32, bool or string);
   NULL means true. name and value must stay valid until rio_compile returns. */
int rio_define(rio_Vm *vm, const char *name, const char *value);
#endif

/* Modules and includes come from the host. kind is a combination of rio_LoadKind flags:
     package  the path is in the library paths (otherwise relative to the main file's directory)
     file     the path names an exact file, from `include "x.rio"`; without it, the path names a
              module: try "path.rio", then the directory's entry file "path/<last>.rio"
   `import .ui.button` asks for module "ui/button"; `import tween` for package module "tween";
   `include "parts/a.rio"` inside shapes/shapes.rio for file "shapes/parts/a.rio".
   Return 0 and fill *out (src must stay valid until compiling ends; isDir: a module path named a
   directory and its entry file was used; name: optional text for error messages), 1 if not found,
   2 if found in more than one place. A source can be read a part at a time instead (src NULL): read,
   with readData, as for rio_compileStream; whatever readData points to must stay valid till then. */
enum { rio_LoadKind_package = 1, rio_LoadKind_file = 2 };
/* reads up to n bytes of a source, from byte at, into buf: how many (0: its end), or -1 if it can't */
typedef int (*rio_SourceFn)(void *userData, char *buf, uint32_t at, uint32_t n);
typedef struct {
  const char *src;
  uint32_t len;
  int isDir;
  const char *name;
  rio_SourceFn read;
  void *readData;
} rio_Source;
typedef int (*rio_LoadFn)(void *userData, const char *path, int kind, rio_Source *out);
void rio_setLoader(rio_Vm *vm, rio_LoadFn fn, void *userData);

/* Compiler capacities. consts is also how many words of mem are kept for constants, and slots (at
   most 65536, counting the constants) caps the register-addressable part of mem. code: room for
   instructions while compiling, when the code lives in mem, to start with: it takes more from the string
   pool's free room as it needs it (a REPL keeps that much for good, and no more). */
typedef struct {
  uint32_t symbols, nameBytes, types, fields, procs, params, consts, slots, blocks, exprDepth, modules, code;
} rio_Limits;

/* How full compiling got, and where the program's memory goes (rio_trackUsage), for showing how close a
   program is to its limits. In bytes, except the limits. The compiler's tables are sized to fill its
   room, so what runs out first is whichever `used` comes nearest its `cap` */
typedef struct {
  rio_Limits cap, used; /* the compiler's tables: their capacities, and the most of each used */
  uint32_t room;        /* the compiler's room: its tables and the string pool (in mem: above the constants,
                           below any source) */
  uint32_t source;      /* the source, when it sat in mem while compiling (else 0) */
  uint32_t window;      /* the room for reading sources a part at a time, out of the string pool (else 0) */
  uint32_t strings, stringCap; /* the string pool: strings, names for the host, the line table */
  /* running, adding up to mem: constants; globals, frames, the call stack and arrays; code (when in
     mem); strings and the runtime tables; left over */
  uint32_t mem, consts, data, code, tables, free;
  uint32_t wanted, left; /* compiling ran out of memory for a big value (an array or struct, over 16
                            words): its bytes, and the room there was for it (else 0) */
} rio_Usage;

#ifndef rio_NO_COMPILER
rio_Limits rio_limitsFor(uint32_t bytes);      /* limits whose tables fill about 60% of `bytes` */
uint32_t rio_limitsSize(const rio_Limits *lim); /* scratch bytes those tables take; strings need more */
/* compile with the compiler's working memory overlaid on the part of mem that stays zero until the
   program runs; limits are scaled to mem. The source can be in mem too, at its top (where a host with no
   other room can read a file): the compiler's memory then ends below it, and compiling needs the source,
   the compiler's tables, the constants and the strings to fit in mem together. The source's memory is
   the program's once compiling is done */
int rio_compile(rio_Vm *vm, const char *src, uint32_t len);
/* each compile from now on fills in *usage (one that fails, as far as it got: the running part is then
   0). After rio_init, which forgets it */
void rio_trackUsage(rio_Vm *vm, rio_Usage *usage);
/* usage as lines of text for people, into out (cap bytes, 0-terminated): its length */
int rio_usageText(const rio_Usage *u, char *out, int cap);
/* same, with the compiler's working memory in a separate buffer that stays valid afterwards (rio_aot
   needs it); limits are scaled to the buffer */
int rio_compileScratch(rio_Vm *vm, const char *src, uint32_t len, void *scratch, uint32_t size);
/* either of the above with explicit limits: scratch NULL overlays mem; lim NULL scales */
int rio_compileWith(rio_Vm *vm, const char *src, uint32_t len, void *scratch, uint32_t size, const rio_Limits *lim);
/* as rio_compile, the main source read a part at a time (from a file, say), through read: it needn't be
   in memory whole, nor any source a loader gives so (rio_Source). The compiler reads through a window
   of its own, rio_setSourceWindow's bytes (2048 by default; set before compiling): a name, a number
   or one line of a type method's declaration has to fit in half of it. The window comes out of the
   string pool, only when compiling reads this way: rio_compileStream, or (for a loader's sources read
   so, the main one in memory) rio_setSourceWindow set */
int rio_compileStream(rio_Vm *vm, rio_SourceFn read, void *userData);
void rio_setSourceWindow(rio_Vm *vm, uint32_t bytes);
/* A check: a source compiled for what it says, not to run: its first error, if any (rio_error,
   rio_errorInfo), and what it declares, in scratch's tables, for rio_complete (an editor's, a language
   server's). The program gets no memory: mem holds only the constants (4 bytes each; a few KB
   is plenty), so a small vm checks a big program, and the code is kept a top-level statement at a
   time (scratch needs room for the biggest). The source is src (len bytes), or read a part at a time
   through read (src NULL); imports come through the loader, as for compiling. An editor checks its
   text up to the start of the cursor's line: the error there (missing 'end', say) is expected, and
   the blocks open at that point, a proc's parameters and locals among them, stay in the tables. Then
   rio_complete takes the text from there (the cursor's line, to the cursor): the rest is in the tables,
   in scope or not, as it should be. Returns 0 (no error) or -1; the vm isn't left ready to run either way */
int rio_check(rio_Vm *vm, const char *src, uint32_t len, rio_SourceFn read, void *readData, void *scratch, uint32_t size);
#endif

/* ------------------------------------------------------------------ running */

int rio_run(rio_Vm *vm);                          /* runs top-level code */
int rio_findProc(rio_Vm *vm, const char *name);   /* -1 if missing */
rio_Val *rio_procArgs(rio_Vm *vm, int proc);      /* params, consecutive words in declaration order */
rio_Val *rio_procResult(rio_Vm *vm, int proc);
int rio_call(rio_Vm *vm, int proc);
void *rio_findGlobal(rio_Vm *vm, const char *name); /* address of a global variable, or NULL */
void *rio_sliceBytes(rio_Vm *vm, const rio_Val *slice); /* bytes of a string/blob/slice value */
/* A struct argument to a host proc (a sig's {...}: its layout) is 2 words: where it is, and its size.
   rio_getStruct copies it into out, a C struct of size bytes (as rio_hostHeader writes them), with
   zeros past the struct it's given (from an image made when it had fewer fields at its end). A String
   or Blob in it is a rio_View, and rio_viewBytes gives its bytes. A &T parameter (&i, &f, &{...}) is
   the same 2 words, of the caller's place: rio_putStruct writes it, no more of in than it holds. */
typedef struct { uint32_t at, len; } rio_View;
void rio_getStruct(rio_Vm *vm, const rio_Val *arg, void *out, uint32_t size);
void rio_putStruct(rio_Vm *vm, const rio_Val *arg, const void *in, uint32_t size);
void *rio_viewBytes(rio_Vm *vm, rio_View v);
void rio_trap(rio_Vm *vm, const char *message);   /* call from ffi to abort with an error */
/* break: safe from a signal handler, an interrupt or another core. A running script stops at its next
   loop iteration with the runtime error "interrupted". */
void rio_interrupt(rio_Vm *vm);

/* ------------------------------------------------------------------ REPL */

#ifndef rio_NO_COMPILER
/* compile and run code a piece at a time; globals, procs, types and methods carry over. The compiler
   keeps its state in scratch for the whole session. A bare expression prints its value. */
enum { rio_ReplResult_error = -1, rio_ReplResult_ok = 0, rio_ReplResult_more = 1 };
int rio_replBegin(rio_Vm *vm, void *scratch, uint32_t size, const rio_Limits *lim);
/* ok: ran. error: see rio_error (a compile error leaves the session unchanged). more: the input is
   unfinished (an open block or bracket): call again with the same text plus more appended. */
int rio_replEval(rio_Vm *vm, const char *src, uint32_t len);
/* for typing an unfinished input: the steps to indent its next line (each a block or bracket still
   open); and whether a line starting with this word belongs a step out (end,
   else, case) */
int rio_replIndent(const char *src, uint32_t len);
int rio_replOutdent(const char *word, uint32_t len);
/* The kinds of names completing offers (a language server's: keyword, variable, constant, struct,
   function, method, field, module, enum member) */
enum { rio_Item_keyword, rio_Item_var, rio_Item_const, rio_Item_type, rio_Item_proc, rio_Item_method, rio_Item_field, rio_Item_module,
       rio_Item_value };
typedef void (*rio_CompleteFn)(void *userData, const char *name, int nameLen, int kind, const char *detail, int detailLen);
/* the names that fit at the cursor (at, in text: the input so far, or an editor's whole text), each
   starting with the word before it, through fn: its name, kind (rio_Item_*) and detail (its type, a
   proc's signature, or ""). After x., Type. or module.: x's fields and methods (and its kind's built-in
   ones), an enum's values and a type's static procs, a module's exported names. Elsewhere: Rio's words
   that fit there (a statement's, an expression's, a type's, after a value: and, or, is ...), the names
   in scope, and names the text declares that aren't compiled yet (an unfinished input's, a proc's
   parameters). Names and types come from what vm's compiler has (a REPL session's); vm may be 0 (just
   the text and the words). Returns where the word being completed starts in text */
int rio_complete(rio_Vm *vm, const char *text, uint32_t at, rio_CompleteFn fn, void *userData);
/* The name that ends at at, in text (as for rio_complete: a check's text from the cut), exactly: what it
   is (rio_complete's kind and detail) and where it's declared, for an editor's hover and go to definition.
   file: the file (as the loader named it; 0: the main source, or none, as for a builtin), line (from 1;
   0: not known), or textAt: where in text, for a name the text declares (-1 otherwise). More than one
   can come, as for a name that's two things (a type, and a module's); the first is the one in scope */
typedef void (*rio_LookupFn)(void *userData, const char *name, int nameLen, int kind, const char *detail, int detailLen,
                             const char *file, int line, int textAt);
int rio_lookupAt(rio_Vm *vm, const char *text, uint32_t at, rio_LookupFn fn, void *userData);
#endif

/* ------------------------------------------------------------------ errors */

const char *rio_error(rio_Vm *vm); /* "line:col: message", or "line: runtime error: message" */
/* structured error for editors: line and col are 1-based (0 = unknown); len is the width of the
   offending token (0 = unknown); msg is the message without the position prefix; file is NULL for
   the main source */
enum { rio_ErrorKind_none, rio_ErrorKind_compile, rio_ErrorKind_runtime };
typedef struct {
  int kind, line, col, len;
  const char *msg, *file;
} rio_Error;
rio_Error rio_errorInfo(rio_Vm *vm);
/* every compile error, for a host that shows them all (rio lsp, the command line): with fn set, a compile
   goes on after an error in a top-level statement of the main source, from the next line starting at
   column 1, and fn gets each error as it's found (e's text only lasts the call). It still fails, with
   the first as rio_error. "undefined name" for something whose own declaration failed isn't one */
typedef void (*rio_ErrorFn)(void *userData, rio_Error e);
void rio_setErrorFn(rio_Vm *vm, rio_ErrorFn fn, void *userData);
int rio_pcLine(rio_Vm *vm, uint32_t pc); /* source line of a bytecode position (0 = unknown) */

/* ------------------------------------------------------------------ native code */

/* Native code for simple loops (jit.c), on ARM with Thumb-2 (the RP2350's Cortex-M33): from now on,
   whatever compiles (rio_compile) or loads (rio_loadImage) also has its simple loops and procs (math,
   compares, branches, arrays and fields, calls, log) compiled to machine code, before it runs. The code
   goes in buf, which must be executable and stay valid; or, with buf NULL, in the vm's own memory, where
   the program left it free (so mem must be executable: SRAM on the RP2350). Not for a REPL's pieces.
   Compiling needs about 8 KB of room for a while, beyond what the code takes. Returns how many loops it
   compiled right away (for a program already compiled; loops and procs), or -1 where there's no native
   code (any other machine). Errors and rio_interrupt work as in the VM. */
typedef uint32_t (*rio_JitFn)(rio_Val *slots, rio_Vm *vm);
int rio_jit(rio_Vm *vm, void *buf, uint32_t size);
void rio_jitOp(rio_Val *slots, rio_Vm *vm, const rio_Instr *ip); /* (for the native code) */

/* ------------------------------------------------------------------ private: the vm's layout */

/* Hosts allocate rio_Vm themselves, so its layout is here, but nothing in it is part of the API.
   Fields named ...At are byte offsets into mem. */

/* a proc at runtime: entry pc and code end, frame slots [frameStart, frameEnd), params in
   [frameStart, paramsEnd), result slot(s) */
typedef struct {
  uint16_t entry, end, frameStart, frameEnd, paramsEnd, result;
  uint8_t resultWords;
} rio_Proc;

/* a registered host proc; argWords and resultWords come from its sig. declared: while compiling, a
   host declaration (or a host module made from the registrations) has bound it. pad: in a loaded
   image made when the proc had fewer parameters, how many words more it takes now (passed as zeros) */
typedef struct {
  const char *name, *sig;
  rio_FfiFn fn;
  uint8_t argWords, resultWords, declared, pad;
} rio_Ffi;

typedef struct {
  const char *name, *value;
} rio_Define;

struct rio_Compiler;

struct rio_Vm {
  uint8_t *mem;
  uint32_t memSize;
  uint32_t top;            /* tables, strings and big arrays fill [top, memSize), growing down */
  uint32_t constCount;     /* constants used, in words from 0 (word 0 is always 0) */
  uint32_t constCap;       /* words reserved for constants; slots start here */
  uint32_t callStackAt;    /* the return stack */
  uint32_t freeAt;         /* memory nothing uses runs from here to top (past the slots, and native code);
                              0: unknown (a REPL's) */
  uint32_t exportsAt, exportCount; /* names the host can look up */
  uint32_t linesAt, linesSize;     /* the packed pc -> line table */
  uint32_t lineCachePos, lineCachePc, lineCacheLine; /* where the last line lookup stopped, so
                                                        lookups in pc order are one pass */
  int errorKind, errorLine, errorCol, errorLen, errorMsgAt; /* errorMsgAt: in errorText */
  const char *errorFile;
  rio_LoadFn loader;
  void *loaderData;
  uint32_t sourceWindow;   /* rio_setSourceWindow's bytes (0: the default) */
  uint32_t modulesAt, moduleCount; /* code ranges of modules, for naming files in errors */
  rio_Instr *code;         /* where the code is now: in mem, or the host's buffer */
  uint32_t codeCap, codeLen;
  uint32_t framedAt;        /* the code before this names its procs' slots from their frames (rio_codeFrames) */
  rio_Instr *hostCode;     /* the host's code buffer, or NULL: the code lives in mem */
  uint32_t hostCodeCap;
  rio_LogFn logFn;
  void *logData;
  rio_ErrorFn errorFn;     /* rio_setErrorFn's */
  void *errorData;
  int logLen, trapped, ready, procCount, ffiCount, defineCount, inRepl, running;
  int checking;            /* compiling for rio_check: not to run */
  volatile int interrupted; /* set by rio_interrupt */
  rio_Define defines[rio_MAX_DEFINES];
  const char *autoImports[rio_MAX_AUTO_IMPORTS];
  int autoImportCount;
  struct {
    const char *name, *src;
    uint32_t len, hash;
  } sources[rio_MAX_SOURCES]; /* rio_addSource's */
  int sourceCount;
  uint32_t sourcesUsed;    /* while compiling: which of them the program took (bits) */
  int hostHeader;          /* compiling for rio_hostHeader: a host proc the host lacks is only recorded */
  struct rio_Compiler *compiler; /* only while compiling, or after rio_compileScratch and in a REPL */
  rio_Usage *usage;        /* rio_trackUsage */
  rio_Proc *procs;         /* per proc, in mem next to the strings */
  rio_JitFn *jitFns;       /* native loops (rio_Op_jit a: which) */
  int (*jitHook)(rio_Vm *vm); /* set by rio_jit: compiles what was just compiled or loaded */
  void *jitBuf;            /* the host's buffer for native code, or NULL: it goes at freeAt */
  uint32_t jitSize, jitUsed, jitCount;
  rio_Ffi ffi[rio_MAX_FFI];
  char logBuf[rio_LOG_BUF_SIZE];
  char errorText[rio_ERROR_BUF_SIZE];
};

#endif
