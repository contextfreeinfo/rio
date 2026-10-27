/* runtime.c - the host's side of a vm: setting it up, finding procs and globals, and errors. */
#include "internal.h"
#ifndef rio_NO_COMPILER
#include "compiler.h"
#endif

/* ------------------------------------------------------------------ setup */

int rio_init(rio_Vm *vm, void *mem, uint32_t memSize, rio_Instr *code, uint32_t codeCap) {
  uintptr_t pad = (16 - ((uintptr_t)mem & 15)) & 15; /* mem starts 16-byte aligned */
  memset(vm, 0, sizeof *vm);
  if (!mem || memSize < pad + 1024 || (code && codeCap < 16)) return -1;
  vm->mem = (uint8_t *)mem + pad;
  vm->memSize = (uint32_t)(memSize - pad) & ~15u;
  vm->hostCode = vm->code = code;
  vm->hostCodeCap = vm->codeCap = !code ? 0 : codeCap > 65535 ? 65535 : codeCap;
  return 0;
}

void rio_setLog(rio_Vm *vm, rio_LogFn fn, void *userData) {
  vm->logFn = fn;
  vm->logData = userData;
}

void rio_setLoader(rio_Vm *vm, rio_LoadFn fn, void *userData) {
  vm->loader = fn;
  vm->loaderData = userData;
}

int rio_addFfi(rio_Vm *vm, const char *name, const char *sig, rio_FfiFn fn) {
  rio_Ffi *f;
  if (vm->ffiCount >= rio_MAX_FFI) return -1;
  f = &vm->ffi[vm->ffiCount++];
  f->name = name;
  f->sig = sig;
  f->fn = fn;
  f->argWords = f->resultWords = f->pad = 0; /* (the compiler reads the sig) */
  return 0;
}

int rio_autoImport(rio_Vm *vm, const char *module) {
  if (vm->autoImportCount >= rio_MAX_AUTO_IMPORTS || !module) return -1;
  vm->autoImports[vm->autoImportCount++] = module;
  return 0;
}

void rio_interrupt(rio_Vm *vm) { vm->interrupted = 1; }

void rio_trap(rio_Vm *vm, const char *message) {
  int n = rio_appendText(vm->errorText, 0, message);
  vm->errorText[n] = 0;
  vm->trapped = 1;
}

/* ------------------------------------------------------------------ errors */

const char *rio_error(rio_Vm *vm) { return vm->errorText; }

rio_Error rio_errorInfo(rio_Vm *vm) {
  rio_Error e;
  e.kind = vm->errorKind;
  e.line = vm->errorLine;
  e.col = vm->errorCol;
  e.len = vm->errorLen;
  e.msg = vm->errorText + vm->errorMsgAt;
  e.file = vm->errorFile;
  return e;
}

/* Decodes the packed line table (see rio_packLines in compile.c) up to pc. It starts where the last
   lookup stopped, unless pc is behind it. */
int rio_pcLine(rio_Vm *vm, uint32_t pc) {
  const uint8_t *table = vm->mem + vm->linesAt;
  uint32_t i, at, line;
#ifndef rio_NO_COMPILER
  if (vm->inRepl && vm->compiler) return (int)rio_compiledLine(vm, pc); /* the table is still in scratch */
#endif
  if (pc < vm->lineCachePc) vm->lineCachePos = vm->lineCachePc = vm->lineCacheLine = 0;
  i = vm->lineCachePos;
  at = vm->lineCachePc;
  line = vm->lineCacheLine;
  while (i < vm->linesSize) {
    uint32_t b = table[i], step, nextLine, n;
    if (b < 0xF0) { /* one byte: a step of 0-14 instructions and -4..+11 lines */
      step = b >> 4;
      nextLine = line + (b & 15) - 4;
      n = 1;
    } else { /* four: 0xF0 | step >> 8, step & 255, then the line, low byte first */
      step = (b & 15) << 8 | table[i + 1];
      nextLine = (uint32_t)table[i + 2] | (uint32_t)table[i + 3] << 8;
      n = 4;
    }
    if (at + step > pc) break;
    at += step;
    line = nextLine;
    i += n;
  }
  vm->lineCachePos = i;
  vm->lineCachePc = at;
  vm->lineCacheLine = line;
  return (int)line;
}

/* ------------------------------------------------------------------ lookups */

static rio_Export *rio_findExport(rio_Vm *vm, const char *name, int kind) {
  rio_Export *exports = (rio_Export *)(void *)(vm->mem + vm->exportsAt);
  uint32_t i, n = (uint32_t)strlen(name);
  if (!vm->ready) return 0;
  for (i = vm->exportCount; i-- > 0;) {
    rio_Export *x = &exports[i];
    if (x->kind == kind && x->nameLen == n && !memcmp(vm->mem + x->nameAt, name, n)) return x;
  }
  return 0;
}

int rio_findProc(rio_Vm *vm, const char *name) {
  rio_Export *x;
#ifndef rio_NO_COMPILER
  if (vm->inRepl && vm->compiler) return rio_replLookup(vm, name, rio_SymKind_proc);
#endif
  x = rio_findExport(vm, name, rio_SymKind_proc);
  return x ? x->value : -1;
}

void *rio_findGlobal(rio_Vm *vm, const char *name) {
  rio_Export *x;
#ifndef rio_NO_COMPILER
  if (vm->inRepl && vm->compiler) {
    int addr = rio_replLookup(vm, name, rio_SymKind_var);
    return addr >= 0 ? vm->mem + addr : 0;
  }
#endif
  x = rio_findExport(vm, name, rio_SymKind_var);
  return x ? vm->mem + x->value : 0;
}

rio_Val *rio_procArgs(rio_Vm *vm, int proc) { return (rio_Val *)(void *)vm->mem + vm->procs[proc].frameStart; }

rio_Val *rio_procResult(rio_Vm *vm, int proc) { return (rio_Val *)(void *)vm->mem + vm->procs[proc].result; }

void *rio_sliceBytes(rio_Vm *vm, const rio_Val *slice) {
  uint32_t at = (uint32_t)slice[0].i, len = (uint32_t)slice[1].i;
  return at <= vm->memSize && len <= vm->memSize - at ? vm->mem + slice[0].i : 0;
}
