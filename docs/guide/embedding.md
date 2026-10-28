# The C API

Everything is in [`src/rio.h`](https://github.com/contextfreeinfo/rio/blob/main/src/rio.h). The [tutorial](../tutorial/embedding.md) shows the
shape of it; this is the rest.

```c
#include "rio.h"
static uint8_t mem[1 << 20];          // the program's code and data, and the compiler's working memory while compiling
static rio_Vm vm;                     // runtime state (~1-12 KB depending on limits)

static void host_print(void *ud, const char *s, int n) { fwrite(s, 1, n, stdout); putchar('\n'); }
static void host_rand(rio_Vm *vm, rio_Val *a) { a[0].i = rand(); }  // args in a[], result to a[0]

rio_init(&vm, mem, sizeof mem, NULL, 0);   // or a separate code buffer: code, its capacity (max 65535)
rio_setLog(&vm, host_print, NULL);
rio_addFfi(&vm, "rand", ">i", host_rand); // sig: i f s b params, '>' result
rio_define(&vm, "LEVEL", "3");             // optional: like -D LEVEL=3
if (rio_compile(&vm, src, len) || rio_run(&vm)) puts(rio_error(&vm));

int f = rio_findProc(&vm, "update");       // procs and globals the host uses are marked: update* :: proc(dt: Float)
rio_procArgs(&vm, f)[0].f = 0.016f;        // params are consecutive words (slices 2, structs n)
rio_call(&vm, f);
float r = rio_procResult(&vm, f)->f;       // if it returns something
int *score = rio_findGlobal(&vm, "score"); // direct access to globals (score* := 0)
```

## Host procs

`rio_addFfi(vm, name, sig, fn)` registers a host proc. `sig` is the parameter types, then optionally
`>` and the result's: `i` Int, `f` Float, `s` String, `b` Blob, and a struct parameter as its layout
in braces (see [structs](#structs)). `name` and `sig` must stay valid until `rio_compile` (or
`rio_loadImage`) returns.

- Arguments arrive in `a[]` as words, in order (see [memory](../spec/host.md#memory)); the result goes
  in `a[0]`. A `String` or `Blob` takes two words; `rio_sliceBytes(vm, &a[i])` gives its bytes.
- `rio_trap(vm, "msg")` stops the program with a runtime error at the line of the call.
- Host procs must not call back into the VM.

**Modules of host procs.** A dotted name, `"pc.cls"`, puts the proc in module `pc` instead of the
global names, so programs `import pc` and call `pc.cls(0)`. If the loader has a source for `pc`, that
source is the module, checked against the registrations (see [declaring host
procs](../spec/host.md#declaring-host-procs)). Otherwise the module is made from the registrations,
with nothing to parse. `rio_autoImport(vm, "pc")` makes `pc` visible everywhere without an import.
When the loader has a source for an auto-imported module, it's compiled before the program (in a REPL,
before the first input), so the module can hold types and constants for its procs (an enum for a
mode, say); without a source, the module is made from the registrations.

## Structs

A struct parameter arrives as two words, where it is and its size, and `rio_getStruct` copies it into
a C struct of the same layout. `rio -H pc.h pc.rio` writes those C structs from the Rio module that
declares the host procs, so its source is what both sides follow: for each struct a host proc takes, a
`typedef`, accessors for its bitfields (`pc_Text_big(&t)`, `pc_Text_set_big(&t, 1)`), a check that its
size is Rio's, and its layout, `pc_Text_LAYOUT`; and for each host proc, its sig, `pc_text_SIG`:

```c
#include "pc.h"   // made by rio -H pc.h pc.rio

static void text(rio_Vm *vm, rio_Val *a) {
  pc_Text t;
  rio_getStruct(vm, a + 2, &t, sizeof t);   // (a[0] and a[1]: the String)
  draw(rio_sliceBytes(vm, a), a[1].i, t.at.x, t.at.y, pc_Text_color(&t), pc_Text_big(&t));
}
...
rio_addFfi(&vm, "pc.text", pc_text_SIG, text);
```

A `&T` parameter (`&i`, `&f`, or a struct's `&{...}`) comes the same way, the caller's place and its
size, and `rio_putStruct(vm, a + i, &t, sizeof t)` writes it back, no more than the place holds. A
struct, union or maybe result (`>{...}`, `>Union{...}`) does too: where it goes is `a[0]` and `a[1]`,
before the parameters, which start at `a[2]`, and the host writes it there with `rio_putStruct`. For a
union or maybe, the header has a C struct for it, `pc_name_Result` (a maybe's `has` and `value`).

A `String` or `Blob` inside is a `rio_View`, whose bytes `rio_viewBytes(vm, v)` gives. Compiling checks
each declaration against the sig, so a header older than the module is an error, not a misread. An
image made when a struct had fewer fields at its end still loads, and `rio_getStruct` fills the rest
with zeros; a struct changed any other way is an error naming the host proc.

`rio_hostHeader` does what `rio -H` does, for a host that makes its own: the module may declare host
procs the vm doesn't have.

## Modules

`rio_setLoader(vm, fn, ud)` supplies module sources: it gets a path like `"ui/button"` or an include
like `"parts/hud.rio"`, flagged as local or package, and returns the source text. The core never
touches a filesystem: a console can read an SD card, and tests can use a table of strings. Instead of
the text, a loader can leave `src` `NULL` and set `read` and `readData` in its `rio_Source`, and then
the compiler reads it a part at a time, as `rio_compileStream` does (see
[memory](memory.md#the-compiler-borrows-the-programs-memory)). `readData` has to stay valid until
compiling ends.

## Errors

`rio_error(vm)` is a ready-to-print string: `"12:7: type mismatch"` for compile errors and
`"40: runtime error: index out of bounds"` for runtime errors. For an editor, use the structured form:

```c
rio_Error e = rio_errorInfo(&vm); // after a failed rio_compile / rio_run / rio_call
// e.kind: rio_ErrorKind_compile or rio_ErrorKind_runtime
// e.line, e.col: 1-based (0 = unknown); e.len: width of the offending token, for underlining
// e.msg: the message without the position, e.g. "undefined name near 'nope'"
// e.file: the module's file, or NULL for the main source
```

- Compile errors point at the token at fault, or, for an error found at the end of an expression,
  its last token.
- Runtime errors report the line, from a compact table built while compiling (about a byte per line
  that has code). `rio_pcLine(vm, pc)` exposes the same lookup.
- Code from `rio -c` reports the same lines, through `#line` directives.

## The REPL

```c
rio_replBegin(&vm, scratch, sizeof scratch, NULL); // limits scaled to the scratch buffer
switch (rio_replEval(&vm, text, len)) {            // compiles the text, then runs just that
case rio_ReplResult_ok: break;                     // ran
case rio_ReplResult_more: /* show a continuation prompt; call again with more text appended */ break;
default: show(rio_error(&vm));                     // rio_ReplResult_error: compile errors undo themselves
}
```

The compiler state stays in `scratch` for the whole session, so this is the one mode where its tables
cost RAM for good: 8 to 16 KB of scratch gives a comfortable REPL. Each input's strings move into `mem`
once it compiles. Hosts that echo typing themselves can indent continuation lines: `rio_replIndent` says
how far (a step per open block or bracket), and `rio_replOutdent` which first words (`end`, `else`,
`case`) go a step back out.

## Stopping a program

`rio_interrupt(vm)` stops a running program at its next loop iteration, with the runtime error
`interrupted`. It only sets a flag, so it's safe from a signal handler, an interrupt or another core.
The check sits on backward jumps and costs nothing measurable. Code from `rio -c` can't be stopped
this way.

## What the host can find

`rio_findProc` and `rio_findGlobal` find `main` and the main file's names marked with `*`. Each costs a
12-byte table entry plus the name. Build with `-Drio_EXPORTS_ALL` (CMake: `-DRIO_EXPORTS_ALL=ON`) to
make every top-level proc and global findable, say for a debugger.
