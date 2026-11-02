/* the line editor's tests (os.h's, for os.readLine and the REPL): keys typed as a terminal sends them,
   and the line they make, and where the cursor is in it ('|') */
#define _CRT_SECURE_NO_WARNINGS
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <string.h>
#include "rio.h"

static uint8_t mem[64];
#define rio_OS_MEM mem
#define rio_OS_MEM_SIZE sizeof mem
#include "os/os.h"

typedef struct {
  const char *keys, *want; /* want: the line, | where the cursor is */
  int result;              /* what the last key says: 1 done, 0 not yet, -1 the end, -2 interrupted */
} Case;

static const Case cases[] = {
  {"hello", "hello|", 0},
  {"hello\r", "hello|", 1},
  {"abc\x1b[D\x1b[DX", "aX|bc", 0},                   /* left, left, then typed */
  {"abc\x01X\x05Y", "XabcY|", 0},                     /* Ctrl+A, Ctrl+E */
  {"abc\x1b[H\x1b[3~", "|bc", 0},                     /* Home, Delete */
  {"abc\x1b[1~\x1b[4~", "abc|", 0},                   /* Home and End, as ESC [ n ~ */
  {"abc\x08\x7f", "a|", 0},                           /* Backspace, both ways */
  {"one two three\x1b[1;5D\x1b[1;5D", "one |two three", 0}, /* Ctrl+Left, by words */
  {"one two\x01\x1b[1;5C", "one| two", 0},            /* Ctrl+Right */
  {"one two\x1b" "b\x1b" "b\x1b" "f", "one| two", 0}, /* Alt+B, Alt+F */
  {"one two\x17", "one |", 0},                        /* Ctrl+W */
  {"one two\x01\x0b", "|", 0},                        /* Ctrl+K */
  {"one two\x1b[D\x1b[D\x15", "|wo", 0},              /* Ctrl+U */
  {"one two\x17\x01\x19", "two|one ", 0},            /* killed, then Ctrl+Y puts it back */
  {"\x04", "|", -1},                                  /* Ctrl+D on an empty line: the end */
  {"ab\x01\x04", "|b", 0},                            /* else it deletes */
  {"ab\x03", "ab|", -2},                              /* Ctrl+C */
  {"\xc3\xa9t\xc3\xa9\x1b[D\x1b[D\x08", "|t\xc3\xa9", 0}, /* a UTF-8 character: moved past, deleted, whole */
  {"x\t", "x  |", 0},                                 /* Tab: two spaces */
};

/* each line typed, and what the editor has */
static int typed(const char *keys, char *line, int *result) {
  static rio_OsLine e;
  const char *k;
  int n;
  rio_osLineBegin(&e);
  rio_osLineIndent(&e); /* (when there's rio_osIndent) */
  *result = 0;
  for (k = keys; *k; k++) *result = rio_osLineKey(&e, (unsigned char)*k);
  memcpy(line, e.buf, (size_t)e.pos);
  line[e.pos] = '|';
  memcpy(line + e.pos + 1, e.buf + e.pos, (size_t)(e.len - e.pos));
  n = e.len + 1;
  line[n] = 0;
  return n;
}

/* Tab's completions, made up: these names, the ones starting with the word before the cursor */
static int offers(const char *s, int n, int at, char *out, int cap, int *from) {
  static const char *const names[] = {"position", "Point", "pointer", "print", "x", 0};
  int i, count = 0, used = 0;
  (void)n;
  *from = at;
  while (*from > 0 && rio_osWordChar((unsigned char)s[*from - 1])) (*from)--;
  for (i = 0; names[i]; i++)
    if (!strncmp(names[i], s + *from, (size_t)(at - *from)) && used + 24 < cap) {
      used += sprintf(out + used, "%s", names[i]) + 1;
      used += sprintf(out + used, "a detail") + 1;
      count++;
    }
  return count;
}

/* indents, made up: depth steps, one less for a line starting end, else or case (as the REPL's) */
static int depth;
static int indent(const char *s, int n) {
  int i = 0, w;
  while (i < n && s[i] == ' ') i++;
  for (w = i; w < n && s[w] >= 'a' && s[w] <= 'z'; w++) {}
  if (depth && (w == n || s[w] == ' ') &&
      ((w - i == 3 && !memcmp(s + i, "end", 3)) || (w - i == 4 && (!memcmp(s + i, "else", 4) || !memcmp(s + i, "case", 4)))))
    return (depth - 1) * 2;
  return depth * 2;
}

int main(void) {
  static char line[rio_LINE + 2];
  int failed = 0, passed = 0, result;
  size_t i;
  rio_osStart(0, 0);
  for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
    typed(cases[i].keys, line, &result);
    if (strcmp(line, cases[i].want) || result != cases[i].result) {
      printf("FAIL case %d: got \"%s\" (%d), want \"%s\" (%d)\n", (int)i, line, result, cases[i].want, cases[i].result);
      failed++;
    } else passed++;
  }
  /* history: Up for the lines before, Down back to the one being typed (kept as it was) */
  rio_osHistoryAdd("first", 5);
  rio_osHistoryAdd("second", 6);
  rio_osHistoryAdd("second", 6); /* (the same again: once) */
  typed("draft\x1b[A", line, &result);
  if (strcmp(line, "second|")) printf("FAIL: Up gives the last line, got \"%s\"\n", line), failed++;
  else passed++;
  typed("draft\x1b[A\x1b[A\x1b[A", line, &result);
  if (strcmp(line, "first|")) printf("FAIL: Up, Up (and no further), got \"%s\"\n", line), failed++;
  else passed++;
  typed("draft\x1b[A\x1b[A\x1b[B\x1b[B", line, &result);
  if (strcmp(line, "draft|")) printf("FAIL: Down, back to the line typed, got \"%s\"\n", line), failed++;
  else passed++;
  { /* Tab: complete the word (as far as what fits agrees), or show what fits; at an indent, indent */
    static const char *const tabs[][2] = {
      {"pos\t", "position|"}, {"pr\t", "print|"}, {"po\t", "po|"}, {"poi\t", "pointer|"}, {"Po\t", "Point|"},
      {"pos)\x1b[D\t", "position|)"}, {"\t", "  |"}, {"  \t", "    |"}, {"a := pos\t", "a := position|"}, {"q\t", "q|"}, {"x\t", "x|"},
    };
    static rio_OsLine e;
    rio_osComplete = offers;
    for (i = 0; i < sizeof tabs / sizeof tabs[0]; i++) {
      typed(tabs[i][0], line, &result);
      if (strcmp(line, tabs[i][1])) printf("FAIL Tab %d: got \"%s\", want \"%s\"\n", (int)i, line, tabs[i][1]), failed++;
      else passed++;
    }
    rio_osLineBegin(&e); /* (nothing to add, and more than one fits: they're to be shown) */
    for (i = 0; i < 3; i++) rio_osLineKey(&e, "po\t"[i]);
    if (!e.list || rio_osOfferCount != 2) printf("FAIL Tab: po should show 2, got %d (%d)\n", rio_osOfferCount, e.list), failed++;
    else passed++;
    rio_osComplete = 0;
  }
  { /* indents: a line starts at its depth's (two spaces a step), end, else and case step out once
       typed, Backspace in the indent goes back a step, and what's pasted, or indented by hand, stays */
    static const struct {
      int depth;
      const char *keys, *want;
    } ins[] = {
      {1, "x", "  x|"}, {0, "x", "x|"}, {2, "", "    |"}, {1, "end\r", "end|"}, {2, "else ", "  else |"},
      {2, "case .x", "  case .x|"}, {1, "endx ", "  endx |"}, {1, "ends\r", "  ends|"}, {1, "\x7f", "|"}, {2, "\x7f", "  |"},
      {2, " \x7f\x7f", "  |"}, {1, "\x7f" "end ", "end |"}, {1, "\t" "end ", "    end |"}, {1, "\x1b[200~end\r", "  end|"},
      {1, "\x1b[201~else ", "else |"}, {1, "x\x7f\x7f", "|"}, {1, "  y\x01" "end", "end|    y"},
    };
    rio_osIndent = indent;
    for (i = 0; i < sizeof ins / sizeof ins[0]; i++) {
      depth = ins[i].depth;
      typed(ins[i].keys, line, &result);
      if (strcmp(line, ins[i].want)) printf("FAIL indent %d: got \"%s\", want \"%s\"\n", (int)i, line, ins[i].want), failed++;
      else passed++;
    }
    rio_osIndent = 0;
    rio_osPasting = 0;
  }
  if (failed) return 1;
  printf("all %d line tests passed\n", passed);
  return 0;
}
