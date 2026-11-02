/* os.h - module os's host procs (os.rio declares them), in C, and the line editor they and the REPL
   share. Not declarations: code, included once, in two places. The rio command includes it (main.c),
   and a program's C (rio -c) has it too, from os_text.h, when the program imports os. The includer has
   rio_Val and defines rio_OS_MEM and rio_OS_MEM_SIZE, the program's memory (the VM's, or C output's
   rio_M), and calls rio_osStart first, with the program's arguments. Each proc is rio_ffi_os_<name>(a),
   as C output calls it: its arguments in a, its result put back in a[0] (and a[1]). */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <direct.h>
#include <io.h>
#else
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#endif

static int rio_osArgc;
static char **rio_osArgv;
static double rio_osStarted;

static double rio_osNow(void) { /* seconds, by a clock that only goes forward */
#ifdef _WIN32
  LARGE_INTEGER t, f;
  QueryPerformanceCounter(&t);
  QueryPerformanceFrequency(&f);
  return (double)t.QuadPart / (double)f.QuadPart;
#else
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (double)t.tv_sec + t.tv_nsec / 1e9;
#endif
}

/* the program's arguments (after its file's), and when it started */
static void rio_osStart(int argc, char **argv) {
  rio_osArgc = argc < 0 ? 0 : argc;
  rio_osArgv = argv;
  rio_osStarted = rio_osNow();
}

/* ------------------------------------------------------------------ arguments in memory */

/* a String or Blob argument (two words: where, how long) as bytes; 0 if it isn't all in memory */
static uint8_t *rio_osBytes(const rio_Val *a) {
  uint32_t at = a[0].u, n = a[1].u, size = (uint32_t)(rio_OS_MEM_SIZE);
  return at <= size && n <= size - at ? (uint8_t *)(rio_OS_MEM) + at : 0;
}

/* a String argument as C text (a path, a name, a command), in buf; 0 if it doesn't fit there */
static const char *rio_osText(const rio_Val *a, char *buf, size_t cap) {
  const uint8_t *s = rio_osBytes(a);
  if (!s || a[1].u >= cap) return 0;
  memcpy(buf, s, a[1].u);
  buf[a[1].u] = 0;
  return buf;
}

/* n bytes into a Blob argument, as many as fit; returns n */
static int32_t rio_osPut(const rio_Val *out, const void *s, size_t n) {
  uint8_t *o = rio_osBytes(out);
  if (o && n) memcpy(o, s, n < out[1].u ? n : out[1].u);
  return (int32_t)n;
}

#define rio_OS_PATH 4096
static char rio_osPath[rio_OS_PATH], rio_osPath2[rio_OS_PATH];

/* ------------------------------------------------------------------ the line editor */

/* A line typed at a terminal, edited as it's typed: the terminal raw while it is (each key as it's
   pressed, and no echo: the line is drawn again after each one), the line scrolling sideways when it's
   wider than the terminal. rio_osLineKey is the editing, a byte at a time (keys come as a terminal
   sends them: ESC [ sequences for the others), apart from the terminal, so tests can type at it. */
#define rio_LINE 4096
#define rio_HISTORY 256
#define rio_OFFERS 8192 /* Tab's completions: their names and details (rio_osComplete) */
typedef struct {
  char buf[rio_LINE], draft[rio_LINE], kill[rio_LINE];
  int len, pos, draftLen, killLen;
  int hist;             /* the history line shown (rio_osHistCount: the one being typed) */
  int esc, escN, esc0, esc1; /* an escape sequence coming: 1 after ESC, 2 in ESC [ (its numbers), 3 ESC O */
  int clear;            /* Ctrl+L: clear the screen first */
  int list;             /* Tab, with nothing it could add: show what fits (rio_osOffers) */
} rio_OsLine;
static char rio_osHist[rio_HISTORY][rio_LINE / 4];
static int rio_osHistCount;

/* a line remembered, for Up and Down (not an empty one, or the same as the last) */
static void rio_osHistoryAdd(const char *s, int n) {
  if (n <= 0 || n >= (int)sizeof rio_osHist[0]) return;
  if (rio_osHistCount && (int)strlen(rio_osHist[rio_osHistCount - 1]) == n && !memcmp(rio_osHist[rio_osHistCount - 1], s, (size_t)n)) return;
  if (rio_osHistCount == rio_HISTORY) {
    memmove(rio_osHist[0], rio_osHist[1], sizeof rio_osHist[0] * (rio_HISTORY - 1));
    rio_osHistCount--;
  }
  memcpy(rio_osHist[rio_osHistCount], s, (size_t)n);
  rio_osHist[rio_osHistCount++][n] = 0;
}

static int rio_osWordChar(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c >= 128; }
static int rio_osCont(int c) { return (c & 0xC0) == 0x80; } /* (a UTF-8 character's later bytes) */

static int rio_osLeft(const rio_OsLine *e, int p) { /* a character back */
  if (p > 0) p--;
  while (p > 0 && rio_osCont((unsigned char)e->buf[p])) p--;
  return p;
}
static int rio_osRight(const rio_OsLine *e, int p) {
  if (p < e->len) p++;
  while (p < e->len && rio_osCont((unsigned char)e->buf[p])) p++;
  return p;
}
static int rio_osWordLeft(const rio_OsLine *e, int p) {
  while (p > 0 && !rio_osWordChar((unsigned char)e->buf[p - 1])) p--;
  while (p > 0 && rio_osWordChar((unsigned char)e->buf[p - 1])) p--;
  return p;
}
static int rio_osWordRight(const rio_OsLine *e, int p) {
  while (p < e->len && !rio_osWordChar((unsigned char)e->buf[p])) p++;
  while (p < e->len && rio_osWordChar((unsigned char)e->buf[p])) p++;
  return p;
}

/* buf[a..b) taken out (kept, for Ctrl+Y, when kill) */
static void rio_osCut(rio_OsLine *e, int a, int b, int kill) {
  if (b <= a) return;
  if (kill) {
    memcpy(e->kill, e->buf + a, (size_t)(b - a));
    e->killLen = b - a;
  }
  memmove(e->buf + a, e->buf + b, (size_t)(e->len - b));
  e->len -= b - a;
  e->pos = a;
}
static void rio_osInsert(rio_OsLine *e, const char *s, int n) {
  if (n <= 0 || e->len + n >= rio_LINE) return;
  memmove(e->buf + e->pos + n, e->buf + e->pos, (size_t)(e->len - e->pos));
  memcpy(e->buf + e->pos, s, (size_t)n);
  e->len += n;
  e->pos += n;
}

/* another history line in place of the one shown (step: -1 older, 1 newer) */
static void rio_osHistoryStep(rio_OsLine *e, int step) {
  int to = e->hist + step;
  if (to < 0 || to > rio_osHistCount) return;
  if (e->hist == rio_osHistCount) { /* (leaving the line being typed: kept) */
    memcpy(e->draft, e->buf, (size_t)e->len);
    e->draftLen = e->len;
  }
  e->hist = to;
  if (to == rio_osHistCount) {
    memcpy(e->buf, e->draft, (size_t)e->draftLen);
    e->len = e->draftLen;
  } else {
    e->len = (int)strlen(rio_osHist[to]);
    memcpy(e->buf, rio_osHist[to], (size_t)e->len);
  }
  e->pos = e->len;
}

/* Tab's completions for the word before the cursor (main.c sets this, for the REPL: rio_complete's):
   into out (cap bytes), the names that fit, each 0-terminated, with its detail (a type) after it,
   0-terminated too; returns how many, and where the word starts in s (*from). 0, as for a program's
   os.readLine: Tab just indents */
static int (*rio_osComplete)(const char *s, int n, int at, char *out, int cap, int *from);
static char rio_osOffers[rio_OFFERS];
static int rio_osOfferCount;

/* Tab: at a line's indent, a step more; after a word, the rest of it, as far as what fits there agrees
   (all of it, if just one name fits); when that adds nothing, the names that fit are shown */
static void rio_osTab(rio_OsLine *e) {
  const char *s = rio_osOffers, *first = rio_osOffers;
  int i, from = e->pos, common = 0, word;
  for (i = 0; i < e->pos && (e->buf[i] == ' ' || e->buf[i] == '\t'); i++) {}
  if (!rio_osComplete || i == e->pos) {
    rio_osInsert(e, "  ", 2);
    return;
  }
  rio_osOfferCount = rio_osComplete(e->buf, e->len, e->pos, rio_osOffers, (int)sizeof rio_osOffers, &from);
  if (rio_osOfferCount <= 0 || from < 0 || from > e->pos) return;
  for (i = 0; i < rio_osOfferCount; i++) { /* (the longest start they all have) */
    int n = (int)strlen(s), m;
    for (m = 0; i && m < common && m < n && first[m] == s[m]; m++) {}
    common = i ? m : n;
    s += n + 1;
    s += strlen(s) + 1;
  }
  word = e->pos - from;
  if (common > word) rio_osInsert(e, first + word, common - word);
  else if (rio_osOfferCount > 1) e->list = 1;
}

/* the line, ready for another */
static void rio_osLineBegin(rio_OsLine *e) {
  e->len = e->pos = e->draftLen = e->esc = e->clear = e->list = 0;
  e->hist = rio_osHistCount;
}

/* a key (a byte) typed: 0 the line goes on, 1 it's done (Enter), -1 the end of the input (Ctrl+D on an
   empty line), -2 interrupted (Ctrl+C) */
static int rio_osLineKey(rio_OsLine *e, int c) {
  if (e->esc == 1) { /* after ESC: [ or O starts a sequence; else Alt with a key */
    e->esc = 0;
    if (c == '[') {
      e->esc = 2;
      e->escN = e->esc0 = e->esc1 = 0;
      return 0;
    }
    if (c == 'O') {
      e->esc = 3;
      return 0;
    }
    if (c == 'b') e->pos = rio_osWordLeft(e, e->pos);
    else if (c == 'f') e->pos = rio_osWordRight(e, e->pos);
    else if (c == 'd') rio_osCut(e, e->pos, rio_osWordRight(e, e->pos), 1);
    else if (c == 127 || c == 8) rio_osCut(e, rio_osWordLeft(e, e->pos), e->pos, 1);
    return 0;
  }
  if (e->esc == 2 && ((c >= '0' && c <= '9') || c == ';')) { /* ESC [ 1 ; 5 C: its numbers */
    if (c == ';') e->escN++;
    else if (e->escN == 0) e->esc0 = e->esc0 * 10 + c - '0';
    else if (e->escN == 1) e->esc1 = e->esc1 * 10 + c - '0';
    return 0;
  }
  if (e->esc >= 2) { /* its last character: which key, and (ESC [ 1 ; m) with what held */
    int ctrl = e->esc1 > 1 && ((e->esc1 - 1) & 6);
    e->esc = 0;
    if (c == 'A') rio_osHistoryStep(e, -1);
    else if (c == 'B') rio_osHistoryStep(e, 1);
    else if (c == 'C') e->pos = ctrl ? rio_osWordRight(e, e->pos) : rio_osRight(e, e->pos);
    else if (c == 'D') e->pos = ctrl ? rio_osWordLeft(e, e->pos) : rio_osLeft(e, e->pos);
    else if (c == 'H') e->pos = 0;
    else if (c == 'F') e->pos = e->len;
    else if (c == '~' && (e->esc0 == 1 || e->esc0 == 7)) e->pos = 0;
    else if (c == '~' && (e->esc0 == 4 || e->esc0 == 8)) e->pos = e->len;
    else if (c == '~' && e->esc0 == 3) rio_osCut(e, e->pos, ctrl ? rio_osWordRight(e, e->pos) : rio_osRight(e, e->pos), ctrl);
    return 0;
  }
  switch (c) {
  case 27: e->esc = 1; return 0;
  case '\r': case '\n': return 1;
  case 3: return -2;                                                 /* Ctrl+C */
  case 4:                                                            /* Ctrl+D: the end, on an empty line */
    if (!e->len) return -1;
    rio_osCut(e, e->pos, rio_osRight(e, e->pos), 0);
    return 0;
  case 1: e->pos = 0; return 0;                                      /* Ctrl+A */
  case 5: e->pos = e->len; return 0;                                 /* Ctrl+E */
  case 2: e->pos = rio_osLeft(e, e->pos); return 0;                  /* Ctrl+B */
  case 6: e->pos = rio_osRight(e, e->pos); return 0;                 /* Ctrl+F */
  case 8: case 127: rio_osCut(e, rio_osLeft(e, e->pos), e->pos, 0); return 0; /* Backspace */
  case 11: rio_osCut(e, e->pos, e->len, 1); return 0;                /* Ctrl+K */
  case 21: rio_osCut(e, 0, e->pos, 1); return 0;                     /* Ctrl+U */
  case 23: rio_osCut(e, rio_osWordLeft(e, e->pos), e->pos, 1); return 0; /* Ctrl+W */
  case 25: rio_osInsert(e, e->kill, e->killLen); return 0;           /* Ctrl+Y */
  case 16: rio_osHistoryStep(e, -1); return 0;                       /* Ctrl+P */
  case 14: rio_osHistoryStep(e, 1); return 0;                        /* Ctrl+N */
  case 12: e->clear = 1; return 0;                                   /* Ctrl+L */
  case 9: rio_osTab(e); return 0;                                    /* Tab: indent, or complete the word */
  default: {
    char ch = (char)c;
    if (c >= 32 || c < 0) rio_osInsert(e, &ch, 1);
    return 0;
  }
  }
}

/* ------------------------------------------------------------------ the terminal */

#ifdef _WIN32
static HANDLE rio_osIn, rio_osOut;
static DWORD rio_osInMode, rio_osOutMode;
static UINT rio_osInCp, rio_osOutCp;
#else
static struct termios rio_osTerm;
#endif

static int rio_osIsTerminal(void) {
#ifdef _WIN32
  return _isatty(_fileno(stdin)) && _isatty(_fileno(stdout));
#else
  return isatty(0) && isatty(1);
#endif
}

/* raw (1) or as it was (0) */
static int rio_osRaw(int on) {
#ifdef _WIN32
  if (on) {
    rio_osIn = GetStdHandle(STD_INPUT_HANDLE);
    rio_osOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleMode(rio_osIn, &rio_osInMode) || !GetConsoleMode(rio_osOut, &rio_osOutMode)) return -1;
    rio_osInCp = GetConsoleCP();
    rio_osOutCp = GetConsoleOutputCP();
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleMode(rio_osOut, rio_osOutMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);
    return SetConsoleMode(rio_osIn, ENABLE_VIRTUAL_TERMINAL_INPUT) ? 0 : -1;
  }
  SetConsoleMode(rio_osIn, rio_osInMode);
  SetConsoleMode(rio_osOut, rio_osOutMode);
  SetConsoleCP(rio_osInCp);
  SetConsoleOutputCP(rio_osOutCp);
  return 0;
#else
  struct termios raw;
  if (!on) return tcsetattr(0, TCSAFLUSH, &rio_osTerm);
  if (tcgetattr(0, &rio_osTerm)) return -1;
  raw = rio_osTerm;
  raw.c_iflag &= ~(tcflag_t)(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_cflag |= CS8;
  raw.c_lflag &= ~(tcflag_t)(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  return tcsetattr(0, TCSAFLUSH, &raw);
#endif
}

static int rio_osReadByte(void) { /* -1 at the end */
#ifdef _WIN32
  unsigned char c;
  DWORD got = 0;
  if (!ReadFile(rio_osIn, &c, 1, &got, 0) || !got) return -1;
  return c;
#else
  unsigned char c;
  ssize_t got;
  do got = read(0, &c, 1);
  while (got < 0 && errno == EINTR);
  return got == 1 ? c : -1;
#endif
}

static int rio_osColumns(void) {
#ifdef _WIN32
  CONSOLE_SCREEN_BUFFER_INFO info;
  if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) return info.srWindow.Right - info.srWindow.Left + 1;
#else
  struct winsize w;
  if (!ioctl(1, TIOCGWINSZ, &w) && w.ws_col) return w.ws_col;
#endif
  return 80;
}

/* a line's colors, as the REPL has them (main.c sets this: rio_highlight's kinds, a byte each, into
   kinds); 0, as for a program's os.readLine: none. The kinds' colors, as ANSI codes: code, keyword,
   type, call, number, string, comment, punctuation (rio.h's rio_Hl_*) */
static void (*rio_osColors)(const char *s, int n, unsigned char *kinds);
static const char *const rio_osSgr[] = {"0", "35", "36", "34", "33", "32", "90", "0"};

static int rio_osWidth(const char *s, int n) { /* columns: a character each */
  int w = 0, i;
  for (i = 0; i < n; i++) w += !rio_osCont((unsigned char)s[i]);
  return w;
}

/* the line drawn again: the prompt, then as much of the text as fits beside it, scrolled sideways so
   the cursor shows */
static void rio_osLineDraw(const rio_OsLine *e, const char *prompt, int promptLen) {
  static char out[rio_LINE * 8 + 64];
  static unsigned char kinds[rio_LINE];
  int cols = rio_osColumns(), room = cols - rio_osWidth(prompt, promptLen) - 1, from = 0, to, n = 0;
  if (room < 8) room = 8;
  while (rio_osWidth(e->buf + from, e->pos - from) > room) from = rio_osRight(e, from); /* (the cursor in view) */
  to = from;
  while (to < e->len && rio_osWidth(e->buf + from, rio_osRight(e, to) - from) <= room) to = rio_osRight(e, to);
  if (e->clear) n += sprintf(out + n, "\x1b[H\x1b[2J");
  out[n++] = '\r';
  memcpy(out + n, prompt, (size_t)promptLen);
  n += promptLen;
  if (rio_osColors && e->len) { /* (the text in its colors: a code where the kind changes) */
    int i, kind = -1;
    rio_osColors(e->buf, e->len, kinds);
    for (i = from; i < to; i++) {
      if (kinds[i] != kind) {
        kind = kinds[i] < sizeof rio_osSgr / sizeof rio_osSgr[0] ? kinds[i] : 0;
        n += sprintf(out + n, "\x1b[%sm", rio_osSgr[kind]);
      }
      out[n++] = e->buf[i];
    }
    n += sprintf(out + n, "\x1b[0m");
  } else {
    memcpy(out + n, e->buf + from, (size_t)(to - from));
    n += to - from;
  }
  n += sprintf(out + n, "\x1b[K\r");
  if (rio_osWidth(prompt, promptLen) + rio_osWidth(e->buf + from, e->pos - from) > 0)
    n += sprintf(out + n, "\x1b[%dC", rio_osWidth(prompt, promptLen) + rio_osWidth(e->buf + from, e->pos - from));
  fwrite(out, 1, (size_t)n, stdout);
  fflush(stdout);
}

static int rio_osLower(int c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }

/* what Tab found fits (rio_osOffers), below the line, in order: a few, a line each with its detail; more,
   in columns (as many as fit) */
static void rio_osShowOffers(void) {
  static const char *names[256], *details[256];
  int count = 0, i, j, wide = 0, cols = rio_osColumns(), across, rows;
  const char *s = rio_osOffers;
  for (i = 0; i < rio_osOfferCount && count < 256; i++) {
    const char *name = s, *t;
    s += strlen(s) + 1;
    for (j = count; j > 0; j--) { /* (sorted as it goes: by name, small letters and capitals together) */
      int a, b, k = 0;
      for (t = names[j - 1]; name[k] && t[k] && rio_osLower(name[k]) == rio_osLower(t[k]); k++) {}
      a = rio_osLower(name[k]);
      b = rio_osLower(t[k]);
      if (a > b || (a == b && strcmp(name, t) >= 0)) break;
      names[j] = names[j - 1];
      details[j] = details[j - 1];
    }
    names[j] = name;
    details[j] = s;
    count++;
    if ((int)strlen(name) > wide) wide = (int)strlen(name);
    s += strlen(s) + 1;
  }
  fputs("\r\n", stdout);
  if (count <= 12) {
    for (i = 0; i < count; i++)
      printf(rio_osColors && *details[i] ? "  %-*s  \x1b[90m%s\x1b[0m\r\n" : "  %-*s  %s\r\n", wide, names[i], details[i]);
  } else {
    across = (cols - 2) / (wide + 2);
    if (across < 1) across = 1;
    rows = (count + across - 1) / across;
    for (i = 0; i < rows; i++) {
      for (j = 0; j < across && j * rows + i < count; j++) printf("  %-*s", wide, names[j * rows + i]);
      fputs("\r\n", stdout);
    }
  }
  if (rio_osOfferCount > count) printf("  (and %d more)\r\n", rio_osOfferCount - count);
  fflush(stdout);
}

/* a line typed, after prompt: at a terminal, edited (rio_osLineKey), else read as it comes. Its length
   (into out, 0-terminated, without its line break), -1 at the end of the input, -2 interrupted
   (Ctrl+C) */
static int rio_osLine(const char *prompt, int promptLen, char *out, int cap) {
  static rio_OsLine e;
  int c, done = 0;
  if (!rio_osIsTerminal() || rio_osRaw(1)) { /* (not a terminal: the bytes, to the line's end) */
    int n = 0;
    fwrite(prompt, 1, (size_t)promptLen, stdout);
    fflush(stdout);
    while ((c = getchar()) != EOF && c != '\n')
      if (n < cap - 1) out[n++] = (char)c;
    if (n && out[n - 1] == '\r') n--;
    out[n] = 0;
    return c == EOF && !n ? -1 : n;
  }
  rio_osLineBegin(&e);
  rio_osLineDraw(&e, prompt, promptLen);
  while (!done) {
    if ((c = rio_osReadByte()) < 0) done = -1;
    else done = rio_osLineKey(&e, c);
    if (e.list) { /* (below it: the line drawn again under them) */
      rio_osShowOffers();
      e.list = 0;
    }
    if (!e.esc) rio_osLineDraw(&e, prompt, promptLen);
    e.clear = 0;
  }
  rio_osRaw(0);
  fputs("\r\n", stdout);
  fflush(stdout);
  if (done == -2) return -2;
  if (done == -1 && !e.len) return -1;
  if (e.len >= cap) e.len = cap - 1;
  memcpy(out, e.buf, (size_t)e.len);
  out[e.len] = 0;
  rio_osHistoryAdd(out, e.len);
  return e.len;
}

/* ------------------------------------------------------------------ the program */

void rio_ffi_os_argCount(rio_Val *a) { a[0].i = rio_osArgc; }
void rio_ffi_os_arg(rio_Val *a) {
  int i = a[0].i;
  a[0].i = i < 0 || i >= rio_osArgc ? -1 : rio_osPut(a + 1, rio_osArgv[i], strlen(rio_osArgv[i]));
}
void rio_ffi_os_env(rio_Val *a) {
  const char *name = rio_osText(a, rio_osPath, sizeof rio_osPath), *v = name ? getenv(name) : 0;
  a[0].i = v ? rio_osPut(a + 2, v, strlen(v)) : -1;
}
void rio_ffi_os_envSet(rio_Val *a) {
  const char *name = rio_osText(a, rio_osPath, sizeof rio_osPath), *v = rio_osText(a + 2, rio_osPath2, sizeof rio_osPath2);
#ifdef _WIN32
  a[0].i = name && v && *name && !_putenv_s(name, v) ? 0 : -1;
#else
  a[0].i = name && v && *name && !setenv(name, v, 1) ? 0 : -1;
#endif
}
void rio_ffi_os_exit(rio_Val *a) {
  fflush(stdout);
  fflush(stderr);
  exit(a[0].i);
}

/* ------------------------------------------------------------------ text in and out */

void rio_ffi_os_write(rio_Val *a) {
  const uint8_t *s = rio_osBytes(a);
  if (s) fwrite(s, 1, a[1].u, stdout);
  fflush(stdout);
}
void rio_ffi_os_writeErr(rio_Val *a) {
  const uint8_t *s = rio_osBytes(a);
  fflush(stdout); /* (in order with what's written out) */
  if (s) fwrite(s, 1, a[1].u, stderr);
  fflush(stderr);
}
void rio_ffi_os_readLine(rio_Val *a) {
  static char line[rio_LINE];
  const uint8_t *prompt = rio_osBytes(a);
  int n = rio_osLine(prompt ? (const char *)prompt : "", prompt ? (int)a[1].u : 0, line, sizeof line);
  if (n == -2) raise(SIGINT); /* (Ctrl+C, read as a key: what Ctrl+C does) */
  a[0].i = n < 0 ? -1 : rio_osPut(a + 2, line, (size_t)n);
}
void rio_ffi_os_readAll(rio_Val *a) {
  static char chunk[1 << 14];
  uint8_t *out = rio_osBytes(a);
  size_t n = 0, got;
  while ((got = fread(chunk, 1, sizeof chunk, stdin)) > 0) {
    if (out && n < a[1].u) memcpy(out + n, chunk, got < a[1].u - n ? got : a[1].u - n);
    n += got;
  }
  a[0].i = (int32_t)n;
}

/* ------------------------------------------------------------------ files and folders */

void rio_ffi_os_fileRead(rio_Val *a) {
  static char chunk[1 << 14];
  const char *path = rio_osText(a, rio_osPath, sizeof rio_osPath);
  uint8_t *out = rio_osBytes(a + 2);
  FILE *f = path ? fopen(path, "rb") : 0;
  size_t n = 0, got;
  if (!f) {
    a[0].i = -1;
    return;
  }
  while ((got = fread(chunk, 1, sizeof chunk, f)) > 0) {
    if (out && n < a[3].u) memcpy(out + n, chunk, got < a[3].u - n ? got : a[3].u - n);
    n += got;
  }
  a[0].i = ferror(f) ? -1 : (int32_t)n;
  fclose(f);
}
static int rio_osWriteFile(rio_Val *a, const char *mode) {
  const char *path = rio_osText(a, rio_osPath, sizeof rio_osPath);
  const uint8_t *data = rio_osBytes(a + 2);
  FILE *f = path && data ? fopen(path, mode) : 0;
  int bad;
  if (!f) return -1;
  bad = fwrite(data, 1, a[3].u, f) != a[3].u;
  return fclose(f) || bad ? -1 : 0;
}
void rio_ffi_os_fileWrite(rio_Val *a) { a[0].i = rio_osWriteFile(a, "wb"); }
void rio_ffi_os_fileAppend(rio_Val *a) { a[0].i = rio_osWriteFile(a, "ab"); }

static int rio_osKind(const char *path) { /* 0 nothing, 1 a file, 2 a folder */
  struct stat s;
  if (!path || stat(path, &s)) return 0;
  return (s.st_mode & S_IFMT) == S_IFDIR ? 2 : 1;
}
void rio_ffi_os_fileSize(rio_Val *a) {
  const char *path = rio_osText(a, rio_osPath, sizeof rio_osPath);
  struct stat s;
  a[0].i = path && !stat(path, &s) && (s.st_mode & S_IFMT) != S_IFDIR ? (s.st_size > 0x7fffffff ? 0x7fffffff : (int32_t)s.st_size) : -1;
}
void rio_ffi_os_fileKind(rio_Val *a) { a[0].i = rio_osKind(rio_osText(a, rio_osPath, sizeof rio_osPath)); }
void rio_ffi_os_fileDelete(rio_Val *a) {
  const char *path = rio_osText(a, rio_osPath, sizeof rio_osPath);
  int kind = rio_osKind(path);
#ifdef _WIN32
  a[0].i = kind == 2 ? (_rmdir(path) ? -1 : 0) : kind == 1 ? (remove(path) ? -1 : 0) : -1;
#else
  a[0].i = kind == 2 ? (rmdir(path) ? -1 : 0) : kind == 1 ? (remove(path) ? -1 : 0) : -1;
#endif
}
void rio_ffi_os_fileRename(rio_Val *a) {
  const char *from = rio_osText(a, rio_osPath, sizeof rio_osPath), *to = rio_osText(a + 2, rio_osPath2, sizeof rio_osPath2);
#ifdef _WIN32
  a[0].i = from && to && MoveFileExA(from, to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) ? 0 : -1;
#else
  a[0].i = from && to && !rename(from, to) ? 0 : -1;
#endif
}

/* a folder's entries, sorted, folders first: "d name" and "f name" lines */
#define rio_OS_NAMES (1 << 16)
static char rio_osNames[rio_OS_NAMES];
static int rio_osNameAt[4096], rio_osNameCount;
static int rio_osNameOrder(const void *x, const void *y) {
  const char *a = rio_osNames + *(const int *)x, *b = rio_osNames + *(const int *)y;
  return a[0] != b[0] ? (a[0] == 'd' ? -1 : 1) : strcmp(a + 2, b + 2);
}
static void rio_osName(int dir, const char *name, int *used) { /* (one more, if it fits) */
  size_t n = strlen(name);
  if (!strcmp(name, ".") || !strcmp(name, "..") || rio_osNameCount == 4096 || *used + (int)n + 3 > rio_OS_NAMES) return;
  rio_osNameAt[rio_osNameCount++] = *used;
  rio_osNames[*used] = dir ? 'd' : 'f';
  rio_osNames[*used + 1] = ' ';
  memcpy(rio_osNames + *used + 2, name, n + 1);
  *used += (int)n + 3;
}
void rio_ffi_os_dir(rio_Val *a) {
  const char *path = rio_osText(a, rio_osPath, sizeof rio_osPath);
  int used = 0, i, at = 0;
  uint8_t *out = rio_osBytes(a + 2);
  rio_osNameCount = 0;
  if (!path) {
    a[0].i = -1;
    return;
  }
#ifdef _WIN32
  {
    WIN32_FIND_DATAA d;
    HANDLE h;
    snprintf(rio_osPath2, sizeof rio_osPath2, "%s/*", path);
    if ((h = FindFirstFileA(rio_osPath2, &d)) == INVALID_HANDLE_VALUE) {
      a[0].i = -1;
      return;
    }
    do rio_osName((d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0, d.cFileName, &used);
    while (FindNextFileA(h, &d));
    FindClose(h);
  }
#else
  {
    DIR *d = opendir(path);
    struct dirent *ent;
    if (!d) {
      a[0].i = -1;
      return;
    }
    while ((ent = readdir(d))) {
      snprintf(rio_osPath2, sizeof rio_osPath2, "%s/%s", path, ent->d_name);
      rio_osName(rio_osKind(rio_osPath2) == 2, ent->d_name, &used);
    }
    closedir(d);
  }
#endif
  qsort(rio_osNameAt, (size_t)rio_osNameCount, sizeof rio_osNameAt[0], rio_osNameOrder);
  for (i = 0; i < rio_osNameCount; i++) { /* (as many lines as fit) */
    const char *s = rio_osNames + rio_osNameAt[i];
    size_t n = strlen(s);
    if (!out || at + n + 1 > a[3].u) break;
    memcpy(out + at, s, n);
    out[at + n] = '\n';
    at += (int)n + 1;
  }
  a[0].i = rio_osNameCount;
}
void rio_ffi_os_dirMake(rio_Val *a) {
  const char *path = rio_osText(a, rio_osPath, sizeof rio_osPath);
#ifdef _WIN32
  a[0].i = path && !_mkdir(path) ? 0 : -1;
#else
  a[0].i = path && !mkdir(path, 0777) ? 0 : -1;
#endif
}
void rio_ffi_os_cwd(rio_Val *a) {
#ifdef _WIN32
  const char *s = _getcwd(rio_osPath, sizeof rio_osPath);
#else
  const char *s = getcwd(rio_osPath, sizeof rio_osPath);
#endif
  a[0].i = s ? rio_osPut(a, s, strlen(s)) : -1;
}
void rio_ffi_os_cwdSet(rio_Val *a) {
  const char *path = rio_osText(a, rio_osPath, sizeof rio_osPath);
#ifdef _WIN32
  a[0].i = path && !_chdir(path) ? 0 : -1;
#else
  a[0].i = path && !chdir(path) ? 0 : -1;
#endif
}

/* ------------------------------------------------------------------ other programs */

#define rio_OS_ARGS 256
static char rio_osCommand[1 << 15]; /* (the arguments' text, or a command line) */
static char *rio_osArgList[rio_OS_ARGS + 1];

static int rio_osExitCode(int status) { /* system's, pclose's and waitpid's status, as an exit code (-1: none) */
#ifdef _WIN32
  return status;
#else
  return status != -1 && WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
}

/* bytes at [at, at + n) of memory, or 0 */
static uint8_t *rio_osAt(uint32_t at, uint32_t n) {
  uint32_t size = (uint32_t)(rio_OS_MEM_SIZE);
  return at <= size && n <= size - at ? (uint8_t *)(rio_OS_MEM) + at : 0;
}
static uint32_t rio_osWord(const uint8_t *p) {
  uint32_t w;
  memcpy(&w, p, 4);
  return w;
}

/* a []String argument (where its elements start, how many: each a String's two words) into
   rio_osArgList, 0-terminated (their text in rio_osCommand): how many, or 0 if they don't fit */
static int rio_osArgs(const rio_Val *a) {
  uint32_t n = a[1].u, used = 0, i;
  const uint8_t *els = n <= rio_OS_ARGS ? rio_osAt(a[0].u, n * 8) : 0;
  if (!els || !n) return 0;
  for (i = 0; i < n; i++) {
    uint32_t len = rio_osWord(els + i * 8 + 4);
    const uint8_t *s = rio_osAt(rio_osWord(els + i * 8), len);
    if (!s || used + len + 1 > sizeof rio_osCommand) return 0;
    memcpy(rio_osCommand + used, s, len);
    rio_osCommand[used + len] = 0;
    rio_osArgList[i] = rio_osCommand + used;
    used += len + 1;
  }
  rio_osArgList[n] = 0;
  return (int)n;
}

#ifdef _WIN32
/* the arguments as a Windows command line, each quoted as the C runtime reads them back (backslashes
   doubled before a quote, and at the end of a quoted one): 0 if it doesn't fit */
static int rio_osCommandLine(char *out, size_t cap, char **args) {
  size_t n = 0;
  int i;
  for (i = 0; args[i]; i++) {
    const char *s = args[i];
    int plain = *s && !strpbrk(s, " \t\n\v\"");
    if (i && n < cap) out[n++] = ' ';
    if (plain) {
      size_t k = strlen(s);
      if (n + k >= cap) return 0;
      memcpy(out + n, s, k);
      n += k;
      continue;
    }
    if (n < cap) out[n++] = '"';
    for (;; s++) {
      size_t slashes = 0, k;
      while (*s == '\\') s++, slashes++;
      k = !*s ? slashes * 2 : *s == '"' ? slashes * 2 + 1 : slashes;
      if (n + k + 2 >= cap) return 0;
      while (k--) out[n++] = '\\';
      if (!*s) break;
      out[n++] = *s;
    }
    if (n < cap) out[n++] = '"';
  }
  if (n >= cap) return 0;
  out[n] = 0;
  return 1;
}
#endif

/* args run (args[0] the program, found on the PATH), waited for, its standard output into out when
   capture (and its standard error, when errorsToo): its exit code, or -1 if it couldn't run; *wrote how
   much it wrote */
static int rio_osSpawn(char **args, uint8_t *out, uint32_t cap, int capture, int errorsToo, int32_t *wrote) {
  static char chunk[1 << 14];
  size_t n = 0;
  *wrote = 0;
  fflush(stdout);
  fflush(stderr);
#ifdef _WIN32
  {
    static char line[1 << 15];
    SECURITY_ATTRIBUTES sa;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    HANDLE r = 0, w = 0;
    DWORD got, code = 0;
    BOOL ok;
    if (!rio_osCommandLine(line, sizeof line, args)) return -1;
    memset(&sa, 0, sizeof sa);
    sa.nLength = sizeof sa;
    sa.bInheritHandle = TRUE;
    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    if (capture) {
      if (!CreatePipe(&r, &w, &sa, 0)) return -1;
      SetHandleInformation(r, HANDLE_FLAG_INHERIT, 0);
      si.dwFlags = STARTF_USESTDHANDLES;
      si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
      si.hStdOutput = w;
      si.hStdError = errorsToo ? w : GetStdHandle(STD_ERROR_HANDLE);
    }
    ok = CreateProcessA(0, line, 0, 0, TRUE, 0, 0, 0, &si, &pi);
    if (capture) CloseHandle(w);
    if (!ok) {
      if (capture) CloseHandle(r);
      return -1;
    }
    while (capture && ReadFile(r, chunk, sizeof chunk, &got, 0) && got) {
      if (out && n < cap) memcpy(out + n, chunk, got < cap - n ? got : cap - n);
      n += got;
    }
    if (capture) CloseHandle(r);
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    *wrote = (int32_t)n;
    return (int)code;
  }
#else
  {
    int pipeOut[2] = {-1, -1}, failed[2], status = 0, err = 0;
    ssize_t got;
    pid_t pid;
    if (capture && pipe(pipeOut)) return -1;
    if (pipe(failed)) { /* (written to only if exec fails: closed on exec) */
      if (capture) close(pipeOut[0]), close(pipeOut[1]);
      return -1;
    }
    fcntl(failed[1], F_SETFD, FD_CLOEXEC);
    if (!(pid = fork())) {
      if (capture) {
        dup2(pipeOut[1], 1);
        if (errorsToo) dup2(pipeOut[1], 2);
        close(pipeOut[0]);
        close(pipeOut[1]);
      }
      close(failed[0]);
      execvp(args[0], args);
      err = errno;
      if (write(failed[1], &err, sizeof err)) {}
      _exit(127);
    }
    close(failed[1]);
    if (capture) close(pipeOut[1]);
    if (pid < 0) {
      close(failed[0]);
      if (capture) close(pipeOut[0]);
      return -1;
    }
    while (capture && ((got = read(pipeOut[0], chunk, sizeof chunk)) > 0 || (got < 0 && errno == EINTR))) {
      if (got < 0) continue;
      if (out && n < cap) memcpy(out + n, chunk, (size_t)got < cap - n ? (size_t)got : cap - n);
      n += (size_t)got;
    }
    if (capture) close(pipeOut[0]);
    got = read(failed[0], &err, sizeof err);
    close(failed[0]);
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
    *wrote = (int32_t)n;
    return got == (ssize_t)sizeof err ? -1 : rio_osExitCode(status);
  }
#endif
}

/* run(args, options?): options, a RunOptions (two words: where, its size), else zeros: out (a Blob: 2
   words), errorsToo, shell */
void rio_ffi_os_run(rio_Val *a) {
  static char chunk[1 << 14];
  const uint8_t *o = a[3].u >= 16 ? rio_osAt(a[2].u, 16) : 0;
  uint32_t outAt = o ? rio_osWord(o) : 0, cap = o ? rio_osWord(o + 4) : 0;
  int errorsToo = o && rio_osWord(o + 8), shell = o && rio_osWord(o + 12), n = rio_osArgs(a);
  uint8_t *out = cap ? rio_osAt(outAt, cap) : 0;
  int32_t wrote = 0;
  if (!n) {
    a[0].i = -1;
    a[1].i = 0;
    return;
  }
  if (!shell) {
    a[0].i = rio_osSpawn(rio_osArgList, out, out ? cap : 0, out != 0, errorsToo, &wrote);
    a[1].i = wrote;
    return;
  }
  fflush(stdout); /* (a command line, for the shell: args[0]) */
  fflush(stderr);
  if (!out) {
    a[0].i = rio_osExitCode(system(rio_osArgList[0]));
    a[1].i = 0;
    return;
  }
  {
    static char line[sizeof rio_osCommand + 8];
    size_t got, total = 0;
    FILE *p;
    snprintf(line, sizeof line, "%s%s", rio_osArgList[0], errorsToo ? " 2>&1" : "");
#ifdef _WIN32
    p = _popen(line, "rb");
#else
    p = popen(line, "r");
#endif
    if (!p) {
      a[0].i = -1;
      a[1].i = 0;
      return;
    }
    while ((got = fread(chunk, 1, sizeof chunk, p)) > 0) {
      if (total < cap) memcpy(out + total, chunk, got < cap - total ? got : cap - total);
      total += got;
    }
#ifdef _WIN32
    a[0].i = rio_osExitCode(_pclose(p));
#else
    a[0].i = rio_osExitCode(pclose(p));
#endif
    a[1].i = (int32_t)total;
  }
}
/* ------------------------------------------------------------------ time */

void rio_ffi_os_clock(rio_Val *a) { a[0].f = (float)clock() / CLOCKS_PER_SEC; }
void rio_ffi_os_time(rio_Val *a) { a[0].f = (float)(rio_osNow() - rio_osStarted); }
void rio_ffi_os_date(rio_Val *a) {
  char s[32];
  time_t now = time(0);
  struct tm *t = localtime(&now);
  size_t n = t ? strftime(s, sizeof s, "%Y-%m-%d %H:%M:%S", t) : 0;
  a[0].i = rio_osPut(a, s, n);
}
void rio_ffi_os_sleep(rio_Val *a) {
  float s = a[0].f;
  if (!(s > 0)) return;
#ifdef _WIN32
  Sleep((DWORD)(s * 1000.0f));
#else
  {
    struct timespec t;
    t.tv_sec = (time_t)s;
    t.tv_nsec = (long)((s - (float)t.tv_sec) * 1e9f);
    while (nanosleep(&t, &t) && errno == EINTR) {}
  }
#endif
}
