# Embedding Rio in C

Rio is made to live inside another program, the host. The host gives it one block of memory, and Rio
never allocates any more:

```c
#include <stdio.h>
#include <string.h>
#include "rio.h"

static uint8_t mem[64 << 10];   // everything: the program's code and data
static rio_Vm vm;

static void print(void *ud, const char *s, int n) { fwrite(s, 1, n, stdout); putchar('\n'); }
static void beep(rio_Vm *vm, rio_Val *a) { printf("beep %d\n", a[0].i); }  // a host proc

int main(void) {
  const char *src = "pitch* := 440\nbeep(pitch)\nupdate* :: proc()\n  pitch += 1\nend\n";
  rio_init(&vm, mem, sizeof mem, NULL, 0);
  rio_setLog(&vm, print, NULL);
  rio_addFfi(&vm, "beep", "i", beep);        // beep(Int): no result
  if (rio_compile(&vm, src, strlen(src)) || rio_run(&vm)) return puts(rio_error(&vm)), 1;
  int update = rio_findProc(&vm, "update");  // marked with * in the source
  for (int i = 0; i < 3; i++) rio_call(&vm, update);
  int *pitch = rio_findGlobal(&vm, "pitch");
  printf("%d\n", *pitch);                     // 443
  return 0;
}
```

- `rio_compile` then `rio_run` compiles the program and runs its top-level code.
- Host procs, registered with `rio_addFfi`, are what a program can do beyond math and text: draw, play
  sound, read files. Their signature letters are `i` Int, `f` Float, `s` String, `b` Blob, with `>` before
  the result: `"ff>f"` takes two `Float`s and gives one.
- Names ending in `*` in the main file are what the host can find and call.

To publish a host's API in Rio, write the host procs out with `@host`, and the compiler checks them
against the real ones:

```rio fragment
beep* :: @host proc(pitch: Int)
```

The [C API guide](../guide/embedding.md) has the rest: modules of host procs, errors, the REPL,
memory limits, and saving compiled programs as images.
