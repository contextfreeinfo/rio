/* image.c - a compiled program as bytes, and loading one into a vm (which needs no compiler).

   The layout: a header of 32-bit words (rio_ImageHeader below), then each ffi function's name and sig
   (0-terminated), each registered host module source it was made with (rio_addSource): its name and its
   hash, 8 hex digits (0-terminated), padding to 4, the code (unless it lives in mem), the constants, and the finalized
   block [top, end): the code when it lives in mem (header word code: its address), strings, exports,
   lines, procs, modules. All other memory starts as zero. Little-endian, as in mem. */
#include "internal.h"

typedef enum {
  rio_Image_magic, rio_Image_version, rio_Image_memSize, rio_Image_codeLen, rio_Image_constCount,
  rio_Image_constCap, rio_Image_callStackAt, rio_Image_top, rio_Image_end, rio_Image_exportsAt,
  rio_Image_exportCount, rio_Image_linesAt, rio_Image_linesSize, rio_Image_modulesAt, rio_Image_moduleCount,
  rio_Image_procCount, rio_Image_procsAt, rio_Image_ffiCount, rio_Image_codeAt, rio_Image_freeAt,
  rio_Image_sourceCount, rio_Image_headerWords
} rio_ImageHeader;
#define rio_IMAGE_VERSION 11 /* (3: logv and pushv, numbered among the others; 4: freeAt; 5: pushf; 6: bpack, geti, seti; 7: bitfields in descriptors; 8: enums 0 to n - 1; 9: forby, ranges printed; 10: bchk, bitfields' value descriptors; 11: host module sources' hashes) */

#define rio_IMAGE_MAGIC 0x494F4952u /* "RIOI" */

/* a hash as 8 hex digits, 0-terminated, into out (9 bytes) */
static rio_NOINLINE void rio_hex8(char *out, uint32_t h) {
  int k;
  for (k = 0; k < 8; k++) out[k] = "0123456789abcdef"[h >> (28 - 4 * k) & 15];
  out[8] = 0;
}

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

/* the header, and the image's whole size; 0 if there's nothing to save */
static rio_NOINLINE uint32_t rio_imageHeader(rio_Vm *vm, uint32_t *h) {
  uint32_t size = rio_Image_headerWords * 4, i;
  uint32_t end = rio_blockEnd(vm), codeAt = rio_codeAt(vm), codeLen = codeAt ? 0 : vm->codeLen; /* codeLen: apart */
  if (!vm->ready || vm->inRepl || vm->running || vm->jitFns || !rio_littleEndian()) return 0; /* (jit: native parts) */
  for (i = vm->constCap * 4; i < vm->top; i++)
    if (vm->mem[i]) return 0; /* it has run: memory isn't fresh */
  for (i = 0; i < (uint32_t)vm->ffiCount; i++) size += (uint32_t)(strlen(vm->ffi[i].name) + strlen(vm->ffi[i].sig) + 2);
  h[rio_Image_sourceCount] = 0;
  for (i = 0; i < (uint32_t)vm->sourceCount; i++)
    if (vm->sourcesUsed >> i & 1) {
      size += (uint32_t)strlen(vm->sources[i].name) + 1 + 9;
      h[rio_Image_sourceCount]++;
    }
  size = rio_align4(size);
  size += codeLen * (uint32_t)sizeof(rio_Instr) + vm->constCap * 4 + (end - vm->top);
  h[rio_Image_magic] = rio_IMAGE_MAGIC;
  h[rio_Image_version] = rio_IMAGE_VERSION;
  h[rio_Image_codeAt] = codeAt;
  h[rio_Image_memSize] = vm->memSize;
  h[rio_Image_codeLen] = vm->codeLen;
  h[rio_Image_constCount] = vm->constCount;
  h[rio_Image_constCap] = vm->constCap;
  h[rio_Image_callStackAt] = vm->callStackAt;
  h[rio_Image_freeAt] = vm->freeAt;
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
  return size;
}

/* the image, in its parts, straight from the vm: no copy of it all at once */
rio_NOINLINE uint32_t rio_writeImage(rio_Vm *vm, rio_WriteFn write, void *userData) { /* (kept apart: rio_saveImage calls it) */
  uint32_t h[rio_Image_headerWords], size = rio_imageHeader(vm, h), at = sizeof h, i;
  static const uint8_t zeros[4] = {0};
  if (!size || write(userData, h, sizeof h)) return 0;
  for (i = 0; i < (uint32_t)vm->ffiCount; i++) {
    uint32_t nameSize = (uint32_t)strlen(vm->ffi[i].name) + 1, sigSize = (uint32_t)strlen(vm->ffi[i].sig) + 1;
    if (write(userData, vm->ffi[i].name, nameSize) || write(userData, vm->ffi[i].sig, sigSize)) return 0;
    at += nameSize + sigSize;
  }
  for (i = 0; i < (uint32_t)vm->sourceCount; i++) { /* (the sources it was made with: name, hash) */
    char hex[9];
    uint32_t nameSize = (uint32_t)strlen(vm->sources[i].name) + 1;
    if (!(vm->sourcesUsed >> i & 1)) continue;
    rio_hex8(hex, vm->sources[i].hash);
    if (write(userData, vm->sources[i].name, nameSize) || write(userData, hex, 9)) return 0;
    at += nameSize + 9;
  }
  if ((at & 3) && write(userData, zeros, 4 - (at & 3))) return 0;
  if (!h[rio_Image_codeAt] && write(userData, vm->code, vm->codeLen * (uint32_t)sizeof(rio_Instr))) return 0;
  if (write(userData, vm->mem, vm->constCap * 4) || write(userData, vm->mem + vm->top, h[rio_Image_end] - vm->top)) return 0;
  return size;
}

typedef struct {
  uint8_t *at;
} rio_ImageOut;
static int rio_toMemory(void *userData, const void *bytes, uint32_t n) {
  rio_ImageOut *o = (rio_ImageOut *)userData;
  memcpy(o->at, bytes, n);
  o->at += n;
  return 0;
}

uint32_t rio_saveImage(rio_Vm *vm, void *out, uint32_t cap) {
  uint32_t h[rio_Image_headerWords], size = rio_imageHeader(vm, h);
  rio_ImageOut o;
  if (!out || !size) return size;
  if (cap < size) return 0;
  o.at = (uint8_t *)out;
  return rio_writeImage(vm, rio_toMemory, &o);
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
           top < h[rio_Image_constCap] * 4 || end < top || end > mem || h[rio_Image_callStackAt] > top || h[rio_Image_freeAt] > top ||
           h[rio_Image_procCount] > 65535 || h[rio_Image_moduleCount] > 65535 ||
           h[rio_Image_exportCount] > mem / sizeof(rio_Export) ||
           procs < top || procs + h[rio_Image_procCount] * sizeof(rio_Proc) > end ||
           modules < top || modules + h[rio_Image_moduleCount] * sizeof(rio_ModuleCode) != end ||
           exports < top || exports + h[rio_Image_exportCount] * sizeof(rio_Export) > end ||
           lines < top || lines + h[rio_Image_linesSize] > end || (procs & 1) ||
           (code && (code < top || (code & 7) || code + h[rio_Image_codeLen] * sizeof(rio_Instr) > end)));
}

/* the start of a load, the same either way; 0, or -1 (the error set) */
static int rio_loadStart(rio_Vm *vm, uint32_t len) {
  vm->ready = 0;
  vm->inRepl = 0;
  vm->compiler = 0;
  vm->errorKind = rio_ErrorKind_none;
  vm->errorText[0] = 0;
  vm->jitFns = 0;
  vm->jitUsed = vm->jitCount = 0;
  if (vm->running) return rio_imageError(vm, "rio is running");
  if (!rio_littleEndian()) return rio_imageError(vm, "needs a little-endian host");
  if (len < rio_Image_headerWords * 4) return rio_imageError(vm, "too short");
  return 0;
}
static int rio_headerChecks(rio_Vm *vm, const uint32_t *h) {
  if (h[rio_Image_magic] != rio_IMAGE_MAGIC) return rio_imageError(vm, "not a rio image");
  if (h[rio_Image_version] != rio_IMAGE_VERSION) return rio_imageError(vm, "unsupported version");
  if (h[rio_Image_memSize] > vm->memSize) return rio_imageError(vm, "needs more memory");
  if (!h[rio_Image_codeAt] && h[rio_Image_codeLen] > vm->hostCodeCap)
    return rio_imageError(vm, vm->hostCode ? "code too large" : "its code goes in a separate buffer, and this vm has none");
  if (h[rio_Image_ffiCount] > rio_MAX_FFI) return rio_imageError(vm, "too many ffi functions");
  if (h[rio_Image_sourceCount] > 16) return rio_imageError(vm, "bad header");
  if (!rio_headerOk(h)) return rio_imageError(vm, "bad header");
  return 0;
}
/* past the sig's part at s: a letter, or a struct's {...} */
static const char *rio_sigPart(const char *s) {
  int depth = 0;
  if (*s == '&') s++; /* (&i, &f, &{...}) */
  if (*s != '{') return s + 1;
  do {
    depth += *s == '{';
    depth -= *s == '}';
    s++;
  } while (*s && depth);
  return s;
}

/* the words a sig's parameters take, and its result ("" if none) */
static int rio_sigWords(const char *sig, const char **result) {
  int n = 0;
  for (; *sig && *sig != '>'; sig = rio_sigPart(sig)) n += *sig == 's' || *sig == 'b' || *sig == '{' || *sig == '&' ? 2 : 1;
  *result = *sig ? sig + 1 : sig;
  return n;
}

/* does an image's sig part [a, ae) fit the host's [h, he): the same, or a struct with fields more at its
   end (which the host reads as zeros: rio_getStruct) */
static int rio_partFits(const char *a, const char *ae, const char *h, const char *he) {
  size_t n = (size_t)(ae - a), m = (size_t)(he - h);
  if (n == m && !memcmp(a, h, n)) return 1;
  if (*a == '&' && *h == '&') return rio_partFits(a + 1, ae, h + 1, he); /* (a place's struct may grow too: the host writes as much as it's given) */
  return *a == '{' && m > n && n > 2 && !memcmp(a, h, n - 1) && h[n - 1] == ',';
}

/* host proc j for an image's, of sig sig: 0 if it won't do; else into *f, with pad set (the host's sig
   may have more parameters at its end: the image passes zeros for them; and a struct's layout more
   fields at its end) */
static int rio_ffiFits(rio_Vm *vm, int j, const char *sig, rio_Ffi *f) {
  const char *host = vm->ffi[j].sig, *hostResult, *result, *a = sig, *h = host;
  int hostWords = rio_sigWords(host, &hostResult), words = rio_sigWords(sig, &result);
  int placed = *result == '{' || *result == 'U'; /* (a struct or union result: where it goes, first, 2 words) */
  for (; *a && *a != '>'; a = rio_sigPart(a), h = rio_sigPart(h)) /* (each of the image's parameters fits the host's) */
    if (!*h || *h == '>' || !rio_partFits(a, rio_sigPart(a), h, rio_sigPart(h))) return 0;
  if (placed ? !(*hostResult == *result && rio_partFits(result, result + strlen(result), hostResult, hostResult + strlen(hostResult)))
             : strcmp(hostResult, result) != 0) /* (a struct result may have fields more on the host: it writes what fits) */
    return 0;
  hostWords += placed * 2;
  words += placed * 2;
  if (*h && *h != '>' && hostWords > rio_FFI_PAD_WORDS) return 0;
  *f = vm->ffi[j];
  f->argWords = (uint8_t)hostWords;
  f->resultWords = (uint8_t)(placed ? 0 : *result == 's' || *result == 'b' ? 2 : *result ? 1 : 0);
  f->pad = (uint8_t)(hostWords - words);
  return 1;
}

/* the host module sources an image was made with: names and hashes (8 hex digits), in its table */
typedef struct {
  const char *name[16], *hash[16];
  int n;
} rio_ImageSources;

/* the ffi table, [q, e) (its padding at the end): each bound by name, in the image's order, into ffi.
   One the host doesn't have, or whose sig doesn't fit, is fine unless the code calls it: its fn is 0,
   its name the image's (for rio_callsOk to name it), its sig "" if it's missing. Then the sources it
   was made with, into *src. 0, or -1 */
static rio_NOINLINE int rio_bindFfi(rio_Vm *vm, const uint32_t *h, const uint8_t *q, const uint8_t *e, rio_Ffi *ffi, rio_ImageSources *src) {
  uint32_t i, j;
  if ((e - q) & 3) return rio_imageError(vm, "wrong size"); /* (padded to whole words) */
  for (i = 0; i < h[rio_Image_ffiCount]; i++) {
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
    if (j >= (uint32_t)vm->ffiCount || !rio_ffiFits(vm, (int)j, sig, &ffi[i])) { /* fine, unless the code calls it */
      ffi[i].name = name;
      ffi[i].sig = j < (uint32_t)vm->ffiCount ? "x" : "";
      ffi[i].fn = 0;
      ffi[i].argWords = ffi[i].resultWords = ffi[i].declared = ffi[i].pad = 0;
    }
  }
  for (src->n = 0; src->n < (int)h[rio_Image_sourceCount]; src->n++) {
    size_t n;
    src->name[src->n] = (const char *)q;
    for (n = 0; q + n < e && q[n]; n++) {}
    if (q + n + 10 > e || q[n + 9]) return rio_imageError(vm, "bad ffi table");
    src->hash[src->n] = (const char *)q + n + 1;
    q += n + 10;
  }
  if (e - q > 3) return rio_imageError(vm, "wrong size");
  return 0;
}
/* after text (n long): which source proc's module was made from, when its source was registered, and
   this host's: " (made with pc 1a2b3c4d, this host's is 5e6f7a8b)" */
static rio_NOINLINE int rio_sourceNote(rio_Vm *vm, char *text, int n, const char *proc, const rio_ImageSources *src) {
  const char *dot = strchr(proc, '.');
  size_t len = dot ? (size_t)(dot - proc) : 0;
  char hex[9];
  int k, j;
  for (k = 0; dot && k < src->n; k++) {
    if (strlen(src->name[k]) != len || memcmp(src->name[k], proc, len)) continue;
    n = rio_appendText(text, n, " (made with ");
    n = rio_appendText(text, n, src->name[k]);
    n = rio_appendText(text, n, " ");
    n = rio_appendText(text, n, src->hash[k]);
    for (j = 0; j < vm->sourceCount && strcmp(vm->sources[j].name, src->name[k]); j++) {}
    if (j < vm->sourceCount) {
      rio_hex8(hex, vm->sources[j].hash);
      n = rio_appendText(text, n, ", this host's is ");
      n = rio_appendText(text, n, hex);
    }
    n = rio_appendText(text, n, ")");
  }
  return n;
}

/* does the code call only ffi functions the host registered, with sigs that fit? The ones it doesn't
   call lose the image's names (which don't outlive the load) */
static rio_NOINLINE int rio_callsOk(rio_Vm *vm, const uint32_t *h, const rio_Instr *code, rio_Ffi *ffi, const rio_ImageSources *src) {
  uint32_t i;
  for (i = 0; i < h[rio_Image_codeLen]; i++) {
    rio_Instr in;
    memcpy(&in, code + i, sizeof in);
    if (in.op == rio_Op_ffi && (in.c >= h[rio_Image_ffiCount] || !ffi[in.c].fn)) {
      char text[rio_ERROR_BUF_SIZE];
      int n = rio_appendText(text, 0, "the program calls ");
      if (in.c >= h[rio_Image_ffiCount]) return rio_imageError(vm, "the program calls an ffi function it doesn't list");
      n = rio_appendText(text, n, ffi[in.c].name);
      n = rio_appendText(text, n, *ffi[in.c].sig ? ", whose sig on this host doesn't fit the image's" : ", which the host didn't register");
      n = rio_sourceNote(vm, text, n, ffi[in.c].name, src);
      text[n] = 0;
      return rio_imageError(vm, text);
    }
  }
  for (i = 0; i < h[rio_Image_ffiCount]; i++)
    if (!ffi[i].fn) ffi[i].name = ffi[i].sig = "";
  return 0;
}
/* the end of a load, with memory in place: the vm set up to run it */
static int rio_loadEnd(rio_Vm *vm, const uint32_t *h, const rio_Ffi *ffi) {
  memset(vm->mem + h[rio_Image_end], 0, vm->memSize - h[rio_Image_end]);
  memcpy(vm->ffi, ffi, h[rio_Image_ffiCount] * sizeof *ffi);
  vm->ffiCount = (int)h[rio_Image_ffiCount];
  vm->memSize = h[rio_Image_memSize];
  vm->codeLen = h[rio_Image_codeLen];
  vm->constCount = h[rio_Image_constCount];
  vm->constCap = h[rio_Image_constCap];
  vm->callStackAt = h[rio_Image_callStackAt];
  vm->freeAt = h[rio_Image_freeAt];
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
/* the bytes after the header and the ffi table: the code apart, the constants, the block */
static uint32_t rio_restSize(const uint32_t *h) {
  return (h[rio_Image_codeAt] ? 0 : h[rio_Image_codeLen] * (uint32_t)sizeof(rio_Instr)) + h[rio_Image_constCap] * 4 +
         (h[rio_Image_end] - h[rio_Image_top]);
}

int rio_loadImage(rio_Vm *vm, const void *image, uint32_t len) {
  const uint8_t *p = (const uint8_t *)image, *q;
  uint32_t h[rio_Image_headerWords], blockSize, codeLen;
  const rio_Instr *code;
  rio_Ffi ffi[rio_MAX_FFI];
  rio_ImageSources src;
  if (rio_loadStart(vm, len)) return -1;
  memcpy(h, p, sizeof h);
  if (rio_headerChecks(vm, h)) return -1;
  if (rio_restSize(h) > len - sizeof h) return rio_imageError(vm, "wrong size");
  q = p + len - rio_restSize(h);
  if (rio_bindFfi(vm, h, p + sizeof h, q, ffi, &src)) return -1;
  blockSize = h[rio_Image_end] - h[rio_Image_top];
  codeLen = h[rio_Image_codeAt] ? 0 : h[rio_Image_codeLen]; /* the code apart from the block */
  if (h[rio_Image_codeAt])
    code = (const rio_Instr *)(const void *)(q + h[rio_Image_constCap] * 4 + (h[rio_Image_codeAt] - h[rio_Image_top]));
  else code = (const rio_Instr *)(const void *)q;
  if (rio_callsOk(vm, h, code, ffi, &src)) return -1; /* (before anything is copied: a failed load leaves the vm as it was) */
  if (codeLen) {
    memcpy(vm->hostCode, q, codeLen * sizeof(rio_Instr));
    q += codeLen * sizeof(rio_Instr);
  }
  memcpy(vm->mem, q, h[rio_Image_constCap] * 4);
  q += h[rio_Image_constCap] * 4;
  memset(vm->mem + h[rio_Image_constCap] * 4, 0, h[rio_Image_top] - h[rio_Image_constCap] * 4);
  memcpy(vm->mem + h[rio_Image_top], q, blockSize);
  return rio_loadEnd(vm, h, ffi);
}

/* the image, len bytes, in its parts, straight into place: no copy of it all at once. Its ffi table
   waits, while it's bound, in memory that's zero once it's loaded: between the constants and the
   program's tables, else above them */
int rio_readImage(rio_Vm *vm, rio_ReadFn read, void *userData, uint32_t len) {
  uint32_t h[rio_Image_headerWords], codeLen, table;
  uint8_t *at;
  rio_Ffi ffi[rio_MAX_FFI];
  rio_ImageSources src;
  if (rio_loadStart(vm, len)) return -1;
  if (read(userData, h, sizeof h)) return rio_imageError(vm, "can't read it");
  if (rio_headerChecks(vm, h)) return -1;
  if (rio_restSize(h) > len - sizeof h) return rio_imageError(vm, "wrong size");
  table = len - (uint32_t)sizeof h - rio_restSize(h);
  if (table <= h[rio_Image_top] - h[rio_Image_constCap] * 4) at = vm->mem + h[rio_Image_constCap] * 4;
  else if (table <= vm->memSize - h[rio_Image_end]) at = vm->mem + h[rio_Image_end];
  else return rio_imageError(vm, "needs more memory");
  if (read(userData, at, table)) return rio_imageError(vm, "can't read it");
  if (rio_bindFfi(vm, h, at, at + table, ffi, &src)) return -1;
  codeLen = h[rio_Image_codeAt] ? 0 : h[rio_Image_codeLen];
  if (codeLen && read(userData, vm->hostCode, codeLen * (uint32_t)sizeof(rio_Instr))) return rio_imageError(vm, "can't read it");
  if (read(userData, vm->mem, h[rio_Image_constCap] * 4)) return rio_imageError(vm, "can't read it");
  memset(vm->mem + h[rio_Image_constCap] * 4, 0, h[rio_Image_top] - h[rio_Image_constCap] * 4);
  if (read(userData, vm->mem + h[rio_Image_top], h[rio_Image_end] - h[rio_Image_top])) return rio_imageError(vm, "can't read it");
  if (rio_callsOk(vm, h, h[rio_Image_codeAt] ? (const rio_Instr *)(const void *)(vm->mem + h[rio_Image_codeAt]) : vm->hostCode, ffi, &src))
    return -1;
  return rio_loadEnd(vm, h, ffi);
}
