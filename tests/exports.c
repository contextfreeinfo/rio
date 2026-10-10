/* which names the host can look up: main and the names marked name*, or with RIO_EXPORTS_ALL every
   top-level proc and global */
#include <stdio.h>
#include <string.h>
#include "rio.h"

static unsigned char mem[1 << 20];
static RioIns code[4096];

int main(void) {
  const char *src = "score* := 1\nlives := 3\nupdate* :: proc()\n  score += 1\nend\nhelper :: proc()\nend\nmain :: proc()\n  update()\nend\n";
  Rio vm; int fails = 0, marked;
#ifdef RIO_EXPORTS_ALL
  marked = 0;
#else
  marked = 1;
#endif
  rio_init(&vm, mem, sizeof mem, code, 4096);
  if (rio_compile(&vm, src, (uint32_t)strlen(src)) || rio_run(&vm)) { printf("FAIL: %s\n", rio_error(&vm)); return 1; }
  if (rio_func(&vm, "main") < 0 || rio_func(&vm, "update") < 0 || !rio_global(&vm, "score")) { printf("FAIL: main, update* and score* are always found\n"); fails++; }
  if ((rio_func(&vm, "helper") >= 0) == marked || (rio_global(&vm, "lives") != 0) == marked) { printf("FAIL: unmarked names %s be found\n", marked ? "shouldn't" : "should"); fails++; }
  if (rio_call(&vm, rio_func(&vm, "main")) || *(int32_t *)rio_global(&vm, "score") != 2) { printf("FAIL: calling main\n"); fails++; }
  if (!fails) printf("all export tests passed (%s)\n", marked ? "marked names only" : "every name");
  return fails != 0;
}
