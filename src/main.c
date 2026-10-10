/* rio command line host.
   rio                     interactive REPL
   rio file.rio            run top-level code, then main() if present
   rio -c out.c file.rio   compile to a standalone C program instead
   -D NAME[=value]         override a top-level constant (or define it), like gcc -D
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
static char src[1 << 20];
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

static void onint(int sig) { (void)sig; gotint = 1; rio_interrupt(&vm); signal(SIGINT, onint); }

static int usage(void) {
  fprintf(stderr, "usage: rio [-D NAME[=value]]...                REPL\n"
                  "       rio [-D NAME[=value]]... file.rio       run\n"
                  "       rio -c out.c [-D NAME[=value]]... file.rio\n");
  return 2;
}

static void report(const char *where) {
  fflush(stdout);
  if (where) fprintf(stderr, "%s%s", where, rio_error_info(&vm).line ? ":" : ": ");
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

int main(int argc, char **argv) {
  FILE *f; size_t n; int fn, i;
  const char *cout = 0, *path = 0;
  if (rio_init(&vm, mem, sizeof mem, code, 65535)) { fprintf(stderr, "init failed\n"); return 1; }
  for (i = 1; i < argc; i++) {
    char *a = argv[i], *eq;
    if (!strcmp(a, "-c") && i + 1 < argc) cout = argv[++i];
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
  signal(SIGINT, onint);
  if (!path) return cout ? usage() : repl();
  if (!(f = fopen(path, "rb"))) { fprintf(stderr, "cannot open %s\n", path); return 2; }
  n = fread(src, 1, sizeof src, f);
  fclose(f);
  /* the C backend reads compiler tables after compiling, so they get their own buffer; running
     compiles in place, with the compiler's state overlaid on script memory */
  if (cout ? rio_compile_scratch(&vm, src, (uint32_t)n, scratch, sizeof scratch) : rio_compile(&vm, src, (uint32_t)n)) {
    report(path);
    return 1;
  }
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
