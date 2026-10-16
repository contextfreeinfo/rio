/* repl.c - a REPL session: each piece of source compiles onto what's there, then runs. The compiler's
   state stays in scratch for the whole session, and a piece that fails to compile is rolled back. */
#include "compiler.h"
#ifndef rio_NO_COMPILER

/* After a piece compiles, its strings move into mem (below the arrays) and its procs get runtime
   entries. firstConst, firstProc: the piece's first. */
static void rio_commit(rio_Vm *vm, uint32_t firstConst, int firstProc) {
  rio_Compiler *c = vm->compiler;
  rio_Val *R = (rio_Val *)vm->mem;
  uint32_t i;
  int k;
  if (c->poolUsed) {
    uint32_t size = rio_align4(c->poolUsed), base;
    if (vm->top < size || vm->top - size < c->slotHigh * 4) rio_FAIL(, "out of memory");
    base = vm->top -= size;
    memcpy(vm->mem + base, c->pool, c->poolUsed);
    for (i = firstConst; i < vm->constCount; i++)
      if (rio_bitGet(c->constIsString, i)) {
        R[i].u = base + (R[i].u & 0x7FFFFFFFu);
        rio_bitClear(c->constIsString, i);
      }
    c->poolUsed = 0;
  }
  for (k = firstProc; k < vm->procCount; k++) rio_fillRuntimeProc(vm, &vm->procs[k], k);
}

int rio_replBegin(rio_Vm *vm, void *scratch, uint32_t size, const rio_Limits *lim) {
  rio_Compiler *c;
  uint32_t procsSize, stackSize;
  if (!scratch) {
    rio_setError(vm, rio_ErrorKind_compile, 0, 0, 0, "a REPL needs its own scratch buffer");
    return -1;
  }
  if (rio_startCompiler(vm, scratch, size, lim, 1)) return -1;
  c = vm->compiler;
  memset(vm->mem, 0, vm->memSize);
  rio_setupCompiler(vm);
  /* reserved up front, since later pieces can add procs and code: their runtime table, the return stack,
     and (when the code lives in mem) room for the code */
  procsSize = rio_align4(c->limits.procs * (uint32_t)sizeof(rio_Proc));
  stackSize = (c->limits.procs + 1) * 4;
  if (!vm->hostCode) {
    uint32_t n = lim && lim->code ? lim->code : rio_limitsFor(vm->memSize).code;
    uint32_t codeSize = (n > 65535 ? 65535 : n) * (uint32_t)sizeof(rio_Instr);
    if (!rio_failed(vm) && vm->top < codeSize + procsSize + stackSize + c->slotHigh * 4 + 64) rio_fail(vm, "out of memory");
    if (!rio_failed(vm)) {
      vm->top -= codeSize;
      vm->code = (rio_Instr *)(void *)(vm->mem + vm->top);
      vm->codeCap = codeSize / (uint32_t)sizeof(rio_Instr);
    }
  }
  if (!rio_failed(vm) && vm->top < procsSize + stackSize + c->slotHigh * 4 + 64) rio_fail(vm, "out of memory");
  if (rio_failed(vm)) {
    vm->compiler = 0;
    return -1;
  }
  vm->top -= procsSize;
  vm->procs = (rio_Proc *)(void *)(vm->mem + vm->top);
  vm->top -= stackSize;
  vm->callStackAt = vm->top;
  rio_commit(vm, 1, 0); /* string values from -D defines */
  if (rio_failed(vm)) {
    vm->compiler = 0;
    return -1;
  }
  vm->inRepl = 1;
  vm->ready = 1;
  return 0;
}

/* where a REPL piece started, so a failed one can be undone: all the tables only ever grow */
typedef struct {
  uint32_t codeLen, constCount, top, liveSlots, slotHigh, poolCap, lineCount, lastLine;
  int symCount, typeCount, fieldCount, paramCount, nameBytes, procCount, moduleCount;
} rio_ReplMark;

static void rio_replMark(rio_Vm *vm, rio_ReplMark *m) {
  rio_Compiler *c = vm->compiler;
  m->codeLen = vm->codeLen;
  m->constCount = vm->constCount;
  m->top = vm->top;
  m->procCount = vm->procCount;
  m->moduleCount = c->moduleCount;
  m->liveSlots = c->liveSlots;
  m->slotHigh = c->slotHigh;
  m->poolCap = c->poolCap;
  m->lineCount = c->lineCount;
  m->lastLine = c->lastLine;
  m->symCount = c->symCount;
  m->typeCount = c->typeCount;
  m->fieldCount = c->fieldCount;
  m->paramCount = c->paramCount;
  m->nameBytes = c->nameBytes;
}

/* undoes a piece that didn't compile; rio_ReplResult_more if it was just unfinished */
static int rio_rollback(rio_Vm *vm, const rio_ReplMark *m) {
  rio_Compiler *c = vm->compiler;
  uint32_t i;
  int more = c->cursor >= c->srcEnd && (c->failMessage == rio_msgMissingEnd || c->failMessage == rio_msgUnclosedBracket ||
                                        c->failMessage == rio_msgExpectedExpr || c->failMessage == rio_msgUnterminatedString);
  for (i = m->constCount; i < vm->constCount; i++) rio_bitClear(c->constIsString, i);
  vm->codeLen = m->codeLen;
  vm->constCount = m->constCount;
  vm->top = m->top;
  vm->procCount = m->procCount;
  c->liveSlots = c->freeSlot = m->liveSlots;
  c->slotHigh = m->slotHigh;
  c->poolCap = m->poolCap;
  c->lineCount = m->lineCount;
  c->lastLine = m->lastLine;
  c->poolUsed = 0;
  c->symCount = m->symCount;
  c->typeCount = m->typeCount;
  c->fieldCount = m->fieldCount;
  c->paramCount = m->paramCount;
  c->nameBytes = m->nameBytes;
  c->blockCount = c->valueCount = c->opCount = c->switchCount = 0;
  c->curProc = -1;
  c->targetType = -1;
  c->lineOverride = 0;
  c->moduleCount = m->moduleCount;
  c->resumeCount = 0;
  c->curModule = c->curFile = 1;
  c->exporting = 0;
  c->failed = 0;
  if (more) {
    vm->errorKind = rio_ErrorKind_none;
    vm->errorText[0] = 0;
    return rio_ReplResult_more;
  }
  return rio_ReplResult_error;
}

int rio_replEval(rio_Vm *vm, const char *src, uint32_t len) {
  rio_Compiler *c = vm->compiler;
  rio_ReplMark mark;
  if (!vm->inRepl || !c) {
    rio_setError(vm, rio_ErrorKind_compile, 0, 0, 0, "no REPL session");
    return rio_ReplResult_error;
  }
  vm->errorKind = rio_ErrorKind_none;
  vm->errorText[0] = 0;
  rio_replMark(vm, &mark);
  c->src = c->cursor = c->lineStart = src;
  c->srcEnd = src + len;
  c->line = 1;
  c->prevLine = c->prevCol = c->prevWidth = 0;
  c->lastLine = c->lastLinePc = 0;
  rio_lex(vm, &c->peek);
  rio_next(vm);
  c->lastLabel = vm->codeLen; /* nothing gets fused with the previous piece's last instruction */
  rio_compileAll(vm);
  if (!rio_failed(vm)) rio_emit(vm, rio_Op_halt, 0, 0, 0);
  if (!rio_failed(vm)) rio_commit(vm, mark.constCount, mark.procCount);
  if (rio_failed(vm)) return rio_rollback(vm, &mark);
  if (vm->jitHook) vm->jitHook(vm);
  vm->interrupted = 0;
  return rio_start(vm, mark.codeLen) ? rio_ReplResult_error : rio_ReplResult_ok;
}

/* REPL lookups go to the live compiler tables: the value of the newest name of this kind, or -1 */
int rio_replLookup(rio_Vm *vm, const char *name, int kind) {
  int i = rio_lookup(vm, name, (int)strlen(name));
  return i >= 0 && vm->compiler->syms[i].kind == kind ? vm->compiler->syms[i].value : -1;
}

/* the module file holding pc (the innermost), from the compiler's table */
const char *rio_replFileOfPc(rio_Vm *vm, uint32_t pc) {
  rio_Compiler *c = vm->compiler;
  const char *file = 0;
  uint32_t best = 0x10000, k;
  for (k = 2; k < (uint32_t)c->moduleCount; k++) {
    rio_Module *m = &c->modules[k];
    if (pc >= m->codeStart && pc < m->codeEnd && (uint32_t)(m->codeEnd - m->codeStart) < best) {
      best = m->codeEnd - m->codeStart;
      file = c->names + m->file;
    }
  }
  return file;
}

/* ------------------------------------------------------------------ typing help */

/* How far to indent the next line of an unfinished input: a step for each open block (proc, struct, enum,
   union, if, for, switch, less each end) and each open bracket; -1 inside an open string, whose next line
   shouldn't get spaces it didn't type. Just the text, not a compile: a guide for typing, with else and
   case a step out from their block's insides (see rio_replOutdent). */
static int rio_isWordChar(int c) { return rio_isAlpha(c) || rio_isDigit(c); }

/* is [p, e) the word w, followed by a non-word character or the end? */
static int rio_wordAt(const char *p, const char *e, const char *w) {
  size_t n = strlen(w);
  return (size_t)(e - p) >= n && !memcmp(p, w, n) && (p + n == e || !rio_isWordChar(p[n]));
}

static int rio_isSpaceOrTab(int c) { return c == ' ' || c == '\t'; }

int rio_replIndent(const char *src, uint32_t len) {
  const char *p = src, *e = src + len;
  int depth = 0, inString = 0;
  while (p < e) { /* a line at a time */
    const char *q;
    if (!inString) { /* its first word */
      while (p < e && rio_isSpaceOrTab(*p)) p++;
      for (q = p; q < e && rio_isWordChar(*q); q++) {}
      if (q < e && *q == ':' && (q + 1 >= e || q[1] != ':') && q > p) { /* a label: the loop after it */
        p = q + 1;
        while (p < e && rio_isSpaceOrTab(*p)) p++;
      }
      if (rio_wordAt(p, e, "end")) depth--;
      else if (rio_wordAt(p, e, "if") || rio_wordAt(p, e, "for") || rio_wordAt(p, e, "switch")) depth++;
    }
    for (; p < e && *p != '\n'; p++) {
      if (inString) {
        if (*p == '\\' && p + 1 < e) p++;
        else if (*p == '"') inString = 0;
        continue;
      }
      if (*p == '"') inString = 1;
      else if (*p == '#') {
        while (p < e && *p != '\n') p++;
        break;
      } else if (*p == '(' || *p == '[' || *p == '{') depth++;
      else if (*p == ')' || *p == ']' || *p == '}') depth--;
      else if (*p == ':' && p + 1 < e && p[1] == ':') { /* name :: proc / struct / enum / union */
        const char *w = p + 2;
        while (w < e && rio_isSpaceOrTab(*w)) w++;
        if (rio_wordAt(w, e, "proc") || rio_wordAt(w, e, "struct") || rio_wordAt(w, e, "enum") || rio_wordAt(w, e, "union")) depth++;
        p++;
      }
    }
    if (p < e) p++; /* (the line break) */
  }
  return inString ? -1 : depth < 0 ? 0 : depth;
}

/* whether a line that starts (after its indent) with these len characters, followed by a space or its
   end, goes a step out from where rio_replIndent put it: end, else, case */
int rio_replOutdent(const char *word, uint32_t len) {
  return (len == 3 && !memcmp(word, "end", 3)) || (len == 4 && (!memcmp(word, "else", 4) || !memcmp(word, "case", 4)));
}

#endif
