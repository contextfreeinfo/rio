/* compile.c - compiling a whole program: sizing and carving the compiler's tables, the built-in names,
   and finishing: sharing proc frames, packing the line table, and laying out the final memory. */
#include "compiler.h"
#ifndef rio_NO_COMPILER

int rio_define(rio_Vm *vm, const char *name, const char *value) {
  if (vm->defineCount >= rio_MAX_DEFINES || !name) return -1;
  vm->defines[vm->defineCount].name = name;
  vm->defines[vm->defineCount++].value = value;
  return 0;
}

/* ------------------------------------------------------------------ limits */

static uint32_t rio_clamp(uint32_t v, uint32_t lo, uint32_t hi) { return v < lo ? lo : v > hi ? hi : v; }

rio_Limits rio_limitsFor(uint32_t bytes) {
  rio_Limits l;
  uint32_t fixed, perSymbol, budget = bytes / 5 * 3;
  l.exprDepth = rio_clamp(bytes / 1024, 32, 256);
  l.blocks = rio_clamp(bytes / 2048, 16, 64);
  l.slots = rio_clamp(bytes / 8, 2048, 65536);
  fixed = (uint32_t)sizeof(rio_Compiler) + l.exprDepth * (uint32_t)(sizeof(rio_Expr) + sizeof(rio_PendingOp)) +
          l.blocks * (uint32_t)sizeof(rio_Block) + l.slots / 8 + 64;
  /* per symbol: its entry, ~12 bytes of name, a share of fields, types, procs and params, and two
     constant words (programs use about two constants per symbol) */
  perSymbol = (uint32_t)(sizeof(rio_Symbol) + 12 + sizeof(rio_Field) / 2 + sizeof(rio_Type) / 8 + sizeof(rio_ProcDef) / 4 +
                         sizeof(rio_Param) / 2 + 8);
  l.symbols = rio_clamp(budget > fixed ? (budget - fixed) / perSymbol : 0, 64, 65535);
  l.consts = rio_clamp(l.symbols * 2, 128, 16384);
  if (l.slots < l.consts + 1024) l.slots = rio_clamp(l.consts + 1024, 2048, 65536);
  l.nameBytes = rio_clamp(l.symbols * 12, 512, 65535);
  l.fields = rio_clamp(l.symbols / 2, 16, 65535);
  l.types = rio_clamp(l.symbols / 8, 16, 65535);
  l.procs = rio_clamp(l.symbols / 4, 8, 65535);
  l.params = rio_clamp(l.symbols / 2, 16, 65535);
  l.modules = rio_clamp(l.symbols / 32, 8, 1024);
  l.code = rio_clamp(bytes / 32, 256, 65535); /* a quarter of the bytes, when the code lives in mem */
  return l;
}

/* n items of size bytes from *at, aligned; returns where they start */
static uint32_t rio_carve(uint32_t *at, uint32_t n, uint32_t size, uint32_t align) {
  uint32_t start = (*at + align - 1) & ~(align - 1);
  *at = start + n * size;
  return start;
}

/* Lays the tables out after the rio_Compiler header (c NULL: just measures). Returns the end offset,
   where the string pool starts. */
static uint32_t rio_layout(rio_Compiler *c, const rio_Limits *l) {
  uint32_t at = (uint32_t)sizeof(rio_Compiler);
  uint8_t *base = (uint8_t *)c;
  uint32_t syms = rio_carve(&at, l->symbols, sizeof(rio_Symbol), 4);
  uint32_t types = rio_carve(&at, l->types, sizeof(rio_Type), 4);
  uint32_t fields = rio_carve(&at, l->fields, sizeof(rio_Field), 2);
  uint32_t procDefs = rio_carve(&at, l->procs, sizeof(rio_ProcDef), 4);
  uint32_t params = rio_carve(&at, l->params, sizeof(rio_Param), 4);
  uint32_t blocks = rio_carve(&at, l->blocks, sizeof(rio_Block), 2);
  uint32_t values = rio_carve(&at, l->exprDepth, sizeof(rio_Expr), 4);
  uint32_t ops = rio_carve(&at, l->exprDepth, sizeof(rio_PendingOp), 8);
  uint32_t modules = rio_carve(&at, l->modules, sizeof(rio_Module), 2);
  uint32_t names = rio_carve(&at, l->nameBytes, 1, 1);
  /* the bit sets, from exposed to localViews, are zeroed together */
  uint32_t exposed = rio_carve(&at, (l->slots + 7) / 8, 1, 1);
  uint32_t constIsString = rio_carve(&at, (l->consts + 7) / 8, 1, 1);
  uint32_t constIsAddr = rio_carve(&at, (l->consts + 7) / 8, 1, 1);
  uint32_t localViews = rio_carve(&at, (l->slots + 7) / 8, 1, 1);
  uint32_t slotMap = rio_carve(&at, l->slots, 2, 2);
  uint32_t code = rio_carve(&at, l->code, sizeof(rio_Instr), 8);
  if (c) {
    c->syms = (rio_Symbol *)(void *)(base + syms);
    c->types = (rio_Type *)(void *)(base + types);
    c->fields = (rio_Field *)(void *)(base + fields);
    c->procDefs = (rio_ProcDef *)(void *)(base + procDefs);
    c->params = (rio_Param *)(void *)(base + params);
    c->blocks = (rio_Block *)(void *)(base + blocks);
    c->values = (rio_Expr *)(void *)(base + values);
    c->ops = (rio_PendingOp *)(void *)(base + ops);
    c->names = (char *)base + names;
    c->exposed = base + exposed;
    c->constIsString = base + constIsString;
    c->constIsAddr = base + constIsAddr;
    c->localViews = base + localViews;
    c->slotMap = (uint16_t *)(void *)(base + slotMap);
    c->modules = (rio_Module *)(void *)(base + modules);
    c->scratchCode = (rio_Instr *)(void *)(base + code);
    memset(c->exposed, 0, localViews + (l->slots + 7) / 8 - exposed);
  }
  return (at + 15) & ~15u;
}

uint32_t rio_limitsSize(const rio_Limits *l) { return rio_layout(0, l) + 16; }

/* ------------------------------------------------------------------ starting */

/* Sets up the compiler's state in scratch, or overlaid on mem when scratch is NULL. repl: for a REPL
   session (the code goes elsewhere). */
/* below: with the compiler in mem, where its room ends (0: the end of mem), as when the source is up there */
int rio_startCompiler(rio_Vm *vm, void *scratch, uint32_t size, const rio_Limits *lim, int repl, uint32_t below) {
  int inMem = !scratch, codeInScratch = !vm->hostCode && !repl;
  uintptr_t pad;
  rio_Compiler *c;
  rio_Limits l;
  uint32_t used;
  vm->ready = 0;
  vm->compiler = 0;
  vm->inRepl = 0;
  vm->errorKind = rio_ErrorKind_none;
  vm->errorText[0] = 0;
  vm->linesSize = 0;
  vm->procs = 0;
  vm->jitFns = 0;
  vm->jitUsed = vm->jitCount = 0;
  if (!below || below > vm->memSize) below = vm->memSize;
  if (lim) l = *lim;
  else { /* the code (when it's compiled into scratch) gets a quarter, the tables scale to the rest */
    uint32_t n = inMem ? below : size;
    l = rio_limitsFor(codeInScratch ? n - n / 4 : n);
    l.code = rio_limitsFor(n).code;
  }
  if (!codeInScratch) l.code = 0;
  if (l.code > 65535) l.code = 65535;
  if (l.consts < 16) l.consts = 16;
  if (l.slots > 65536) l.slots = 65536;
  if (inMem) { /* overlay: everything above the constants is free until the program runs */
    if (l.consts * 4 + 1024 > below) {
      rio_setError(vm, rio_ErrorKind_compile, 0, 0, 0, "not enough memory for the compiler");
      return -1;
    }
    scratch = vm->mem + l.consts * 4;
    size = below - l.consts * 4;
  }
  pad = (16 - ((uintptr_t)scratch & 15)) & 15;
  used = rio_limitsSize(&l);
  if (l.slots < l.consts + 64 || l.consts * 4 + 256 > vm->memSize || size < pad + used + 64) {
    rio_setError(vm, rio_ErrorKind_compile, 0, 0, 0, inMem ? "not enough memory for the compiler" : "scratch too small for these limits");
    return -1;
  }
  c = vm->compiler = (rio_Compiler *)(void *)((uint8_t *)scratch + pad);
  memset(c, 0, sizeof *c);
  c->limits = l;
  c->room = size - (uint32_t)pad;
  c->source = inMem ? vm->memSize - below : 0;
  used = rio_layout(c, &l);
  c->pool = (uint8_t *)c + used;
  c->poolCap = c->lineTableTop = (size - (uint32_t)pad - used) & ~3u;
  vm->constCap = l.consts;
  vm->top = vm->memSize;
  vm->constCount = 1; /* word 0 stays 0 */
  vm->codeLen = 0;
  vm->procCount = 0;
  ((rio_Val *)vm->mem)[0].u = 0;
  if (codeInScratch) {
    vm->code = c->scratchCode;
    vm->codeCap = l.code;
  } else if (vm->hostCode) {
    vm->code = vm->hostCode;
    vm->codeCap = vm->hostCodeCap;
  }
  return 0;
}

/* the built-in types and names, the host's ffi functions, and the -D defines. sources: auto-imports may
   come from the loader's sources (rio_openAutoImports), else they're made from the host procs */
void rio_setupCompiler(rio_Vm *vm, int sources) {
  static const uint8_t kinds[] = {rio_TypeKind_void, rio_TypeKind_int, rio_TypeKind_float, rio_TypeKind_byte,
                                  rio_TypeKind_slice, rio_TypeKind_slice, rio_TypeKind_bool};
  static const uint8_t sizes[] = {0, 4, 4, 1, 8, 8, 4};
  static const char *const typeNames[] = {"Int", "Float", "String", "Blob", "Bool"};
  static const uint8_t typeIds[] = {rio_TypeId_int, rio_TypeId_float, rio_TypeId_string, rio_TypeId_blob, rio_TypeId_bool};
  rio_Compiler *c = vm->compiler;
  int i;
  c->liveSlots = c->freeSlot = c->slotHigh = vm->constCap;
  c->curProc = -1;
  c->lastLabel = rio_NONE;
  c->targetType = -1;
  c->switchCount = 0;
  c->curModule = 0;
  c->moduleCount = 2;
  memset(c->modules, 0, 2 * sizeof(rio_Module));
  for (i = 0; i < rio_TypeId_builtinCount; i++) {
    c->types[i].kind = kinds[i];
    c->types[i].size = sizes[i];
    c->types[i].elem = rio_TypeId_byte; /* (String and Blob: of bytes) */
    c->types[i].hasRefs = i == rio_TypeId_string || i == rio_TypeId_blob;
  }
  c->typeCount = rio_TypeId_builtinCount;
  for (i = 0; i < 5; i++) rio_addSymbol(vm, typeNames[i], (int)strlen(typeNames[i]), rio_SymKind_type, typeIds[i], 0);
  rio_addSymbol(vm, "true", 4, rio_SymKind_const, rio_TypeId_bool, 1);
  rio_addSymbol(vm, "false", 5, rio_SymKind_const, rio_TypeId_bool, 0);
  for (i = 0; i < rio_Builtin_count; i++)
    if (i < rio_Builtin_push || i == rio_Builtin_cap || i == rio_Builtin_hexBlob) /* the rest are methods */
      rio_addSymbol(vm, rio_builtinNames[i], (int)strlen(rio_builtinNames[i]), rio_SymKind_builtin, 0, i);
  for (i = 0; i < vm->ffiCount; i++) { /* the ffi functions' sigs: param types, then > and the result's */
    rio_Ffi *f = &vm->ffi[i];
    rio_FfiDef *def = &c->ffi[i];
    const char *g;
    def->firstParam = (uint16_t)c->paramCount;
    def->paramCount = 0;
    def->resultType = rio_TypeId_void;
    f->argWords = f->resultWords = f->declared = 0;
    for (g = f->sig; *g; g++) {
      int isResult = *g == '>', ch = isResult ? g[1] : *g;
      int t = ch == 'i' ? rio_TypeId_int : ch == 'f' ? rio_TypeId_float : ch == 's' ? rio_TypeId_string : ch == 'b' ? rio_TypeId_blob : -1;
      if (t < 0) rio_FAIL(, "bad ffi signature");
      if (c->paramCount >= (int)c->limits.params) rio_FAIL(, "too many params: the host procs' don't fit the compiler's tables (give it more room)");
      if (isResult) {
        def->resultType = (uint16_t)t;
        f->resultWords = (uint8_t)rio_typeWords(vm, t);
        break;
      }
      c->params[c->paramCount].refType = 0;
      c->params[c->paramCount++].type = (uint16_t)t;
      def->paramCount++;
      f->argWords = (uint8_t)(f->argWords + rio_typeWords(vm, t));
    }
    if (!strchr(f->name, '.')) rio_addSymbol(vm, f->name, (int)strlen(f->name), rio_SymKind_ffi, 0, i); /* (pc.cls: in module pc) */
  }
  c->autoSourceCount = 0;
  for (i = 0; i < vm->autoImportCount; i++) { /* host modules visible everywhere, as if imported */
    const char *name = vm->autoImports[i];
    int n = (int)strlen(name), m;
    rio_Source s;
    memset(&s, 0, sizeof s);
    if (sources && vm->loader && !vm->loader(vm->loaderData, name, rio_LoadKind_package, &s)) { /* (compiled first, from its source) */
      c->autoSources[c->autoSourceCount++] = i;
      continue;
    }
    m = rio_hostModule(vm, name, n);
    if (rio_failed(vm)) return;
    if (m < 0) {
      char text[rio_ERROR_BUF_SIZE];
      int len = rio_appendText(text, 0, "auto-import ");
      len = rio_appendText(text, len, name);
      len = rio_appendText(text, len, ": the host registered no procs under that name");
      text[len] = 0;
      rio_failAt(vm, 0, 0, 0, text);
      c->failMessage = 0;
      return;
    }
    rio_addSymbol(vm, name, n, rio_SymKind_module, 0, m);
  }
  c->firstDefine = c->symCount;
  for (i = 0; i < vm->defineCount; i++) {
    const char *name = vm->defines[i].name;
    int j, n = (int)strlen(name);
    rio_Expr e;
    for (j = 0; j < n; j++)
      if (!(rio_isAlpha(name[j]) || (j && rio_isDigit(name[j])))) break;
    if (!n || j < n) {
      char text[rio_ERROR_BUF_SIZE];
      int len = rio_appendText(text, 0, "-D: bad name ");
      len = rio_appendText(text, len, name);
      text[len] = 0;
      rio_failAt(vm, 0, 0, 0, text);
      c->failMessage = 0;
      return;
    }
    e = rio_defineValue(vm, i, -1);
    rio_addSymbol(vm, name, n, rio_SymKind_const, e.type, e.value);
  }
  c->curModule = c->curFile = 1; /* the main source */
  c->modules[1].state = rio_ModuleState_compiling;
}

/* statements until the source ends, going back to each file that imported or included one */
void rio_compileAll(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  while (!rio_failed(vm)) {
    if (c->tok.kind != rio_Tok_eof) {
      rio_statement(vm);
      continue;
    }
    if (c->blockCount) rio_FAIL(, rio_msgMissingEnd);
    if (!c->resumeCount) break;
    rio_endFile(vm);
  }
}

/* ------------------------------------------------------------------ finishing */

/* a proc's runtime entry, from what the compiler knows */
void rio_fillRuntimeProc(rio_Vm *vm, rio_Proc *out, int proc) {
  rio_Compiler *c = vm->compiler;
  rio_ProcDef *p = &c->procDefs[proc];
  uint32_t paramWords = 0;
  int j;
  for (j = 0; j < p->paramCount; j++) paramWords += (uint32_t)rio_typeWords(vm, c->params[p->firstParam + j].type);
  out->entry = p->entry;
  out->end = p->end;
  out->frameStart = p->frameStart;
  out->frameEnd = p->frameEnd;
  out->paramsEnd = (uint16_t)(p->frameStart + paramWords);
  out->result = (uint16_t)(p->resultAt / 4);
  out->resultWords = (uint8_t)(p->resultType ? rio_typeWords(vm, p->resultType) : 0);
}

/* which operands of an instruction are slots: bit 1 a, 2 b, 4 c (unused ones are 0, which never moves) */
static int rio_slotOperands(int op) {
  switch (op) {
  case rio_Op_ldw: case rio_Op_ldb: case rio_Op_lea: case rio_Op_ldw2: case rio_Op_stw: case rio_Op_stb: case rio_Op_stw2:
  case rio_Op_copy: case rio_Op_movn: return 3;
  case rio_Op_zero: case rio_Op_ffi: return 1;
  case rio_Op_call: return 0;
  case rio_Op_calli: case rio_Op_getret: return 3;
  default: return rio_isJump(op) ? 3 : 7;
  }
}

/* the proc whose code starts at pc: one defined before proc `before` */
static int rio_procAt(rio_Vm *vm, int pc, int before) {
  rio_ProcDef *procs = vm->compiler->procDefs;
  int lo = 0, hi = before - 1, m;
  while (lo <= hi) { /* procs are compiled in order, so their code is too */
    m = (lo + hi) / 2;
    if (procs[m].entry == pc) return m;
    if (procs[m].entry < pc) lo = m + 1;
    else hi = m - 1;
  }
  for (m = 0; m < before && procs[m].entry != pc; m++) {}
  if (m == before) rio_FAIL(0, "internal error: call to an unknown proc");
  return m;
}

/* where each proc's shared frame goes, and the depth of calls: see rio_shareFrames. Returns the top of
   the shared area (in slots from its start) into *top, and the longest chain of calls. */
static uint32_t rio_placeFrames(rio_Vm *vm, uint32_t *top) {
  rio_Compiler *c = vm->compiler;
  uint32_t i, s, depth = 0, n = (uint32_t)vm->procCount;
  *top = 0;
  /* two passes: procs that never call through a proc value first (every proc used as a value is one),
     then the rest, which may call any proc used as a value */
  for (i = 0; i < 2 * n; i++) {
    int k = (int)(i % n), j, pc;
    rio_ProcDef *p = &c->procDefs[k];
    uint32_t start = 0, callDepth = 0;
    if ((i < n) == (p->callsValues != 0)) continue;
    for (pc = p->entry; pc < p->end; pc++) { /* above everything it can call */
      if (vm->code[pc].op == rio_Op_calli) {
        for (j = 0; j < vm->procCount; j++) {
          rio_ProcDef *callee = &c->procDefs[j];
          if (!callee->usedAsValue) continue;
          if (callee->sharedEnd > start) start = callee->sharedEnd;
          if (callee->depth > callDepth) callDepth = callee->depth;
        }
        continue;
      }
      if (vm->code[pc].op != rio_Op_call) continue;
      j = rio_procAt(vm, vm->code[pc].c, k);
      if (c->procDefs[j].sharedEnd > start) start = c->procDefs[j].sharedEnd;
      if (c->procDefs[j].depth > callDepth) callDepth = c->procDefs[j].depth;
    }
    p->shared = !vm->inRepl && p->frameEnd > p->frameStart;
    for (s = p->frameStart; p->shared && s < p->frameEnd; s++)
      if (rio_bitGet(c->exposed, s)) p->shared = 0; /* an address is taken: it keeps its memory */
    p->sharedStart = (uint16_t)start;
    p->sharedEnd = (uint16_t)(start + (p->shared ? p->frameEnd - p->frameStart : 0));
    p->depth = (uint16_t)(callDepth + 1);
    if (p->sharedEnd > *top) *top = p->sharedEnd;
    if (p->depth > depth) depth = p->depth;
  }
  return depth;
}

/* Procs that are never running at the same time share memory. No recursion means the call graph is known
   and has no cycles, and callees are compiled first. A proc's frame moves into the shared area, just above
   the shared frames of everything it can call, unless a slot of it ever has its address taken: those must
   keep their memory for good (a view of them may live on). The slots that stay are packed together,
   starting right after the constants actually used (the room reserved for constants is only needed while
   compiling); every operand, address constant and table that names a slot is renumbered. Returns the
   longest chain of calls, which sizes the return stack. */
static uint32_t rio_shareFrames(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  rio_Val *R = (rio_Val *)vm->mem;
  uint16_t *map = c->slotMap;
  uint32_t lo = vm->constCap, hi = c->slotHigh, s, keep, top, i;
  uint32_t start = vm->inRepl ? lo : vm->constCount; /* where the slots that stay start */
  uint32_t depth = rio_placeFrames(vm, &top);
  int k, pc;
  if (!top && start == lo) return depth; /* nothing moves */
  /* the map: slots that stay, packed from start; then the shared frames, after them */
  for (s = lo; s < hi; s++) map[s] = 0;
  for (k = 0; k < vm->procCount; k++)
    for (s = c->procDefs[k].frameStart; c->procDefs[k].shared && s < c->procDefs[k].frameEnd; s++) map[s] = 1;
  for (s = lo, keep = start; s < hi; s++)
    if (!map[s]) map[s] = (uint16_t)keep++;
  for (k = 0; k < vm->procCount; k++) {
    rio_ProcDef *p = &c->procDefs[k];
    for (s = p->frameStart; p->shared && s < p->frameEnd; s++) map[s] = (uint16_t)(keep + p->sharedStart + (s - p->frameStart));
  }
#define rio_MAP_SLOT(x) ((uint32_t)(x) >= lo && (uint32_t)(x) < hi ? map[x] : (x))
#define rio_MAP_ADDR(a) \
  ((uint32_t)(a) / 4 >= lo && (uint32_t)(a) / 4 < hi ? (uint32_t)map[(a) / 4] * 4 + ((a) & 3) : (uint32_t)(a))
  for (pc = 0; pc < (int)vm->codeLen; pc++) { /* operands */
    rio_Instr *in = &vm->code[pc];
    int which;
    if (in->op == rio_Op_data) { /* only slice's has a slot */
      if (pc && vm->code[pc - 1].op == rio_Op_slice) in->a = (uint16_t)rio_MAP_SLOT(in->a);
      continue;
    }
    which = rio_slotOperands(in->op);
    if (which & 1) in->a = (uint16_t)rio_MAP_SLOT(in->a);
    if (which & 2) in->b = (uint16_t)rio_MAP_SLOT(in->b);
    if (which & 4) in->c = (uint16_t)rio_MAP_SLOT(in->c);
  }
  for (i = 1; i < vm->constCount; i++) /* address constants */
    if (rio_bitGet(c->constIsAddr, i)) R[i].u = rio_MAP_ADDR(R[i].u);
  for (s = start; s < lo; s++) rio_bitClear(c->exposed, s); /* unused constant room */
  for (s = lo; s < hi; s++) { /* address-taken marks follow their slots, which only move down */
    int wasExposed = rio_bitGet(c->exposed, s);
    rio_bitClear(c->exposed, s);
    if (wasExposed && map[s] < keep) rio_bitSet(c->exposed, map[s]);
  }
  for (k = 0; k < c->symCount; k++) /* variables */
    if (c->syms[k].kind == rio_SymKind_var) c->syms[k].value = (int32_t)rio_MAP_ADDR(c->syms[k].value);
  for (k = 0; k < vm->procCount; k++) { /* frames and results */
    rio_ProcDef *p = &c->procDefs[k];
    if (p->frameEnd > p->frameStart) {
      uint32_t n = p->frameEnd - p->frameStart;
      p->frameStart = map[p->frameStart];
      p->frameEnd = (uint16_t)(p->frameStart + n);
    }
    if (p->resultAt) p->resultAt = rio_MAP_ADDR(p->resultAt);
  }
#undef rio_MAP_SLOT
#undef rio_MAP_ADDR
  c->slotHigh = c->liveSlots = c->freeSlot = keep + top;
  vm->constCap = start;
  return depth;
}

/* Packs the pc -> line table in place, from the 4-byte (pc, line) entries written while compiling. It's
   read only for runtime errors (and by the C backend), so it trades lookup speed for size: one byte for a
   step of 0-14 instructions and -4..+11 lines, which is nearly every entry, else four bytes:
   0xF0 | step >> 8, step & 255, then the line (low byte first). rio_markLine keeps steps under 4096, and no
   entry grows, so packing in place never overtakes what's still to be read. Returns the size. */
static uint32_t rio_packLines(rio_Compiler *c) {
  uint8_t *table = c->pool + c->poolCap;
  uint32_t i, size = 0, pc = 0;
  int line = 0, lineStep;
  uint16_t entry[2];
  for (i = 0; i < c->lineCount; i++) {
    uint32_t step;
    memcpy(entry, table + i * 4, 4);
    step = entry[0] - pc;
    lineStep = (int)entry[1] - line;
    if (step <= 14 && lineStep >= -4 && lineStep <= 11) table[size++] = (uint8_t)(step << 4 | (uint32_t)(lineStep + 4));
    else {
      table[size++] = (uint8_t)(0xF0 | step >> 8);
      table[size++] = (uint8_t)step;
      table[size++] = (uint8_t)entry[1];
      table[size++] = (uint8_t)(entry[1] >> 8);
    }
    pc = entry[0];
    line = entry[1];
  }
  return size;
}

/* the names the host can look up (rio_findProc, rio_findGlobal): main and the main file's names marked
   name*, or when built with rio_EXPORTS_ALL every top-level proc and global of the main file */
static int rio_isHostName(rio_Compiler *c, rio_Symbol *y) {
  if ((y->kind != rio_SymKind_proc && y->kind != rio_SymKind_var) || y->module != 1) return 0;
#ifdef rio_EXPORTS_ALL
  (void)c;
  return 1;
#else
  return y->exported || (y->nameLen == 4 && !memcmp(c->names + y->name, "main", 4));
#endif
}

/* Lays out the final memory: the block of strings, exports, the line table, procs and modules goes just
   below the big arrays (and the code, when it lives in mem, just below that); string constants get their
   real addresses; everything the compiler used is zeroed.
     compiling:  [consts][ compiler state + string pool ........................................]
     running:    [consts][ frames/globals | return stack ][ ... ][code][strings+tables][arrays ] */
/* how full compiling got (rio_Usage), while the compiler's tables are still there */
static void rio_noteCompile(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  rio_Usage *u = vm->usage;
  if (!u || !c) return;
  memset(u, 0, sizeof *u);
  u->cap = c->limits;
  u->used = c->high;
  u->used.types = (uint32_t)c->typeCount;
  u->used.fields = (uint32_t)c->fieldCount;
  u->used.procs = (uint32_t)vm->procCount;
  u->used.params = (uint32_t)c->paramCount;
  u->used.consts = vm->constCount;
  u->used.slots = c->slotHigh;
  u->used.modules = (uint32_t)c->moduleCount;
  u->used.code = vm->code == c->scratchCode ? vm->codeLen : 0;
  u->room = c->room;
  u->source = c->source;
  u->strings = c->poolUsed + (c->lineTableTop - c->poolCap); /* (the strings, and the line table growing down from the top) */
  u->stringCap = c->lineTableTop;
  u->mem = vm->memSize;
  u->wanted = c->bigWanted;
  u->left = c->bigLeft;
}

static void rio_finish(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  rio_Val *R = (rio_Val *)vm->mem;
  rio_Export *x;
  uint32_t depth = rio_shareFrames(vm), firstFree = vm->constCap * 4; /* after sharing, slots start right after the constants */
  uint32_t i, n = 0, p, exportsAt, linesAt, procsAt, modulesAt, size, base, lineSize, codeSize = 0, codeAt = 0;
  uint32_t moduleCount = (uint32_t)(c->moduleCount - 2), fileNamesAt;
  int k;
  rio_noteCompile(vm);
  c->freeSlot = c->slotHigh;
  vm->callStackAt = (uint32_t)rio_allocSlots(vm, depth + 1) * 4; /* the return stack: the longest chain of calls */
  /* the block, built in the pool after the strings: exported names, then module file names */
  p = c->poolUsed;
  for (k = 0; k < c->symCount; k++) {
    rio_Symbol *y = &c->syms[k];
    if (!rio_isHostName(c, y)) continue;
    if (y->nameLen > c->poolCap - p) rio_FAIL(, "out of compiler memory");
    memcpy(c->pool + p, c->names + y->name, y->nameLen);
    p += y->nameLen;
    n++;
  }
  fileNamesAt = p;
  for (k = 2; k < c->moduleCount; k++) {
    uint32_t len = (uint32_t)strlen(c->names + c->modules[k].file) + 1;
    if (len > c->poolCap - p) rio_FAIL(, "out of compiler memory");
    memcpy(c->pool + p, c->names + c->modules[k].file, len);
    p += len;
  }
  { /* the line table was written top-down: reverse it in place so it ascends by pc */
    uint8_t *a = c->pool + c->poolCap, *z = c->pool + c->lineTableTop - 4, t[4];
    for (; a < z; a += 4, z -= 4) {
      memcpy(t, a, 4);
      memcpy(a, z, 4);
      memcpy(z, t, 4);
    }
  }
  lineSize = rio_packLines(c);
  /* block layout: strings and names | exports | pc->line table | per-proc table | modules */
  exportsAt = rio_align4(p);
  linesAt = exportsAt + n * (uint32_t)sizeof(rio_Export);
  procsAt = rio_align4(linesAt + lineSize);
  modulesAt = rio_align4(procsAt + (uint32_t)vm->procCount * (uint32_t)sizeof(rio_Proc));
  size = modulesAt + moduleCount * (uint32_t)sizeof(rio_ModuleCode);
  if (linesAt > c->poolCap || size > c->lineTableTop) rio_FAIL(, "out of compiler memory");
  if (vm->top < size || ((vm->top - size) & ~3u) < c->slotHigh * 4) rio_FAIL(, "out of memory");
  base = (vm->top - size) & ~3u;
  if (vm->code == c->scratchCode) { /* the code lives in mem: just below the block */
    codeSize = vm->codeLen * (uint32_t)sizeof(rio_Instr);
    if (base < codeSize || ((base - codeSize) & ~7u) < c->slotHigh * 4) rio_FAIL(, "out of memory");
    codeAt = (base - codeSize) & ~7u;
  }
  x = (rio_Export *)(void *)(c->pool + exportsAt);
  p = c->poolUsed;
  for (k = 0; k < c->symCount; k++) {
    rio_Symbol *y = &c->syms[k];
    if (!rio_isHostName(c, y)) continue;
    x->nameAt = base + p;
    x->nameLen = y->nameLen;
    x->kind = y->kind;
    x->pad = 0;
    x->value = y->value;
    p += y->nameLen;
    x++;
  }
  memmove(c->pool + linesAt, c->pool + c->poolCap, lineSize); /* lines go right after the exports */
  for (i = 1; i < vm->constCount; i++) /* strings: pool offsets become addresses */
    if (rio_bitGet(c->constIsString, i)) R[i].u = base + (R[i].u & 0x7FFFFFFFu);
  for (k = 0; k < vm->procCount; k++) rio_fillRuntimeProc(vm, (rio_Proc *)(void *)(c->pool + procsAt) + k, k);
  for (k = 2, p = fileNamesAt; k < c->moduleCount; k++) {
    rio_ModuleCode *m = (rio_ModuleCode *)(void *)(c->pool + modulesAt) + (k - 2);
    m->start = c->modules[k].codeStart;
    m->end = c->modules[k].codeEnd;
    m->fileAt = base + p;
    p += (uint32_t)strlen(c->names + c->modules[k].file) + 1;
  }
  vm->modulesAt = base + modulesAt;
  vm->moduleCount = moduleCount;
  vm->exportsAt = base + exportsAt;
  vm->exportCount = n;
  vm->top = base;
  vm->linesAt = base + linesAt;
  vm->linesSize = lineSize;
  vm->lineCachePos = vm->lineCachePc = vm->lineCacheLine = 0;
  vm->procs = (rio_Proc *)(void *)(vm->mem + base + procsAt);
  if (vm->usage) { /* where the memory goes, running */
    rio_Usage *u = vm->usage;
    u->consts = vm->constCap * 4;
    u->code = codeSize;
    u->tables = size;
    u->free = (codeAt ? codeAt : base) - c->slotHigh * 4;
    u->data = vm->memSize - u->consts - u->code - u->tables - u->free;
  }
  /* Nothing in c is read after this: the moves and the zeroing may overwrite it. Both regions move within
     mem when scratch is overlaid on it: the one moving up goes first if the code moves up. */
  if (codeAt) {
    uint8_t *from = (uint8_t *)c->scratchCode, *to = vm->mem + codeAt;
    if (to >= from) {
      memmove(vm->mem + base, c->pool, size);
      memmove(to, from, codeSize);
    } else {
      memmove(to, from, codeSize);
      memmove(vm->mem + base, c->pool, size);
    }
    vm->code = (rio_Instr *)(void *)to;
    vm->codeCap = vm->codeLen;
    vm->top = codeAt;
  } else memmove(vm->mem + base, c->pool, size);
  memset(vm->mem + firstFree, 0, (codeAt ? codeAt : base) - firstFree);
  if (codeAt) memset(vm->mem + codeAt + codeSize, 0, base - codeAt - codeSize);
  memset(vm->mem + base + size, 0, vm->memSize - base - size);
}

/* ------------------------------------------------------------------ compiling */

int rio_compileWith(rio_Vm *vm, const char *src, uint32_t len, void *scratch, uint32_t size, const rio_Limits *lim) {
  int inMem = !scratch;
  rio_Compiler *c;
  /* a source in mem itself: up in what stays zero until the program runs, with the compiler's room ending
     below it (the program's layout takes that memory back once compiling is done) */
  uint32_t below = 0;
  if (inMem && src >= (const char *)vm->mem && src < (const char *)vm->mem + vm->memSize && !(below = (uint32_t)(src - (const char *)vm->mem) & ~15u))
    below = 16; /* (no room at all: an error, below) */
  if (rio_startCompiler(vm, scratch, size, lim, 0, below)) return -1;
  c = vm->compiler;
  rio_setupCompiler(vm, 1);
  c->src = c->cursor = c->lineStart = src;
  c->srcEnd = src + len;
  c->line = 1;
  if (!rio_failed(vm)) {
    if (c->autoSourceCount) rio_openAutoImports(vm); /* (each compiled before the main source starts) */
    else {
      rio_lex(vm, &c->peek);
      rio_next(vm);
    }
    if (!rio_failed(vm)) rio_compileAll(vm);
  }
  if (!rio_failed(vm)) rio_emit(vm, rio_Op_halt, 0, 0, 0);
  if (!rio_failed(vm)) rio_finish(vm);
  if (rio_failed(vm)) {
    rio_noteCompile(vm); /* (how far it got: the compiler's tables are still there) */
    if (inMem) vm->compiler = 0;
    return -1;
  }
  if (inMem) vm->compiler = 0; /* (overwritten by the program's memory) */
  vm->ready = 1;
  if (vm->jitHook) vm->jitHook(vm);
  return 0;
}

void rio_trackUsage(rio_Vm *vm, rio_Usage *usage) { vm->usage = usage; }

/* a line of the usage text: name, used of cap, and the percent */
static int rio_usageLine(char *out, int n, int cap, const char *name, uint32_t used, uint32_t of) {
  char line[96];
  int k = rio_appendText(line, 0, "  ");
  k = rio_appendText(line, k, name);
  while (k < 18) line[k++] = ' ';
  k += rio_formatInt(line + k, (int32_t)used);
  k = rio_appendText(line, k, " of ");
  k += rio_formatInt(line + k, (int32_t)of);
  k = rio_appendText(line, k, ": ");
  k += rio_formatInt(line + k, of ? (int32_t)((uint64_t)used * 100 / of) : 0);
  k = rio_appendText(line, k, "%\n");
  if (n + k >= cap) return n;
  memcpy(out + n, line, (size_t)k);
  return n + k;
}

static int rio_usageBytes(char *out, int n, int cap, const char *name, uint32_t bytes) {
  char line[96];
  int k = rio_appendText(line, 0, "  ");
  k = rio_appendText(line, k, name);
  while (k < 18) line[k++] = ' ';
  k += rio_formatInt(line + k, (int32_t)bytes);
  k = rio_appendText(line, k, " bytes\n");
  if (n + k >= cap) return n;
  memcpy(out + n, line, (size_t)k);
  return n + k;
}

int rio_usageText(const rio_Usage *u, char *out, int cap) {
  int n = 0;
  if (cap < 1) return 0;
  n = rio_usageBytes(out, n, cap, "compiler room", u->room);
  if (u->source) n = rio_usageBytes(out, n, cap, "source in mem", u->source);
  n = rio_usageLine(out, n, cap, "symbols", u->used.symbols, u->cap.symbols);
  n = rio_usageLine(out, n, cap, "names", u->used.nameBytes, u->cap.nameBytes);
  n = rio_usageLine(out, n, cap, "types", u->used.types, u->cap.types);
  n = rio_usageLine(out, n, cap, "fields", u->used.fields, u->cap.fields);
  n = rio_usageLine(out, n, cap, "procs", u->used.procs, u->cap.procs);
  n = rio_usageLine(out, n, cap, "params", u->used.params, u->cap.params);
  n = rio_usageLine(out, n, cap, "constants", u->used.consts, u->cap.consts);
  n = rio_usageLine(out, n, cap, "slots", u->used.slots, u->cap.slots);
  n = rio_usageLine(out, n, cap, "nesting", u->used.blocks, u->cap.blocks);
  n = rio_usageLine(out, n, cap, "expression", u->used.exprDepth, u->cap.exprDepth);
  n = rio_usageLine(out, n, cap, "modules", u->used.modules, u->cap.modules);
  if (u->cap.code) n = rio_usageLine(out, n, cap, "instructions", u->used.code, u->cap.code);
  n = rio_usageLine(out, n, cap, "string pool", u->strings, u->stringCap);
  if (u->wanted) { /* (what didn't fit) */
    n = rio_usageBytes(out, n, cap, "big value", u->wanted);
    n = rio_usageBytes(out, n, cap, "room for it", u->left);
  }
  if (u->tables) { /* (compiled: where the memory goes, running) */
    n = rio_usageBytes(out, n, cap, "constants", u->consts);
    n = rio_usageBytes(out, n, cap, "data", u->data);
    if (u->code) n = rio_usageBytes(out, n, cap, "code", u->code);
    n = rio_usageBytes(out, n, cap, "strings, tables", u->tables);
    n = rio_usageBytes(out, n, cap, "free", u->free);
  }
  out[n] = 0;
  return n;
}

int rio_compile(rio_Vm *vm, const char *src, uint32_t len) { return rio_compileWith(vm, src, len, 0, 0, 0); }

int rio_compileScratch(rio_Vm *vm, const char *src, uint32_t len, void *scratch, uint32_t size) {
  if (!scratch) {
    rio_setError(vm, rio_ErrorKind_compile, 0, 0, 0, "scratch too small for these limits");
    vm->ready = 0;
    return -1;
  }
  return rio_compileWith(vm, src, len, scratch, size, 0);
}

#endif
