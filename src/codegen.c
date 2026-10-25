/* codegen.c - emitting instructions, and the storage they work on: slots, constants and memory
   allocated from the top. Then getting values into slots and storing them, which is where the
   peephole work happens (fused index and load or store, retargeting a result into its destination,
   fused compare and branch). */
#include "compiler.h"
#ifndef rio_NO_COMPILER

/* ------------------------------------------------------------------ the line table */

/* Records the line of the instruction about to be emitted: (u16 pc, u16 line) entries grow down from
   the top of the string pool, one per change of line. */
static void rio_markLine(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  uint32_t line = c->lineOverride ? c->lineOverride : (uint32_t)c->prevLine;
  uint16_t entry[2];
  if (line == c->lastLine && vm->codeLen - c->lastLinePc < 4096) return; /* (steps stay under 4096: see rio_packLines) */
  if (c->poolCap - c->poolUsed < 4) rio_FAIL(, "out of compiler memory");
  c->poolCap -= 4;
  entry[0] = (uint16_t)vm->codeLen;
  entry[1] = (uint16_t)(line > 0xFFFF ? 0xFFFF : line);
  memcpy(c->pool + c->poolCap, entry, 4);
  c->lineCount++;
  c->lastLine = line;
  c->lastLinePc = vm->codeLen;
}

/* the line of pc, while the table is still in the compiler's form */
uint32_t rio_compiledLine(rio_Vm *vm, uint32_t pc) {
  rio_Compiler *c = vm->compiler;
  uint32_t at, line = 0;
  uint16_t entry[2];
  for (at = c->lineTableTop; at > c->poolCap; at -= 4) {
    memcpy(entry, c->pool + at - 4, 4);
    if (entry[0] > pc) break;
    line = entry[1];
  }
  return line;
}

/* ------------------------------------------------------------------ instructions */

int rio_emit(rio_Vm *vm, int op, int a, int b, int c) {
  rio_Instr *in;
  if (vm->codeLen >= vm->codeCap) rio_FAIL(0, "code too large");
  rio_markLine(vm);
  in = &vm->code[vm->codeLen];
  in->op = (uint8_t)op;
  in->x = 0;
  in->a = (uint16_t)a;
  in->b = (uint16_t)b;
  in->c = (uint16_t)c;
  return (int)vm->codeLen++;
}

/* an instruction with only a, and its extra operand x */
void rio_emitX(rio_Vm *vm, int op, int x, int a) {
  rio_emit(vm, op, a, 0, 0);
  vm->code[vm->codeLen - 1].x = (uint8_t)x;
}

/* the data word after an instruction: a, and a 32-bit size */
void rio_emitData(rio_Vm *vm, int a, uint32_t size) { rio_emit(vm, rio_Op_data, a, (int)(size & 0xFFFF), (int)(size >> 16)); }

/* the pc here, as a jump target: nothing before it gets fused with what comes after */
int rio_label(rio_Vm *vm) { return (int)(vm->compiler->lastLabel = vm->codeLen); }

/* Jump lists: jumps whose target isn't known yet are chained through their c operands, ending in
   rio_NONE. Adds jump to list; returns the new list. */
int rio_jumpListAdd(rio_Vm *vm, int list, int jump) {
  if (jump == rio_NONE || rio_failed(vm)) return list; /* (after an error, jump lists are unreliable) */
  vm->code[jump].c = (uint16_t)list;
  return jump;
}

/* points every jump in list at pc to */
void rio_patchJumps(rio_Vm *vm, int list, int to) {
  while (list != rio_NONE && !rio_failed(vm)) {
    int next = vm->code[list].c;
    vm->code[list].c = (uint16_t)to;
    list = next;
  }
}

/* ------------------------------------------------------------------ storage */

/* n slots from the first free one */
int rio_allocSlots(rio_Vm *vm, int n) {
  rio_Compiler *c = vm->compiler;
  int first = (int)c->freeSlot;
  c->freeSlot += (uint32_t)n;
  if (c->freeSlot > c->slotHigh) {
    c->slotHigh = c->freeSlot;
    if (c->slotHigh > c->limits.slots || c->slotHigh * 4 > vm->top) rio_FAIL(0, "out of slots");
  }
  return first;
}

/* bytes for good, from the top of mem down (big values); returns their address */
uint32_t rio_allocHigh(rio_Vm *vm, uint32_t bytes) {
  rio_Compiler *c = vm->compiler;
  bytes = rio_align4(bytes);
  if (vm->top < bytes || vm->top - bytes < c->slotHigh * 4) {
    c->bigWanted = bytes;
    c->bigLeft = vm->top > c->slotHigh * 4 ? vm->top - c->slotHigh * 4 : 0;
    rio_FAIL(0, "out of memory");
  }
  c->bigBytes += bytes;
  if (c->curProc >= 0) c->procDefs[c->curProc].bigBytes += bytes; /* (for the memory report) */
  return vm->top -= bytes;
}

/* n consecutive constants with these values, reusing ones already there; bit j of addrBits: word j is a
   slot address (those are renumbered later, so they never match plain numbers). Returns the first. */
int rio_findConsts(rio_Vm *vm, const uint32_t *values, int n, int addrBits) {
  rio_Compiler *c = vm->compiler;
  rio_Val *R = (rio_Val *)vm->mem;
  uint32_t i;
  int j;
  for (i = 1; i + (uint32_t)n <= vm->constCount; i++) {
    for (j = 0; j < n; j++) {
      uint32_t k = i + (uint32_t)j;
      if (R[k].u != values[j] || rio_bitGet(c->constIsString, k) || rio_bitGet(c->constIsAddr, k) != (addrBits >> j & 1))
        break;
    }
    if (j == n) return (int)i;
  }
  if (vm->constCount + (uint32_t)n > vm->constCap) rio_FAIL(0, "too many constants");
  for (j = 0; j < n; j++, vm->constCount++) {
    R[vm->constCount].u = values[j];
    if (addrBits >> j & 1) rio_bitSet(c->constIsAddr, vm->constCount);
  }
  return (int)vm->constCount - n;
}

int rio_constSlot(rio_Vm *vm, uint32_t value) { return rio_findConsts(vm, &value, 1, 0); }

/* Marks the slots of [addr, addr + bytes) as address-taken (they can't become C locals in AOT, and
   their proc keeps its own frame); returns addr. */
uint32_t rio_expose(rio_Vm *vm, uint32_t addr, uint32_t bytes) {
  rio_Compiler *c = vm->compiler;
  uint32_t w;
  for (w = addr / 4; w < c->limits.slots && w * 4 < addr + bytes; w++) rio_bitSet(c->exposed, w);
  return addr;
}

/* a constant holding the address of [addr, addr + bytes) */
int rio_addrConst(rio_Vm *vm, uint32_t addr, uint32_t bytes) {
  uint32_t v = rio_expose(vm, addr, bytes);
  return rio_findConsts(vm, &v, 1, 1);
}

/* ------------------------------------------------------------------ peepholes */

/* may the last instruction be changed? not when a jump lands after it, or it's the previous REPL piece's */
static int rio_canFuse(rio_Vm *vm) { return vm->compiler->lastLabel != vm->codeLen; }

/* the last instruction (and its data word), if it's an idx writing temporary slot d */
static rio_Instr *rio_lastIndex(rio_Vm *vm, int d) {
  rio_Instr *in = vm->codeLen >= 2 ? &vm->code[vm->codeLen - 2] : 0;
  return in && d >= (int)vm->compiler->liveSlots && rio_canFuse(vm) && in->op == rio_Op_idx && in->a == d ? in : 0;
}

/* if the last instruction wrote temporary slot `from`, makes it write `to` instead */
int rio_retarget(rio_Vm *vm, int from, int to) {
  rio_Instr *in;
  if (!vm->codeLen || !rio_canFuse(vm) || from < (int)vm->compiler->liveSlots) return 0;
  in = &vm->code[vm->codeLen - 1];
  if (in->op == rio_Op_data && vm->codeLen >= 2 && (in[-1].op == rio_Op_ldx || in[-1].op == rio_Op_ldxb)) in--;
  if ((in->op > rio_Op_lastScalar && in->op != rio_Op_ldx && in->op != rio_Op_ldxb) || in->a != from) return 0;
  in->a = (uint16_t)to;
  return 1;
}

/* ------------------------------------------------------------------ views of local memory */

/* A proc's own memory keeps its type for good, so a view of it is always memory-safe, but the next call
   rewrites it: returning one, or storing one where it outlives the call, is almost always a bug. A view
   that comes in as a parameter may be one of the caller's, so it's only borrowed: it can't be stored
   where it outlives the call either (its bytes can be copied). And a call's result, if it's a view, may
   view what was passed in, so it's taken as a view of the caller's own memory when any argument was. So
   every rule is about one proc's own code: no proc's body changes what its callers may do. */

/* e is storage of the proc being compiled */
int rio_isLocalMemory(rio_Vm *vm, const rio_Expr *e) {
  rio_ProcDef *p;
  uint32_t a;
  if (vm->compiler->curProc < 0 || e->kind != rio_ExprKind_static) return 0;
  p = &vm->compiler->procDefs[vm->compiler->curProc];
  a = (uint32_t)e->value;
  return (a / 4 >= p->frameStart && a < vm->top) || (a >= vm->top && a < p->bigTop);
}

int rio_isLocalView(rio_Vm *vm, const rio_Expr *e) {
  int k = rio_type(vm, e->type)->kind;
  return e->localView || ((k == rio_TypeKind_array || k == rio_TypeKind_list) && rio_isLocalMemory(vm, e));
}

/* marks (or unmarks) the slots at addr as holding a local view */
void rio_markLocalView(rio_Vm *vm, uint32_t addr, int words, int on) {
  uint32_t s = addr / 4;
  for (; words-- > 0; s++) {
    if (on) rio_bitSet(vm->compiler->localViews, s);
    else rio_bitClear(vm->compiler->localViews, s);
  }
}

/* marks (or unmarks) the slots at addr as holding a view that came in through a parameter */
void rio_markParamView(rio_Vm *vm, uint32_t addr, int words, int fromParam) {
  uint32_t s = addr / 4;
  for (; words-- > 0; s++) {
    if (fromParam) rio_bitSet(vm->compiler->paramViews, s);
    else rio_bitClear(vm->compiler->paramViews, s);
  }
}

/* A view stored into dst: into a local of this proc, that local now holds (or no longer holds) a view of
   the proc's own memory, or one that came in through a parameter; anywhere else, it mustn't be either. */
static void rio_storeView(rio_Vm *vm, const rio_Expr *dst, const rio_Expr *src) {
  rio_Compiler *c = vm->compiler;
  int isLocal = rio_isLocalView(vm, src);
  if (c->curProc < 0 || c->storingArgs) return;
  if (dst->kind == rio_ExprKind_static && rio_isLocalMemory(vm, dst) && (uint32_t)dst->value < vm->top) {
    rio_markLocalView(vm, (uint32_t)dst->value, rio_typeWords(vm, dst->type), isLocal);
    rio_markParamView(vm, (uint32_t)dst->value, rio_typeWords(vm, dst->type), src->fromParam);
  } else if (isLocal)
    rio_FAIL(, "this view of the proc's own memory would outlive the call: make that memory global, or have the caller pass it in");
  else if (src->fromParam)
    rio_FAIL(, "this view came in as a parameter, so it may view the caller's own memory: copy its bytes (a Blob builder's pushAll) to keep them past the call");
}

/* ------------------------------------------------------------------ values */

/* a value in the compiler's temporary slots (a call's or an expression's result), not a variable */
int rio_isTemp(rio_Vm *vm, const rio_Expr *e) {
  return e->kind == rio_ExprKind_static && (uint32_t)e->value >= vm->compiler->liveSlots * 4 && (uint32_t)e->value < vm->top;
}

void rio_needValue(rio_Vm *vm, rio_Expr *e) {
  if (e->kind > rio_ExprKind_indirect) rio_FAIL(, e->kind == rio_ExprKind_void ? "no value" : "not a value");
  if (e->access & rio_Access_ref) rio_FAIL(, "&x only goes to a & parameter or name := &x");
}

/* Proc e, used where a proc value goes (of type `type`, or its own signature when type < 0), becomes the
   value. Only a proc that never calls through a proc value can be one, so calls through values can't
   recurse. */
void rio_toProcValue(rio_Vm *vm, rio_Expr *e, int type) {
  rio_Compiler *c = vm->compiler;
  rio_ProcDef *p = &c->procDefs[e->value];
  int params[16], i;
  if (p->paramCount > 16) rio_FAIL(, "too many parameters for a proc value");
  for (i = 0; i < p->paramCount; i++) {
    rio_Param *param = &c->params[p->firstParam + i];
    if (param->refType) rio_FAIL(, "a proc with & parameters can't be a value");
    params[i] = param->type;
  }
  i = rio_procTypeOf(vm, params, p->paramCount, p->resultType);
  if (type >= 0 && i != type) rio_FAIL(, "this proc's parameters or result don't match");
  if (p->callsValues) rio_FAIL(, "this proc calls procs through values, so it can't be a value itself (that could recurse)");
  p->usedAsValue = 1;
  *e = rio_exprOf(rio_ExprKind_const, i, e->value + 1);
}

/* makes e a value of type `type`, or fails: Int constants become Floats, arrays view as slices, lists as
   builders, and procs become proc values */
void rio_coerce(rio_Vm *vm, rio_Expr *e, int type) {
  int from, fromKind, toKind;
  if (e->kind == rio_ExprKind_proc && rio_type(vm, type)->kind == rio_TypeKind_proc) { /* a proc where a proc value goes */
    rio_toProcValue(vm, e, type);
    return;
  }
  rio_needValue(vm, e);
  from = rio_valueType(e->type);
  type = rio_valueType(type);
  if (e->kind == rio_ExprKind_const && from == rio_TypeId_int && type == rio_TypeId_float) {
    rio_Val v;
    v.f = (float)e->value;
    e->value = v.i;
    e->type = rio_TypeId_float;
    return;
  }
  if (from == type) return;
  fromKind = rio_type(vm, from)->kind;
  toKind = rio_type(vm, type)->kind;
  if (fromKind == rio_TypeKind_array && toKind == rio_TypeKind_slice && rio_sliceOf(vm, rio_type(vm, from)->elem) == type) {
    if (rio_isTemp(vm, e)) rio_FAIL(, "a computed array has no home to view: store it in a variable first");
    return;
  }
  if (fromKind == rio_TypeKind_list && toKind == rio_TypeKind_builder && rio_type(vm, type)->elem == rio_type(vm, from)->elem)
    return;
  if ((fromKind == rio_TypeKind_array || fromKind == rio_TypeKind_slice) && toKind == rio_TypeKind_builder)
    rio_FAIL(, "a builder needs somewhere to keep its length: declare one first, name: [..]T = array");
  rio_FAIL(, "type mismatch");
}

/* makes e indirect through a constant holding its address, for an instruction that takes an address
   (bytes: how much of it is used) */
static void rio_indirectViaConst(rio_Vm *vm, rio_Expr *e, uint32_t bytes) {
  e->kind = rio_ExprKind_indirect;
  e->value = rio_addrConst(vm, (uint32_t)e->value, bytes);
  e->offset = 0;
}

/* A scalar value: the slot holding it. reuse: e's own temporary slot may be written (fusing an index
   with its load). */
int rio_toSlot(rio_Vm *vm, rio_Expr *e, int reuse) {
  int k, d;
  rio_needValue(vm, e);
  k = rio_type(vm, e->type)->kind;
  if (k != rio_TypeKind_int && k != rio_TypeKind_float && k != rio_TypeKind_byte && k != rio_TypeKind_bool &&
      k != rio_TypeKind_enum && k != rio_TypeKind_proc)
    rio_FAIL(0, "expected a number");
  if (e->kind == rio_ExprKind_const) return rio_constSlot(vm, (uint32_t)e->value);
  if (e->kind == rio_ExprKind_static) {
    if (k != rio_TypeKind_byte && e->value < rio_SLOTS_END) return e->value >> 2; /* it is a slot */
    rio_indirectViaConst(vm, e, k == rio_TypeKind_byte ? 1 : 4);
  }
  if (reuse && rio_lastIndex(vm, e->value)) { /* fuse index + load */
    vm->code[vm->codeLen - 2].op = k == rio_TypeKind_byte ? rio_Op_ldxb : rio_Op_ldx;
    vm->code[vm->codeLen - 1].a = (uint16_t)e->offset;
    return e->value;
  }
  d = reuse && e->value >= (int)vm->compiler->liveSlots ? e->value : rio_allocSlots(vm, 1);
  rio_emit(vm, k == rio_TypeKind_byte ? rio_Op_ldb : rio_Op_ldw, d, e->value, e->offset);
  return d;
}

/* a slice, String, Blob or array value: 2 consecutive slots (address, length) */
int rio_toSlot2(rio_Vm *vm, rio_Expr *e) {
  rio_Type *ty;
  int d;
  rio_needValue(vm, e);
  ty = rio_type(vm, e->type);
  if (ty->kind == rio_TypeKind_array) {
    if (e->kind == rio_ExprKind_static) { /* both known: constants */
      uint32_t v[2];
      v[0] = rio_expose(vm, (uint32_t)e->value, ty->size);
      v[1] = ty->count;
      return rio_findConsts(vm, v, 2, 1);
    }
    d = rio_allocSlots(vm, 2);
    rio_emit(vm, rio_Op_lea, d, e->value, e->offset);
    rio_emit(vm, rio_Op_mov, d + 1, rio_constSlot(vm, ty->count), 0);
    return d;
  }
  if (ty->kind != rio_TypeKind_slice) rio_FAIL(0, "expected a slice");
  if (e->kind == rio_ExprKind_static) {
    if (e->value < rio_SLOTS_END - 4) return e->value >> 2; /* both words are slots */
    rio_indirectViaConst(vm, e, 8);
  }
  d = rio_allocSlots(vm, 2);
  rio_emit(vm, rio_Op_ldw2, d, e->value, e->offset);
  return d;
}

/* a list or builder: 3 consecutive slots (address of the length word, address of the data, capacity) */
int rio_toSlot3(rio_Vm *vm, rio_Expr *e) {
  rio_Type *ty;
  int d;
  rio_needValue(vm, e);
  ty = rio_type(vm, e->type);
  if (ty->kind == rio_TypeKind_list) { /* the length word comes first, then the elements */
    if (e->kind == rio_ExprKind_static) {
      uint32_t v[3];
      v[0] = rio_expose(vm, (uint32_t)e->value, ty->size);
      v[1] = v[0] + 4;
      v[2] = ty->count;
      return rio_findConsts(vm, v, 3, 3);
    }
    d = rio_allocSlots(vm, 3);
    rio_emit(vm, rio_Op_lea, d, e->value, e->offset);
    rio_emit(vm, rio_Op_lea, d + 1, e->value, e->offset + 4);
    rio_emit(vm, rio_Op_mov, d + 2, rio_constSlot(vm, ty->count), 0);
    return d;
  }
  if (ty->kind != rio_TypeKind_builder) rio_FAIL(0, "expected a list");
  if (e->kind == rio_ExprKind_static) {
    if (e->value < rio_SLOTS_END - 12) return e->value >> 2; /* all three words are slots */
    rio_indirectViaConst(vm, e, 12);
  }
  d = rio_allocSlots(vm, 3);
  rio_emit(vm, rio_Op_ldw2, d, e->value, e->offset);
  rio_emit(vm, rio_Op_ldw, d + 2, e->value, e->offset + 8);
  return d;
}

/* the slots holding a slice-like or builder value */
int rio_toViewSlots(rio_Vm *vm, rio_Expr *e) {
  return rio_type(vm, e->type)->kind == rio_TypeKind_builder ? rio_toSlot3(vm, e) : rio_toSlot2(vm, e);
}

/* a struct or array: a slot holding its address */
int rio_toAddrSlot(rio_Vm *vm, rio_Expr *e) {
  int d;
  rio_needValue(vm, e);
  if (e->kind == rio_ExprKind_static) return rio_addrConst(vm, (uint32_t)e->value, rio_type(vm, e->type)->size);
  if (!e->offset) return e->value;
  d = rio_allocSlots(vm, 1);
  rio_emit(vm, rio_Op_lea, d, e->value, e->offset);
  return d;
}

/* ------------------------------------------------------------------ storing */

static void rio_storeAggregate(rio_Vm *vm, rio_Expr *dst, rio_Expr *src, const rio_Type *ty) {
  int srcAddr, dstAddr;
  if (dst->kind == rio_ExprKind_static && src->kind == rio_ExprKind_static && dst->value < rio_SLOTS_END &&
      src->value < rio_SLOTS_END && !((dst->value | src->value | (int32_t)ty->size) & 3) &&
      ty->size <= rio_SMALL_WORDS * 4) {
    /* both in slots: copy the words, without taking either address */
    if (dst->value != src->value) rio_emit(vm, rio_Op_movn, dst->value >> 2, src->value >> 2, (int)(ty->size / 4));
    return;
  }
  srcAddr = rio_toAddrSlot(vm, src);
  dstAddr = rio_toAddrSlot(vm, dst);
  if (ty->size > 0xFFFF) rio_FAIL(, "value too large to copy");
  rio_emit(vm, rio_Op_copy, dstAddr, srcAddr, (int)ty->size);
}

/* three words; ordered so an overlapping source is read before it's overwritten */
static void rio_storeBuilder(rio_Vm *vm, rio_Expr *dst, rio_Expr *src) {
  int s = rio_toSlot3(vm, src);
  if (dst->kind == rio_ExprKind_static && dst->value < rio_SLOTS_END - 12) {
    int d = dst->value >> 2;
    if (d > s) {
      rio_emit(vm, rio_Op_mov, d + 2, s + 2, 0);
      rio_emit(vm, rio_Op_mov2, d, s, 0);
    } else if (d < s) {
      rio_emit(vm, rio_Op_mov2, d, s, 0);
      rio_emit(vm, rio_Op_mov, d + 2, s + 2, 0);
    }
    return;
  }
  if (dst->kind == rio_ExprKind_static) rio_indirectViaConst(vm, dst, 12);
  rio_emit(vm, rio_Op_stw2, dst->value, s, dst->offset);
  rio_emit(vm, rio_Op_stw, dst->value, s + 2, dst->offset + 8);
}

static void rio_storeSlice(rio_Vm *vm, rio_Expr *dst, rio_Expr *src) {
  int s = rio_toSlot2(vm, src);
  if (dst->kind == rio_ExprKind_static && dst->value < rio_SLOTS_END - 4) {
    if (dst->value >> 2 != s) rio_emit(vm, rio_Op_mov2, dst->value >> 2, s, 0);
    return;
  }
  if (dst->kind == rio_ExprKind_static) rio_indirectViaConst(vm, dst, 8);
  rio_emit(vm, rio_Op_stw2, dst->value, s, dst->offset);
}

static void rio_storeScalar(rio_Vm *vm, rio_Expr *dst, rio_Expr *src, int kind) {
  int s;
  if (dst->kind == rio_ExprKind_static && kind != rio_TypeKind_byte && dst->value < rio_SLOTS_END) { /* into a slot */
    int d = dst->value >> 2;
    if (src->kind == rio_ExprKind_static && rio_type(vm, src->type)->kind != rio_TypeKind_byte &&
        src->value < rio_SLOTS_END && rio_retarget(vm, src->value >> 2, d))
      return; /* the value was just computed: computed straight into d now */
    s = rio_toSlot(vm, src, 1);
    if (s != d) rio_emit(vm, rio_Op_mov, d, s, 0);
    return;
  }
  s = rio_toSlot(vm, src, 1);
  if (dst->kind == rio_ExprKind_indirect && rio_lastIndex(vm, dst->value)) { /* fuse index + store */
    rio_Instr *in = &vm->code[vm->codeLen - 2];
    in->op = kind == rio_TypeKind_byte ? rio_Op_stxb : rio_Op_stx;
    in->a = in->b;
    in->b = in->c;
    in->c = (uint16_t)s;
    vm->code[vm->codeLen - 1].a = (uint16_t)dst->offset;
    return;
  }
  if (dst->kind == rio_ExprKind_static) rio_indirectViaConst(vm, dst, kind == rio_TypeKind_byte ? 1 : 4);
  rio_emit(vm, kind == rio_TypeKind_byte ? rio_Op_stb : rio_Op_stw, dst->value, s, dst->offset);
}

/* dst = src, for a value of dst's type */
void rio_store(rio_Vm *vm, rio_Expr *dst, rio_Expr *src) {
  rio_Type *ty = rio_type(vm, dst->type);
  int k = ty->kind;
  if (k == rio_TypeKind_slice || k == rio_TypeKind_builder) rio_storeView(vm, dst, src);
  if (k == rio_TypeKind_union && rio_storeIntoUnion(vm, dst, src)) return; /* a value of one of its types: wrapped */
  rio_coerce(vm, src, dst->type);
  if (rio_isAggregate(k)) rio_storeAggregate(vm, dst, src, ty);
  else if (k == rio_TypeKind_builder) rio_storeBuilder(vm, dst, src);
  else if (k == rio_TypeKind_slice) rio_storeSlice(vm, dst, src);
  else rio_storeScalar(vm, dst, src, k);
}

/* ------------------------------------------------------------------ conditions */

/* turns a conditional jump into the one taken in the opposite case; 0 if it isn't one */
int rio_invertJump(rio_Instr *in) {
  uint16_t t;
  switch (in->op) {
  case rio_Op_jz: in->op = rio_Op_jnz; break;
  case rio_Op_jnz: in->op = rio_Op_jz; break;
  case rio_Op_jeq: in->op = rio_Op_jne; break;
  case rio_Op_jne: in->op = rio_Op_jeq; break;
  case rio_Op_jfeq: in->op = rio_Op_jfne; break;
  case rio_Op_jfne: in->op = rio_Op_jfeq; break;
  case rio_Op_jflt: in->op = rio_Op_jfnlt; break;
  case rio_Op_jfnlt: in->op = rio_Op_jflt; break;
  case rio_Op_jfle: in->op = rio_Op_jfnle; break;
  case rio_Op_jfnle: in->op = rio_Op_jfle; break;
  case rio_Op_jlt: /* not a < b is b <= a */
  case rio_Op_jle: /* not a <= b is b < a */
    in->op = in->op == rio_Op_jlt ? rio_Op_jle : rio_Op_jlt;
    t = in->a;
    in->a = in->b;
    in->b = t;
    break;
  default: return 0;
  }
  return 1;
}

/* Emits a jump taken when Bool e is false (rio_NONE: never), fusing a compare just before it into a
   compare-and-branch. Returns the jump, to patch. */
int rio_jumpIfFalse(rio_Vm *vm, rio_Expr *e) {
  rio_needValue(vm, e);
  if (e->type != rio_TypeId_bool) rio_FAIL(0, "condition must be Bool");
  if (e->kind == rio_ExprKind_const) return e->value ? rio_NONE : rio_emit(vm, rio_Op_jmp, 0, 0, rio_NONE);
  if (e->kind == rio_ExprKind_static && e->value < rio_SLOTS_END && (e->value >> 2) >= (int)vm->compiler->liveSlots &&
      vm->codeLen && rio_canFuse(vm)) {
    rio_Instr *in = &vm->code[vm->codeLen - 1];
    if (in->a == e->value >> 2 && in->op >= rio_Op_eq && in->op <= rio_Op_fle) { /* a compare made e */
      in->op = (uint8_t)(rio_Op_jeq + (in->op - rio_Op_eq));
      in->a = in->b;
      in->b = in->c;
      in->c = rio_NONE;
      rio_invertJump(in);
      return (int)vm->codeLen - 1;
    }
  }
  return rio_emit(vm, rio_Op_jz, rio_toSlot(vm, e, 1), 0, rio_NONE);
}

#endif
