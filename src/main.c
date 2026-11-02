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
