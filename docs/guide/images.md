# Images

An image is a compiled program as bytes, ready to run: the bytecode, the constants, and the strings and
tables that sit at the top of `mem` (everything else starts as zero). A host can load one without the
compiler, and loading is just copying.

```sh
rio -M 256K -i game.img game.rio   # compile for a vm with 256 KB of mem, and save the image
rio game.img                       # run an image (the CLI tells images from source by their first bytes)
```

```c
uint32_t n = rio_saveImage(&vm, NULL, 0);    // after compiling, before running: the size
rio_saveImage(&vm, buf, n);                  // returns n, or 0 if it doesn't fit

rio_init(&vm2, mem, sizeof mem, NULL, 0);
rio_addFfi(&vm2, "clock", ">f", host_clock); // the same host procs, in any order
if (rio_loadImage(&vm2, buf, n) || rio_run(&vm2)) puts(rio_error(&vm2));
```

A host without room for a whole image beside the vm, such as one saving to a file, can write and read
it a part at a time, straight from and into the vm's memory:

```c
static int put(void *f, const void *b, uint32_t n) { return fwrite(b, 1, n, f) != n; } // 0: written
static int get(void *f, void *b, uint32_t n) { return fread(b, 1, n, f) != n; }        // 0: all n read

uint32_t n = rio_writeImage(&vm, put, file);  // after compiling, before running: the size, or 0
if (rio_readImage(&vm2, get, file, n) || rio_run(&vm2)) puts(rio_error(&vm2));
```

- **Addresses are absolute,** so compile for the memory the program will run in (`-M`, default 8 MB in
  the CLI) and load into at least that much. Code compiled into `mem` loads back there; code compiled
  into a separate buffer needs a loading vm with one too.
- **Host procs are bound by name** when loading, only the ones the code calls, each to a host proc
  with the same sig, or one with more parameters at its end and the same result: the image passes
  zeros for those, and a struct parameter's layout may have more fields at its end (the host reads
  zeros for them), so host procs can grow without breaking older images, as long as zero means what
  it meant before. One that's missing or doesn't fit is an error naming it.
- **`-Drio_NO_COMPILER`** builds a VM that only loads images, leaving out the compiler and the REPL: on
  a Cortex-M33 at `-Os`, about 11 KB of flash instead of 68 KB. As wasm
  (`zig cc -target wasm32-wasi -mexec-model=reactor -Os`), a minimal browser host is 28 KB, 13 KB
  gzipped. `tests/vm.c` is such a host; ctest runs the test suite through it.
- **Images are trusted like code:** the loader checks the header and sizes, but not the bytecode, so
  load only images you made.
- **The format isn't stable yet.** Images carry a version, and a vm refuses other versions; for now,
  rebuild images along with Rio. Little-endian hosts only.

Runtime errors from an image report lines and module file names as usual; the main file has no name in
the image, so the host names it.
