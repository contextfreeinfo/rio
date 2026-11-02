/* rio command line host.
   rio                     interactive REPL
   rio file.rio [args...]  run top-level code, then main() if present (args: the program's, os.arg)
   rio -c out.c file.rio   compile to a standalone C program instead
   rio -m file.rio         report where the program's memory goes, instead of running it
   rio -i out.img file.rio compile to an image (see rio_loadImage) instead of running it
   rio -H out.h file.rio   write C for a host implementing the host procs file.rio declares (rio_hostHeader)
   rio file.img            run an image
   rio fmt [-w | -l] file.rio...  lay the files out the one way (rio_format): printed, or -w rewrites
                           the ones it changes, or -l lists them (exiting 1 if any)
   -j                      compile simple loops to native code first (ARM with Thumb-2 only)
   -M size                 the memory the program gets (default 8M, up to 1G; K, M and G suffixes),
                           allocated once at the start. An image needs at least the memory it was
                           compiled with to run
   -D NAME[=value]         override a top-level constant (or define it), like gcc -D
   -L dir                  look for packages (import name) in dir: each dir/name or dir/name.rio
   -help, -license         this, and rio's license (a release is the one program, so it carries them)
   Ctrl-C stops a running program. Programs here have module os (import os: src/os, apart from Rio itself): files,
   folders, running programs, the environment and arguments, text in and out, time. */
#define _CRT_SECURE_NO_WARNINGS
#define _DEFAULT_SOURCE /* (POSIX's clock_gettime, setenv and the like, under -std=c99: os.h's) */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if defined(__linux__) || defined(__APPLE__)
#include <sys/mman.h>
#endif
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
#include "compiler.h" /* (for the memory report) */
#include "aot.h"

#define rio_MEM_DEFAULT (8ul << 20) /* -M: the program's memory, unless it's given */
#define rio_MEM_MAX (1ul << 30)
static uint8_t *rio_mem;           /* everything, the code included: allocated once, at the start (rio
                                      itself never allocates, compiling or running) */
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

/* ------------------------------------------------------------------ module os */

#define rio_OS_MEM rio_vm.mem
#define rio_OS_MEM_SIZE rio_vm.memSize
#include "os/os.h"      /* (its procs, in C: rio_ffi_os_<name>) */
#include "os/os_host.h" /* (os.rio: its source and sigs: rio -H src/os/os_host.h src/os/os.rio) */
#include "os/os_text.h" /* (os.h as text, for C output: rio tools/ctext.rio src/os/os.h src/os/os_text.h rio_osCode) */

#define rio_OS_VM(name, sig) static void rio_osVm_##name(rio_Vm *vm, rio_Val *a) { (void)vm; rio_ffi_os_##name(a); }
os_HOST_PROCS(rio_OS_VM)
#undef rio_OS_VM

/* module os, for import os: its source and its procs */
static int rio_addOs(void) {
#define rio_OS_ADD(name, sig) || rio_addFfi(&rio_vm, "os." #name, sig, rio_osVm_##name)
  return rio_addSource(&rio_vm, "os", os_SOURCE, sizeof os_SOURCE - 1) os_HOST_PROCS(rio_OS_ADD);
#undef rio_OS_ADD
}

/* the host procs as C, for C output: os's, when the program imports os (else none) */
static const char *rio_hostFfi(void) {
  static char text[sizeof rio_osCode + 256];
  if (!(rio_vm.sourcesUsed & 1)) return 0; /* (os: the first source added) */
  snprintf(text, sizeof text, "#define rio_OS_MEM rio_M\n#define rio_OS_MEM_SIZE rio_MEM_SIZE\n"
           "#define rio_HOST_START(argc, argv) rio_osStart((argc) - 1, (argv) + 1)\n%s", rio_osCode);
  return text;
}

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

/* -window: sources read a part at a time, through a window of this many bytes (as a host reading
   files would), to try that way of compiling; the texts are in memory all the same */
static int rio_window;
typedef struct { const char *src; uint32_t len; } rio_MemText;
static rio_MemText rio_texts[64];
static int rio_textCount;
static int rio_readText(void *userData, char *buf, uint32_t at, uint32_t n) {
  const rio_MemText *t = (const rio_MemText *)userData;
  if (at >= t->len) return 0;
  if (n > t->len - at) n = t->len - at;
  memcpy(buf, t->src + at, n);
  return (int)n;
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
  if (rio_window && rio_textCount < 64) { /* (read a part at a time) */
    rio_texts[rio_textCount].src = out->src;
    rio_texts[rio_textCount].len = out->len;
    out->read = rio_readText;
    out->readData = &rio_texts[rio_textCount++];
    out->src = 0;
  }
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
                  "       rio fmt [-w | -l] file.rio...\n"
                  "  -M size (like 256K): memory for the program\n"
                  "       rio -help | -license\n");
  return 2;
}
static const char rio_help[] =
  "rio: the Rio language (https://github.com/contextfreeinfo/rio)\n"
  "\n"
  "  rio                     an interactive REPL\n"
  "  rio file.rio            run top-level code, then main() if present\n"
  "  rio -c out.c file.rio   compile to a standalone C program instead\n"
  "  rio -m file.rio         report where the program's memory goes, instead of running it\n"
  "  rio -i out.img file.rio compile to an image instead of running it\n"
  "  rio file.img            run an image\n"
  "  rio fmt file.rio...     the files laid out the one way (only spaces change), printed;\n"
  "                          -w rewrites the ones that change, -l lists them (exiting 1 if any)\n"
  "  -j                      compile simple loops to native code first (ARM with Thumb-2 only)\n"
  "  -M size                 the memory the program gets (default 8M, up to 1G; K, M and G\n"
  "                          suffixes). An image needs at least the memory it was compiled with\n"
  "  -D NAME[=value]         override a top-level constant (or define it), like gcc -D\n"
  "  -L dir                  look for packages (import name) in dir: each dir/name or dir/name.rio\n"
  "  -window n               compile reading the sources n bytes at a time, as a host reading files\n"
  "                          may (to try that way: they're in memory all the same)\n"
  "  -help                   this\n"
  "  -license                rio's license\n"
  "\n"
  "Ctrl-C stops a running program.\n";
static const char rio_license[] =
  "rio\n"
  "\n"
  "To the extent possible under law, Thomas J. Palmer has waived all copyright and related or\n"
  "neighboring rights to rio. This work is published from: United States.\n"
  "\n"
  "That's CC0 1.0 Universal, a public domain dedication:\n"
  "https://creativecommons.org/publicdomain/zero/1.0/legalcode\n";

/* the error, after the file it's in */
static void rio_report(const char *where) {
  rio_Error e = rio_errorInfo(&rio_vm);
  fflush(stdout);
  if (e.file) where = e.file; /* the error is in a module */
  if (where) fprintf(stderr, "%s%s", where, e.line ? ":" : ": ");
  fprintf(stderr, "%s\n", rio_error(&rio_vm));
}

/* the REPL's colors (os.h's line editor: rio_osColors): rio_highlight's kind for each byte of a line */
static unsigned char *rio_colorKinds;
static void rio_colorToken(void *userData, uint32_t at, uint32_t len, int kind) {
  (void)userData;
  memset(rio_colorKinds + at, kind, len);
}
static void rio_colorLine(const char *s, int n, unsigned char *kinds) {
  memset(kinds, rio_Hl_code, (size_t)n);
  rio_colorKinds = kinds;
  rio_highlight(s, (uint32_t)n, rio_colorToken, 0);
}

/* the REPL's Tab (os.h's line editor: rio_osComplete): rio_complete's names, each once, for what's typed
   so far (the unfinished input's lines before this one, and this one to the cursor) */
static char rio_pending[1 << 16];
static size_t rio_pendingLen;
typedef struct {
  char *out;
  int cap, used, count;
} rio_Offers;
static void rio_offer(void *userData, const char *name, int nameLen, int kind, const char *detail, int detailLen) {
  rio_Offers *o = (rio_Offers *)userData;
  int i, at = 0;
  (void)kind;
  for (i = 0; i < o->count; i++) { /* (a name once: the first, the newest) */
    int n = (int)strlen(o->out + at);
    if (n == nameLen && !memcmp(o->out + at, name, (size_t)n)) return;
    at += n + 1;
    at += (int)strlen(o->out + at) + 1;
  }
  if (o->used + nameLen + detailLen + 2 > o->cap) return;
  memcpy(o->out + o->used, name, (size_t)nameLen);
  o->used += nameLen;
  o->out[o->used++] = 0;
  memcpy(o->out + o->used, detail, (size_t)detailLen);
  o->used += detailLen;
  o->out[o->used++] = 0;
  o->count++;
}
static int rio_completeLine(const char *s, int n, int at, char *out, int cap, int *from) {
  static char text[sizeof rio_pending + rio_LINE];
  rio_Offers o;
  (void)n;
  o.out = out;
  o.cap = cap;
  o.used = o.count = 0;
  memcpy(text, rio_pending, rio_pendingLen);
  memcpy(text + rio_pendingLen, s, (size_t)at);
  *from = rio_complete(&rio_vm, text, (uint32_t)(rio_pendingLen + (size_t)at), rio_offer, &o) - (int)rio_pendingLen;
  return o.count;
}

/* the REPL's indents (os.h's rio_osIndent): a step (two spaces) for each block and bracket the input so
   far leaves open, a step less for a line starting end, else or case */
static int rio_indentLine(const char *s, int n) {
  int depth = rio_replIndent(rio_pending, (uint32_t)rio_pendingLen), i = 0, w;
  while (i < n && (s[i] == ' ' || s[i] == '\t')) i++;
  for (w = i; w < n && ((s[w] >= 'a' && s[w] <= 'z') || (s[w] >= 'A' && s[w] <= 'Z')); w++) {}
  if (depth && w > i && (w == n || s[w] == ' ') && rio_replOutdent(s + i, (uint32_t)(w - i))) depth--;
  return depth * 2;
}

/* the REPL's history file: ~/.rio_history (HOME, or USERPROFILE on Windows); 0 if there's no home */
static const char *rio_historyPath(void) {
  static char path[1024];
  const char *home = getenv("HOME");
#ifdef _WIN32
  if (!home) home = getenv("USERPROFILE");
#endif
  if (!home || snprintf(path, sizeof path, "%s/.rio_history", home) >= (int)sizeof path) return 0;
  return path;
}

/* each line is added to what's pending; unfinished input (an open block, bracket or string) waits for
   more, and a blank line gives up on it (not one in a paste). Lines are typed with the line editor (os.h's: arrows, Home,
   End, Ctrl+Left and Right by words, Up and Down for the lines typed before, kept in ~/.rio_history,
   Tab to complete a name; a line starts at its indent, and end, else and case step out) */
static int rio_repl(void) {
  char line[rio_LINE + 1];
  size_t k;
  int result, got, blank;
  const char *history = rio_historyPath();
  FILE *h;
  if (rio_replBegin(&rio_vm, rio_scratch, sizeof rio_scratch, 0)) {
    rio_report(0);
    return 1;
  }
  if (history && (h = fopen(history, "rb"))) { /* (the lines typed before, the last 256) */
    int lines = 0, i;
    while (fgets(line, sizeof line, h)) {
      k = strcspn(line, "\r\n");
      rio_osHistoryAdd(line, (int)k);
      lines++;
    }
    fclose(h);
    if (lines > 1000 && (h = fopen(history, "wb"))) { /* (grown long: just those kept) */
      for (i = 0; i < rio_osHistCount; i++) fprintf(h, "%s\n", rio_osHist[i]);
      fclose(h);
    }
  }
  if (!getenv("NO_COLOR") && !(getenv("TERM") && !strcmp(getenv("TERM"), "dumb"))) rio_osColors = rio_colorLine; /* (no-color.org) */
  rio_osComplete = rio_completeLine;
  rio_osIndent = rio_indentLine;
  printf("rio REPL. Ctrl-C stops a running program, or drops what's typed; Ctrl-D quits.\n");
  for (;;) {
    rio_gotInterrupt = 0;
    got = rio_osLine(rio_pendingLen ? ". " : "> ", 2, line, (int)sizeof line - 1); /* (as wide: lines line up) */
    if (got == -2 || (got < 0 && rio_gotInterrupt)) { /* Ctrl+C: what's typed dropped */
      clearerr(stdin);
      rio_pendingLen = 0;
      continue;
    }
    if (got < 0) break;
    blank = (int)strspn(line, " \t") == got; /* (spaces only: an indent it was given, and no more) */
    if (!blank && history && (h = fopen(history, "ab"))) {
      fprintf(h, "%s\n", line);
      fclose(h);
    }
    line[got] = '\n'; /* (the line, with its line break, as the REPL takes it) */
    line[got + 1] = 0;
    k = (size_t)got + 1;
    if (rio_pendingLen && blank && !rio_osPasting && !(rio_osIsTerminal() && rio_osWaiting())) { /* (a paste's blank lines are kept) */
      puts("(unfinished input dropped)");
      rio_pendingLen = 0;
      continue;
    }
    if (rio_pendingLen + k >= sizeof rio_pending) {
      puts("(input too long)");
      rio_pendingLen = 0;
      continue;
    }
    memcpy(rio_pending + rio_pendingLen, line, k);
    rio_pendingLen += k;
    result = rio_replEval(&rio_vm, rio_pending, (uint32_t)rio_pendingLen);
    if (result == rio_ReplResult_more) continue;
    if (result) rio_report(0);
    rio_pendingLen = 0;
  }
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
  uint32_t consts = vm->constCount * 4, data = (vm->constCap - vm->constCount) * 4, slots = (c->slotHigh - vm->constCap) * 4, codeSize = vm->codeLen * (uint32_t)sizeof(rio_Instr);
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
  total = consts + data + slots + c->bigBytes + tables + codeSize;
  printf("memory: %u bytes in use (of %u)\n", total, vm->memSize);
  printf("  %-24s %8u   (%u words; rio_Limits.consts is only room while compiling)\n", "constants", consts, vm->constCount);
  printf("  %-24s %8u   (%u of them starting as literals, kept with the constants: nothing runs to set them)\n", "globals",
         globals + data + (c->bigBytes - procBig), data);
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

/* -M size: 4K to 1G, with K, M and G suffixes; 0 if it's wrong */
static unsigned long rio_parseMemSize(const char *text) {
  char *end;
  unsigned long size = strtoul(text, &end, 0);
  if (size > rio_MEM_MAX) return 0;
  if (*end == 'K' || *end == 'k') size <<= 10, end++;
  else if (*end == 'M' || *end == 'm') size <<= 20, end++;
  else if (*end == 'G' || *end == 'g') size <<= 30, end++;
  return *end || size < 4096 || size > rio_MEM_MAX ? 0 : size;
}

static void rio_freeMem(void) { free(rio_mem); }

/* local imports are relative to the main file's directory */
static void rio_setBaseDir(const char *path) {
  const char *slash = strrchr(path, '/'), *backslash = strrchr(path, '\\');
  const char *last = slash > backslash ? slash : backslash;
  size_t len = last ? (size_t)(last - path + 1) : 0;
  if (len >= sizeof rio_baseDir) len = 0;
  memcpy(rio_baseDir, path, len);
  rio_baseDir[len] = 0;
}

/* rio fmt: each file through rio_format */
static int rio_fmtMain(int argc, char **argv) {
  int i, write = 0, list = 0, status = 0;
  for (i = 0; i < argc && argv[i][0] == '-'; i++) {
    if (!strcmp(argv[i], "-w")) write = 1;
    else if (!strcmp(argv[i], "-l")) list = 1;
    else return rio_usage();
  }
  if (i >= argc || (write && list)) return rio_usage();
#ifdef _WIN32
  _setmode(_fileno(stdout), _O_BINARY); /* (the files' own line ends, \n or \r\n) */
#endif
  for (; i < argc; i++) {
    static char out[3 * sizeof rio_src];
    FILE *f = fopen(argv[i], "rb");
    size_t n;
    int32_t len;
    if (!f) {
      fprintf(stderr, "cannot open %s\n", argv[i]);
      status = 2;
      continue;
    }
    n = fread(rio_src, 1, sizeof rio_src, f);
    fclose(f);
    if (n == sizeof rio_src) {
      fprintf(stderr, "%s: too big\n", argv[i]);
      status = 2;
      continue;
    }
    if ((len = rio_format(rio_src, (uint32_t)n, out, sizeof out)) < 0) {
      fprintf(stderr, len == -1 ? "%s: too big\n" : "%s: rio fmt would change its tokens (a bug in rio fmt): left as it is\n", argv[i]);
      status = 2;
      continue;
    }
    if (!write && !list) fwrite(out, 1, (size_t)len, stdout);
    else if ((size_t)len != n || memcmp(out, rio_src, n)) {
      if (list) {
        printf("%s\n", argv[i]);
        status = status ? status : 1;
      } else if (!(f = fopen(argv[i], "wb")) || fwrite(out, 1, (size_t)len, f) != (size_t)len || fclose(f)) {
        fprintf(stderr, "cannot write %s\n", argv[i]);
        status = 2;
      }
    }
  }
  return status;
}

/* where the program's file is in argv (argc: none): rio's own flags come before it, its arguments after */
/* ------------------------------------------------------------------ rio lsp: a language server */

/* rio lsp: the Language Server Protocol over standard input and output (an editor runs it: VS Code's
   extension, editors/vscode). Documents are kept whole (full sync); each change is checked
   (rio_check: its first error, a diagnostic); completion checks the document up to the cursor's line,
   then asks rio_complete; hover and go to definition, rio_lookupAt (with the comment lines above a
   declaration, for hover); formatting is rio_format's. Imports are found from the document's folder, as
   rio finds them, and import os works. Not yet: renames, references */
#define rio_LSP_DOCS 16
#define rio_LSP_TEXT (512 << 10) /* a document's most */
static struct {
  char uri[1024];
  char text[rio_LSP_TEXT];
  uint32_t len;
} rio_lspDocs[rio_LSP_DOCS];
static char rio_lspIn[2 * rio_LSP_TEXT + 65536], rio_lspOut[1 << 20];
static uint8_t rio_lspMem[64 << 10]; /* (a check's vm: just the constants) */
static uint32_t rio_lspOutLen;

/* the value after "key": in a JSON message, from s (the first such key: messages are small and their
   keys distinct enough), or 0 */
static const char *rio_jsonAt(const char *s, const char *key) {
  char k[64];
  const char *p;
  snprintf(k, sizeof k, "\"%s\"", key);
  if (!s || !(p = strstr(s, k))) return 0;
  p += strlen(k);
  while (*p == ' ' || *p == ':') p++;
  return p;
}
/* a JSON string at v, unescaped (\uXXXX as UTF-8), into out: its length, or -1 */
static int rio_jsonString(const char *v, char *out, int cap) {
  int n = 0;
  if (!v || *v != '"') return -1;
  for (v++; *v && *v != '"' && n < cap - 4; v++) {
    int ch = (unsigned char)*v;
    if (ch == '\\') {
      ch = *++v;
      if (ch == 'n') ch = '\n';
      else if (ch == 't') ch = '\t';
      else if (ch == 'r') ch = '\r';
      else if (ch == 'b') ch = '\b';
      else if (ch == 'f') ch = '\f';
      else if (ch == 'u') {
        unsigned u = 0;
        int k;
        for (k = 1; k <= 4 && v[k]; k++) u = u * 16 + (unsigned)(v[k] <= '9' ? v[k] - '0' : (v[k] | 32) - 'a' + 10);
        v += 4;
        if (u < 0x80) out[n++] = (char)u;
        else if (u < 0x800) out[n++] = (char)(0xC0 | u >> 6), out[n++] = (char)(0x80 | (u & 63));
        else out[n++] = (char)(0xE0 | u >> 12), out[n++] = (char)(0x80 | (u >> 6 & 63)), out[n++] = (char)(0x80 | (u & 63));
        continue;
      }
    }
    out[n++] = (char)ch;
  }
  out[n] = 0;
  return n;
}
static void rio_lspPut(const char *s, int n) {
  if (n < 0) n = (int)strlen(s);
  if (rio_lspOutLen + (uint32_t)n < sizeof rio_lspOut) memcpy(rio_lspOut + rio_lspOutLen, s, (size_t)n), rio_lspOutLen += (uint32_t)n;
}
static void rio_lspPutString(const char *s, int n) { /* (as a JSON string) */
  int i;
  rio_lspPut("\"", 1);
  for (i = 0; i < n; i++) {
    char e[8];
    unsigned char ch = (unsigned char)s[i];
    if (ch == '"' || ch == '\\') e[0] = '\\', e[1] = (char)ch, rio_lspPut(e, 2);
    else if (ch == '\n') rio_lspPut("\\n", 2);
    else if (ch < 32) snprintf(e, sizeof e, "\\u%04x", ch), rio_lspPut(e, 6);
    else rio_lspPut(s + i, 1);
  }
  rio_lspPut("\"", 1);
}
static void rio_lspSend(void) { /* (the message built in rio_lspOut) */
  printf("Content-Length: %u\r\n\r\n", (unsigned)rio_lspOutLen);
  fwrite(rio_lspOut, 1, rio_lspOutLen, stdout);
  fflush(stdout);
  rio_lspOutLen = 0;
}
static void rio_lspReply(const char *id, const char *result) {
  rio_lspPut("{\"jsonrpc\":\"2.0\",\"id\":", -1);
  rio_lspPut(id, (int)strcspn(id, ",}"));
  rio_lspPut(",\"result\":", -1);
  rio_lspPut(result, -1);
  rio_lspPut("}", 1);
  rio_lspSend();
}

/* a file: URI's path (%XX decoded; file:///c:/x as c:/x) */
static void rio_lspPath(const char *uri, char *out, size_t cap) {
  size_t n = 0;
  if (!strncmp(uri, "file://", 7)) uri += 7;
  if (uri[0] == '/' && uri[1] && (uri[2] == ':' || (uri[2] == '%' && uri[3] == '3'))) uri++; /* (/c:/: Windows's) */
  for (; *uri && n + 1 < cap; uri++) {
    if (*uri == '%' && uri[1] && uri[2]) {
      char h[3] = {uri[1], uri[2], 0};
      out[n++] = (char)strtol(h, 0, 16);
      uri += 2;
    } else out[n++] = *uri;
  }
  out[n] = 0;
}

/* A project's auto-imports, from the rio.json in a document's folder or the nearest one above it:
   {"autoImports": {"pc": "src/core/pc.rio"}} (paths from rio.json's folder), as a host (rinc) imports
   them for every file. Their host procs, which nothing here runs, are registered with what their
   declarations say (rio_hostHeader's sigs) and do nothing, so calls to them check as the host's would */
#define rio_LSP_IMPORTS 4
static char rio_lspConfig[1024]; /* (the rio.json loaded, "" none) */
static struct {
  char name[64], path[1600];
  const char *src;
  uint32_t len;
} rio_lspImports[rio_LSP_IMPORTS];
static int rio_lspImportCount, rio_lspHostCount;
static char rio_lspImportText[1 << 20], rio_lspHostNames[rio_MAX_FFI][96], rio_lspHostSigs[rio_MAX_FFI][512];
static void rio_lspNoHost(rio_Vm *vm, rio_Val *a) {
  (void)vm;
  (void)a;
}
static void rio_lspLoadConfig(const char *dir) {
  static char text[1 << 16], header[1 << 20], found[1024];
  static uint8_t headerMem[256 << 10]; /* (rio_hostHeader compiles: its constants need room) */
  char here[1024], *slash;
  const char *p;
  FILE *f = 0;
  size_t used = 0;
  snprintf(here, sizeof here, "%s", dir);
  for (;;) { /* (rio.json: here, or the nearest folder above) */
    snprintf(found, sizeof found, "%s/rio.json", here);
    if ((f = fopen(found, "rb")) != 0) break;
    if ((slash = strrchr(here, '/')) == 0 && (slash = strrchr(here, '\\')) == 0) break;
    *slash = 0;
  }
  if (!f) found[0] = 0;
  if (!strcmp(found, rio_lspConfig)) return; /* (the one loaded already) */
  snprintf(rio_lspConfig, sizeof rio_lspConfig, "%s", found);
  rio_lspImportCount = rio_lspHostCount = 0;
  if (!f) return;
  text[fread(text, 1, sizeof text - 1, f)] = 0;
  fclose(f);
  *strrchr(found, '/') = 0; /* (its folder: where its paths start) */
  for (p = rio_jsonAt(text, "autoImports"); p && *p && *p != '}' && rio_lspImportCount < rio_LSP_IMPORTS;) {
    char name[64], file[512], path[1600];
    FILE *m;
    uint32_t n, k;
    while (*p && *p != '"' && *p != '}') p++;
    if (*p != '"' || rio_jsonString(p, name, sizeof name) < 0) break;
    p = strchr(p + 1, '"') + 1; /* (past the name, to its file) */
    while (*p && *p != '"') p++;
    if (rio_jsonString(p, file, sizeof file) < 0) break;
    p = strchr(p + 1, '"') + 1;
    snprintf(path, sizeof path, "%s/%s", found, file);
    if (!(m = fopen(path, "rb"))) continue;
    n = (uint32_t)fread(rio_lspImportText + used, 1, sizeof rio_lspImportText - used - 1, m);
    fclose(m);
    snprintf(rio_lspImports[rio_lspImportCount].name, sizeof rio_lspImports[0].name, "%s", name);
    snprintf(rio_lspImports[rio_lspImportCount].path, sizeof rio_lspImports[0].path, "%s", path);
    rio_lspImports[rio_lspImportCount].src = rio_lspImportText + used;
    rio_lspImports[rio_lspImportCount++].len = n;
    rio_init(&rio_vm, headerMem, sizeof headerMem, 0, 0); /* (its host procs' sigs: name_SIG "...", as rio -H writes them) */
    if (rio_hostHeader(&rio_vm, rio_lspImportText + used, n, rio_scratch, sizeof rio_scratch, name, header, sizeof header)) {
      const char *line = header;
      size_t prefix = strlen(name);
      while ((line = strstr(line, "#define ")) != 0 && rio_lspHostCount < rio_MAX_FFI) {
        const char *q = line + 8, *sig = strstr(q, "_SIG \""), *end = strchr(q, '\n');
        line = q;
        if (!sig || (end && sig > end) || strncmp(q, name, prefix) || q[prefix] != '_') continue;
        k = (uint32_t)(sig - q - prefix - 1); /* (pc_cls_SIG: pc.cls; pc_Color_rgb_SIG: pc.Color_rgb, the static one's) */
        snprintf(rio_lspHostNames[rio_lspHostCount], sizeof rio_lspHostNames[0], "%s.%.*s", name, (int)k, q + prefix + 1);
        { /* (its parts: "strings" and NAME_LAYOUT macros, each #defined as a string in the same header) */
          char *out = rio_lspHostSigs[rio_lspHostCount];
          const char *s = sig + 5;
          int len = 0, cap = (int)sizeof rio_lspHostSigs[0];
          out[0] = 0;
          const char *stop = end ? end : s + strlen(s);
          while (s < stop && *s != '\r') {
            if (*s == '"') {
              len += rio_jsonString(s, out + len, cap - len);
              s = strchr(s + 1, '"') + 1;
            } else if (rio_isAlpha(*s)) {
              char macro[128];
              const char *m = s, *def;
              while (rio_isAlpha(*s) || (*s >= '0' && *s <= '9')) s++;
              snprintf(macro, sizeof macro, "#define %.*s \"", (int)(s - m), m);
              if ((def = strstr(header, macro)) != 0) len += rio_jsonString(def + strlen(macro) - 1, out + len, cap - len);
            } else s++;
          }
        }
        rio_lspHostCount++;
      }
    }
    used += n + 1;
  }
}

/* a check of document d's first len bytes, its imports from its folder: 0, or -1 (rio_errorInfo says) */
static int rio_lspCheck(int d, uint32_t len) {
  char path[1024], *slash;
  int i;
  rio_lspPath(rio_lspDocs[d].uri, path, sizeof path);
  rio_setBaseDir(path); /* (its folder, with its slash: where its includes and imports are) */
  if ((slash = strrchr(path, '/')) != 0 || (slash = strrchr(path, '\\')) != 0) *slash = 0;
  else strcpy(path, ".");
  rio_lspLoadConfig(path);
  rio_srcLen = rio_fileNamesLen = 0;
  rio_textCount = rio_window = 0;
  rio_init(&rio_vm, rio_lspMem, sizeof rio_lspMem, 0, 0);
  if (rio_addOs()) return -1;
  for (i = 0; i < rio_lspHostCount; i++)
    if (rio_addFfi(&rio_vm, rio_lspHostNames[i], rio_lspHostSigs[i], rio_lspNoHost)) break;
  for (i = 0; i < rio_lspImportCount; i++)
    if (rio_addSource(&rio_vm, rio_lspImports[i].name, rio_lspImports[i].src, rio_lspImports[i].len) || rio_autoImport(&rio_vm, rio_lspImports[i].name))
      break;
  rio_setLoader(&rio_vm, rio_loadFile, 0);
  return rio_check(&rio_vm, rio_lspDocs[d].text, len, 0, 0, rio_scratch, sizeof rio_scratch);
}

static void rio_lspDiagnose(int d) {
  char head[64];
  rio_lspPut("{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\",\"params\":{\"uri\":", -1);
  rio_lspPutString(rio_lspDocs[d].uri, (int)strlen(rio_lspDocs[d].uri));
  rio_lspPut(",\"diagnostics\":[", -1);
  if (rio_lspCheck(d, rio_lspDocs[d].len)) {
    rio_Error e = rio_errorInfo(&rio_vm);
    int line = e.line > 0 ? e.line - 1 : 0, col = e.col > 0 ? e.col - 1 : 0, w = e.len > 0 ? e.len : 1;
    char msg[rio_ERROR_BUF_SIZE + 1100];
    if (e.file) { /* (in a file it imports: shown at the top of this one) */
      snprintf(msg, sizeof msg, "%s:%d:%d: %s", e.file, e.line, e.col, e.msg);
      line = col = 0;
      w = 1;
    } else snprintf(msg, sizeof msg, "%s", e.msg ? e.msg : rio_error(&rio_vm));
    snprintf(head, sizeof head, "{\"range\":{\"start\":{\"line\":%d,\"character\":%d},", line, col);
    rio_lspPut(head, -1);
    snprintf(head, sizeof head, "\"end\":{\"line\":%d,\"character\":%d}},", line, col + w);
    rio_lspPut(head, -1);
    rio_lspPut("\"severity\":1,\"source\":\"rio\",\"message\":", -1);
    rio_lspPutString(msg, (int)strlen(msg));
    rio_lspPut("}", 1);
  }
  rio_lspPut("]}}", 3);
  rio_lspSend();
}

static void rio_lspOffer(void *userData, const char *name, int nameLen, int kind, const char *detail, int detailLen) {
  static const int kinds[] = {14, 6, 21, 22, 3, 2, 5, 9, 20}; /* (rio_Item_* as LSP's CompletionItemKind) */
  char k[32];
  int *count = (int *)userData;
  if ((*count)++) rio_lspPut(",", 1);
  rio_lspPut("{\"label\":", -1);
  rio_lspPutString(name, nameLen);
  snprintf(k, sizeof k, ",\"kind\":%d", kinds[kind >= 0 && kind < 9 ? kind : 0]);
  rio_lspPut(k, -1);
  if (detailLen) {
    rio_lspPut(",\"detail\":", -1);
    rio_lspPutString(detail, detailLen);
  }
  rio_lspPut("}", 1);
}

/* where line and character are in document d's text (characters as bytes: ASCII's the same) */
static uint32_t rio_lspOffset(int d, int line, int character, uint32_t *lineStart) {
  uint32_t at = 0;
  const char *t = rio_lspDocs[d].text;
  while (line > 0 && at < rio_lspDocs[d].len)
    if (t[at++] == '\n') line--;
  *lineStart = at;
  while (character-- > 0 && at < rio_lspDocs[d].len && t[at] != '\n') at++;
  return at;
}

/* hover and go to definition: what the name at the cursor is (rio_lookupAt, after a check up to its line),
   and where it's declared */
static struct {
  int found, kind, line, textAt;
  char name[128], detail[256], file[1024];
} rio_lspFound;
static void rio_lspFind(void *userData, const char *name, int nameLen, int kind, const char *detail, int detailLen, const char *file,
                        int line, int textAt) {
  (void)userData;
  if (rio_lspFound.found++) return; /* (the first: the one in scope) */
  rio_lspFound.kind = kind;
  rio_lspFound.line = line;
  rio_lspFound.textAt = textAt;
  snprintf(rio_lspFound.name, sizeof rio_lspFound.name, "%.*s", nameLen, name);
  snprintf(rio_lspFound.detail, sizeof rio_lspFound.detail, "%.*s", detailLen, detail);
  snprintf(rio_lspFound.file, sizeof rio_lspFound.file, "%s", file ? file : "");
}

/* the name at line and character of document d, found: 1, and rio_lspFound says what it is (its line made
   one in d, from 1, when the text itself declares it), else 0 */
static int rio_lspFindAt(int d, int line, int character) {
  uint32_t lineStart, at = rio_lspOffset(d, line, character, &lineStart), end = at;
  const char *t = rio_lspDocs[d].text;
  while (end < rio_lspDocs[d].len && (rio_isAlpha(t[end]) || (t[end] >= '0' && t[end] <= '9'))) end++; /* (to the word's end) */
  if (end == lineStart || !(rio_isAlpha(t[end - 1]) || (t[end - 1] >= '0' && t[end - 1] <= '9'))) return 0;
  memset(&rio_lspFound, 0, sizeof rio_lspFound);
  rio_lspCheck(d, lineStart);
  rio_lookupAt(&rio_vm, t + lineStart, end - lineStart, rio_lspFind, 0);
  if (rio_lspFound.found && rio_lspFound.textAt >= 0) rio_lspFound.line = line + 1, rio_lspFound.file[0] = 0;
  return rio_lspFound.found && rio_lspFound.kind != rio_Item_keyword;
}

/* the text a declaration's in: document d (no file), else its file, read; its length, or -1 */
static long rio_lspSource(int d, const char *file, const char **text) {
  static char buf[rio_LSP_TEXT];
  FILE *f;
  size_t n;
  int i;
  if (!*file) {
    *text = rio_lspDocs[d].text;
    return (long)rio_lspDocs[d].len;
  }
  for (i = 0; i < rio_lspImportCount; i++) /* (an auto-import's: its text, and its file, from now on) */
    if (!strcmp(file, rio_lspImports[i].name) || !strcmp(file, rio_lspImports[i].path) ||
        (!strncmp(file, rio_lspImports[i].name, strlen(rio_lspImports[i].name)) && !strcmp(file + strlen(rio_lspImports[i].name), ".rio"))) {
      snprintf(rio_lspFound.file, sizeof rio_lspFound.file, "%s", rio_lspImports[i].path);
      *text = rio_lspImports[i].src;
      return (long)rio_lspImports[i].len;
    }
  if (!(f = fopen(file, "rb"))) return -1;
  n = fread(buf, 1, sizeof buf - 1, f);
  fclose(f);
  *text = buf;
  return (long)n;
}

/* where line (from 1) starts in text, or -1 */
static long rio_lspLineAt(const char *text, long len, int line) {
  long at = 0;
  while (--line > 0 && at < len) {
    while (at < len && text[at] != '\n') at++;
    at++;
  }
  return at <= len ? at : -1;
}

static void rio_lspHover(const char *id, int d, int line, int character) {
  static const char *const kinds[] = {"", "", "", "", "", "", "", "module ", ""};
  const char *text;
  long len, at, start;
  char head[512];
  if (!rio_lspFindAt(d, line, character)) {
    rio_lspReply(id, "null");
    return;
  }
  if (rio_lspFound.kind == rio_Item_proc || rio_lspFound.kind == rio_Item_method || rio_lspFound.kind == rio_Item_type)
    snprintf(head, sizeof head, "%s :: %s", rio_lspFound.name, rio_lspFound.detail[0] ? rio_lspFound.detail : "?");
  else if (rio_lspFound.detail[0])
    snprintf(head, sizeof head, "%s%s: %s", kinds[rio_lspFound.kind], rio_lspFound.name, rio_lspFound.detail);
  else snprintf(head, sizeof head, "%s", rio_lspFound.name);
  rio_lspPut("{\"jsonrpc\":\"2.0\",\"id\":", -1);
  rio_lspPut(id, (int)strcspn(id, ",}"));
  rio_lspPut(",\"result\":{\"contents\":{\"kind\":\"markdown\",\"value\":", -1);
  {
    static char md[8192];
    int n = snprintf(md, sizeof md, "```rio\n%s\n```", head);
    /* (the comment lines just above the declaration, and one on its own line: what it says it is) */
    if (rio_lspFound.line > 0 && (len = rio_lspSource(d, rio_lspFound.file, &text)) >= 0 && (at = rio_lspLineAt(text, len, rio_lspFound.line)) >= 0) {
      const char *hash = 0, *p;
      long first = at, e = at;
      while (e < len && text[e] != '\n') e++;
      for (p = text + at; p < text + e; p++) /* (a comment after the code, outside a string) */
        if (*p == '"') {
          for (p++; p < text + e && *p != '"'; p++)
            if (*p == '\\') p++;
        } else if (*p == '#') {
          hash = p;
          break;
        }
      for (;;) { /* (back over the comment lines above) */
        long s = first - 1, q;
        if (s <= 0) break;
        start = s;
        while (start > 0 && text[start - 1] != '\n') start--;
        for (q = start; q < s && (text[q] == ' ' || text[q] == '\t'); q++) {}
        if (q >= s || text[q] != '#') break;
        first = start;
      }
      if (first < at || hash) n += snprintf(md + n, sizeof md - (size_t)n, "\n\n");
      while (first < at && n < (int)sizeof md - 2) { /* (each comment line's words, # left off) */
        long s = first;
        while (s < at && (text[s] == ' ' || text[s] == '\t')) s++;
        s++;
        if (s < at && text[s] == ' ') s++;
        while (s < at && text[s] != '\n' && n < (int)sizeof md - 2) md[n++] = text[s++] == '\r' ? ' ' : text[s - 1];
        md[n++] = ' ';
        first = s + 1;
      }
      if (hash) {
        for (p = hash + 1; *p == ' '; p++) {}
        while (p < text + e && *p != '\r' && n < (int)sizeof md - 1) md[n++] = *p++;
      }
    }
    rio_lspPutString(md, n);
  }
  rio_lspPut("}}}", 3);
  rio_lspSend();
}

static void rio_lspDefinition(const char *id, int d, int line, int character) {
  const char *text;
  long len, at;
  int col = 0, n = (int)strlen(rio_lspFound.name);
  char range[160];
  if (!rio_lspFindAt(d, line, character) || rio_lspFound.line <= 0 || (len = rio_lspSource(d, rio_lspFound.file, &text)) < 0 ||
      (at = rio_lspLineAt(text, len, rio_lspFound.line)) < 0) {
    rio_lspReply(id, "null");
    return;
  }
  n = (int)strlen(rio_lspFound.name);
  { /* (the name on its line: its column) */
    long p;
    for (p = at; p + n <= len && text[p] != '\n'; p++)
      if (!memcmp(text + p, rio_lspFound.name, (size_t)n) && (p == at || !rio_isAlpha(text[p - 1])) &&
          (p + n >= len || !(rio_isAlpha(text[p + n]) || (text[p + n] >= '0' && text[p + n] <= '9')))) {
        col = (int)(p - at);
        break;
      }
  }
  rio_lspPut("{\"jsonrpc\":\"2.0\",\"id\":", -1);
  rio_lspPut(id, (int)strcspn(id, ",}"));
  rio_lspPut(",\"result\":{\"uri\":", -1);
  if (!rio_lspFound.file[0]) rio_lspPutString(rio_lspDocs[d].uri, (int)strlen(rio_lspDocs[d].uri));
  else {
    char uri[1200], *u;
    snprintf(uri, sizeof uri, "file://%s%s", rio_lspFound.file[0] == '/' ? "" : "/", rio_lspFound.file);
    for (u = uri; *u; u++)
      if (*u == '\\') *u = '/';
    rio_lspPutString(uri, (int)strlen(uri));
  }
  snprintf(range, sizeof range, ",\"range\":{\"start\":{\"line\":%d,\"character\":%d},\"end\":{\"line\":%d,\"character\":%d}}}}",
           rio_lspFound.line - 1, col, rio_lspFound.line - 1, col + n);
  rio_lspPut(range, -1);
  rio_lspSend();
}

static int rio_lspDoc(const char *msg, int open) { /* (the document a message is about: by its uri) */
  char uri[1024];
  int i, free = -1;
  if (rio_jsonString(rio_jsonAt(msg, "uri"), uri, sizeof uri) < 0) return -1;
  for (i = 0; i < rio_LSP_DOCS; i++) {
    if (!strcmp(rio_lspDocs[i].uri, uri)) return i;
    if (free < 0 && !rio_lspDocs[i].uri[0]) free = i;
  }
  if (!open) return -1;
  if (free < 0) free = 0; /* (full: the first goes) */
  snprintf(rio_lspDocs[free].uri, sizeof rio_lspDocs[free].uri, "%s", uri);
  return free;
}

static int rio_lspMain(void) {
  static char formatted[rio_LSP_TEXT + 4096];
#ifdef _WIN32
  _setmode(_fileno(stdin), _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  for (;;) {
    char line[256], method[64];
    long size = -1;
    const char *id;
    int d;
    while (fgets(line, sizeof line, stdin)) { /* the headers, to a blank line */
      if (!strncmp(line, "Content-Length:", 15)) size = atol(line + 15);
      if (line[0] == '\r' || line[0] == '\n') break;
    }
    if (size < 0 || size >= (long)sizeof rio_lspIn) return size < 0 ? 0 : 1;
    if (fread(rio_lspIn, 1, (size_t)size, stdin) != (size_t)size) return 1;
    rio_lspIn[size] = 0;
    if (rio_jsonString(rio_jsonAt(rio_lspIn, "method"), method, sizeof method) < 0) continue;
    id = rio_jsonAt(rio_lspIn, "id");
    if (!strcmp(method, "initialize"))
      rio_lspReply(id, "{\"capabilities\":{\"textDocumentSync\":1,\"completionProvider\":{\"triggerCharacters\":[\".\"]},"
                       "\"documentFormattingProvider\":true,\"hoverProvider\":true,\"definitionProvider\":true},\"serverInfo\":{\"name\":\"rio\"}}");
    else if (!strcmp(method, "shutdown")) rio_lspReply(id, "null");
    else if (!strcmp(method, "exit")) return 0;
    else if ((!strcmp(method, "textDocument/didOpen") || !strcmp(method, "textDocument/didChange")) &&
             (d = rio_lspDoc(rio_lspIn, 1)) >= 0) {
      const char *text = !strcmp(method, "textDocument/didOpen") ? rio_jsonAt(rio_lspIn, "text") : rio_jsonAt(rio_jsonAt(rio_lspIn, "contentChanges"), "text");
      int n = rio_jsonString(text, rio_lspDocs[d].text, rio_LSP_TEXT);
      rio_lspDocs[d].len = n > 0 ? (uint32_t)n : 0;
      rio_lspDiagnose(d);
    } else if (!strcmp(method, "textDocument/didClose") && (d = rio_lspDoc(rio_lspIn, 0)) >= 0) rio_lspDocs[d].uri[0] = 0;
    else if (!strcmp(method, "textDocument/completion") && id) {
      const char *pos = rio_jsonAt(rio_lspIn, "position");
      uint32_t lineStart, at;
      int count = 0;
      if ((d = rio_lspDoc(rio_lspIn, 0)) < 0 || !pos) {
        rio_lspReply(id, "[]");
        continue;
      }
      at = rio_lspOffset(d, atoi(rio_jsonAt(pos, "line")), atoi(rio_jsonAt(pos, "character")), &lineStart);
      rio_lspCheck(d, lineStart); /* (up to the cursor's line: what's open there stays in the tables) */
      rio_lspPut("{\"jsonrpc\":\"2.0\",\"id\":", -1);
      rio_lspPut(id, (int)strcspn(id, ",}"));
      rio_lspPut(",\"result\":[", -1);
      rio_complete(&rio_vm, rio_lspDocs[d].text + lineStart, at - lineStart, rio_lspOffer, &count);
      rio_lspPut("]}", 2);
      rio_lspSend();
    } else if ((!strcmp(method, "textDocument/hover") || !strcmp(method, "textDocument/definition")) && id) {
      const char *pos = rio_jsonAt(rio_lspIn, "position");
      if ((d = rio_lspDoc(rio_lspIn, 0)) < 0 || !pos) rio_lspReply(id, "null");
      else if (method[13] == 'h') rio_lspHover(id, d, atoi(rio_jsonAt(pos, "line")), atoi(rio_jsonAt(pos, "character")));
      else rio_lspDefinition(id, d, atoi(rio_jsonAt(pos, "line")), atoi(rio_jsonAt(pos, "character")));
    } else if (!strcmp(method, "textDocument/formatting") && id) {
      int32_t n;
      int lines = 0;
      uint32_t i;
      char range[96];
      if ((d = rio_lspDoc(rio_lspIn, 0)) < 0 || (n = rio_format(rio_lspDocs[d].text, rio_lspDocs[d].len, formatted, sizeof formatted)) < 0 ||
          ((uint32_t)n == rio_lspDocs[d].len && !memcmp(formatted, rio_lspDocs[d].text, (size_t)n))) {
        rio_lspReply(id, "[]");
        continue;
      }
      for (i = 0; i < rio_lspDocs[d].len; i++) lines += rio_lspDocs[d].text[i] == '\n';
      rio_lspPut("{\"jsonrpc\":\"2.0\",\"id\":", -1);
      rio_lspPut(id, (int)strcspn(id, ",}"));
      snprintf(range, sizeof range, ",\"result\":[{\"range\":{\"start\":{\"line\":0,\"character\":0},\"end\":{\"line\":%d,\"character\":0}},", lines + 1);
      rio_lspPut(range, -1);
      rio_lspPut("\"newText\":", -1);
      rio_lspPutString(formatted, n);
      rio_lspPut("}]}", 3);
      rio_lspSend();
    } else if (id && strncmp(method, "$/", 2) && strcmp(method, "initialized")) rio_lspReply(id, "null"); /* (anything else asked: nothing) */
  }
}

static int rio_programAt(int argc, char **argv) {
  int i;
  for (i = 1; i < argc; i++) {
    const char *a = argv[i];
    if (a[0] != '-') return i;
    if (!strcmp(a, "-c") || !strcmp(a, "-i") || !strcmp(a, "-H") || !strcmp(a, "-M") || !strcmp(a, "-window") ||
        ((!strcmp(a, "-L") || !strcmp(a, "-D"))))
      i++; /* (and its value) */
  }
  return argc;
}

int main(int argc, char **argv) {
  FILE *f;
  size_t n;
  int proc, i, memReport = 0, jit = 0, programArgs = argc, at = rio_programAt(argc, argv);
  const char *cOut = 0, *imageOut = 0, *headerOut = 0, *path = 0;
  unsigned long memSize = rio_MEM_DEFAULT;
  if (argc > 1 && !strcmp(argv[1], "fmt")) return rio_fmtMain(argc - 2, argv + 2);
  if (argc > 1 && !strcmp(argv[1], "lsp")) return rio_lspMain();
  for (i = 1; i < at; i++) { /* (these on their own) */
    if (!strcmp(argv[i], "-help") || !strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) return fputs(rio_help, stdout) < 0;
    if (!strcmp(argv[i], "-license") || !strcmp(argv[i], "--license")) return fputs(rio_license, stdout) < 0;
  }
  for (i = 1; i < at; i++) /* -M first: rio_init comes before -D */
    if (!strcmp(argv[i], "-M") && i + 1 < argc) {
      memSize = rio_parseMemSize(argv[++i]);
      if (!memSize) {
        fprintf(stderr, "-M: 4K to 1G\n");
        return 2;
      }
    }
  if (!(rio_mem = (uint8_t *)calloc(1, memSize))) {
    fprintf(stderr, "can't get %lu bytes for -M\n", memSize);
    return 1;
  }
  atexit(rio_freeMem);
  if (rio_init(&rio_vm, rio_mem, (uint32_t)memSize, 0, 0)) {
    fprintf(stderr, "init failed\n");
    return 1;
  }
  for (i = 1; i < argc; i++) {
    char *arg = argv[i], *eq;
    if (!strcmp(arg, "-c") && i + 1 < argc) cOut = argv[++i];
    else if (!strcmp(arg, "-i") && i + 1 < argc) imageOut = argv[++i];
    else if (!strcmp(arg, "-H") && i + 1 < argc) headerOut = argv[++i];
    else if (!strcmp(arg, "-j")) jit = 1;
    else if (!strcmp(arg, "-M") && i + 1 < argc) i++;
    else if (!strcmp(arg, "-m")) memReport = 1;
    else if (!strcmp(arg, "-window") && i + 1 < argc) rio_window = atoi(argv[++i]);
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
    } else if (arg[0] == '-') return rio_usage();
    else { /* the program's file: the rest are its arguments */
      path = arg;
      programArgs = i + 1;
      break;
    }
  }
  rio_osStart(argc - programArgs, argv + programArgs);
  rio_setLog(&rio_vm, rio_printLine, 0);
  if (rio_addOs()) {
    fprintf(stderr, "can't add module os\n");
    return 1;
  }
  rio_setLoader(&rio_vm, rio_loadFile, 0);
  if (rio_window) rio_setSourceWindow(&rio_vm, (uint32_t)rio_window);
  signal(SIGINT, rio_onInterrupt);
  if (!path) return cOut || memReport || imageOut || headerOut ? rio_usage() : rio_repl();
  rio_setBaseDir(path);
  if (rio_readFile(path) < 0) {
    fprintf(stderr, "cannot open %s\n", path);
    return 2;
  }
  n = rio_srcLen;
  if (headerOut) { /* the C header, named for the file: pc.rio's pc_Text, pc_text_SIG */
    static char header[1 << 20], prefix[64];
    const char *base = path + strlen(path);
    uint32_t size;
    size_t k = 0;
    while (base > path && base[-1] != '/' && base[-1] != '\\') base--;
    while (base[k] && base[k] != '.' && k < sizeof prefix - 1) prefix[k] = base[k], k++;
    if (!(size = rio_hostHeader(&rio_vm, rio_src, (uint32_t)n, rio_scratch, sizeof rio_scratch, prefix, header, sizeof header))) {
      rio_report(path);
      return 1;
    }
    if (!(f = fopen(headerOut, "wb")) || fwrite(header, 1, size, f) != size || fclose(f)) {
      fprintf(stderr, "cannot write %s\n", headerOut);
      return 2;
    }
    return 0;
  }
  if (n >= 4 && !memcmp(rio_src, "RIOI", 4)) { /* an image: just run it */
    if (cOut || memReport || imageOut) return rio_usage();
    if (rio_loadImage(&rio_vm, rio_src, (uint32_t)n)) {
      rio_report(path);
      return 1;
    }
  } else {
    /* the C backend and the memory report read compiler tables after compiling, so they get their own
       buffer; running compiles in place, with the compiler's state overlaid on script memory */
    int failed;
    if (memReport) { /* first, compiled in place, as for running: how near its limits it came */
      static rio_Vm probe;
      static char text[2048];
      rio_Usage u;
      size_t loaded = rio_srcLen;
      probe = rio_vm; /* (set up, not compiled yet) */
      rio_trackUsage(&probe, &u);
      failed = rio_compile(&probe, rio_src, (uint32_t)n);
      rio_srcLen = loaded;
      rio_usageText(&u, text, sizeof text);
      printf("compiled in place (%s):\n%s\n", failed ? rio_error(&probe) : "fits", text);
    }
    if (cOut || memReport) failed = rio_compileScratch(&rio_vm, rio_src, (uint32_t)n, rio_scratch, sizeof rio_scratch);
    else if (rio_window) { /* (read a part at a time) */
      static rio_MemText main;
      main.src = rio_src;
      main.len = (uint32_t)n;
      failed = rio_compileStream(&rio_vm, rio_readText, &main);
    } else failed = rio_compile(&rio_vm, rio_src, (uint32_t)n);
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
      error = rio_aot(&rio_vm, f, rio_hostFfi());
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
