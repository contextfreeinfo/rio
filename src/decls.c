/* decls.c - types as written, and declarations: procs and methods, structs, unions and enums, and
   variables (with destructuring). The rules: docs/spec/declarations.md and docs/spec/types.md. */
#include "compiler.h"
#ifndef rio_NO_COMPILER

/* ------------------------------------------------------------------ types */

static int rio_parseValueType(rio_Vm *vm);

/* proc(T, U) -> R, or any other type */
/* At a proc type's parameter: how many names come before its type, as in a proc's own parameters (a,
   b: Int, or c?: Int, optional), or 0 for none (Int, Float: just types). Names are for reading; procs
   take arguments by position. */
static int rio_paramNames(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  const char *p, *e;
  int count = 0;
  if (c->tok.kind != rio_Tok_id) return 0;
  p = rio_lookAhead(vm);
  e = c->srcEnd;
  for (;;) {
    if (p >= e || !rio_isAlpha(*p)) return 0;
    while (p < e && (rio_isAlpha(*p) || rio_isDigit(*p))) p++;
    count++;
    while (p < e && rio_isSpace(*p)) p++;
    if (p < e && *p == '?') /* (optional) */
      for (p++; p < e && rio_isSpace(*p); p++) {}
    if (p < e && *p == ',') {
      for (p++; p < e && rio_isSpace(*p); p++) {}
      continue;
    }
    return p + 1 < e && p[0] == ':' && p[1] != ':' && p[1] != '=' ? count : 0;
  }
}

static int rio_genericOk(rio_Vm *vm, int t, int top);

/* proc[T, U](...) after Name ::, a generic proc type: its Ts named while it's read (each a type of a
   proc that isn't one). How many, or 0 for none. */
static int rio_typeParamsOfType(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  int k = 0, exporting = c->exporting;
  if (c->tok.kind != '[' || c->tok.newline) return 0;
  if (!c->namingType) rio_FAIL_AT_TOKEN(0, "a generic proc type is only named: Less :: (proc[T](a, b: &T) -> Bool)");
  c->exporting = 0; /* (that's the type's name, after) */
  rio_next(vm);
  while (c->tok.kind == rio_Tok_id && !rio_failed(vm)) {
    if (k >= 8) rio_FAIL(0, "too many type parameters");
    rio_addSymbol(vm, c->tok.text, c->tok.len, rio_SymKind_type, rio_newType(vm, rio_TypeKind_param, rio_NO_PROC, (uint32_t)k, 0), 0);
    k++;
    rio_next(vm);
    if (c->tok.kind != ',') break;
    rio_next(vm);
  }
  c->exporting = exporting;
  if (!k) rio_FAIL_AT_TOKEN(0, "a type parameter's name expected: proc[T](...)");
  rio_expect(vm, ']', "']' expected");
  return k;
}

int rio_parseType(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  int params[16], n = 0, result = rio_TypeId_void, optional = 0, refs = 0, ref, symAt = c->symCount, generic, made;
  if (c->tok.kind != rio_Tok_proc) return rio_parseValueType(vm);
  rio_next(vm);
  generic = rio_typeParamsOfType(vm);
  c->namingType = 0;
  rio_expect(vm, '(', "'(' expected");
  while (c->tok.kind != ')' && !rio_failed(vm)) {
    int names = rio_paramNames(vm), t, j, opt = 0; /* a, b: T, c?: T, or just T */
    if (n >= 16) rio_FAIL(0, "too many parameters");
    for (j = 0; j < names; j++) { /* (each name, a ? if it's optional, and the , or : after it) */
      rio_next(vm);
      if (c->tok.kind == '?') {
        opt |= 1 << j;
        rio_next(vm);
      }
      rio_next(vm);
    }
    if ((ref = c->tok.kind == '&')) rio_next(vm);
    if (c->tok.kind == rio_Tok_proc) rio_FAIL_AT_TOKEN(0, "a proc type can't take or return procs");
    t = rio_parseValueType(vm);
    for (j = 0; j < (names ? names : 1); j++) {
      if (n >= 16) rio_FAIL(0, "too many parameters");
      if (ref && (opt >> j & 1)) rio_FAIL(0, "a & parameter can't be optional: it needs a place");
      refs |= ref << n;
      if (opt >> j & 1) optional++;
      else if (optional) rio_FAIL(0, rio_msgOptionalLast);
      params[n++] = t;
    }
    if (c->tok.kind != ',') break;
    rio_next(vm);
  }
  rio_expect(vm, ')', "')' expected");
  if (c->tok.kind == rio_Tok_arrow) {
    rio_next(vm);
    if (c->tok.kind == rio_Tok_proc) rio_FAIL_AT_TOKEN(0, "a proc type can't take or return procs");
    result = rio_parseValueType(vm);
  }
  made = rio_procTypeOf(vm, params, n, result, optional, refs);
  if (!generic) return made;
  c->symCount = symAt; /* (its Ts were named only in it) */
  if (!rio_genericOk(vm, made, 0)) rio_FAIL(0, "a T goes here only as &T or []T, in a proc type too (its size is known only while running)");
  return rio_newType(vm, rio_TypeKind_generic, made, (uint32_t)generic, 0);
}

/* a type by its name, maybe through modules (mod.Type), and Blob[N], Blob[..N], Blob[..] */
static int rio_parseTypeName(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  int i, t;
  if (c->tok.kind != rio_Tok_id || (i = rio_lookup(vm, c->tok.text, c->tok.len)) < 0) rio_FAIL_AT_TOKEN(0, "type expected");
  while (c->syms[i].kind == rio_SymKind_module) { /* mod.Type */
    int module = c->syms[i].value;
    rio_next(vm);
    if (c->tok.kind != '.') rio_FAIL(0, "'.' expected");
    rio_next(vm);
    if (c->tok.kind != rio_Tok_id) rio_FAIL_AT_TOKEN(0, "type expected");
    if ((i = rio_lookupExport(vm, module, c->tok.text, c->tok.len)) < 0) rio_FAIL_AT_TOKEN(0, "not exported by that module");
  }
  if (c->syms[i].kind != rio_SymKind_type) rio_FAIL_AT_TOKEN(0, "type expected");
  t = c->syms[i].type;
  rio_next(vm);
  if (rio_type(vm, t)->kind == rio_TypeKind_generic) { /* Less[Point]: filled in */
    int args[8], n = 0;
    if (c->tok.kind != '[' || c->tok.newline) rio_FAIL(0, "this type takes types: Name[T]");
    rio_next(vm);
    while (!rio_failed(vm)) {
      if (n >= 8) rio_FAIL(0, "too many types");
      args[n++] = rio_parseType(vm);
      if (c->tok.kind != ',') break;
      rio_next(vm);
    }
    rio_expect(vm, ']', "']' expected");
    if (rio_failed(vm)) return 0;
    if ((uint32_t)n != rio_type(vm, t)->count) rio_FAIL(0, "wrong number of types");
    return rio_bindType(vm, -1, rio_type(vm, t)->elem, args);
  }
  if ((t == rio_TypeId_int || t == rio_TypeId_bool || rio_type(vm, t)->kind == rio_TypeKind_enum) && c->tok.kind == '(' && !c->tok.newline)
    rio_FAIL(0, "a bitfield (Int(n), Bool(1), or an enum's E(n)) is only a struct's field");
  if (t == rio_TypeId_blob && c->tok.kind == '[' && !c->tok.newline) { /* Blob[N], Blob[..N], Blob[..] */
    rio_next(vm);
    if (c->tok.kind == '.' && c->peek.kind == '.') {
      rio_next(vm);
      rio_next(vm);
      t = c->tok.kind == ']' ? rio_builderViewOf(vm, rio_TypeId_byte) : rio_builderOf(vm, rio_TypeId_byte, (uint32_t)rio_parsePositiveConst(vm));
    } else t = rio_arrayOf(vm, rio_TypeId_byte, (uint32_t)rio_parsePositiveConst(vm));
    rio_expect(vm, ']', "']' expected");
  }
  return t;
}

/* Any type but a bare proc type: prefixes ([N] [] [..N] [..] [Enum] ?), then a name, maybe through modules
   (and Blob[N], Blob[..N], Blob[..]), or after prefixes a proc type: [4]proc(Float) -> Float. The
   prefixes apply innermost last: [2][3]T is 2 of [3]T, and
   ?[]?Int a maybe slice of maybe Ints. */
static int rio_parseValueType(rio_Vm *vm) {
  /* per prefix: n > 0 an array of n; -1 a slice; -2 a builder view; -3 - n a builder of n; and an enum, if any;
     or a maybe */
  rio_Compiler *c = vm->compiler;
  int32_t prefix[16];
  int count = 0, t, enumOf[16];
  uint8_t maybe[16];
  while (c->tok.kind == '[' || c->tok.kind == '?') {
    if (count >= 16) rio_FAIL(0, "type too deep");
    maybe[count] = c->tok.kind == '?';
    enumOf[count] = 0;
    rio_next(vm);
    if (maybe[count]) {
      prefix[count++] = 0;
      continue;
    }
    if (c->tok.kind == ']') prefix[count++] = -1;
    else if (c->tok.kind == '.' && c->peek.kind == '.') {
      rio_next(vm);
      rio_next(vm);
      prefix[count++] = c->tok.kind == ']' ? -2 : -3 - rio_parsePositiveConst(vm);
    } else { /* [N]T, [N * 2]T, [m.N + 1]T (constant Int arithmetic folds), or [Enum]T, [m.Enum]T */
      rio_Expr e;
      if (c->tok.kind == rio_Tok_id && c->tok.len == 1 && c->tok.text[0] == '_' && c->peek.kind == ']')
        rio_FAIL_AT_TOKEN(0, "[_] is for a literal, sized by its values: [_]T{a, b}");
      e = rio_parseExpr(vm);
      if (e.kind == rio_ExprKind_type && rio_type(vm, e.type)->kind == rio_TypeKind_enum) {
        enumOf[count] = e.type;
        prefix[count++] = 0;
      } else if (e.kind == rio_ExprKind_const && e.type == rio_TypeId_int && e.value > 0) prefix[count++] = e.value;
      else rio_FAIL(0, "an array size is a positive Int constant or an enum");
    }
    rio_expect(vm, ']', "']' expected");
  }
  if (c->tok.kind == rio_Tok_proc && count && maybe[count - 1]) rio_FAIL_AT_TOKEN(0, "a proc value can't be a maybe: it's unset until given a proc");
  t = c->tok.kind == rio_Tok_proc && count ? rio_parseType(vm) : rio_parseTypeName(vm); /* ([4]proc(Float) -> Float: proc values as elements) */
  while (count) {
    int32_t n = prefix[--count];
    if (maybe[count]) t = rio_maybeOf(vm, t);
    else if (enumOf[count]) t = rio_enumArrayOf(vm, t, enumOf[count]);
    else if (n == -1) t = rio_sliceOf(vm, t);
    else if (n == -2) t = rio_builderViewOf(vm, t);
    else if (n < -2) t = rio_builderOf(vm, t, (uint32_t)(-3 - n));
    else t = rio_arrayOf(vm, t, (uint32_t)n);
  }
  return t;
}

/* `a, b, c:`, the names before a field's or parameter's type; returns how many */
int rio_parseNames(rio_Vm *vm, const char **names, int *lens, int *optional) {
  rio_Compiler *c = vm->compiler;
  int n = 0;
  if (optional) *optional = 0;
  for (;;) {
    if (c->tok.kind != rio_Tok_id || n >= 16) rio_FAIL(0, "name expected");
    names[n] = rio_keep(vm, c->tok.text, c->tok.len);
    lens[n++] = c->tok.len;
    rio_next(vm);
    if (c->tok.kind == '?') { /* name?: optional (a parameter's) */
      if (!optional) rio_FAIL_AT_TOKEN(0, "only a parameter can be optional (name?: T)");
      *optional |= 1 << (n - 1);
      rio_next(vm);
    }
    if (c->tok.kind != ',') break;
    rio_next(vm);
  }
  rio_expect(vm, ':', "':' expected");
  return n;
}

/* ------------------------------------------------------------------ procs */

/* a parameter of the proc being defined (the caller checks there's room): addr its slot, refType the T
   of a &T parameter (else 0) */
static void rio_addParam(rio_Vm *vm, int type, int refType, int addr) {
  rio_Compiler *c = vm->compiler;
  rio_Param *param = &c->params[c->paramCount++];
  param->type = (uint16_t)type;
  param->refType = (uint16_t)refType;
  param->addr = (uint32_t)addr;
}

/* The start of a proc (receiver: a method's type, or -1): a jump for top-level code to go around it, its
   entry, and its frame, with self first for a method. Returns the proc, or -1. */
int rio_beginProc(rio_Vm *vm, const char *name, int nameLen, int receiver) { return rio_beginProcAt(vm, -1, name, nameLen, receiver, 0); }

/* the same, at index at, kept for it already (a host proc's, used as a value: rio_ffiProcValue), or -1;
   isStatic: a method with no self */
int rio_beginProcAt(rio_Vm *vm, int at, const char *name, int nameLen, int receiver, int isStatic) {
  rio_Compiler *c = vm->compiler;
  rio_ProcDef *p;
  rio_Block *b;
  int index, skip = rio_emit(vm, rio_Op_jmp, 0, 0, rio_NONE); /* top-level code goes around it */
  index = at >= 0 ? at : vm->procCount++;
  p = &c->procDefs[index];
  p->firstParam = (uint16_t)c->paramCount;
  p->paramCount = 0;
  p->defined = 0;
  p->resultType = rio_TypeId_void;
  p->resultAt = 0;
  p->bigBytes = 0;
  p->shared = 0;
  p->bigTop = vm->top;
  p->callsValues = p->usedAsValue = 0;
  p->optional = 0;
  p->typeParams = p->viewTypes = 0;
  p->isStatic = (uint8_t)isStatic;
  rio_addSymbol(vm, name, nameLen, receiver < 0 ? rio_SymKind_proc : rio_SymKind_method, receiver < 0 ? 0 : receiver, index);
  if (isStatic) receiver = -1; /* (no self) */
  p->selfByRef = (uint8_t)(receiver >= 0 && rio_type(vm, receiver)->kind == rio_TypeKind_struct);
  b = rio_pushBlock(vm, rio_Block_proc);
  b->a = (uint16_t)skip;
  b->b = (uint16_t)index;
  /* a frame starts above every slot used so far, so each slot has one owner (top-level code or one proc):
     frame sharing moves a proc's slots, and must not move a dead top-level block's with them */
  c->liveSlots = c->freeSlot = c->slotHigh;
  p->frameStart = (uint16_t)c->liveSlots;
  if (receiver >= 0) { /* self: a struct receiver by address, other receivers as a copy */
    int addr = rio_allocSlots(vm, p->selfByRef ? 1 : rio_typeWords(vm, receiver)) * 4;
    if (c->paramCount >= (int)c->limits.params) rio_FAIL(-1, "too many parameters");
    rio_addParam(vm, p->selfByRef ? rio_TypeId_int : receiver, 0, addr);
    if (p->selfByRef) rio_markParamView(vm, (uint32_t)addr, 1, 1); /* (the caller's place, or views from it: borrowed) */
    else rio_markHeld(vm, receiver, (uint32_t)addr, 0, 1);
    rio_addSymbol(vm, "self", 4, p->selfByRef ? rio_SymKind_ref : rio_SymKind_var, receiver, p->selfByRef ? addr / 4 : addr);
    p->paramCount++;
  }
  return index;
}

/* a parameter of the proc being begun: name: type, or name: &type (how & 1), or name?: type (how & 2:
   optional, so a call may leave it out, and then it's zero; only the last ones) */
void rio_procParam(rio_Vm *vm, int proc, const char *name, int nameLen, int type, int how) {
  rio_Compiler *c = vm->compiler;
  rio_Type *ty = rio_type(vm, type);
  int addr, isRef = how & 1;
  if (how & 2) {
    if (isRef) rio_FAIL(, "a & parameter can't be optional: it needs a place");
    c->procDefs[proc].optional++;
  } else if (c->procDefs[proc].optional) rio_FAIL(, rio_msgOptionalLast);
  if (ty->kind == rio_TypeKind_void) rio_FAIL(, "invalid parameter type");
  if (!isRef && ty->kind == rio_TypeKind_builder) rio_FAIL(, "invalid parameter type (pass builders as [..]T)");
  if (!isRef && rio_isBig(ty)) rio_FAIL(, "a big value (over 16 words) goes by reference: use &T, or []T for an array");
  if (ty->kind == rio_TypeKind_param && !isRef) rio_FAIL(, "a T goes by reference: x: &T, or xs: []T");
  addr = rio_allocSlots(vm, isRef ? 1 : rio_typeWords(vm, type)) * 4; /* a & parameter is one word holding the address,
                                                                     read through like self */
  if (c->paramCount >= (int)c->limits.params) rio_FAIL(, "too many parameters");
  rio_addParam(vm, isRef ? rio_TypeId_int : type, isRef ? type : 0, addr);
  if (isRef) rio_markParamView(vm, (uint32_t)addr, 1, 1); /* (the caller's place, or views from the caller: borrowed) */
  else rio_markHeld(vm, type, (uint32_t)addr, 0, 1);
  rio_addSymbol(vm, name, nameLen, isRef ? rio_SymKind_ref : rio_SymKind_var, type, isRef ? addr / 4 : addr);
  c->procDefs[proc].paramCount++;
}

/* the result of the proc being begun */
void rio_setProcResult(rio_Vm *vm, int proc, int type) {
  rio_ProcDef *p = &vm->compiler->procDefs[proc];
  rio_Type *ty = rio_type(vm, type);
  p->resultType = (uint16_t)type;
  if (ty->kind == rio_TypeKind_builder) rio_FAIL(, "invalid return type");
  if (rio_isBig(ty)) rio_FAIL(, "a big value (over 16 words) can't be returned: fill in a &T parameter instead");
  p->resultAt = (uint32_t)rio_allocSlots(vm, rio_typeWords(vm, type)) * 4;
}

/* after its parameters and result: the body starts here */
void rio_beginBody(rio_Vm *vm, int proc) {
  rio_Compiler *c = vm->compiler;
  c->liveSlots = c->freeSlot;
  c->curProc = proc;
  c->unreachable = 0;
  c->procDefs[proc].entry = (uint16_t)rio_label(vm);
}

/* type t mentions a generic proc's type parameter (which: type, or any if type < 0) */
int rio_mentions(rio_Vm *vm, int t, int type) {
  rio_Type *y = rio_type(vm, t);
  uint32_t i;
  switch (y->kind) {
  case rio_TypeKind_param: return type < 0 || t == type;
  case rio_TypeKind_slice: case rio_TypeKind_array: case rio_TypeKind_builder: case rio_TypeKind_builderView: return rio_mentions(vm, y->elem, type);
  case rio_TypeKind_union: case rio_TypeKind_proc: case rio_TypeKind_struct:
    for (i = 0; i < y->fieldCount; i++)
      if (rio_mentions(vm, rio_field(vm, y->firstField + i)->type, type)) return 1;
    return y->kind == rio_TypeKind_proc && rio_mentions(vm, y->elem, type);
  default: return 0;
  }
}

/* A type a generic proc may name, inside its parameters' types and its result's: a T's size is known
   only while running, so a T is only somewhere (top: a parameter itself, T or &T), in a []T, or behind
   one in a proc type's parameters */
static int rio_genericOk(rio_Vm *vm, int t, int top) {
  rio_Type *y = rio_type(vm, t);
  uint32_t i;
  if (!rio_mentions(vm, t, -1)) return 1;
  if (y->kind == rio_TypeKind_param) return top;
  if (y->kind == rio_TypeKind_slice) return rio_type(vm, y->elem)->kind == rio_TypeKind_param || rio_genericOk(vm, y->elem, 0);
  if (y->kind == rio_TypeKind_struct) { /* (a generic struct's, filled in with Ts: its fields are) */
    for (i = 0; i < y->fieldCount; i++)
      if (!rio_genericOk(vm, rio_field(vm, y->firstField + i)->type, 0)) return 0;
    return 1;
  }
  if (y->kind != rio_TypeKind_proc) return 0;
  for (i = 0; i < y->fieldCount; i++) /* (a proc type's &T: offset 1) */
    if (!rio_genericOk(vm, rio_field(vm, y->firstField + i)->type, rio_field(vm, y->firstField + i)->offset)) return 0;
  return rio_genericOk(vm, y->elem, 0);
}

/* After a generic proc's parameters and result: each checked, each T given by one (so a call can tell
   what it is), and a hidden parameter for each T: where its size is */
static void rio_genericParams(rio_Vm *vm, int proc) {
  rio_Compiler *c = vm->compiler;
  rio_ProcDef *p = &c->procDefs[proc];
  int i, j;
  for (i = 0; i < p->paramCount; i++) {
    rio_Param *q = &c->params[p->firstParam + i];
    if (!rio_genericOk(vm, q->refType ? q->refType : q->type, 1))
      rio_FAIL(, "a T goes here only as &T or []T, in a proc type too (its size is known only while running)");
  }
  for (j = 0; j < p->typeParams; j++) {
    int t = p->firstTypeParam + j, found = 0;
    for (i = 0; i < p->paramCount && !found; i++) {
      rio_Param *q = &c->params[p->firstParam + i];
      found = rio_mentions(vm, q->refType ? q->refType : q->type, t);
    }
    if (!found) rio_FAIL(, "each type parameter is in a parameter's type, so a call can tell what it is");
  }
  for (j = 0; j < p->typeParams; j++) {
    int addr = rio_allocSlots(vm, 1) * 4;
    if (c->paramCount >= (int)c->limits.params) rio_FAIL(, "too many parameters");
    rio_addParam(vm, rio_TypeId_int, 0, addr);
    p->paramCount++;
  }
}

/* at a generic proc's start: each T's size, from where its hidden parameter says, into a slot */
static void rio_loadTypeSizes(rio_Vm *vm, int proc) {
  rio_Compiler *c = vm->compiler;
  rio_ProcDef *p = &c->procDefs[proc];
  int j, s = rio_allocSlots(vm, p->typeParams), hidden = p->paramCount - p->typeParams;
  c->liveSlots = c->freeSlot;
  p->sizeSlots = (uint16_t)s;
  for (j = 0; j < p->typeParams; j++) rio_emit(vm, rio_Op_ldw, s + j, (int)(c->params[p->firstParam + hidden + j].addr / 4), 0);
}

/* name :: proc(params) -> result, after the name and ::; receiver: a method's type, or -1 (isStatic: one
   with no self). The body follows as statements, until its end (in rio_statement). */
void rio_procDef(rio_Vm *vm, const char *name, int nameLen, int receiver, int isStatic) {
  rio_Compiler *c = vm->compiler;
  const char *typeNames[8];
  int index, typeLens[8], typeCount = 0, k;
  if (c->blockCount || c->curProc >= 0) rio_FAIL(, "procs must be top-level");
  if (vm->procCount >= (int)c->limits.procs) rio_FAIL(, "too many procs");
  if (receiver >= 0 && rio_type(vm, receiver)->kind == rio_TypeKind_generic) rio_FAIL(, "a generic struct's methods aren't ready yet: write a generic proc taking it");
  rio_next(vm);
  if (c->tok.kind == '[' && !c->tok.newline) { /* proc[T, U]: generic, the types named for the proc */
    rio_next(vm);
    while (c->tok.kind == rio_Tok_id && !rio_failed(vm)) {
      if (typeCount >= 8) rio_FAIL(, "too many type parameters");
      for (k = 0; k < typeCount; k++)
        if (typeLens[k] == c->tok.len && !memcmp(typeNames[k], c->tok.text, (size_t)c->tok.len)) rio_FAIL_AT_TOKEN(, "type parameter named twice");
      typeNames[typeCount] = rio_keep(vm, c->tok.text, c->tok.len);
      typeLens[typeCount++] = c->tok.len;
      rio_next(vm);
      if (c->tok.kind != ',') break;
      rio_next(vm);
    }
    if (!typeCount) rio_FAIL_AT_TOKEN(, "a type parameter's name expected: proc[T](...)");
    rio_expect(vm, ']', "']' expected");
    if (receiver >= 0) rio_FAIL(, "a method can't be generic yet");
  }
  rio_expect(vm, '(', "'(' expected");
  if ((index = rio_beginProcAt(vm, -1, name, nameLen, receiver, isStatic)) < 0) return;
  for (k = 0; k < typeCount; k++) { /* (in the proc's scope: they go at its end) */
    int t = rio_newType(vm, rio_TypeKind_param, index, (uint32_t)k, 0);
    if (!k) c->procDefs[index].firstTypeParam = (uint16_t)t;
    rio_addSymbol(vm, typeNames[k], typeLens[k], rio_SymKind_type, t, 0);
  }
  c->procDefs[index].typeParams = (uint8_t)typeCount;
  while (c->tok.kind != ')' && !rio_failed(vm)) { /* a, b: T  or  a: &T */
    const char *names[16];
    int lens[16], opt, n = rio_parseNames(vm, names, lens, &opt), isRef = c->tok.kind == '&', t, j;
    if (isRef) rio_next(vm);
    t = rio_parseType(vm);
    for (j = 0; j < n && !rio_failed(vm); j++) rio_procParam(vm, index, names[j], lens[j], t, isRef | (opt >> j & 1) << 1);
    if (rio_failed(vm)) return;
    if (c->tok.kind != ',') break;
    rio_next(vm);
  }
  rio_expect(vm, ')', "')' expected");
  if (typeCount) rio_genericParams(vm, index); /* (its hidden parameters, right after the others) */
  if (c->tok.kind == rio_Tok_arrow) {
    rio_next(vm);
    rio_setProcResult(vm, index, rio_parseType(vm));
    if (rio_failed(vm)) return;
    if (typeCount && !rio_genericOk(vm, c->procDefs[index].resultType, 0)) rio_FAIL(, "a generic proc can't return a T yet: fill in a &T parameter");
  }
  rio_beginBody(vm, index);
  if (typeCount) rio_loadTypeSizes(vm, index);
}

/* Type.name :: proc(...), or :: static proc(...) for one called on the type, with no self */
void rio_methodDef(rio_Vm *vm, int receiver) {
  rio_Compiler *c = vm->compiler;
  const char *name;
  int nameLen, line, col, width, isStatic;
  rio_next(vm);
  rio_next(vm);
  if (c->tok.kind != rio_Tok_id) rio_FAIL(, "method name expected");
  name = rio_keep(vm, c->tok.text, c->tok.len);
  nameLen = c->tok.len;
  line = c->tok.line;
  col = c->tok.col;
  width = c->tok.width;
  if (rio_findField(vm, rio_type(vm, receiver), name, nameLen) >= 0) rio_FAIL_AT_TOKEN(, "a method can't have the same name as a field");
  if (rio_lookupMethod(vm, receiver, name, nameLen) >= 0) rio_FAIL_AT_TOKEN(, "method already defined");
  rio_next(vm);
  if (c->tok.kind == '*') {
    c->exporting = 1;
    rio_next(vm);
  }
  rio_expect(vm, rio_Tok_colonColon, "'::' expected");
  isStatic = rio_tokIs(vm, "static", 6) && c->peek.kind == rio_Tok_proc; /* (static: a word only there) */
  if (isStatic) rio_next(vm);
  if (rio_atHost(vm)) { /* Int.toFloat :: host proc() -> Float: a builtin method, declared */
    rio_hostDecl(vm, name, nameLen, receiver, line, col);
    return;
  }
  if (rio_type(vm, receiver)->kind != rio_TypeKind_struct && rio_builtinNamed(name, nameLen, rio_Builtin_toInt, rio_Builtin_asString) >= 0) {
    rio_failAt(vm, line, col, width, "that conversion is built in");
    return;
  }
  if (c->tok.kind != rio_Tok_proc) rio_FAIL(, "'proc' expected");
  rio_procDef(vm, name, nameLen, receiver, isStatic);
}

/* ------------------------------------------------------------------ structs, unions and enums */

/* a field of the type being declared */
static rio_Field *rio_addField(rio_Vm *vm, const char *name, int nameLen, int type, int offset) {
  int i = rio_newField(vm);
  rio_Field *f;
  if (i < 0) return 0;
  f = rio_field(vm, i);
  f->name = (uint16_t)rio_addName(vm, name, nameLen);
  f->nameLen = (uint16_t)nameLen;
  f->type = (uint16_t)type;
  f->offset = (uint16_t)offset;
  f->bitAt = 0;
  f->bits = 0;
  return f;
}

/* A type's fields so far (count of them, from its firstField), moved to the end of the table if making
   the type of its next field (a proc type, a maybe) added fields of that type's own after them, so its
   own stay together. */
static void rio_keepFieldsTogether(rio_Vm *vm, rio_Type *t, int count) {
  rio_Compiler *c = vm->compiler;
  int i, first = c->fieldCount;
  if (t->firstField + count == c->fieldCount) return;
  for (i = 0; i < count; i++) {
    int f = rio_newField(vm);
    if (f < 0) return;
    *rio_field(vm, f) = *rio_field(vm, t->firstField + i);
  }
  t->firstField = (uint16_t)first;
}

/* ?T: a union of nil and T, made once for each T. Not of a union that can already be nil, or a maybe */
int rio_maybeOf(rio_Vm *vm, int t) {
  rio_Compiler *c = vm->compiler;
  rio_Type *y = rio_type(vm, t), *m;
  const char *name = "value";
  int i, nameLen = 5;
  if (y->kind == rio_TypeKind_void) rio_FAIL(0, "invalid type");
  if (y->kind == rio_TypeKind_union && y->fieldCount && rio_field(vm, y->firstField)->type == rio_TypeId_void)
    rio_FAIL(0, "this can already be nil: no ? needed");
  for (i = rio_TypeId_builtinCount; i < c->typeCount; i++) {
    m = rio_type(vm, i);
    if (m->kind == rio_TypeKind_union && m->elem == t) return i;
  }
  for (i = c->symCount - 1; i >= 0; i--) /* (T's name, for messages: this switch has no case for T) */
    if (c->syms[i].kind == rio_SymKind_type && c->syms[i].type == t) {
      name = c->names + c->syms[i].name;
      nameLen = c->syms[i].nameLen;
      break;
    }
  i = rio_newType(vm, rio_TypeKind_union, t, 0, 4 + rio_align4(y->size));
  m = rio_type(vm, i);
  m->firstField = (uint16_t)c->fieldCount;
  m->hasRefs = y->hasRefs;
  if (!rio_addField(vm, "nil", 3, rio_TypeId_void, 4) || !rio_addField(vm, name, nameLen, t, 4)) return 0;
  m->fieldCount = 2;
  if (m->size > 0xFFFF) rio_FAIL(0, "too large");
  return i;
}

/* Name :: struct, then fields (a, b: T) one per line, then end. Fields are laid out in order. A bitfield,
   Int(n) (unsigned) or Int(-n) (signed), n 1 to 31, Bool(1), or an enum's E(n) (n bits enough for all its
   values), takes the next n bits of the word the bitfields before it started, from its low bits, or a
   word of its own when they don't fit (never crossing words); any other field starts a new word. */
void rio_structDef(rio_Vm *vm, const char *name, int nameLen) {
  rio_Compiler *c = vm->compiler;
  int index, offset = 0, bitWord = 0, bitUsed = 32; /* (the word bitfields are filling, and its bits taken) */
  int symAt = c->symCount, generic, firstParam = c->typeCount, k;
  rio_Type *st;
  if (c->blockCount || c->curProc >= 0) rio_FAIL(, "structs must be top-level");
  rio_next(vm);
  c->namingType = 1;
  generic = rio_typeParamsOfType(vm); /* struct[K, V]: its Ts named in it */
  c->namingType = 0;
  index = rio_newType(vm, rio_TypeKind_struct, 0, 0, 0);
  st = rio_type(vm, index);
  st->firstField = (uint16_t)c->fieldCount;
  while (c->tok.kind != rio_Tok_end && !rio_failed(vm)) {
    const char *names[16];
    int lens[16], n, t, j, bits = 0, i;
    if (c->tok.kind == rio_Tok_eof) rio_FAIL(, rio_msgMissingEnd);
    n = rio_parseNames(vm, names, lens, 0);
    if (c->tok.kind == rio_Tok_id && c->peek.kind == '(' && (i = rio_lookup(vm, c->tok.text, c->tok.len)) >= 0 &&
        c->syms[i].kind == rio_SymKind_type && (c->syms[i].type == rio_TypeId_int || c->syms[i].type == rio_TypeId_bool ||
                                                rio_type(vm, c->syms[i].type)->kind == rio_TypeKind_enum)) { /* Int(n), Bool(1), E(n): a bitfield */
      rio_Expr e;
      t = c->syms[i].type;
      rio_next(vm);
      rio_next(vm);
      e = rio_parseExpr(vm);
      if (rio_failed(vm)) return;
      if (e.kind != rio_ExprKind_const || e.type != rio_TypeId_int || e.value == 0 || e.value > 31 || e.value < -31)
        rio_FAIL(, "a bitfield is Int(n), n 1 to 31 bits (negative: signed)");
      bits = e.value;
      if (t == rio_TypeId_bool && bits != 1) rio_FAIL(, "a Bool bitfield is Bool(1)");
      if (t != rio_TypeId_int && t != rio_TypeId_bool) { /* (an enum's: unsigned, with room for every value) */
        uint32_t values = rio_type(vm, t)->fieldCount;
        if (bits < 0) rio_FAIL(, "an enum's bitfield is unsigned: E(n), n 1 to 31 bits");
        if (bits < 31 && values > 1u << bits) rio_FAIL(, "too few bits for every value of this enum");
      }
      rio_expect(vm, ')', "')' expected");
    } else t = rio_parseType(vm);
    if (rio_type(vm, t)->kind == rio_TypeKind_void) rio_FAIL(, "invalid field type");
    if (!rio_genericOk(vm, t, 0)) rio_FAIL(, "a T goes in a field only in a []T or a proc type, so every field's size is the same whatever T is");
    rio_keepFieldsTogether(vm, st, st->fieldCount);
    for (j = 0; j < n; j++) {
      rio_Field *f;
      if (bits) {
        int width = bits < 0 ? -bits : bits;
        if (bitUsed + width > 32) { /* (a new word) */
          bitWord = offset;
          bitUsed = 0;
          offset += 4;
        }
        if (!(f = rio_addField(vm, names[j], lens[j], t, bitWord))) return;
        f->bitAt = (uint8_t)bitUsed;
        f->bits = (int8_t)bits;
        bitUsed += width;
        st->fieldCount++;
        if (offset > 0xFFFF) rio_FAIL(, "struct too large");
        continue;
      }
      bitUsed = 32;
      if (!rio_addField(vm, names[j], lens[j], t, offset)) return;
      offset += (int)rio_type(vm, t)->size;
      st->fieldCount++;
      st->hasRefs |= rio_type(vm, t)->hasRefs;
      if (offset > 0xFFFF) rio_FAIL(, "struct too large");
    }
  }
  rio_next(vm);
  st->size = (uint32_t)offset;
  if (generic) { /* (a template: filled in where it's named, Map[Int, Point]) */
    c->symCount = symAt;
    for (k = 0; k < generic; k++)
      if (!rio_mentions(vm, index, firstParam + k)) rio_FAIL(, "each type parameter is in a field's type");
    index = rio_newType(vm, rio_TypeKind_generic, index, (uint32_t)generic, 0);
  }
  rio_addSymbol(vm, name, nameLen, rio_SymKind_type, index, 0);
}

/* Name :: union, then its types one per line or comma-separated (nil may come first), then end. The
   first listed is what a new variable holds. Every value takes the biggest type's room. */
void rio_unionDef(rio_Vm *vm, const char *name, int nameLen) {
  rio_Compiler *c = vm->compiler;
  int index, n = 0, j;
  uint32_t biggest = 0;
  rio_Type *u;
  if (c->blockCount || c->curProc >= 0) rio_FAIL(, "unions must be top-level");
  rio_next(vm);
  if (!c->tok.newline) rio_FAIL_AT_TOKEN(, "list a union's types on the lines below it, then end");
  index = rio_newType(vm, rio_TypeKind_union, 0, 0, 4);
  u = rio_type(vm, index);
  u->firstField = (uint16_t)c->fieldCount;
  while (c->tok.kind != rio_Tok_end && !rio_failed(vm)) {
    const char *typeName = rio_keep(vm, c->tok.text, c->tok.len);
    int typeNameLen = c->tok.len, t;
    if (c->tok.kind == rio_Tok_eof) rio_FAIL(, rio_msgMissingEnd);
    if (c->tok.kind == rio_Tok_nil) {
      if (n) rio_FAIL_AT_TOKEN(, "nil can only come first");
      rio_next(vm);
      t = rio_TypeId_void;
    } else {
      t = rio_parseType(vm);
      if (rio_type(vm, t)->kind == rio_TypeKind_void) rio_FAIL(, "invalid type in a union");
      rio_keepFieldsTogether(vm, u, n);
    }
    for (j = 0; j < n; j++)
      if (rio_field(vm, u->firstField + j)->type == t) rio_FAIL(, "type listed twice");
    if (!rio_addField(vm, typeName, typeNameLen, t, 4)) return; /* (the value starts after the tag) */
    if (t != rio_TypeId_void) {
      uint32_t size = rio_type(vm, t)->size;
      if (size > biggest) biggest = size;
      u->hasRefs |= rio_type(vm, t)->hasRefs;
    }
    n++;
    if (c->tok.kind == ',') rio_next(vm);
  }
  rio_next(vm);
  if (!n) rio_FAIL(, "a union lists at least one type");
  if (n > 256) rio_FAIL(, "too many types in one union");
  u->fieldCount = (uint16_t)n;
  u->size = 4 + rio_align4(biggest);
  if (u->size > 0xFFFF) rio_FAIL(, "union too large");
  rio_addSymbol(vm, name, nameLen, rio_SymKind_type, index, 0);
}

/* Name :: enum, then values one per line or comma-separated, then end; nil may come first. The values
   are 0, 1, 2 ... in the order listed (so new variables start at the first, and [Enum]T has one element
   for each). */
void rio_enumDef(rio_Vm *vm, const char *name, int nameLen) {
  rio_Compiler *c = vm->compiler;
  int index, n = 0, j;
  rio_Type *et;
  if (c->blockCount || c->curProc >= 0) rio_FAIL(, "enums must be top-level");
  rio_next(vm);
  if (!c->tok.newline) rio_FAIL_AT_TOKEN(, "list an enum's values on the lines below it, then end");
  index = rio_newType(vm, rio_TypeKind_enum, 0, 0, 4);
  et = rio_type(vm, index);
  et->firstField = (uint16_t)c->fieldCount;
  while (c->tok.kind != rio_Tok_end && !rio_failed(vm)) {
    const char *valueName = rio_keep(vm, c->tok.text, c->tok.len);
    int valueNameLen = c->tok.len;
    rio_Field *f;
    if (c->tok.kind == rio_Tok_eof) rio_FAIL(, rio_msgMissingEnd);
    if (c->tok.kind == rio_Tok_nil) {
      if (n) rio_FAIL_AT_TOKEN(, "nil can only come first");
      rio_next(vm);
    } else {
      if (c->tok.kind != rio_Tok_id) rio_FAIL_AT_TOKEN(, "value name expected");
      rio_next(vm);
      if (c->tok.kind == '=') rio_FAIL_AT_TOKEN(, "an enum's values are 0, 1, 2 ... as listed: give other numbers as constants (WALL :: 3)");
    }
    for (j = 0; j < n; j++) {
      rio_Field *g = rio_field(vm, et->firstField + j);
      if (rio_nameIs(vm, g->name, g->nameLen, valueName, valueNameLen)) rio_FAIL(, "value named twice");
    }
    if (!(f = rio_addField(vm, valueName, valueNameLen, 0, (uint16_t)n))) return;
    n++;
    if (c->tok.kind == ',') rio_next(vm);
  }
  rio_next(vm);
  if (!n) rio_FAIL(, "an enum needs at least one value");
  et->fieldCount = (uint16_t)n;
  et->elem = (uint16_t)vm->constCount; /* (its names, for log and print: (address, length) strings) */
  for (j = 0; j < n; j++) {
    rio_Field *g = rio_field(vm, et->firstField + j);
    rio_stringConst(vm, c->names + g->name, g->nameLen);
  }
  rio_addSymbol(vm, name, nameLen, rio_SymKind_type, index, 0);
}

/* ------------------------------------------------------------------ variables */

/* name: [..]T = an array or slice: a builder view over that memory, starting empty. Its length word is a
   fourth slot right after the builder view's three. */
static void rio_declareBuilderOver(rio_Vm *vm, const char *name, int nameLen, int type, rio_Expr *e) {
  rio_Compiler *c = vm->compiler;
  int s, d;
  if (rio_type(vm, e->type)->elem != rio_type(vm, type)->elem) rio_FAIL(, "type mismatch");
  if (e->access || e->type == rio_TypeId_string) rio_FAIL(, "can't build on read-only data");
  if (rio_type(vm, e->type)->kind == rio_TypeKind_array && rio_isTemp(vm, e))
    rio_FAIL(, "a computed array has no home to view: store it in a variable first");
  s = rio_toSlot2(vm, e);
  c->freeSlot = c->liveSlots;
  d = rio_allocSlots(vm, 4);
  c->liveSlots = c->freeSlot;
  rio_emit(vm, rio_Op_mov2, d + 1, s, 0); /* first: s may sit where the builder view now goes */
  rio_emit(vm, rio_Op_mov, d, rio_addrConst(vm, (uint32_t)(d + 3) * 4, 4), 0);
  rio_emit(vm, rio_Op_mov, d + 3, rio_constSlot(vm, 0), 0);
  if (c->curProc >= 0) rio_markLocalView(vm, (uint32_t)d * 4, 3, 1); /* its length word is in this frame */
  rio_addSymbol(vm, name, nameLen, rio_SymKind_var, type, d * 4);
}

/* A union variable from a value of one of its types (or nil): the store wraps it. */
static void rio_declareUnion(rio_Vm *vm, const char *name, int nameLen, int type, rio_Expr *e) {
  rio_Compiler *c = vm->compiler;
  uint32_t at;
  rio_Expr d;
  if (rio_unionMember(vm, type, e) < 0) rio_FAIL(, "type mismatch");
  if (c->curProc >= 0 && rio_type(vm, type)->hasRefs && rio_isBig(rio_type(vm, type)))
    rio_FAIL(, "a big union (over 16 words) holding slices or strings must be global");
  at = rio_isBig(rio_type(vm, type)) ? rio_allocHigh(vm, rio_type(vm, type)->size) : (uint32_t)rio_allocSlots(vm, rio_typeWords(vm, type)) * 4;
  d = rio_exprOf(rio_ExprKind_static, type, (int32_t)at);
  rio_store(vm, &d, e);
  if (!rio_isBig(rio_type(vm, type))) c->liveSlots = c->freeSlot;
  rio_addSymbol(vm, name, nameLen, rio_SymKind_var, type, (int32_t)at);
}

/* Declares a variable of type `type` (-1: from the value), initialized from *init or zeroed. adopt: the
   value may already sit in the next free slot and simply become the variable. */
static void rio_declare(rio_Vm *vm, const char *name, int nameLen, int type, rio_Expr *init, int adopt) {
  rio_Compiler *c = vm->compiler;
  int has = init != 0, k, big, localView = 0;
  uint32_t addr;
  rio_Type *ty;
  rio_Expr e = has ? *init : rio_voidExpr();
  if (has) {
    if (e.kind == rio_ExprKind_proc || e.kind == rio_ExprKind_ffi) /* f := byScore, f := pc.cls */
      rio_toProcValue(vm, &e, type >= 0 && rio_type(vm, type)->kind == rio_TypeKind_proc ? type : -1);
    rio_needValue(vm, &e);
    localView = rio_isLocalView(vm, &e);
    if (type >= 0 && rio_type(vm, type)->kind == rio_TypeKind_union && e.type != type) {
      rio_declareUnion(vm, name, nameLen, type, &e);
      return;
    }
    if (type >= 0 && rio_type(vm, type)->kind == rio_TypeKind_builderView &&
        (rio_type(vm, e.type)->kind == rio_TypeKind_array || rio_type(vm, e.type)->kind == rio_TypeKind_slice)) {
      rio_declareBuilderOver(vm, name, nameLen, type, &e);
      return;
    }
    if (type < 0) { /* the value's type: a builder's is a builder view of it (but a new builder, as from a
                       literal, is itself) */
      type = rio_valueType(e.type);
      if (rio_type(vm, type)->kind == rio_TypeKind_builder && !rio_isTemp(vm, &e)) type = rio_builderViewOf(vm, rio_type(vm, type)->elem);
      else if (rio_isBig(rio_type(vm, type)) && !rio_isTemp(vm, &e))
        rio_FAIL(, rio_type(vm, type)->kind == rio_TypeKind_array
                       ? "this copies a big array (over 16 words): write its type to copy it (x: [N]T = a), or view it with a[]"
                       : "this copies a big struct (over 16 words): write its type to copy it (x: T = s), or use &s");
    }
    rio_coerce(vm, &e, type);
  }
  ty = rio_type(vm, type);
  k = ty->kind;
  if (k == rio_TypeKind_void) rio_FAIL(, "variable has no type");
  if (k == rio_TypeKind_param) rio_FAIL(, "a T can't be kept in a variable: copy between places (a = b), or swap them (xs.swap(i, j))");
  if (rio_mentions(vm, type, -1) && k != rio_TypeKind_slice && k != rio_TypeKind_proc && k != rio_TypeKind_struct)
    rio_FAIL(, "a T goes in a variable only as []T, or in a generic struct (for now)");
  if (has && (e.access & rio_Access_literal) && e.type == type && c->curProc < 0 && !c->blockCount && !vm->inRepl &&
      rio_literalData(vm, &e)) { /* a top-level variable from a literal of constants: its starting value */
    rio_addSymbol(vm, name, nameLen, rio_SymKind_var, type, e.value);
    return;
  }
  if (c->curProc >= 0 && rio_isAggregate(k) && ty->hasRefs && (k == rio_TypeKind_builder || rio_isBig(ty)))
    rio_FAIL(, k == rio_TypeKind_builder ? "a builder holding slices or strings must be global"
                                         : "a big struct, array or union (over 16 words) holding slices or strings must be global");
  big = rio_isBig(ty);
  if (big) addr = rio_allocHigh(vm, ty->size);
  else {
    int isView = k == rio_TypeKind_slice || k == rio_TypeKind_builderView;
    if (has && e.kind == rio_ExprKind_indirect &&
        (k == rio_TypeKind_int || k == rio_TypeKind_float || k == rio_TypeKind_bool || k == rio_TypeKind_enum || k == rio_TypeKind_proc || isView)) {
      int s = isView ? rio_toViewSlots(vm, &e) : rio_toSlot(vm, &e, 1); /* loaded into slots */
      e.kind = rio_ExprKind_static;
      e.value = s * 4;
      e.type = (uint16_t)type;
    }
    if (adopt && has && e.kind == rio_ExprKind_static && e.value == (int32_t)c->liveSlots * 4 && !e.access) {
      /* the value is already in the next slots: they become the variable */
      c->liveSlots += (uint32_t)rio_typeWords(vm, type);
      c->freeSlot = c->liveSlots;
      if (isView && c->curProc >= 0) {
        rio_markLocalView(vm, (uint32_t)e.value, rio_typeWords(vm, type), localView);
        rio_markParamView(vm, (uint32_t)e.value, rio_typeWords(vm, type), e.fromParam);
      }
      rio_addSymbol(vm, name, nameLen, rio_SymKind_var, type, e.value);
      return;
    }
    addr = (uint32_t)rio_allocSlots(vm, rio_typeWords(vm, type)) * 4;
  }
  if (has) {
    rio_Expr d = rio_exprOf(rio_ExprKind_static, type, (int32_t)addr);
    rio_store(vm, &d, &e);
  } else if (!big && addr < rio_SLOTS_END) { /* zeroed word by word */
    int w;
    for (w = 0; w < rio_typeWords(vm, type); w++) rio_emit(vm, rio_Op_mov, (int)(addr >> 2) + w, rio_constSlot(vm, 0), 0);
  } else if (!(big && c->curProc < 0 && !c->blockCount)) /* (a big top-level global starts zeroed) */
    rio_emit(vm, rio_Op_zero, rio_addrConst(vm, addr, ty->size), (int)(ty->size & 0xFFFF), (int)(ty->size >> 16));
  if (!big) c->liveSlots = c->freeSlot; /* after the store, so it can still write straight into the variable */
  if ((k == rio_TypeKind_slice || k == rio_TypeKind_builderView) && c->curProc >= 0 && !big) {
    rio_markLocalView(vm, addr, rio_typeWords(vm, type), localView);
    rio_markParamView(vm, addr, rio_typeWords(vm, type), has ? e.fromParam : 0);
  }
  rio_addSymbol(vm, name, nameLen, rio_SymKind_var, type, (int32_t)addr);
}

/* name := value, name: T = value, name: T, and name := &place */
void rio_declareVar(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  const char *name = rio_keep(vm, c->tok.text, c->tok.len);
  int nameLen = c->tok.len, type = -1, has = 0;
  rio_Expr e;
  rio_next(vm);
  if (c->tok.kind == ':') {
    rio_next(vm);
    type = rio_parseType(vm);
    if (c->tok.kind == '=') {
      rio_next(vm);
      has = 1;
    }
  } else {
    rio_next(vm);
    has = 1;
  }
  if (!has) {
    rio_declare(vm, name, nameLen, type, 0, 1);
    return;
  }
  if (c->tok.kind == rio_Tok_if || c->tok.kind == rio_Tok_switch) { /* x := if ...: each branch's value */
    rio_valueBlock(vm, rio_Value_declare, 0, type, name, nameLen);
    return;
  }
  c->targetType = type;
  e = rio_parseExpr(vm);
  if (e.access & rio_Access_ref) { /* name := &place: bound once to that place, like a for-each element */
    int s, d, local, fromParam;
    if (type >= 0) rio_FAIL(, "a reference takes its type from its place: write name := &x");
    if (c->exporting) rio_FAIL(, "a reference can't be exported");
    e.access = 0;
    local = rio_isLocalMemory(vm, &e) || rio_isLocalView(vm, &e);
    fromParam = e.fromParam;
    s = rio_toAddrSlot(vm, &e);
    c->freeSlot = c->liveSlots;
    d = rio_allocSlots(vm, 1);
    c->liveSlots = c->freeSlot;
    if (s != d) rio_emit(vm, rio_Op_mov, d, s, 0);
    if (c->curProc >= 0) { /* (its slot's marks: what reading through it may view) */
      rio_markLocalView(vm, (uint32_t)d * 4, 1, local);
      rio_markParamView(vm, (uint32_t)d * 4, 1, fromParam);
    }
    rio_addSymbol(vm, name, nameLen, rio_SymKind_ref, e.type, d);
    return;
  }
  rio_declare(vm, name, nameLen, type, &e, 1);
}

/* {a, b as c} := value: new variables holding copies of some of a struct's fields */
void rio_destructure(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  const char *fields[32], *names[32];
  int fieldLens[32], nameLens[32], n = 0, i;
  rio_Expr e;
  rio_next(vm);
  while (c->tok.kind != '}' && !rio_failed(vm)) {
    if (c->tok.kind != rio_Tok_id) rio_FAIL(, "field name expected");
    if (n >= 32) rio_FAIL(, "too many names");
    fields[n] = names[n] = rio_keep(vm, c->tok.text, c->tok.len);
    fieldLens[n] = nameLens[n] = c->tok.len;
    rio_next(vm);
    if (rio_tokIs(vm, "as", 2)) { /* 'as' only means something here */
      rio_next(vm);
      if (c->tok.kind != rio_Tok_id) rio_FAIL(, "name expected after 'as'");
      names[n] = rio_keep(vm, c->tok.text, c->tok.len);
      nameLens[n] = c->tok.len;
      rio_next(vm);
    }
    n++;
    if (c->tok.kind != ',') break;
    rio_next(vm);
  }
  rio_expect(vm, '}', "'}' expected");
  if (c->tok.kind != rio_Tok_colonEq) rio_FAIL(, "':=' expected: destructuring declares new names");
  rio_next(vm);
  e = rio_parseExpr(vm);
  rio_needValue(vm, &e);
  if (rio_type(vm, e.type)->kind != rio_TypeKind_struct) rio_FAIL(, "only a struct can be destructured");
  for (i = 0; i < n; i++) {
    rio_Type *st = rio_type(vm, e.type);
    rio_Expr fe = e;
    int index = rio_findField(vm, st, fields[i], fieldLens[i]), k;
    rio_Field *f;
    if (index < 0) rio_FAIL(, "no such field to destructure");
    f = rio_field(vm, st->firstField + index);
    rio_offsetPlace(&fe, f->offset);
    fe.type = f->type;
    k = rio_type(vm, f->type)->kind;
    if (fe.kind == rio_ExprKind_indirect && !rio_isAggregate(k)) { /* load without disturbing the struct's address */
      int s = k == rio_TypeKind_slice || k == rio_TypeKind_builderView ? rio_toViewSlots(vm, &fe) : rio_toSlot(vm, &fe, 0);
      fe = rio_exprOf(rio_ExprKind_static, rio_valueType(f->type), s * 4);
    }
    rio_declare(vm, names[i], nameLens[i], -1, &fe, 0);
  }
}

#endif
