/* rio command line host.
   rio                     interactive REPL
   rio file.rio            run top-level code, then main() if present
   rio -c out.c file.rio   compile to a standalone C program instead
   rio -m file.rio         report where the program's memory goes, instead of running it
   -D NAME[=value]         override a top-level constant (or define it), like gcc -D
   -L dir                  look for packages (import name) in dir: each dir/name or dir/name.rio
   Ctrl-C stops a running program. */
#define _CRT_SECURE_NO_WARNINGS
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "rio.h"
#include "aot.h"

static uint8_t mem[8 << 20];
static RioIns code[65535];
static char src[4 << 20]; /* the main file and every module, kept until compiling ends */
static size_t srcn;
static char libs[16][512], base[512], names[1 << 16];
static int nlibs;
static size_t namesn;
static uint8_t scratch[2 << 20];
static Rio vm;
static volatile sig_atomic_t gotint;

static void out(void *ud, const char *s, int n) { (void)ud; fwrite(s, 1, (size_t)n, stdout); fputc('\n', stdout); }
static void ffi_clock(Rio *v, RioVal *a) { (void)v; a[0].f = (float)clock() / CLOCKS_PER_SEC; }
static void ffi_putc(Rio *v, RioVal *a) { (void)v; fputc(a[0].i, stdout); }
/* the same host functions, as C source for AOT output */
static const char *ffi_src =
  "void rio_ffi_clock(RioVal *a) { a[0].f = (float)clock() / CLOCKS_PER_SEC; }\n"
  "void rio_ffi_putc(RioVal *a) { fputc(a[0].i, stdout); }\n";

/* read a whole file into the source arena; -1 if missing */
static long readfile(const char *path) {
  FILE *f = fopen(path, "rb"); size_t n;
  if (!f) return -1;
  n = fread(src + srcn, 1, sizeof src - srcn - 1, f);
  fclose(f);
  srcn += n;
  return (long)n;
}
static int exists(const char *path) { FILE *f = fopen(path, "rb"); if (f) fclose(f); return f != 0; }
/* path.rio, or the directory path/ with its entry file path/<last>.rio */
static int resolve(const char *dir, const char *path, int file, char *out, size_t cap, int *isdir) {
  const char *last = strrchr(path, '/');
  last = last ? last + 1 : path;
  if (file) { snprintf(out, cap, "%s%s", dir, path); *isdir = 0; return exists(out); } /* include "x.rio" */
  snprintf(out, cap, "%s%s.rio", dir, path);
  if (exists(out)) { *isdir = 0; return 1; }
  snprintf(out, cap, "%s%s/%s.rio", dir, path, last);
  if (exists(out)) { *isdir = 1; return 1; }
  return 0;
}
static int loader(void *ud, const char *path, int kind, RioSource *o) {
  char found[1024], tryp[1024], *name; int isdir = 0, n = 0, i, d, file = kind & RIO_LOAD_FILE; long len;
  (void)ud;
  if (!(kind & RIO_LOAD_PKG)) n = resolve(base, path, file, found, sizeof found, &isdir);
  else
    for (i = 0; i < nlibs; i++)
      if (resolve(libs[i], path, file, tryp, sizeof tryp, &d)) { if (n++) return 2; memcpy(found, tryp, sizeof found); isdir = d; }
  if (!n) return 1;
  o->src = src + srcn;
  if ((len = readfile(found)) < 0) return 1;
  o->len = (uint32_t)len; o->isdir = isdir;
  name = names + namesn; namesn += (size_t)snprintf(name, sizeof names - namesn, "%s", found) + 1;
  o->name = name;
  return 0;
}

static void onint(int sig) { (void)sig; gotint = 1; rio_interrupt(&vm); signal(SIGINT, onint); }

static int usage(void) {
  fprintf(stderr, "usage: rio [-D NAME[=value]]... [-L dir]...                REPL\n"
                  "       rio [-D NAME[=value]]... [-L dir]... file.rio       run\n"
                  "       rio -c out.c [-D NAME[=value]]... [-L dir]... file.rio\n"
                  "       rio -m [-D NAME[=value]]... [-L dir]... file.rio    memory report\n");
  return 2;
}

static void report(const char *where) {
  RioError e = rio_error_info(&vm);
  fflush(stdout);
  if (e.file) where = e.file; /* the error is in a module */
  if (where) fprintf(stderr, "%s%s", where, e.line ? ":" : ": ");
  fprintf(stderr, "%s\n", rio_error(&vm));
}

/* each line is added to what's pending; unfinished input (an open block, bracket or string) waits
   for more, and a blank line gives up on it */
static int repl(void) {
  static char acc[1 << 16];
  char line[1024]; size_t n = 0, k; int r;
  if (rio_repl_begin(&vm, scratch, sizeof scratch, 0)) { report(0); return 1; }
  printf("rio REPL. Ctrl-C stops a running program; Ctrl-D (Ctrl-Z Enter on Windows) quits.\n");
  for (;;) {
    fputs(n ? ".. " : "> ", stdout);
    fflush(stdout);
    gotint = 0;
    if (!fgets(line, sizeof line, stdin)) {
      if (gotint) { clearerr(stdin); n = 0; putchar('\n'); continue; }
      break;
    }
    k = strlen(line);
    if (n && (line[0] == '\n' || line[0] == '\r')) { puts("(unfinished input dropped)"); n = 0; continue; }
    if (n + k >= sizeof acc) { puts("(input too long)"); n = 0; continue; }
    memcpy(acc + n, line, k); n += k;
    r = rio_repl_eval(&vm, acc, (uint32_t)n);
    if (r == RIO_MORE) continue;
    if (r) report(0);
    n = 0;
  }
  putchar('\n');
  return 0;
}

#define MOPENUM(o) M_##o,
enum { RIO_OPS(MOPENUM) M_NOPS };
/* -m: memory by kind and by proc, and what sharing proc frames along the call graph could save.
   Procs are defined before use, so every callee has a lower index than its callers. */
static const char *procname(int fi) {
  static char b[96]; RioC *c = vm.c; int i, j;
  for (i = 0; i < c->nsym; i++) {
    RioSym *y = &c->sym[i];
    if ((y->k != RIO_S_FN && y->k != RIO_S_METH) || y->v != fi) continue;
    b[0] = 0;
    if (y->k == RIO_S_METH)
      for (j = 0; j < c->nsym; j++)
        if (c->sym[j].k == RIO_S_TYPE && c->sym[j].t == y->t) { snprintf(b, 48, "%.*s.", (int)c->sym[j].len, c->names + c->sym[j].name); break; }
    snprintf(b + strlen(b), 48, "%.*s", (int)y->len, c->names + y->name);
    return b;
  }
  snprintf(b, sizeof b, "proc %d", fi);
  return b;
}
static void memreport(void) {
  static uint32_t frame[65536], chain[65536]; static int next[65536], order[65536];
  RioC *c = vm.c; int nf = vm.nfunc, i, j, pc, best = -1, shown, nshared = 0;
  uint32_t consts = vm.kcap * 4, slots = (c->hwm - vm.kcap) * 4, table = vm.memsize - vm.hi - c->big;
  uint32_t pbig = 0, own = 0, area = 0, all = 0, depth = 0, rstack, globals, total;
  for (i = 0; i < nf; i++) {
    RioFunc *f = &vm.func[i]; RioCFunc *cf = &c->func[i]; uint32_t cb = 0;
    frame[i] = (uint32_t)(f->fe - f->fs) * 4; all += frame[i]; pbig += cf->big; next[i] = -1;
    if (cf->shared) nshared++; else own += frame[i];
    if (cf->oe * 4u > area) area = cf->oe * 4u;
    if (cf->dep > depth) depth = cf->dep;
    for (pc = f->pc; pc < f->end; pc++) /* its callees: compiled earlier, so already measured */
      if (code[pc].op == M_CALL)
        for (j = 0; j < i; j++) if (vm.func[j].pc == code[pc].c) { if (chain[j] > cb) { cb = chain[j]; next[i] = j; } break; }
    chain[i] = frame[i] + cf->big + cb;
    if (best < 0 || chain[i] > chain[best]) best = i;
    order[i] = i;
  }
  rstack = (depth + 1) * 4;
  globals = slots - own - area - rstack;
  total = consts + slots + c->big + table;
  printf("memory: %u bytes in use (of %u)\n", total, vm.memsize);
  printf("  %-24s %8u   (%u words; RioLimits.consts is only room while compiling)\n", "constants", consts, vm.nk);
  printf("  %-24s %8u\n", "globals", globals + (c->big - pbig));
  printf("  %-24s %8u   (%d procs share it; without sharing their frames would take %u)\n", "shared frames", area, nshared, all - own);
  printf("  %-24s %8u   (%d procs with an address-taken slot keep their own frame; %u of it in big locals)\n", "own frames", own + pbig, nf - nshared, pbig);
  printf("  %-24s %8u   (the longest chain of calls: %u)\n", "call stack", rstack, depth);
  printf("  %-24s %8u\n", "tables", table);
  printf("    %-22s %8u   (string literals, exported names, file names)\n", "strings", vm.exports - vm.hi);
#ifdef RIO_EXPORTS_ALL
  printf("    %-22s %8u   (%u names the host can look up: every top-level proc and global, from RIO_EXPORTS_ALL)\n", "exports", vm.lines - vm.exports, vm.nexports);
#else
  printf("    %-22s %8u   (%u names the host can look up: main and names marked name*)\n", "exports", vm.lines - vm.exports, vm.nexports);
#endif
  printf("    %-22s %8u   (pc -> source line, packed: read only for runtime errors)\n", "line table", vm.nlines);
  printf("    %-22s %8u   (%d procs)\n", "proc table", (uint32_t)nf * (uint32_t)sizeof(RioFunc), nf);
  printf("  %-24s %8u   (%u instructions, in the code buffer, not memory)\n", "code", vm.pc * (uint32_t)sizeof(RioIns), vm.pc);
  for (i = 1; i < nf; i++) { int k = order[i]; for (j = i; j > 0 && frame[order[j - 1]] + c->func[order[j - 1]].big < frame[k] + c->func[k].big; j--) order[j] = order[j - 1]; order[j] = k; }
  shown = nf < 15 ? nf : 15;
  if (nf) printf("biggest procs (bytes: frame + big locals)\n");
  for (i = 0; i < shown; i++) {
    int k = order[i];
    printf("  %-24s %8u   %s%s\n", procname(k), frame[k] + c->func[k].big, c->func[k].shared ? "shared" : "own memory: an address is taken",
           c->func[k].big ? ", big locals" : "");
  }
  if (nf > shown) printf("  ... %d more\n", nf - shown);
  if (best >= 0) {
    printf("heaviest call chain: %u bytes of frames\n  ", chain[best]);
    for (i = best; i >= 0; i = next[i]) printf("%s%s", procname(i), next[i] >= 0 ? " -> " : "\n");
  }
}

int main(int argc, char **argv) {
  FILE *f; size_t n; int fn, i;
  const char *cout = 0, *path = 0; int mrep = 0;
  if (rio_init(&vm, mem, sizeof mem, code, 65535)) { fprintf(stderr, "init failed\n"); return 1; }
  for (i = 1; i < argc; i++) {
    char *a = argv[i], *eq;
    if (!strcmp(a, "-c") && i + 1 < argc) cout = argv[++i];
    else if (!strcmp(a, "-m")) mrep = 1;
    else if (!strncmp(a, "-L", 2)) {
      const char *d = a[2] ? a + 2 : i + 1 < argc ? argv[++i] : 0;
      size_t dl;
      if (!d || nlibs >= 16) return usage();
      dl = strlen(d);
      snprintf(libs[nlibs++], sizeof libs[0], "%s%s", d, dl && d[dl - 1] != '/' && d[dl - 1] != '\\' ? "/" : "");
    }
    else if (!strncmp(a, "-D", 2)) {
      char *d = a[2] ? a + 2 : i + 1 < argc ? argv[++i] : 0;
      if (!d) return usage();
      if ((eq = strchr(d, '='))) *eq++ = 0;
      if (rio_define(&vm, d, eq)) { fprintf(stderr, "too many -D defines\n"); return 2; }
    } else if (a[0] == '-' || path) return usage();
    else path = a;
  }
  rio_set_log(&vm, out, 0);
  rio_ffi(&vm, "clock", ">f", ffi_clock);
  rio_ffi(&vm, "putc", "i", ffi_putc);
  rio_set_loader(&vm, loader, 0);
  signal(SIGINT, onint);
  if (!path) return cout || mrep ? usage() : repl();
  { /* local imports are relative to the main file's directory */
    const char *sl = strrchr(path, '/'), *bs = strrchr(path, '\\');
    size_t bl = (size_t)((sl > bs ? sl : bs) ? (sl > bs ? sl : bs) - path + 1 : 0);
    memcpy(base, path, bl < sizeof base ? bl : 0); base[bl < sizeof base ? bl : 0] = 0;
  }
  if (readfile(path) < 0) { fprintf(stderr, "cannot open %s\n", path); return 2; }
  n = srcn;
  (void)f;
  /* the C backend reads compiler tables after compiling, so they get their own buffer; running
     compiles in place, with the compiler's state overlaid on script memory */
  if (cout || mrep ? rio_compile_scratch(&vm, src, (uint32_t)n, scratch, sizeof scratch) : rio_compile(&vm, src, (uint32_t)n)) {
    report(path);
    return 1;
  }
  if (mrep) { memreport(); return 0; }
  if (cout) {
    int err;
    if (!(f = fopen(cout, "w"))) { fprintf(stderr, "cannot write %s\n", cout); return 2; }
    err = rio_aot(&vm, f, ffi_src);
    if (fclose(f) || err) { fprintf(stderr, "error writing %s\n", cout); return 1; }
    return 0;
  }
  if (rio_run(&vm) || ((fn = rio_func(&vm, "main")) >= 0 && rio_call(&vm, fn))) {
    report(path);
    return 1;
  }
  return 0;
}
