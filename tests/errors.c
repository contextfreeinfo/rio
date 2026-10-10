/* error reporting tests: each snippet must fail with exactly this kind, position and message */
#include <stdio.h>
#include <string.h>
#include "rio.h"

static unsigned char mem[1 << 20];
static RioIns code[4096];
static Rio vm;

typedef struct {
  const char *src, *def, *defval; /* optional -D */
  uint32_t memsize;               /* 0 = all of mem */
  int kind, line, col, len;       /* line/col/len -1 = don't check */
  const char *msg;                /* substring of the message */
} Case;

static const Case cases[] = {
  /* compile errors point at the token at fault */
  {"x := 1\ny := x + nope\n", 0, 0, 0, RIO_ECOMPILE, 2, 10, 4, "undefined name"},
  {"x: Nope = 1\n", 0, 0, 0, RIO_ECOMPILE, 1, 4, 4, "type expected"},
  {"v := 3\nif v\nend\n", 0, 0, 0, RIO_ECOMPILE, 2, 4, 1, "condition must be Bool"},
  {"s := \"abc\"\ns[0] = 1\n", 0, 0, 0, RIO_ECOMPILE, 2, 6, 1, "cannot assign"},
  {"f :: proc(n: Int) -> Int\n  return f(n)\nend\n", 0, 0, 0, RIO_ECOMPILE, 2, 10, 1, "recursion is not allowed"},
  {"V :: struct\n  x: Float\nend\nv := V{1}\n", 0, 0, 0, RIO_ECOMPILE, 4, 8, 1, "must be named"},
  {"V :: struct\n  x: Float\nend\nv := V{x = 1, x = 2}\n", 0, 0, 0, RIO_ECOMPILE, 4, 15, 1, "field set twice"},
  {"for i in 0..<3\n  break\nend\nbreak\n", 0, 0, 0, RIO_ECOMPILE, 4, 1, 5, "not inside a loop"},
  {"x := 1\nelse\n", 0, 0, 0, RIO_ECOMPILE, 2, 1, 4, "'else' without 'if'"},
  {"x := 1\nend\n", 0, 0, 0, RIO_ECOMPILE, 2, 1, 3, "'end' without block"},
  {"x := 1\nreturn\n", 0, 0, 0, RIO_ECOMPILE, 2, 1, 6, "return outside proc"},
  {"xs: [..2]Int\nxs.format(1)\n",0, 0, 0, RIO_ECOMPILE, 2, 10, 1, "format needs a Blob list"},
  {"S :: struct\n  x: Int\nend\nS.f :: proc()\n  self.f()\nend\n", 0, 0, 0, RIO_ECOMPILE, 5, 8, 1, "recursion is not allowed"},
  {"S :: struct\n  x: Int\nend\nS.x :: proc()\nend\n", 0, 0, 0, RIO_ECOMPILE, 4, 3, 1, "same name as a field"},
  {"S :: struct\n  x: Int\nend\nS.f :: proc()\nend\nS.f :: proc()\nend\n", 0, 0, 0, RIO_ECOMPILE, 6, 3, 1, "method already defined"},
  {"S :: struct\n  x: Int\nend\nS.f :: proc()\nend\ns: S\ng := s.f\n", 0, 0, 0, RIO_ECOMPILE, 7, 8, 1, "method calls need ()"},
  {"S :: struct\n  x: Int\nend\ns: S\ns.nope()\n", 0, 0, 0, RIO_ECOMPILE, 5, 3, 4, "no such field or method"},
  {"xs: [..2]Int\npush(xs, 1)\n", 0, 0, 0, RIO_ECOMPILE, 2, 1, 4, "undefined name"},
  {"V :: struct\n  x: Float\nend\nv := {x = 1}\n", 0, 0, 0, RIO_ECOMPILE, 4, 6, 1, "can't tell which struct"},
  {"V :: struct\n  x: Float\nend\nlog({x = 1})\n", 0, 0, 0, RIO_ECOMPILE, 4, 5, 1, "can't tell which struct"},
  {"V :: struct\n  x: Float\nend\nn: Int = {x = 1}\n", 0, 0, 0, RIO_ECOMPILE, 4, 10, 1, "can't tell which struct"},
  {"V :: struct\n  x, y: Int\nend\nxs: [2]Int\nv := V{xs[0]}\n", 0, 0, 0, RIO_ECOMPILE, 5, 13, 1, "only a name or a.b path"},
  {"V :: struct\n  x, y: Int\nend\nz := 1\nv := V{z}\n", 0, 0, 0, RIO_ECOMPILE, 5, 9, 1, "no such field"},
  {"V :: struct\n  x, y: Int\nend\nx := 1\nv := V{x, x = 2}\n", 0, 0, 0, RIO_ECOMPILE, 5, 11, 1, "field set twice"},
  {"V :: struct\n  x, y: Int\nend\nv: V\n{x} = v\n", 0, 0, 0, RIO_ECOMPILE, 5, 5, 1, "':=' expected"},
  {"V :: struct\n  x, y: Int\nend\nv: V\n{x as} := v\n", 0, 0, 0, RIO_ECOMPILE, 5, 6, 1, "name expected after 'as'"},
  {"{x} := 5\n", 0, 0, 0, RIO_ECOMPILE, 1, 8, 1, "only a struct can be destructured"},
  {"V :: struct\n  x, y: Int\nend\nv: V\n{x, z} := v\n", 0, 0, 0, RIO_ECOMPILE, 5, 11, 1, "no such field to destructure"},
  {"x := Float(3)\n", 0, 0, 0, RIO_ECOMPILE, 1, 11, 1, "convert with x.toInt()"},
  {"s := \"a\".toFloat()\n", 0, 0, 0, RIO_ECOMPILE, 1, 18, 1, "no such conversion"},
  {"Int.toFloat :: proc() -> Float\n  return 0\nend\n", 0, 0, 0, RIO_ECOMPILE, 1, 5, 7, "that conversion is built in"},
  /* ...or at the last token of an expression that can only be judged once it's complete */
  {"x := 1\n  y := 2.0 + \"s\"\n", 0, 0, 0, RIO_ECOMPILE, 2, 14, 3, "type mismatch"},
  {"x := (1 + 2\n", 0, 0, 0, RIO_ECOMPILE, 1, 11, 1, "unclosed bracket"},
  {"if true\n  x := 1\n", 0, 0, 0, RIO_ECOMPILE, 2, 8, 1, "missing 'end'"},
  {"s := \"abc\n", 0, 0, 0, RIO_ECOMPILE, -1, -1, -1, "unterminated string"},
  /* runtime errors report the line */
  {"xs: [4]Int\ni := 2\nf :: proc(n: Int) -> Int\n  return xs[n * 3]\nend\nlog(f(i))\n", 0, 0, 0, RIO_ERUNTIME, 4, 0, 0, "index out of bounds"},
  {"xs: [..2]Int\nfor j in 0..<3\n  xs.push(j)\nend\nlog(xs.pop(), xs.pop(), xs.pop())\n", 0, 0, 0, RIO_ERUNTIME, 5, 0, 0, "pop from empty list"},
  {"main :: proc()\n  a := 0\n  b := 5\n  log(b / a)\nend\n", 0, 0, 0, RIO_ERUNTIME, 4, 0, 0, "division by zero"},
  {"a: [3]Int\nn := 5\nb := a[1:n]\n", 0, 0, 0, RIO_ERUNTIME, 3, 0, 0, "slice out of bounds"},
  {"xs: [2]Int\ni := 0\nfor xs[i] == 0\n  i += 1\nend\n", 0, 0, 0, RIO_ERUNTIME, 3, 0, 0, "index out of bounds"}, /* rotated loop condition */
  {"main :: proc()\n  log(\"x\")\n  boom()\nend\n", 0, 0, 0, RIO_ERUNTIME, 3, 0, 0, "the host said no"},
  /* errors with no source position */
  {"N :: 10\n", "N", "2.5", 0, RIO_ECOMPILE, 0, 0, 0, "-D N: value doesn't fit Int"},
  {"N :: 10\n", "9x", "1", 0, RIO_ECOMPILE, 0, 0, 0, "-D: bad name 9x"},
  {"x := 1\n", 0, 0, 4096, RIO_ECOMPILE, 0, 0, 0, "not enough memory for the compiler"},
  /* success clears the previous error */
  {"x := 1\n", 0, 0, 0, RIO_ENONE, 0, 0, 0, ""},
};

static void boom(Rio *v, RioVal *a) { (void)a; rio_trap(v, "the host said no"); }

int main(void) {
  int i, fails = 0, n = (int)(sizeof cases / sizeof cases[0]);
  for (i = 0; i < n; i++) {
    const Case *c = &cases[i]; RioError e; int f;
    rio_init(&vm, mem, c->memsize ? c->memsize : sizeof mem, code, 4096);
    rio_ffi(&vm, "boom", "", boom);
    if (c->def) rio_define(&vm, c->def, c->defval);
    if (!rio_compile(&vm, c->src, (uint32_t)strlen(c->src)) && !rio_run(&vm)) {
      if ((f = rio_func(&vm, "main")) >= 0) rio_call(&vm, f);
    }
    e = rio_error_info(&vm);
    if (e.kind != c->kind || (c->line >= 0 && e.line != c->line) || (c->col >= 0 && e.col != c->col) ||
        (c->len >= 0 && e.len != c->len) || !strstr(e.msg, c->msg)) {
      printf("FAIL case %d: got kind %d %d:%d len %d \"%s\"; want kind %d %d:%d len %d \"%s\"\n",
             i, e.kind, e.line, e.col, e.len, e.msg, c->kind, c->line, c->col, c->len, c->msg);
      fails++;
    }
  }
  { /* explicit limits: tiny tables fail cleanly, and a scratch buffer too small for them is refused */
    static unsigned char scratch[1 << 16];
    static const char *many = "a := 1\nb := 2\nc := 3\nd := 4\ne := 5\nf := 6\ng := 7\nh := 8\n";
    RioLimits l = rio_limits_for(sizeof scratch);
    RioError e;
    l.syms = 33; /* the builtins take 28, so the 6th global is one too many */
    rio_init(&vm, mem, sizeof mem, code, 4096);
    if (!rio_compile_ex(&vm, many, (uint32_t)strlen(many), scratch, sizeof scratch, &l) ||
        !strstr((e = rio_error_info(&vm)).msg, "too many symbols") || e.line != 6) {
      printf("FAIL explicit limits: \"%s\"\n", rio_error(&vm)); fails++; n++;
    } else n++;
    l = rio_limits_for(1 << 20);
    rio_init(&vm, mem, sizeof mem, code, 4096);
    if (!rio_compile_ex(&vm, many, (uint32_t)strlen(many), scratch, sizeof scratch, &l) ||
        !strstr(rio_error(&vm), "scratch too small for these limits")) {
      printf("FAIL limits bigger than scratch: \"%s\"\n", rio_error(&vm)); fails++; n++;
    } else n++;
  }
  if (!fails) printf("all %d error tests passed\n", n);
  return fails != 0;
}
