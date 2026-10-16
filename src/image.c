/* image.c - a compiled program as bytes, and loading one into a vm (which needs no compiler).

   The layout: a header of 32-bit words (rio_ImageHeader below), then each ffi function's name and sig
   (0-terminated), padding to 4, the code (unless it lives in mem), the constants, and the finalized
   block [top, end): the code when it lives in mem (header word code: its address), strings, exports,
   lines, procs, modules. All other memory starts as zero. Little-endian, as in mem. */
#include "internal.h"

typedef enum {
  rio_Image_magic, rio_Image_version, rio_Image_memSize, rio_Image_codeLen, rio_Image_constCount,
  rio_Image_constCap, rio_Image_callStackAt, rio_Image_top, rio_Image_end, rio_Image_exportsAt,
  rio_Image_exportCount, rio_Image_linesAt, rio_Image_linesSize, rio_Image_modulesAt, rio_Image_moduleCount,
  rio_Image_procCount, rio_Image_procsAt, rio_Image_ffiCount, rio_Image_codeAt,
  rio_Image_headerWords
} rio_ImageHeader;
#define rio_IMAGE_VERSION 2
#define rio_IMAGE_MAGIC 0x494F4952u /* "RIOI" */

static int rio_littleEndian(void) {
  uint32_t one = 1;
  return *(uint8_t *)&one == 1;
}

/* ------------------------------------------------------------------ saving */

#ifndef rio_NO_COMPILER
static uint32_t rio_blockEnd(rio_Vm *vm) { return vm->modulesAt + vm->moduleCount * (uint32_t)sizeof(rio_ModuleCode); }

/* where the code is in mem, or 0: in the host's buffer */
static uint32_t rio_codeAt(rio_Vm *vm) {
  uint8_t *p = (uint8_t *)vm->code;
  return p >= vm->mem && p < vm->mem + vm->memSize ? (uint32_t)(p - vm->mem) : 0;
}

uint32_t rio_saveImage(rio_Vm *vm, void *out, uint32_t cap) {
  uint32_t h[rio_Image_headerWords], size = rio_Image_headerWords * 4, i;
  uint32_t end = rio_blockEnd(vm), codeAt = rio_codeAt(vm), codeLen = codeAt ? 0 : vm->codeLen; /* codeLen: apart */
  uint8_t *o = (uint8_t *)out;
  if (!vm->ready || vm->inRepl || vm->running || vm->jitFns || !rio_littleEndian()) return 0; /* (jit: native parts) */
  for (i = vm->constCap * 4; i < vm->top; i++)
    if (vm->mem[i]) return 0; /* it has run: memory isn't fresh */
  for (i = 0; i < (uint32_t)vm->ffiCount; i++) size += (uint32_t)(strlen(vm->ffi[i].name) + strlen(vm->ffi[i].sig) + 2);
  size = rio_align4(size);
  size += codeLen * (uint32_t)sizeof(rio_Instr) + vm->constCap * 4 + (end - vm->top);
  if (!out) return size;
  if (cap < size) return 0;
  h[rio_Image_magic] = rio_IMAGE_MAGIC;
  h[rio_Image_version] = rio_IMAGE_VERSION;
  h[rio_Image_codeAt] = codeAt;
  h[rio_Image_memSize] = vm->memSize;
  h[rio_Image_codeLen] = vm->codeLen;
  h[rio_Image_constCount] = vm->constCount;
  h[rio_Image_constCap] = vm->constCap;
  h[rio_Image_callStackAt] = vm->callStackAt;
  h[rio_Image_top] = vm->top;
  h[rio_Image_end] = end;
  h[rio_Image_exportsAt] = vm->exportsAt;
  h[rio_Image_exportCount] = vm->exportCount;
  h[rio_Image_linesAt] = vm->linesAt;
  h[rio_Image_linesSize] = vm->linesSize;
  h[rio_Image_modulesAt] = vm->modulesAt;
  h[rio_Image_moduleCount] = vm->moduleCount;
  h[rio_Image_procCount] = (uint32_t)vm->procCount;
  h[rio_Image_procsAt] = (uint32_t)((uint8_t *)vm->procs - vm->mem);
  h[rio_Image_ffiCount] = (uint32_t)vm->ffiCount;
  memcpy(o, h, sizeof h);
  o += sizeof h;
  for (i = 0; i < (uint32_t)vm->ffiCount; i++) {
    size_t nameSize = strlen(vm->ffi[i].name) + 1, sigSize = strlen(vm->ffi[i].sig) + 1;
    memcpy(o, vm->ffi[i].name, nameSize);
    o += nameSize;
    memcpy(o, vm->ffi[i].sig, sigSize);
    o += sigSize;
  }
  while ((o - (uint8_t *)out) & 3) *o++ = 0;
  memcpy(o, vm->code, codeLen * sizeof(rio_Instr));
  o += codeLen * sizeof(rio_Instr);
  memcpy(o, vm->mem, vm->constCap * 4);
  o += vm->constCap * 4;
  memcpy(o, vm->mem + vm->top, end - vm->top);
  return size;
}
#endif

/* ------------------------------------------------------------------ loading */

static int rio_imageError(rio_Vm *vm, const char *message) {
  char text[rio_ERROR_BUF_SIZE];
  int n = rio_appendText(text, 0, "image: ");
  n = rio_appendText(text, n, message);
  text[n] = 0;
  rio_setError(vm, rio_ErrorKind_compile, 0, 0, 0, text);
  return -1;
}

/* do the header's sizes and addresses fit together, and in the memory it asks for? */
static int rio_headerOk(const uint32_t *h) {
  uint32_t mem = h[rio_Image_memSize], top = h[rio_Image_top], end = h[rio_Image_end];
  uint32_t procs = h[rio_Image_procsAt], modules = h[rio_Image_modulesAt], exports = h[rio_Image_exportsAt];
  uint32_t lines = h[rio_Image_linesAt], code = h[rio_Image_codeAt];
  return !(h[rio_Image_constCap] > mem / 4 || h[rio_Image_constCount] > h[rio_Image_constCap] ||
           top < h[rio_Image_constCap] * 4 || end < top || end > mem || h[rio_Image_callStackAt] > top ||
           h[rio_Image_procCount] > 65535 || h[rio_Image_moduleCount] > 65535 ||
           h[rio_Image_exportCount] > mem / sizeof(rio_Export) ||
           procs < top || procs + h[rio_Image_procCount] * sizeof(rio_Proc) > end ||
           modules < top || modules + h[rio_Image_moduleCount] * sizeof(rio_ModuleCode) != end ||
           exports < top || exports + h[rio_Image_exportCount] * sizeof(rio_Export) > end ||
           lines < top || lines + h[rio_Image_linesSize] > end || (procs & 1) ||
           (code && (code < top || (code & 7) || code + h[rio_Image_codeLen] * sizeof(rio_Instr) > end)));
}

int rio_loadImage(rio_Vm *vm, const void *image, uint32_t len) {
  const uint8_t *p = (const uint8_t *)image, *q, *e = p + len;
  uint32_t h[rio_Image_headerWords], i, j, memSize, blockSize, codeLen;
  const rio_Instr *code;
  rio_Ffi ffi[rio_MAX_FFI];
  vm->ready = 0;
  vm->inRepl = 0;
  vm->compiler = 0;
  vm->errorKind = rio_ErrorKind_none;
  vm->errorText[0] = 0;
  vm->jitFns = 0;
  vm->jitUsed = vm->jitCount = 0;
  if (vm->running) return rio_imageError(vm, "rio is running");
  if (!rio_littleEndian()) return rio_imageError(vm, "needs a little-endian host");
  if (len < sizeof h) return rio_imageError(vm, "too short");
  memcpy(h, p, sizeof h);
  q = p + sizeof h;
  if (h[rio_Image_magic] != rio_IMAGE_MAGIC) return rio_imageError(vm, "not a rio image");
  if (h[rio_Image_version] != rio_IMAGE_VERSION) return rio_imageError(vm, "unsupported version");
  memSize = h[rio_Image_memSize];
  if (memSize > vm->memSize) return rio_imageError(vm, "needs more memory");
  if (!h[rio_Image_codeAt] && h[rio_Image_codeLen] > vm->hostCodeCap)
    return rio_imageError(vm, vm->hostCode ? "code too large" : "its code goes in a separate buffer, and this vm has none");
  if (h[rio_Image_ffiCount] > rio_MAX_FFI) return rio_imageError(vm, "too many ffi functions");
  if (!rio_headerOk(h)) return rio_imageError(vm, "bad header");

  for (i = 0; i < h[rio_Image_ffiCount]; i++) { /* bind by name, in the image's order */
    const char *name = (const char *)q, *sig;
    size_t n;
    for (n = 0; q + n < e && q[n]; n++) {}
    if (q + n + 1 >= e) return rio_imageError(vm, "bad ffi table");
    sig = name + n + 1;
    q += n + 1;
    for (n = 0; q + n < e && q[n]; n++) {}
    if (q + n >= e) return rio_imageError(vm, "bad ffi table");
    q += n + 1;
    for (j = 0; j < (uint32_t)vm->ffiCount && strcmp(vm->ffi[j].name, name); j++) {}
    if (j < (uint32_t)vm->ffiCount) {
      if (strcmp(vm->ffi[j].sig, sig)) return rio_imageError(vm, "an ffi function's sig differs from the image's");
      ffi[i] = vm->ffi[j];
    } else { /* fine, unless the code calls it */
      ffi[i].name = "";
      ffi[i].sig = "";
      ffi[i].fn = 0;
      ffi[i].argWords = ffi[i].resultWords = 0;
    }
  }
  q = p + ((q - p + 3) & ~(uintptr_t)3);
  blockSize = h[rio_Image_end] - h[rio_Image_top];
  codeLen = h[rio_Image_codeAt] ? 0 : h[rio_Image_codeLen]; /* the code apart from the block */
  if (q > e || (uint32_t)(e - q) != codeLen * sizeof(rio_Instr) + h[rio_Image_constCap] * 4 + blockSize)
    return rio_imageError(vm, "wrong size");
  if (h[rio_Image_codeAt])
    code = (const rio_Instr *)(const void *)(q + h[rio_Image_constCap] * 4 + (h[rio_Image_codeAt] - h[rio_Image_top]));
  else code = (const rio_Instr *)(const void *)q;
  for (i = 0; i < h[rio_Image_codeLen]; i++) { /* before anything is copied: a failed load leaves the vm as it was */
    rio_Instr in;
    memcpy(&in, code + i, sizeof in);
    if (in.op == rio_Op_ffi && (in.c >= h[rio_Image_ffiCount] || !ffi[in.c].fn))
      return rio_imageError(vm, "the program calls an ffi function the host didn't register");
  }

  if (codeLen) {
    memcpy(vm->hostCode, q, codeLen * sizeof(rio_Instr));
    q += codeLen * sizeof(rio_Instr);
  }
  memcpy(vm->mem, q, h[rio_Image_constCap] * 4);
  q += h[rio_Image_constCap] * 4;
  memset(vm->mem + h[rio_Image_constCap] * 4, 0, h[rio_Image_top] - h[rio_Image_constCap] * 4);
  memcpy(vm->mem + h[rio_Image_top], q, blockSize);
  memset(vm->mem + h[rio_Image_end], 0, vm->memSize - h[rio_Image_end]);
  memcpy(vm->ffi, ffi, h[rio_Image_ffiCount] * sizeof *ffi);
  vm->ffiCount = (int)h[rio_Image_ffiCount];
  vm->memSize = memSize;
  vm->codeLen = h[rio_Image_codeLen];
  vm->constCount = h[rio_Image_constCount];
  vm->constCap = h[rio_Image_constCap];
  vm->callStackAt = h[rio_Image_callStackAt];
  vm->top = h[rio_Image_top];
  vm->exportsAt = h[rio_Image_exportsAt];
  vm->exportCount = h[rio_Image_exportCount];
  vm->linesAt = h[rio_Image_linesAt];
  vm->linesSize = h[rio_Image_linesSize];
  vm->code = h[rio_Image_codeAt] ? (rio_Instr *)(void *)(vm->mem + h[rio_Image_codeAt]) : vm->hostCode;
  vm->codeCap = h[rio_Image_codeLen];
  vm->modulesAt = h[rio_Image_modulesAt];
  vm->moduleCount = h[rio_Image_moduleCount];
  vm->procCount = (int)h[rio_Image_procCount];
  vm->procs = (rio_Proc *)(void *)(vm->mem + h[rio_Image_procsAt]);
  vm->lineCachePos = vm->lineCachePc = vm->lineCacheLine = 0;
  vm->logLen = 0;
  vm->trapped = 0;
  vm->ready = 1;
  if (vm->jitHook) vm->jitHook(vm);
  return 0;
}
