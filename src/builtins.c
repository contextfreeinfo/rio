/* builtins.c - calls to the builtins: log, math, assert, and the methods every sequence, list, enum and
   number has. Each takes the call's arguments from the value stack (from call->valueBase) and pushes its
   result. What they do: docs/spec/builtins.md. */
#include "compiler.h"
#ifndef rio_NO_COMPILER

const char *const rio_builtinNames[rio_Builtin_count] = {
  "log", "len", "min", "max", "abs", "sqrt", "sin", "cos", "tan", "asin", "acos",
  "atan", "exp", "ln", "floor", "ceil", "round", "atan2", "pow", "fmod", "assert",
  "push", "pop", "clear", "cap", "remove", "swapRemove", "pushAll", "format",
  "toInt", "toFloat", "toBool", "asString", "fromInt", "has", "hexBlob",
};

/* the builtin in [first, last] with this name, or -1 */
int rio_builtinNamed(const char *s, int n, int first, int last) {
  int k;
  for (k = first; k <= last; k++)
    if ((int)strlen(rio_builtinNames[k]) == n && !memcmp(rio_builtinNames[k], s, (size_t)n)) return k;
  return -1;
}

/* the call's result replaces its arguments */
static void rio_callResult(rio_Vm *vm, const rio_PendingOp *call, rio_Expr result) {
  vm->compiler->valueCount = call->valueBase;
  rio_pushResult(vm, result, call->freeAtStart);
}

/* a new temporary for the call's result, in its first free slot */
static int rio_resultSlot(rio_Vm *vm, const rio_PendingOp *call) {
  vm->compiler->valueCount = call->valueBase;
  vm->compiler->freeSlot = call->freeAtStart;
  return rio_allocSlots(vm, 1);
}

static int rio_argCount(rio_Vm *vm, const rio_PendingOp *call) { return vm->compiler->valueCount - call->valueBase; }

/* ------------------------------------------------------------------ log and format */

/* printing by descriptor (print.h): structs, arrays, slices, lists, builders and unions */

/* a word at the constants' end, for a descriptor */
static void rio_descPush(rio_Vm *vm, uint32_t v) {
  if (vm->constCount >= vm->constCap) rio_FAIL(, "too many constants");
  ((rio_Val *)vm->mem)[vm->constCount++].u = v;
}

/* a struct's name, from its symbol, as a string constant: its slot */
static int rio_typeNameConst(rio_Vm *vm, int type) {
  rio_Compiler *c = vm->compiler;
  int i;
  for (i = c->symCount - 1; i >= 0; i--)
    if (c->syms[i].kind == rio_SymKind_type && c->syms[i].type == type)
      return rio_stringConst(vm, c->names + c->syms[i].name, c->syms[i].nameLen).value / 4;
  return rio_stringConst(vm, "", 0).value / 4;
}

/* the descriptor of a type, for printing: its first constant (print.h says what's in one). Made the
   first time a type is printed, and kept: what a struct's or union's holds comes first, then, for a
   struct, its name's and its fields' names' strings, one after another, then its own words */
int rio_descriptor(rio_Vm *vm, int type) {
  rio_Compiler *c = vm->compiler;
  rio_Type *ty = rio_type(vm, type);
  int k = ty->kind, i, slot, names = 0, n = ty->fieldCount;
  uint32_t v[4], text;
  for (i = 0; i < c->descCount; i++)
    if (c->descType[i] == type) return c->descSlot[i];
  switch (k) {
  case rio_TypeKind_int: case rio_TypeKind_byte: case rio_TypeKind_float: case rio_TypeKind_bool: case rio_TypeKind_proc:
    v[0] = k == rio_TypeKind_int ? rio_Print_int : k == rio_TypeKind_byte ? rio_Print_byte : k == rio_TypeKind_float ? rio_Print_float
         : k == rio_TypeKind_bool ? rio_Print_bool : rio_Print_proc;
    return rio_findConsts(vm, v, 1, 0);
  case rio_TypeKind_enum:
    v[0] = rio_Print_enum; v[1] = ty->count; v[2] = ty->elem; v[3] = ty->fieldCount;
    return rio_findConsts(vm, v, 4, 0);
  case rio_TypeKind_array: case rio_TypeKind_slice: case rio_TypeKind_list: case rio_TypeKind_builder: {
    int e = rio_descriptor(vm, ty->elem), w = 0;
    text = ty->elem == rio_TypeId_byte ? rio_Print_text : 0;
    if (k == rio_TypeKind_array) { v[w++] = rio_Print_array | text; v[w++] = ty->count; }
    else v[w++] = (k == rio_TypeKind_slice ? rio_Print_slice : k == rio_TypeKind_list ? rio_Print_list : rio_Print_builder) | text;
    v[w++] = rio_type(vm, ty->elem)->size;
    v[w++] = (uint32_t)e;
    if (k == rio_TypeKind_list) v[w++] = ty->count;
    return rio_failed(vm) ? 0 : rio_findConsts(vm, v, w, 0);
  }
  case rio_TypeKind_struct: case rio_TypeKind_union:
    break;
  default:
    rio_FAIL(0, "cannot print this type");
  }
  for (i = 0; i < n; i++) { /* (what it holds, first) */
    int t = rio_field(vm, ty->firstField + i)->type;
    if (t != rio_TypeId_void) rio_descriptor(vm, t);
  }
  if (k == rio_TypeKind_struct) {
    names = rio_typeNameConst(vm, type);
    for (i = 0; i < n; i++) {
      rio_Field *f = rio_field(vm, ty->firstField + i);
      rio_stringConst(vm, c->names + f->name, f->nameLen); /* (at names + 2 + 2 * i) */
    }
  }
  if (rio_failed(vm)) return 0;
  slot = (int)vm->constCount;
  rio_descPush(vm, k == rio_TypeKind_struct ? rio_Print_struct : rio_Print_union);
  if (k == rio_TypeKind_struct) rio_descPush(vm, (uint32_t)names);
  rio_descPush(vm, (uint32_t)n);
  for (i = 0; i < n; i++) {
    rio_Field *f = rio_field(vm, ty->firstField + i);
    int d = f->type == rio_TypeId_void ? 0 : rio_descriptor(vm, f->type);
    if (k == rio_TypeKind_struct) {
      rio_descPush(vm, (uint32_t)(names + 2 + 2 * i));
      rio_descPush(vm, f->offset);
    }
    rio_descPush(vm, (uint32_t)d);
  }
  if (rio_failed(vm)) return 0;
  if (c->descCount < 32) {
    c->descType[c->descCount] = (uint16_t)type;
    c->descSlot[c->descCount++] = (uint16_t)slot;
  }
  return slot;
}

/* a value printed by descriptor, as logv and pushv take it: a slice's or builder's slots, else a slot
   holding its address (a proc value's: none, it prints as proc) */
int rio_printOperand(rio_Vm *vm, rio_Expr *e) {
  rio_Type *ty;
  int d;
  rio_needValue(vm, e);
  ty = rio_type(vm, e->type);
  if (ty->kind == rio_TypeKind_slice) return rio_toSlot2(vm, e);
  if (ty->kind == rio_TypeKind_builder) return rio_toSlot3(vm, e);
  if (ty->kind == rio_TypeKind_proc) return rio_constSlot(vm, 0);
  if (e->kind == rio_ExprKind_static) return rio_addrConst(vm, (uint32_t)e->value, ty->size);
  d = rio_allocSlots(vm, 1);
  rio_emit(vm, rio_Op_lea, d, e->value, e->offset);
  return d;
}

/* logs an enum by name */
void rio_logEnumName(rio_Vm *vm, rio_Expr *e, int space) {
  rio_Type *et = rio_type(vm, e->type);
  rio_emit(vm, rio_Op_logn, rio_toSlot(vm, e, 1), (int)et->count, et->elem);
  vm->code[vm->codeLen - 1].x = (uint8_t)space;
  rio_emitData(vm, 0, et->fieldCount);
}

/* log(...): each argument is logged as soon as it's done (after a space, but the first) */
void rio_logArg(rio_Vm *vm, rio_PendingOp *call) {
  rio_Expr e = rio_popValue(vm);
  rio_Type *ty = rio_type(vm, e.type);
  int space = call->argCount > 0;
  if (ty->kind == rio_TypeKind_slice && ty->elem == rio_TypeId_byte) rio_emitX(vm, rio_Op_logs, space, rio_toSlot2(vm, &e));
  else if (ty->kind == rio_TypeKind_bool) rio_emitX(vm, rio_Op_logb, space, rio_toSlot(vm, &e, 1));
  else if (ty->kind == rio_TypeKind_enum) rio_logEnumName(vm, &e, space);
  else if (ty->kind == rio_TypeKind_float) rio_emitX(vm, rio_Op_logf, space, rio_toSlot(vm, &e, 1));
  else if (ty->kind == rio_TypeKind_int || ty->kind == rio_TypeKind_byte) rio_emitX(vm, rio_Op_logi, space, rio_toSlot(vm, &e, 1));
  else { /* the rest, by descriptor */
    int s = rio_printOperand(vm, &e), d = rio_descriptor(vm, e.type);
    if (rio_failed(vm)) return;
    rio_emit(vm, rio_Op_logv, s, d, 0);
    vm->code[vm->codeLen - 1].x = (uint8_t)space;
  }
  vm->compiler->freeSlot = e.tempStart;
}

/* b.format(...): the first argument is the builder, whose length is kept so a format that doesn't fit
   can be undone, with an ok flag after it; each other argument is appended, and anded into the flag */
void rio_formatArg(rio_Vm *vm, rio_PendingOp *call) {
  rio_Expr e = rio_popValue(vm);
  rio_Type *ty = rio_type(vm, e.type);
  int k = ty->kind;
  if (call->argCount == 0) { /* the builder */
    int t;
    if ((k != rio_TypeKind_list && k != rio_TypeKind_builder) || ty->elem != rio_TypeId_byte) rio_FAIL(, "format needs a Blob list");
    call->c = rio_toSlot3(vm, &e);
    t = rio_allocSlots(vm, 2);
    rio_emit(vm, rio_Op_ldw, t, call->c, 0);
    rio_emit(vm, rio_Op_mov, t + 1, rio_constSlot(vm, 1), 0);
    call->fieldsSet = (uint64_t)t;
  } else {
    int ok = (int)call->fieldsSet + 1, s, d;
    if ((k == rio_TypeKind_slice || k == rio_TypeKind_array) && ty->elem == rio_TypeId_byte) {
      s = rio_toSlot2(vm, &e);
      d = rio_allocSlots(vm, 1);
      rio_emit(vm, rio_Op_pushs, d, call->c, s);
      rio_emitData(vm, 0, 1);
    } else if (k == rio_TypeKind_enum) { /* its name */
      int v = rio_toSlot(vm, &e, 1), name = rio_allocSlots(vm, 2);
      rio_emit(vm, rio_Op_enumname, name, v, (int)ty->count);
      rio_emitData(vm, ty->elem, ty->fieldCount);
      d = rio_allocSlots(vm, 1);
      rio_emit(vm, rio_Op_pushs, d, call->c, name);
      rio_emitData(vm, 0, 1);
    } else if (k == rio_TypeKind_int || k == rio_TypeKind_float || k == rio_TypeKind_bool || k == rio_TypeKind_byte) {
      s = rio_toSlot(vm, &e, 1);
      d = rio_allocSlots(vm, 1);
      rio_emit(vm, rio_Op_pusht, d, call->c, s);
      vm->code[vm->codeLen - 1].x = (uint8_t)(k == rio_TypeKind_float ? 1 : k == rio_TypeKind_bool ? 2 : 0);
    } else { /* the rest, by descriptor */
      int desc;
      s = rio_printOperand(vm, &e);
      desc = rio_descriptor(vm, e.type);
      if (rio_failed(vm)) return;
      d = rio_allocSlots(vm, 1);
      rio_emit(vm, rio_Op_pushv, d, call->c, s);
      rio_emitData(vm, 0, (uint32_t)desc);
    }
    rio_emit(vm, rio_Op_and, ok, ok, d);
    vm->compiler->freeSlot = e.tempStart;
  }
}

/* the end of format: if anything didn't fit, the old length goes back; the ok flag is the result */
static void rio_finishFormat(rio_Vm *vm, rio_PendingOp *call) {
  int t = (int)call->fieldsSet, j;
  if (call->argCount < 1) rio_FAIL(, "format needs a Blob list");
  j = rio_emit(vm, rio_Op_jnz, t + 1, 0, rio_NONE);
  rio_emit(vm, rio_Op_stw, call->c, t, 0);
  rio_patchJumps(vm, j, rio_label(vm));
  vm->compiler->freeSlot = (uint32_t)t + 2;
  rio_pushResult(vm, rio_exprOf(rio_ExprKind_static, rio_TypeId_bool, (t + 1) * 4), call->freeAtStart);
}

/* ------------------------------------------------------------------ math and len */

/* len, abs and the math functions (log ends its line) */
static void rio_mathCall(rio_Vm *vm, rio_PendingOp *call) {
  rio_Compiler *c = vm->compiler;
  int id = call->b, n = rio_argCount(vm, call), t, instr = 0, which = 0, s1, s2 = 0, d;
  rio_Expr *a = &c->values[call->valueBase], r;
  if (id == rio_Builtin_log) {
    rio_emit(vm, rio_Op_loge, 0, 0, 0);
    c->freeSlot = call->freeAtStart;
    rio_pushResult(vm, rio_voidExpr(), call->freeAtStart);
    return;
  }
  if (n != (id >= rio_Builtin_atan2 ? 2 : 1)) rio_FAIL(, "wrong number of arguments");
  if (id == rio_Builtin_len) {
    int k = rio_type(vm, a->type)->kind;
    if (k == rio_TypeKind_array) r = rio_exprOf(rio_ExprKind_const, rio_TypeId_int, (int32_t)rio_type(vm, a->type)->count);
    else if (k == rio_TypeKind_slice) { /* its second word */
      r = *a;
      rio_offsetPlace(&r, 4);
      r.type = rio_TypeId_int;
      r.access = rio_Access_readOnly;
    } else if (k == rio_TypeKind_list) { /* the length word comes first */
      r = *a;
      r.type = rio_TypeId_int;
      r.access = rio_Access_readOnly;
    } else if (k == rio_TypeKind_builder) {
      r = rio_exprOf(rio_ExprKind_indirect, rio_TypeId_int, rio_toSlot3(vm, a));
      r.access = rio_Access_readOnly;
    } else rio_FAIL(, "len() is for arrays, slices, lists and strings");
    rio_callResult(vm, call, r);
    return;
  }
  t = rio_valueType(a->type);
  if (id == rio_Builtin_abs && t == rio_TypeId_int) {
    if (a->kind == rio_ExprKind_const) {
      r = *a;
      r.value = a->value < 0 ? (int32_t)(0u - (uint32_t)a->value) : a->value;
      rio_callResult(vm, call, r);
      return;
    }
    instr = rio_Op_iabs;
  } else if (id == rio_Builtin_abs || (id >= rio_Builtin_sqrt && id <= rio_Builtin_round)) {
    rio_coerce(vm, a, rio_TypeId_float);
    instr = rio_Op_m1;
    which = id == rio_Builtin_abs ? rio_Math1_abs : rio_Math1_sqrt + (id - rio_Builtin_sqrt);
    if (a->kind == rio_ExprKind_const) { /* folded */
      rio_Val v;
      v.i = a->value;
      v.f = rio_math1[which](v.f);
      rio_callResult(vm, call, rio_exprOf(rio_ExprKind_const, rio_TypeId_float, v.i));
      return;
    }
  } else {
    rio_coerce(vm, a, rio_TypeId_float);
    rio_coerce(vm, a + 1, rio_TypeId_float);
    instr = rio_Op_m2;
    which = rio_Math2_atan2 + (id - rio_Builtin_atan2);
    s2 = rio_toSlot(vm, a + 1, 1);
  }
  s1 = rio_toSlot(vm, a, 1);
  d = rio_resultSlot(vm, call);
  rio_emit(vm, instr, d, s1, s2);
  vm->code[vm->codeLen - 1].x = (uint8_t)which;
  rio_pushResult(vm, rio_exprOf(rio_ExprKind_static, instr == rio_Op_iabs ? rio_TypeId_int : rio_TypeId_float, d * 4), call->freeAtStart);
}

/* assert(cond) or assert(cond, why): a runtime error at this line when cond is false */
static void rio_assertCall(rio_Vm *vm, rio_PendingOp *call) {
  rio_Compiler *c = vm->compiler;
  int n = rio_argCount(vm, call), message = 0, ifFalse, ifTrue;
  rio_Expr *a = &c->values[call->valueBase];
  if (n < 1 || n > 2) rio_FAIL(, "assert takes a condition, and optionally a message");
  if (n == 2) {
    if (a[1].type != rio_TypeId_string) rio_FAIL(, "assert's message is a String");
    message = rio_toSlot2(vm, a + 1);
  }
  ifFalse = rio_jumpIfFalse(vm, a);                      /* to the failure */
  ifTrue = rio_emit(vm, rio_Op_jmp, 0, 0, rio_NONE);     /* past it */
  rio_patchJumps(vm, ifFalse, rio_label(vm));
  rio_emit(vm, rio_Op_fail, message, 0, 0);
  rio_patchJumps(vm, ifTrue, rio_label(vm));
  c->valueCount = call->valueBase;
  c->freeSlot = call->freeAtStart;
  rio_pushResult(vm, rio_voidExpr(), call->freeAtStart);
}

static void rio_minMaxCall(rio_Vm *vm, rio_PendingOp *call) {
  rio_Compiler *c = vm->compiler;
  rio_Expr *a = &c->values[call->valueBase];
  int t, s1, s2, d, isMax = call->b == rio_Builtin_max;
  if (rio_argCount(vm, call) != 2) rio_FAIL(, "wrong number of arguments");
  rio_needValue(vm, a);
  rio_needValue(vm, a + 1);
  if (a[0].kind == rio_ExprKind_const && rio_valueType(a[1].type) == rio_TypeId_float) rio_coerce(vm, a, rio_TypeId_float);
  if (a[1].kind == rio_ExprKind_const && rio_valueType(a[0].type) == rio_TypeId_float) rio_coerce(vm, a + 1, rio_TypeId_float);
  t = rio_valueType(a->type);
  if (t != rio_valueType(a[1].type) || (t != rio_TypeId_int && t != rio_TypeId_float)) rio_FAIL(, "type mismatch");
  s1 = rio_toSlot(vm, a, 1);
  s2 = rio_toSlot(vm, a + 1, 1);
  d = rio_resultSlot(vm, call);
  if (t == rio_TypeId_int) rio_emit(vm, isMax ? rio_Op_imax : rio_Op_imin, d, s1, s2);
  else rio_emit(vm, isMax ? rio_Op_fmax : rio_Op_fmin, d, s1, s2);
  rio_pushResult(vm, rio_exprOf(rio_ExprKind_static, t, d * 4), call->freeAtStart);
}

/* ------------------------------------------------------------------ list methods */

/* xs.push(x): reserve an element (address 0 when full), and store into it */
static rio_Expr rio_pushCall(rio_Vm *vm, rio_PendingOp *call, rio_Expr *x, int builder, int elem, int size) {
  rio_Expr dst;
  int t = rio_allocSlots(vm, 1), full, d;
  rio_emit(vm, rio_Op_pusha, t, builder, 0);
  rio_emitData(vm, 0, (uint32_t)size);
  full = rio_emit(vm, rio_Op_jz, t, 0, rio_NONE);
  dst = rio_exprOf(rio_ExprKind_indirect, elem, t);
  rio_store(vm, &dst, x);
  rio_patchJumps(vm, full, rio_label(vm));
  vm->compiler->freeSlot = call->freeAtStart;
  d = rio_allocSlots(vm, 1);
  rio_emit(vm, rio_Op_ne, d, t, rio_constSlot(vm, 0));
  return rio_exprOf(rio_ExprKind_static, rio_TypeId_bool, d * 4);
}

/* push pop clear cap remove swapRemove pushAll */
static void rio_listCall(rio_Vm *vm, rio_PendingOp *call) {
  static const signed char args[] = {2, 1, 1, 1, 2, 2, 2}; /* counting the list itself */
  rio_Compiler *c = vm->compiler;
  int id = call->b, k, elem, builder, d, s, size;
  rio_Expr *a = &c->values[call->valueBase], r = rio_voidExpr();
  if (rio_argCount(vm, call) != args[id - rio_Builtin_push]) rio_FAIL(, "wrong number of arguments");
  rio_needValue(vm, a);
  k = rio_type(vm, a->type)->kind;
  if (id == rio_Builtin_cap) {
    if (k == rio_TypeKind_list || k == rio_TypeKind_array) r = rio_exprOf(rio_ExprKind_const, rio_TypeId_int, (int32_t)rio_type(vm, a->type)->count);
    else if (k == rio_TypeKind_builder) {
      r = rio_exprOf(rio_ExprKind_static, rio_TypeId_int, (rio_toSlot3(vm, a) + 2) * 4);
      r.access = rio_Access_readOnly;
    } else if (k == rio_TypeKind_slice) {
      r = *a;
      rio_offsetPlace(&r, 4);
      r.type = rio_TypeId_int;
      r.access = rio_Access_readOnly;
    } else rio_FAIL(, "cap() is for lists, arrays and slices");
    rio_callResult(vm, call, r);
    return;
  }
  if (k != rio_TypeKind_list && k != rio_TypeKind_builder) rio_FAIL(, "expected a list");
  elem = rio_type(vm, a->type)->elem;
  size = (int)rio_type(vm, elem)->size;
  builder = rio_toSlot3(vm, a);
  if (id == rio_Builtin_clear) rio_emit(vm, rio_Op_stw, builder, rio_constSlot(vm, 0), 0);
  else if (id == rio_Builtin_pop) {
    c->freeSlot = call->freeAtStart;
    d = rio_allocSlots(vm, 1);
    rio_emit(vm, rio_Op_popa, d, builder, 0);
    rio_emitData(vm, 0, (uint32_t)size);
    r = rio_exprOf(rio_ExprKind_indirect, elem, d);
  } else if (id == rio_Builtin_remove || id == rio_Builtin_swapRemove) {
    rio_coerce(vm, a + 1, rio_TypeId_int);
    s = rio_toSlot(vm, a + 1, 1);
    rio_emit(vm, id == rio_Builtin_remove ? rio_Op_lrem : rio_Op_lswap, builder, s, 0);
    rio_emitData(vm, 0, (uint32_t)size);
  } else if (id == rio_Builtin_pushAll) {
    rio_Expr *x = a + 1;
    int xk;
    rio_needValue(vm, x);
    xk = rio_type(vm, x->type)->kind;
    if (xk == rio_TypeKind_list || xk == rio_TypeKind_builder) { /* its live elements */
      int other = rio_toSlot3(vm, x);
      s = rio_allocSlots(vm, 2);
      rio_emit(vm, rio_Op_lview, s, other, 0);
    } else if (xk == rio_TypeKind_slice || xk == rio_TypeKind_array) s = rio_toSlot2(vm, x);
    else rio_FAIL(, "pushAll needs a slice, array or list");
    if (rio_type(vm, x->type)->elem != elem) rio_FAIL(, "type mismatch");
    c->freeSlot = call->freeAtStart;
    d = rio_allocSlots(vm, 1);
    rio_emit(vm, rio_Op_pushs, d, builder, s);
    rio_emitData(vm, 0, (uint32_t)size);
    r = rio_exprOf(rio_ExprKind_static, rio_TypeId_bool, d * 4);
  } else r = rio_pushCall(vm, call, a + 1, builder, elem, size);
  if (r.kind == rio_ExprKind_void) c->freeSlot = call->freeAtStart;
  rio_callResult(vm, call, r);
}

/* ------------------------------------------------------------------ hexBlob, enums, conversions */

static int rio_hexDigit(int c) {
  return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
}

/* hexBlob("0a 1b ..."): the bytes those hex digits spell, decoded while compiling, as a Blob of their own
   (writable: a call gives the same bytes each time it runs, like a global). Only a literal: the string's
   bytes are decoded in place, then the Blob takes over its constant. */
static void rio_hexBlobCall(rio_Vm *vm, rio_PendingOp *call) {
  rio_Compiler *c = vm->compiler;
  rio_Val *R = (rio_Val *)vm->mem;
  rio_Expr *a = &c->values[call->valueBase], r;
  uint32_t k, at, n, i, len = 0;
  int high = -1;
  if (call->argCount != 1) rio_FAIL(, "hexBlob takes one string literal");
  k = (uint32_t)a->value / 4;
  if (a->kind != rio_ExprKind_static || a->type != rio_TypeId_string || !a->access || k + 1 >= vm->constCount ||
      !rio_bitGet(c->constIsString, k))
    rio_FAIL(, "hexBlob takes a string literal (it's decoded while compiling)");
  at = R[k].u & 0x7FFFFFFFu;
  n = R[k + 1].u;
  for (i = 0; i < n; i++) {
    int ch = c->pool[at + i], d = rio_hexDigit(ch);
    if (d < 0) {
      if (rio_isSpace(ch)) continue;
      rio_FAIL(, "hexBlob: only hex digits and spaces");
    }
    if (high < 0) high = d;
    else {
      c->pool[at + len++] = (uint8_t)(high << 4 | d);
      high = -1;
    }
  }
  if (high >= 0) rio_FAIL(, "hexBlob: an odd number of hex digits");
  R[k + 1].u = len;
  if (c->poolUsed == at + n) c->poolUsed = at + len; /* (the literal was the last thing pooled: give back what's left) */
  r = *a;
  r.type = rio_TypeId_blob;
  r.access = 0;
  c->valueCount = call->valueBase;
  c->freeSlot = call->freeAtStart;
  rio_pushResult(vm, r, call->freeAtStart);
}

/* Enum.fromInt(n): checked; Enum.has(n) -> Bool */
static void rio_enumCall(rio_Vm *vm, rio_PendingOp *call) {
  rio_Compiler *c = vm->compiler;
  rio_Expr *a = &c->values[call->valueBase];
  rio_Type *et = rio_type(vm, call->c);
  int s, d;
  if (rio_argCount(vm, call) != 1) rio_FAIL(, "wrong number of arguments");
  rio_coerce(vm, a, rio_TypeId_int);
  if (a->kind == rio_ExprKind_const) { /* known now: check now */
    int found = rio_enumIndex(vm, call->c, a->value) >= 0;
    if (call->b == rio_Builtin_fromInt && !found) rio_FAIL(, "not a value of this enum");
    rio_callResult(vm, call, call->b == rio_Builtin_fromInt ? rio_exprOf(rio_ExprKind_const, call->c, a->value)
                                                            : rio_exprOf(rio_ExprKind_const, rio_TypeId_bool, found));
    return;
  }
  s = rio_toSlot(vm, a, 1);
  d = rio_resultSlot(vm, call);
  if (call->b == rio_Builtin_fromInt) {
    rio_emit(vm, rio_Op_mov, d, s, 0);
    rio_emit(vm, rio_Op_enumck, 0, d, (int)et->count);
    vm->code[vm->codeLen - 1].x = 1;
    rio_emitData(vm, 0, et->fieldCount);
    rio_pushResult(vm, rio_exprOf(rio_ExprKind_static, call->c, d * 4), call->freeAtStart);
  } else {
    rio_emit(vm, rio_Op_enumck, d, s, (int)et->count);
    rio_emitData(vm, 0, et->fieldCount);
    rio_pushResult(vm, rio_exprOf(rio_ExprKind_static, rio_TypeId_bool, d * 4), call->freeAtStart);
  }
}

/* x.toInt() x.toFloat() x.toBool() b.asString() */
static void rio_convertCall(rio_Vm *vm, rio_PendingOp *call) {
  rio_Compiler *c = vm->compiler;
  rio_Expr *a = &c->values[call->valueBase], r;
  int from, s, d;
  int to = call->b == rio_Builtin_toInt ? rio_TypeId_int
           : call->b == rio_Builtin_toFloat ? rio_TypeId_float
           : call->b == rio_Builtin_toBool ? rio_TypeId_bool
                                           : rio_TypeId_string;
  if (rio_argCount(vm, call) != 1) rio_FAIL(, "conversions take no arguments");
  rio_needValue(vm, a);
  from = rio_valueType(a->type);
  r = *a;
  if (to == from) { /* nothing to do */
  } else if (to == rio_TypeId_string && from == rio_TypeId_blob) { /* the same bytes, read-only */
    r.type = rio_TypeId_string;
    r.access = rio_Access_readOnly;
  } else if (to == rio_TypeId_int && from == rio_TypeId_bool) r.type = rio_TypeId_int;
  else if (to == rio_TypeId_bool && from == rio_TypeId_int) {
    if (a->kind == rio_ExprKind_const) r = rio_exprOf(rio_ExprKind_const, rio_TypeId_bool, a->value != 0);
    else {
      s = rio_toSlot(vm, a, 1);
      c->freeSlot = call->freeAtStart;
      d = rio_allocSlots(vm, 1);
      rio_emit(vm, rio_Op_ne, d, s, rio_constSlot(vm, 0));
      r = rio_exprOf(rio_ExprKind_static, rio_TypeId_bool, d * 4);
    }
  } else if (to == rio_TypeId_int && rio_type(vm, from)->kind == rio_TypeKind_enum) r.type = rio_TypeId_int; /* an enum is its Int */
  else if ((to == rio_TypeId_float && from == rio_TypeId_int) || (to == rio_TypeId_int && from == rio_TypeId_float)) {
    if (a->kind == rio_ExprKind_const) {
      rio_Val v;
      v.i = a->value;
      if (to == rio_TypeId_float) v.f = (float)v.i;
      else v.i = rio_floatToInt(v.f);
      r = rio_exprOf(rio_ExprKind_const, to, v.i);
    } else {
      s = rio_toSlot(vm, a, 1);
      c->freeSlot = call->freeAtStart;
      d = rio_allocSlots(vm, 1);
      rio_emit(vm, to == rio_TypeId_float ? rio_Op_itof : rio_Op_ftoi, d, s, 0);
      r = rio_exprOf(rio_ExprKind_static, to, d * 4);
    }
  } else rio_FAIL(, "no such conversion");
  rio_callResult(vm, call, r);
}

void rio_finishBuiltinCall(rio_Vm *vm, rio_PendingOp *call) {
  int id = call->b;
  if (id == rio_Builtin_min || id == rio_Builtin_max) rio_minMaxCall(vm, call);
  else if (id == rio_Builtin_assert) rio_assertCall(vm, call);
  else if (id == rio_Builtin_format) rio_finishFormat(vm, call);
  else if (id == rio_Builtin_hexBlob) rio_hexBlobCall(vm, call);
  else if (id >= rio_Builtin_fromInt) rio_enumCall(vm, call);
  else if (id >= rio_Builtin_toInt) rio_convertCall(vm, call);
  else if (id >= rio_Builtin_push) rio_listCall(vm, call);
  else rio_mathCall(vm, call);
}

#endif
