/* rio command line host.
   rio                     interactive REPL
   rio file.rio            run top-level code, then main() if present
   rio -c out.c file.rio   compile to a standalone C program instead
   rio -m file.rio         report where the program's memory goes, instead of running it
   rio -i out.img file.rio compile to an image (see rio_loadImage) instead of running it
   rio file.img            run an image
   -j                      compile simple loops to native code first (ARM with Thumb-2 only)
   -M size                 the memory the program gets (default 8M; K and M suffixes). An image
                           needs at least the memory it was compiled with to run
   -D NAME[=value]         override a top-level constant (or define it), like gcc -D
   -L dir                  look for packages (import name) in dir: each dir/name or dir/name.rio
   Ctrl-C stops a running program. */
#define _CRT_SECURE_NO_WARNINGS
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if defined(__linux__) || defined(__APPLE__)
#include <sys/mman.h>
#endif
#include "compiler.h" /* (for the memory report) */
#include "aot.h"

static uint8_t rio_mem[8 << 20];  /* everything, the code included */
static char rio_src[4 << 20];     /* the main file and every module, kept until compiling ends */
static size_t rio_srcLen;
static char rio_libs[16][512], rio_baseDir[512], rio_fileNames[1 << 16];
static int rio_libCount;
static size_t rio_fileNamesLen;
static uint8_t rio_scratch[2 << 20];
static rio_Vm rio_vm;
static volatile sig_atomic_t rio_gotInterrupt;

static void rio_printLine(void *userData, const char *s, int n) {
  (void)userData;
  fwrite(s, 1, (size_t)n, stdout);
  fputc('\n', stdout);
}
static void rio_ffiClock(rio_Vm *vm, rio_Val *a) {
  (void)vm;
  a[0].f = (float)clock() / CLOCKS_PER_SEC;
}
static void rio_ffiPutc(rio_Vm *vm, rio_Val *a) {
  (void)vm;
  fputc(a[0].i, stdout);
}
/* the same host functions, as C source for AOT output */
static const char *rio_ffiSource =
  "void rio_ffi_clock(rio_Val *a) { a[0].f = (float)clock() / CLOCKS_PER_SEC; }\n"
  "void rio_ffi_putc(rio_Val *a) { fputc(a[0].i, stdout); }\n";

/* ------------------------------------------------------------------ files and modules */

/* reads a whole file into the source arena; -1 if missing */
static long rio_readFile(const char *path) {
  FILE *f = fopen(path, "rb");
  size_t n;
  if (!f) return -1;
  n = fread(rio_src + rio_srcLen, 1, sizeof rio_src - rio_srcLen - 1, f);
  fclose(f);
  rio_srcLen += n;
  return (long)n;
}

static int rio_exists(const char *path) {
  FILE *f = fopen(path, "rb");
  if (f) fclose(f);
  return f != 0;
}

/* a + b + c + d into out; 0 if it doesn't fit (a cut-off path could name some other file) */
static int rio_joinPath(char *out, size_t cap, const char *a, const char *b, const char *c, const char *d) {
  int n = snprintf(out, cap, "%s%s%s%s", a, b, c, d);
  return n >= 0 && (size_t)n < cap;
}

/* path.rio, or the directory path/ with its entry file path/<last>.rio (isFile: exactly path) */
static int rio_resolve(const char *dir, const char *path, int isFile, char *out, size_t cap, int *isDir) {
  const char *last = strrchr(path, '/');
  last = last ? last + 1 : path;
  *isDir = 0;
  if (isFile) return rio_joinPath(out, cap, dir, path, "", "") && rio_exists(out); /* include "x.rio" */
  if (rio_joinPath(out, cap, dir, path, ".rio", "") && rio_exists(out)) return 1;
  if (rio_joinPath(out, cap, dir, path, "/", last) && strlen(out) + 4 < cap && (strcat(out, ".rio"), rio_exists(out))) {
    *isDir = 1;
    return 1;
  }
  return 0;
}

static int rio_loadFile(void *userData, const char *path, int kind, rio_Source *out) {
  char found[1024], attempt[1024], *name;
  int isDir = 0, count = 0, i, dir, isFile = kind & rio_LoadKind_file;
  long len;
  (void)userData;
  if (!(kind & rio_LoadKind_package)) count = rio_resolve(rio_baseDir, path, isFile, found, sizeof found, &isDir);
  else
    for (i = 0; i < rio_libCount; i++)
      if (rio_resolve(rio_libs[i], path, isFile, attempt, sizeof attempt, &dir)) {
        if (count++) return 2; /* in two library paths */
        memcpy(found, attempt, sizeof found);
        isDir = dir;
      }
  if (!count) return 1;
  out->src = rio_src + rio_srcLen;
  if ((len = rio_readFile(found)) < 0) return 1;
  out->len = (uint32_t)len;
  out->isDir = isDir;
  { /* keep the file name for error messages */
    size_t size = strlen(found) + 1;
    if (size > sizeof rio_fileNames - rio_fileNamesLen) return 1;
    name = rio_fileNames + rio_fileNamesLen;
    memcpy(name, found, size);
    rio_fileNamesLen += size;
  }
  out->name = name;
  return 0;
}

/* ------------------------------------------------------------------ errors and the REPL */

static void rio_onInterrupt(int sig) {
  (void)sig;
  rio_gotInterrupt = 1;
  rio_interrupt(&rio_vm);
  signal(SIGINT, rio_onInterrupt);
}

static int rio_usage(void) {
  fprintf(stderr, "usage: rio [-D NAME[=value]]... [-L dir]...                REPL\n"
                  "       rio [-D NAME[=value]]... [-L dir]... file.rio       run\n"
                  "       rio -c out.c [-D NAME[=value]]... [-L dir]... file.rio\n"
                  "       rio -m [-D NAME[=value]]... [-L dir]... file.rio    memory report\n"
                  "       rio -i out.img [-D NAME[=value]]... [-L dir]... file.rio\n"
                  "  -M size (like 256K): memory for the program\n");
  return 2;
}

/* the error, after the file it's in */
static void rio_report(const char *where) {
  rio_Error e = rio_errorInfo(&rio_vm);
  fflush(stdout);
  if (e.file) where = e.file; /* the error is in a module */
  if (where) fprintf(stderr, "%s%s", where, e.line ? ":" : ": ");
  fprintf(stderr, "%s\n", rio_error(&rio_vm));
}

/* each line is added to what's pending; unfinished input (an open block, bracket or string) waits for
   more, and a blank line gives up on it */
static int rio_repl(void) {
  static char pending[1 << 16];
  char line[1024];
  size_t n = 0, k;
  int result;
  if (rio_replBegin(&rio_vm, rio_scratch, sizeof rio_scratch, 0)) {
    rio_report(0);
    return 1;
  }
  printf("rio REPL. Ctrl-C stops a running program; Ctrl-D (Ctrl-Z Enter on Windows) quits.\n");
  for (;;) {
    fputs(n ? ". " : "> ", stdout); /* (as wide: lines line up) */
    fflush(stdout);
    rio_gotInterrupt = 0;
    if (!fgets(line, sizeof line, stdin)) {
      if (rio_gotInterrupt) {
        clearerr(stdin);
        n = 0;
        putchar('\n');
        continue;
      }
      break;
    }
    k = strlen(line);
    if (n && (line[0] == '\n' || line[0] == '\r')) {
      puts("(unfinished input dropped)");
      n = 0;
      continue;
    }
    if (n + k >= sizeof pending) {
      puts("(input too long)");
      n = 0;
      continue;
    }
    memcpy(pending + n, line, k);
    n += k;
    result = rio_replEval(&rio_vm, pending, (uint32_t)n);
    if (result == rio_ReplResult_more) continue;
    if (result) rio_report(0);
    n = 0;
  }
  putchar('\n');
  return 0;
}

/* ------------------------------------------------------------------ the memory report */

/* -m: memory by kind and by proc, and what sharing proc frames along the call graph saves. Procs are
   defined before use, so every callee has a lower index than its callers. */

static const char *rio_procLabel(int proc) {
  static char b[96];
  rio_Compiler *c = rio_vm.compiler;
  int i, j;
  for (i = 0; i < c->symCount; i++) {
    rio_Symbol *y = &c->syms[i];
    if ((y->kind != rio_SymKind_proc && y->kind != rio_SymKind_method) || y->value != proc) continue;
    b[0] = 0;
    if (y->kind == rio_SymKind_method) /* Type.name */
      for (j = 0; j < c->symCount; j++)
        if (c->syms[j].kind == rio_SymKind_type && c->syms[j].type == y->type) {
          snprintf(b, 48, "%.*s.", (int)c->syms[j].nameLen, c->names + c->syms[j].name);
          break;
        }
    snprintf(b + strlen(b), 48, "%.*s", (int)y->nameLen, c->names + y->name);
    return b;
  }
  snprintf(b, sizeof b, "proc %d", proc);
  return b;
}

static void rio_memoryReport(void) {
  static uint32_t frame[65536], chain[65536];
  static int next[65536], order[65536];
  rio_Vm *vm = &rio_vm;
  rio_Compiler *c = vm->compiler;
  int procCount = vm->procCount, i, j, pc, heaviest = -1, shown, sharedCount = 0;
  uint32_t consts = vm->constCap * 4, slots = (c->slotHigh - vm->constCap) * 4, codeSize = vm->codeLen * (uint32_t)sizeof(rio_Instr);
  uint32_t tables = vm->memSize - vm->top - c->bigBytes - codeSize; /* (the code sits just below the tables) */
  uint32_t procBig = 0, own = 0, area = 0, all = 0, depth = 0, callStack, globals, total;
  for (i = 0; i < procCount; i++) {
    rio_Proc *p = &vm->procs[i];
    rio_ProcDef *def = &c->procDefs[i];
    uint32_t heaviestCallee = 0;
    frame[i] = (uint32_t)(p->frameEnd - p->frameStart) * 4;
    all += frame[i];
    procBig += def->bigBytes;
    next[i] = -1;
    if (def->shared) sharedCount++;
    else own += frame[i];
    if (def->sharedEnd * 4u > area) area = def->sharedEnd * 4u;
    if (def->depth > depth) depth = def->depth;
    for (pc = p->entry; pc < p->end; pc++) /* its callees: compiled earlier, so already measured */
      if (vm->code[pc].op == rio_Op_call)
        for (j = 0; j < i; j++)
          if (vm->procs[j].entry == vm->code[pc].c) {
            if (chain[j] > heaviestCallee) {
              heaviestCallee = chain[j];
              next[i] = j;
            }
            break;
          }
    chain[i] = frame[i] + def->bigBytes + heaviestCallee;
    if (heaviest < 0 || chain[i] > chain[heaviest]) heaviest = i;
    order[i] = i;
  }
  callStack = (depth + 1) * 4;
  globals = slots - own - area - callStack;
  total = consts + slots + c->bigBytes + tables + codeSize;
  printf("memory: %u bytes in use (of %u)\n", total, vm->memSize);
  printf("  %-24s %8u   (%u words; rio_Limits.consts is only room while compiling)\n", "constants", consts, vm->constCount);
  printf("  %-24s %8u\n", "globals", globals + (c->bigBytes - procBig));
  printf("  %-24s %8u   (%d procs share it; without sharing their frames would take %u)\n", "shared frames", area, sharedCount, all - own);
  printf("  %-24s %8u   (%d procs with an address-taken slot keep their own frame; %u of it in big locals)\n", "own frames",
         own + procBig, procCount - sharedCount, procBig);
  printf("  %-24s %8u   (the longest chain of calls: %u)\n", "call stack", callStack, depth);
  printf("  %-24s %8u\n", "tables", tables);
  printf("    %-22s %8u   (string literals, exported names, file names)\n", "strings", vm->exportsAt - vm->top - codeSize);
#ifdef rio_EXPORTS_ALL
  printf("    %-22s %8u   (%u names the host can look up: every top-level proc and global, from rio_EXPORTS_ALL)\n", "exports",
         vm->linesAt - vm->exportsAt, vm->exportCount);
#else
  printf("    %-22s %8u   (%u names the host can look up: main and names marked name*)\n", "exports", vm->linesAt - vm->exportsAt,
         vm->exportCount);
#endif
  printf("    %-22s %8u   (pc -> source line, packed: read only for runtime errors)\n", "line table", vm->linesSize);
  printf("    %-22s %8u   (%d procs)\n", "proc table", (uint32_t)procCount * (uint32_t)sizeof(rio_Proc), procCount);
  printf("  %-24s %8u   (%u instructions, 8 bytes each)\n", "code", codeSize, vm->codeLen);
  for (i = 1; i < procCount; i++) { /* biggest first (insertion sort) */
    int k = order[i];
    for (j = i; j > 0 && frame[order[j - 1]] + c->procDefs[order[j - 1]].bigBytes < frame[k] + c->procDefs[k].bigBytes; j--)
      order[j] = order[j - 1];
    order[j] = k;
  }
  shown = procCount < 15 ? procCount : 15;
  if (procCount) printf("biggest procs (bytes: frame + big locals)\n");
  for (i = 0; i < shown; i++) {
    int k = order[i];
    printf("  %-24s %8u   %s%s\n", rio_procLabel(k), frame[k] + c->procDefs[k].bigBytes,
           c->procDefs[k].shared ? "shared" : "own memory: an address is taken", c->procDefs[k].bigBytes ? ", big locals" : "");
  }
  if (procCount > shown) printf("  ... %d more\n", procCount - shown);
  { /* unions: every value takes the size of the biggest type */
    int k, any = 0;
    for (k = 0; k < c->symCount; k++) {
      rio_Symbol *y = &c->syms[k];
      rio_Type *u;
      uint32_t smallest = 0xFFFFFFFFu;
      int f;
      if (y->kind != rio_SymKind_type || c->types[y->type].kind != rio_TypeKind_union) continue;
      u = &c->types[y->type];
      for (f = 0; f < u->fieldCount; f++) {
        rio_Field *field = &c->fields[u->firstField + f];
        if (field->type && c->types[field->type].size < smallest) smallest = c->types[field->type].size;
      }
      if (!any++) printf("unions (bytes per value: a 4-byte tag and the biggest type)\n");
      printf("  %-24.*s %8u   (smallest type %u bytes)\n", (int)y->nameLen, c->names + y->name, u->size, smallest == 0xFFFFFFFFu ? 0 : smallest);
    }
  }
  if (heaviest >= 0) {
    printf("heaviest call chain: %u bytes of frames\n  ", chain[heaviest]);
    for (i = heaviest; i >= 0; i = next[i]) printf("%s%s", rio_procLabel(i), next[i] >= 0 ? " -> " : "\n");
  }
}

/* ------------------------------------------------------------------ main */

/* -M size: 4K to all of rio_mem, with K and M suffixes; 0 if it's wrong */
static unsigned long rio_parseMemSize(const char *text) {
  char *end;
  unsigned long size = strtoul(text, &end, 0);
  if (*end == 'K' || *end == 'k') size <<= 10, end++;
  else if (*end == 'M' || *end == 'm') size <<= 20, end++;
  return *end || size < 4096 || size > sizeof rio_mem ? 0 : size;
}

/* local imports are relative to the main file's directory */
static void rio_setBaseDir(const char *path) {
  const char *slash = strrchr(path, '/'), *backslash = strrchr(path, '\\');
  const char *last = slash > backslash ? slash : backslash;
  size_t len = last ? (size_t)(last - path + 1) : 0;
  if (len >= sizeof rio_baseDir) len = 0;
  memcpy(rio_baseDir, path, len);
  rio_baseDir[len] = 0;
}

int main(int argc, char **argv) {
  FILE *f;
  size_t n;
  int proc, i, memReport = 0, jit = 0;
  const char *cOut = 0, *imageOut = 0, *path = 0;
  unsigned long memSize = sizeof rio_mem;
  for (i = 1; i < argc; i++) /* -M first: rio_init comes before -D */
    if (!strcmp(argv[i], "-M") && i + 1 < argc) {
      memSize = rio_parseMemSize(argv[++i]);
      if (!memSize) {
        fprintf(stderr, "-M: 4K to %uM\n", (unsigned)(sizeof rio_mem >> 20));
        return 2;
      }
    }
  if (rio_init(&rio_vm, rio_mem, (uint32_t)memSize, 0, 0)) {
    fprintf(stderr, "init failed\n");
    return 1;
  }
  for (i = 1; i < argc; i++) {
    char *arg = argv[i], *eq;
    if (!strcmp(arg, "-c") && i + 1 < argc) cOut = argv[++i];
    else if (!strcmp(arg, "-i") && i + 1 < argc) imageOut = argv[++i];
    else if (!strcmp(arg, "-j")) jit = 1;
    else if (!strcmp(arg, "-M") && i + 1 < argc) i++;
    else if (!strcmp(arg, "-m")) memReport = 1;
    else if (!strncmp(arg, "-L", 2)) {
      const char *dir = arg[2] ? arg + 2 : i + 1 < argc ? argv[++i] : 0;
      size_t len;
      if (!dir || rio_libCount >= 16) return rio_usage();
      len = strlen(dir);
      snprintf(rio_libs[rio_libCount++], sizeof rio_libs[0], "%s%s", dir, len && dir[len - 1] != '/' && dir[len - 1] != '\\' ? "/" : "");
    } else if (!strncmp(arg, "-D", 2)) {
      char *define = arg[2] ? arg + 2 : i + 1 < argc ? argv[++i] : 0;
      if (!define) return rio_usage();
      if ((eq = strchr(define, '='))) *eq++ = 0;
      if (rio_define(&rio_vm, define, eq)) {
        fprintf(stderr, "too many -D defines\n");
        return 2;
      }
    } else if (arg[0] == '-' || path) return rio_usage();
    else path = arg;
  }
  rio_setLog(&rio_vm, rio_printLine, 0);
  rio_addFfi(&rio_vm, "clock", ">f", rio_ffiClock);
  rio_addFfi(&rio_vm, "putc", "i", rio_ffiPutc);
  rio_setLoader(&rio_vm, rio_loadFile, 0);
  signal(SIGINT, rio_onInterrupt);
  if (!path) return cOut || memReport || imageOut ? rio_usage() : rio_repl();
  rio_setBaseDir(path);
  if (rio_readFile(path) < 0) {
    fprintf(stderr, "cannot open %s\n", path);
    return 2;
  }
  n = rio_srcLen;
  if (n >= 4 && !memcmp(rio_src, "RIOI", 4)) { /* an image: just run it */
    if (cOut || memReport || imageOut) return rio_usage();
    if (rio_loadImage(&rio_vm, rio_src, (uint32_t)n)) {
      rio_report(path);
      return 1;
    }
  } else {
    /* the C backend and the memory report read compiler tables after compiling, so they get their own
       buffer; running compiles in place, with the compiler's state overlaid on script memory */
    int failed = cOut || memReport ? rio_compileScratch(&rio_vm, rio_src, (uint32_t)n, rio_scratch, sizeof rio_scratch)
                                   : rio_compile(&rio_vm, rio_src, (uint32_t)n);
    if (failed) {
      rio_report(path);
      return 1;
    }
    if (memReport) {
      rio_memoryReport();
      return 0;
    }
    if (cOut) {
      int error;
      if (!(f = fopen(cOut, "w"))) {
        fprintf(stderr, "cannot write %s\n", cOut);
        return 2;
      }
      error = rio_aot(&rio_vm, f, rio_ffiSource);
      if (fclose(f) || error) {
        fprintf(stderr, "error writing %s\n", cOut);
        return 1;
      }
      return 0;
    }
    if (imageOut) { /* the image goes in the scratch buffer, which compiling in place left unused */
      uint32_t size = rio_saveImage(&rio_vm, rio_scratch, sizeof rio_scratch);
      if (!size) {
        fprintf(stderr, "image too large\n");
        return 1;
      }
      if (!(f = fopen(imageOut, "wb"))) {
        fprintf(stderr, "cannot write %s\n", imageOut);
        return 2;
      }
      if (fwrite(rio_scratch, 1, size, f) != size || fclose(f)) {
        fprintf(stderr, "error writing %s\n", imageOut);
        return 1;
      }
      return 0;
    }
  }
  if (jit) { /* native loops need executable memory */
    void *buf = 0;
    uint32_t size = 1 << 20;
#if defined(__linux__) || defined(__APPLE__)
    buf = mmap(0, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buf == MAP_FAILED) buf = 0;
#endif
    if (!buf || rio_jit(&rio_vm, buf, size) < 0) {
      fprintf(stderr, "rio: -j: no native code here\n");
      return 2;
    }
  }
  if (rio_run(&rio_vm) || ((proc = rio_findProc(&rio_vm, "main")) >= 0 && rio_call(&rio_vm, proc))) {
    rio_report(path);
    return 1;
  }
  return 0;
}
