/* decls.c - types as written, and declarations: procs and methods, structs, unions and enums, and
   variables (with destructuring). The rules: docs/spec/declarations.md and docs/spec/types.md. */
#include "compiler.h"
#ifndef rio_NO_COMPILER

/* ------------------------------------------------------------------ types */

static int rio_parseValueType(rio_Vm *vm);

/* proc(T, U) -> R, or any other type */
/* At a proc type's parameter: how many names come before its type, as in a proc's own parameters (a,
   b: Int), or 0 for none (Int, Float: just types). Names are for reading; procs take arguments by
   position. */
static int rio_paramNames(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  const char *p = c->tok.text, *e = c->srcEnd;
  int count = 0;
  if (c->tok.kind != rio_Tok_id) return 0;
  for (;;) {
    if (p >= e || !rio_isAlpha(*p)) return 0;
    while (p < e && (rio_isAlpha(*p) || rio_isDigit(*p))) p++;
    count++;
    while (p < e && rio_isSpace(*p)) p++;
    if (p < e && *p == ',') {
      for (p++; p < e && rio_isSpace(*p); p++) {}
      continue;
    }
    return p + 1 < e && p[0] == ':' && p[1] != ':' && p[1] != '=' ? count : 0;
  }
}

int rio_parseType(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  int params[16], n = 0, result = rio_TypeId_void;
  if (c->tok.kind != rio_Tok_proc) return rio_parseValueType(vm);
  rio_next(vm);
  rio_expect(vm, '(', "'(' expected");
  while (c->tok.kind != ')' && !rio_failed(vm)) {
    int names = rio_paramNames(vm), t, j; /* a, b: T, or just T */
    if (n >= 16) rio_FAIL(0, "too many parameters");
    for (j = 0; j < names * 2; j++) rio_next(vm); /* (each name, and the , or : after it) */
    if (c->tok.kind == '&') rio_FAIL_AT_TOKEN(0, "proc values can't take & parameters");
    if (c->tok.kind == rio_Tok_proc) rio_FAIL_AT_TOKEN(0, "a proc type can't take or return procs");
    t = rio_parseValueType(vm);
    for (j = 0; j < (names ? names : 1); j++) {
      if (n >= 16) rio_FAIL(0, "too many parameters");
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
  return rio_procTypeOf(vm, params, n, result);
}

/* Any type but a proc type: prefixes ([N] [] [..N] [..] [Enum]), then a name, maybe through modules
   (and Blob[N], Blob[..N], Blob[..]). The prefixes apply innermost last: [2][3]T is 2 of [3]T. */
static int rio_parseValueType(rio_Vm *vm) {
  /* per prefix: n > 0 an array of n; -1 a slice; -2 a builder; -3 - n a list of n; and an enum, if any */
  rio_Compiler *c = vm->compiler;
  int32_t prefix[16];
  int count = 0, t, i, enumOf[16];
  while (c->tok.kind == '[') {
    if (count >= 16) rio_FAIL(0, "type too deep");
    rio_next(vm);
    enumOf[count] = 0;
    if (c->tok.kind == ']') prefix[count++] = -1;
    else if (c->tok.kind == '.' && c->peek.kind == '.') {
      rio_next(vm);
      rio_next(vm);
      prefix[count++] = c->tok.kind == ']' ? -2 : -3 - rio_parsePositiveConst(vm);
    } else { /* [N]T, [N * 2]T, [m.N + 1]T (constant Int arithmetic folds), or [Enum]T, [m.Enum]T */
      rio_Expr e = rio_parseExpr(vm);
      if (e.kind == rio_ExprKind_type && rio_type(vm, e.type)->kind == rio_TypeKind_enum) {
        enumOf[count] = e.type;
        prefix[count++] = 0;
      } else if (e.kind == rio_ExprKind_const && e.type == rio_TypeId_int && e.value > 0) prefix[count++] = e.value;
      else rio_FAIL(0, "an array size is a positive Int constant or an enum");
    }
    rio_expect(vm, ']', "']' expected");
  }
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
  if (t == rio_TypeId_blob && c->tok.kind == '[' && !c->tok.newline) { /* Blob[N], Blob[..N], Blob[..] */
    rio_next(vm);
    if (c->tok.kind == '.' && c->peek.kind == '.') {
      rio_next(vm);
      rio_next(vm);
      t = c->tok.kind == ']' ? rio_builderOf(vm, rio_TypeId_byte) : rio_listOf(vm, rio_TypeId_byte, (uint32_t)rio_parsePositiveConst(vm));
    } else t = rio_arrayOf(vm, rio_TypeId_byte, (uint32_t)rio_parsePositiveConst(vm));
    rio_expect(vm, ']', "']' expected");
  }
  while (count) {
    int32_t n = prefix[--count];
    if (enumOf[count]) t = rio_enumArrayOf(vm, t, enumOf[count]);
    else if (n == -1) t = rio_sliceOf(vm, t);
    else if (n == -2) t = rio_builderOf(vm, t);
    else if (n < -2) t = rio_listOf(vm, t, (uint32_t)(-3 - n));
    else t = rio_arrayOf(vm, t, (uint32_t)n);
  }
  return t;
}

/* `a, b, c:`, the names before a field's or parameter's type; returns how many */
int rio_parseNames(rio_Vm *vm, const char **names, int *lens) {
  rio_Compiler *c = vm->compiler;
  int n = 0;
  for (;;) {
    if (c->tok.kind != rio_Tok_id || n >= 16) rio_FAIL(0, "name expected");
    names[n] = c->tok.text;
    lens[n++] = c->tok.len;
    rio_next(vm);
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
int rio_beginProc(rio_Vm *vm, const char *name, int nameLen, int receiver) {
  rio_Compiler *c = vm->compiler;
  rio_ProcDef *p;
  rio_Block *b;
  int index, skip = rio_emit(vm, rio_Op_jmp, 0, 0, rio_NONE); /* top-level code goes around it */
  index = vm->procCount++;
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
  rio_addSymbol(vm, name, nameLen, receiver < 0 ? rio_SymKind_proc : rio_SymKind_method, receiver < 0 ? 0 : receiver, index);
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
    rio_addSymbol(vm, "self", 4, p->selfByRef ? rio_SymKind_ref : rio_SymKind_var, receiver, p->selfByRef ? addr / 4 : addr);
    p->paramCount++;
  }
  return index;
}

/* a parameter of the proc being begun: name: type, or name: &type (isRef) */
void rio_procParam(rio_Vm *vm, int proc, const char *name, int nameLen, int type, int isRef) {
  rio_Compiler *c = vm->compiler;
  rio_Type *ty = rio_type(vm, type);
  int addr;
  if (ty->kind == rio_TypeKind_void ||
      (!isRef && (ty->kind == rio_TypeKind_list ||
                  ((ty->kind == rio_TypeKind_struct || ty->kind == rio_TypeKind_array || ty->kind == rio_TypeKind_union) && ty->hasRefs))))
    rio_FAIL(, "invalid parameter type (pass lists as [..]T)");
  if (!isRef && rio_isBig(ty)) rio_FAIL(, "a big value (over 16 words) goes by reference: use &T, or []T for an array");
  addr = rio_allocSlots(vm, isRef ? 1 : rio_typeWords(vm, type)) * 4; /* a & parameter is one word holding the address,
                                                                     read through like self */
  if (c->paramCount >= (int)c->limits.params) rio_FAIL(, "too many parameters");
  rio_addParam(vm, isRef ? rio_TypeId_int : type, isRef ? type : 0, addr);
  rio_addSymbol(vm, name, nameLen, isRef ? rio_SymKind_ref : rio_SymKind_var, type, isRef ? addr / 4 : addr);
  c->procDefs[proc].paramCount++;
}

/* the result of the proc being begun */
void rio_setProcResult(rio_Vm *vm, int proc, int type) {
  rio_ProcDef *p = &vm->compiler->procDefs[proc];
  rio_Type *ty = rio_type(vm, type);
  p->resultType = (uint16_t)type;
  if (ty->kind == rio_TypeKind_list ||
      ((ty->kind == rio_TypeKind_array || ty->kind == rio_TypeKind_struct || ty->kind == rio_TypeKind_union) && ty->hasRefs))
    rio_FAIL(, "invalid return type");
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

/* name :: proc(params) -> result, after the name and ::; receiver: a method's type, or -1. The body
   follows as statements, until its end (in rio_statement). */
void rio_procDef(rio_Vm *vm, const char *name, int nameLen, int receiver) {
  rio_Compiler *c = vm->compiler;
  int index;
  if (c->blockCount || c->curProc >= 0) rio_FAIL(, "procs must be top-level");
  if (vm->procCount >= (int)c->limits.procs) rio_FAIL(, "too many procs");
  rio_next(vm);
  rio_expect(vm, '(', "'(' expected");
  if ((index = rio_beginProc(vm, name, nameLen, receiver)) < 0) return;
  while (c->tok.kind != ')' && !rio_failed(vm)) { /* a, b: T  or  a: &T */
    const char *names[16];
    int lens[16], n = rio_parseNames(vm, names, lens), isRef = c->tok.kind == '&', t, j;
    if (isRef) rio_next(vm);
    t = rio_parseType(vm);
    for (j = 0; j < n && !rio_failed(vm); j++) rio_procParam(vm, index, names[j], lens[j], t, isRef);
    if (rio_failed(vm)) return;
    if (c->tok.kind != ',') break;
    rio_next(vm);
  }
  rio_expect(vm, ')', "')' expected");
  if (c->tok.kind == rio_Tok_arrow) {
    rio_next(vm);
    rio_setProcResult(vm, index, rio_parseType(vm));
    if (rio_failed(vm)) return;
  }
  rio_beginBody(vm, index);
}

/* Type.name :: proc(...) */
void rio_methodDef(rio_Vm *vm, int receiver) {
  rio_Compiler *c = vm->compiler;
  const char *name;
  int nameLen, line, col, width;
  rio_next(vm);
  rio_next(vm);
  if (c->tok.kind != rio_Tok_id) rio_FAIL(, "method name expected");
  name = c->tok.text;
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
  if (c->tok.kind == '@') { /* Int.toFloat :: @host proc() -> Float: a builtin method, declared */
    rio_hostDecl(vm, name, nameLen, receiver, line, col);
    return;
  }
  if (rio_type(vm, receiver)->kind != rio_TypeKind_struct && rio_builtinNamed(name, nameLen, rio_Builtin_toInt, rio_Builtin_asString) >= 0) {
    rio_failAt(vm, line, col, width, "that conversion is built in");
    return;
  }
  if (c->tok.kind != rio_Tok_proc) rio_FAIL(, "'proc' expected");
  rio_procDef(vm, name, nameLen, receiver);
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
  return f;
}

/* Name :: struct, then fields (a, b: T) one per line, then end. Fields are laid out in order. */
void rio_structDef(rio_Vm *vm, const char *name, int nameLen) {
  rio_Compiler *c = vm->compiler;
  int index, offset = 0;
  rio_Type *st;
  if (c->blockCount || c->curProc >= 0) rio_FAIL(, "structs must be top-level");
  rio_next(vm);
  index = rio_newType(vm, rio_TypeKind_struct, 0, 0, 0);
  st = rio_type(vm, index);
  st->firstField = (uint16_t)c->fieldCount;
  while (c->tok.kind != rio_Tok_end && !rio_failed(vm)) {
    const char *names[16];
    int lens[16], n, t, j;
    if (c->tok.kind == rio_Tok_eof) rio_FAIL(, rio_msgMissingEnd);
    n = rio_parseNames(vm, names, lens);
    t = rio_parseType(vm);
    if (rio_type(vm, t)->kind == rio_TypeKind_void) rio_FAIL(, "invalid field type");
    for (j = 0; j < n; j++) {
      if (!rio_addField(vm, names[j], lens[j], t, offset)) return;
      offset += (int)rio_type(vm, t)->size;
      st->fieldCount++;
      st->hasRefs |= rio_type(vm, t)->hasRefs;
      if (offset > 0xFFFF) rio_FAIL(, "struct too large");
    }
  }
  rio_next(vm);
  st->size = (uint32_t)offset;
  rio_addSymbol(vm, name, nameLen, rio_SymKind_type, index, 0);
}

/* Name :: union [@diverse], then its types one per line or comma-separated (nil may come first), then
   end. The first listed is what a new variable holds. Types more than 64 bytes apart in size waste space
   in every value (arrays of the union most of all), so that's an error unless marked @diverse. */
void rio_unionDef(rio_Vm *vm, const char *name, int nameLen) {
  rio_Compiler *c = vm->compiler;
  int index, n = 0, j, diverse = 0;
  uint32_t biggest = 0, smallest = 0xFFFFFFFFu;
  rio_Type *u;
  if (c->blockCount || c->curProc >= 0) rio_FAIL(, "unions must be top-level");
  rio_next(vm);
  if (c->tok.kind == '@') {
    rio_next(vm);
    if (!rio_tokIs(vm, "diverse", 7)) rio_FAIL_AT_TOKEN(, "unknown note for a union (@diverse allows types of very different sizes)");
    diverse = 1;
    rio_next(vm);
  }
  if (!c->tok.newline) rio_FAIL_AT_TOKEN(, "list a union's types on the lines below it, then end");
  index = rio_newType(vm, rio_TypeKind_union, 0, 0, 4);
  u = rio_type(vm, index);
  u->firstField = (uint16_t)c->fieldCount;
  while (c->tok.kind != rio_Tok_end && !rio_failed(vm)) {
    const char *typeName = c->tok.text;
    int typeNameLen = c->tok.len, t;
    if (c->tok.kind == rio_Tok_eof) rio_FAIL(, rio_msgMissingEnd);
    if (c->tok.kind == rio_Tok_nil) {
      if (n) rio_FAIL_AT_TOKEN(, "nil can only come first");
      rio_next(vm);
      t = rio_TypeId_void;
    } else {
      t = rio_parseType(vm);
      if (rio_type(vm, t)->kind == rio_TypeKind_void) rio_FAIL(, "invalid type in a union");
    }
    for (j = 0; j < n; j++)
      if (rio_field(vm, u->firstField + j)->type == t) rio_FAIL(, "type listed twice");
    if (!rio_addField(vm, typeName, typeNameLen, t, 4)) return; /* (the value starts after the tag) */
    if (t != rio_TypeId_void) {
      uint32_t size = rio_type(vm, t)->size;
      if (size > biggest) biggest = size;
      if (size < smallest) smallest = size;
      u->hasRefs |= rio_type(vm, t)->hasRefs;
    }
    n++;
    if (c->tok.kind == ',') rio_next(vm);
  }
  rio_next(vm);
  if (!n) rio_FAIL(, "a union lists at least one type");
  if (n > 256) rio_FAIL(, "too many types in one union");
  if (!diverse && biggest > smallest && biggest - smallest > rio_SMALL_WORDS * 4)
    rio_FAIL(, "these types differ in size by more than 64 bytes, so every value takes the biggest size: keep the big data elsewhere, or mark the union @diverse");
  u->fieldCount = (uint16_t)n;
  u->size = 4 + rio_align4(biggest);
  if (u->size > 0xFFFF) rio_FAIL(, "union too large");
  rio_addSymbol(vm, name, nameLen, rio_SymKind_type, index, 0);
}

/* Name :: enum, then values one per line or comma-separated, then end. A value is name or name = n
   (otherwise one more than the last); nil may come first. Values increase down the list, so the order
   listed is the order of the values (and of arrays indexed by the enum). Some value must be 0: new
   variables start there. */
void rio_enumDef(rio_Vm *vm, const char *name, int nameLen) {
  rio_Compiler *c = vm->compiler;
  rio_Val *R = (rio_Val *)vm->mem;
  int index, n = 0, hasZero = 0, j;
  int32_t v = 0;
  rio_Type *et;
  if (c->blockCount || c->curProc >= 0) rio_FAIL(, "enums must be top-level");
  rio_next(vm);
  if (!c->tok.newline) rio_FAIL_AT_TOKEN(, "list an enum's values on the lines below it, then end");
  index = rio_newType(vm, rio_TypeKind_enum, 0, 0, 4);
  et = rio_type(vm, index);
  et->firstField = (uint16_t)c->fieldCount;
  while (c->tok.kind != rio_Tok_end && !rio_failed(vm)) {
    const char *valueName = c->tok.text;
    int valueNameLen = c->tok.len;
    rio_Field *f;
    if (c->tok.kind == rio_Tok_eof) rio_FAIL(, rio_msgMissingEnd);
    if (c->tok.kind == rio_Tok_nil) {
      if (n) rio_FAIL_AT_TOKEN(, "nil can only come first");
      rio_next(vm);
      v = 0;
    } else {
      if (c->tok.kind != rio_Tok_id) rio_FAIL_AT_TOKEN(, "value name expected");
      rio_next(vm);
      if (c->tok.kind == '=') {
        rio_Expr e;
        rio_next(vm);
        e = rio_parseExpr(vm);
        if (e.kind != rio_ExprKind_const || e.type != rio_TypeId_int) rio_FAIL(, "an enum value is an Int constant");
        v = e.value;
      }
    }
    for (j = 0; j < n; j++) {
      rio_Field *g = rio_field(vm, et->firstField + j);
      if (rio_nameIs(vm, g->name, g->nameLen, valueName, valueNameLen)) rio_FAIL(, "value named twice");
    }
    if (n && v <= rio_enumValue(rio_field(vm, et->firstField + n - 1)))
      rio_FAIL(, "values increase down the list: this one isn't bigger than the one before");
    if (!(f = rio_addField(vm, valueName, valueNameLen, (uint16_t)((uint32_t)v >> 16), (uint16_t)v))) return;
    hasZero |= v == 0;
    n++;
    v++;
    if (c->tok.kind == ',') rio_next(vm);
  }
  rio_next(vm);
  if (!n) rio_FAIL(, "an enum needs at least one value");
  if (!hasZero) rio_FAIL(, "an enum needs a value that's 0: new variables start there (list nil first for an empty one)");
  et->fieldCount = (uint16_t)n;
  /* the tables for fromInt, has and log: the values, then their names as (address, length) strings */
  if (vm->constCount + (uint32_t)n > vm->constCap) rio_FAIL(, "too many constants");
  et->count = vm->constCount;
  for (j = 0; j < n; j++) R[vm->constCount++].u = (uint32_t)rio_enumValue(rio_field(vm, et->firstField + j));
  et->elem = (uint16_t)vm->constCount;
  for (j = 0; j < n; j++) {
    rio_Field *g = rio_field(vm, et->firstField + j);
    rio_stringConst(vm, c->names + g->name, g->nameLen);
  }
  rio_addSymbol(vm, name, nameLen, rio_SymKind_type, index, 0);
}

/* ------------------------------------------------------------------ variables */

/* name: [..]T = an array or slice: a builder over that memory, starting empty. Its length word is a
   fourth slot right after the builder's three. */
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
  rio_emit(vm, rio_Op_mov2, d + 1, s, 0); /* first: s may sit where the builder now goes */
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
    if (e.kind == rio_ExprKind_proc) /* f := byScore */
      rio_toProcValue(vm, &e, type >= 0 && rio_type(vm, type)->kind == rio_TypeKind_proc ? type : -1);
    rio_needValue(vm, &e);
    localView = rio_isLocalView(vm, &e);
    if (type >= 0 && rio_type(vm, type)->kind == rio_TypeKind_union && e.type != type) {
      rio_declareUnion(vm, name, nameLen, type, &e);
      return;
    }
    if (type >= 0 && rio_type(vm, type)->kind == rio_TypeKind_builder &&
        (rio_type(vm, e.type)->kind == rio_TypeKind_array || rio_type(vm, e.type)->kind == rio_TypeKind_slice)) {
      rio_declareBuilderOver(vm, name, nameLen, type, &e);
      return;
    }
    if (type < 0) { /* the value's type: a list's is a builder, a view of it (but a new list, as from a
                       literal, is itself) */
      type = rio_valueType(e.type);
      if (rio_type(vm, type)->kind == rio_TypeKind_list && !rio_isTemp(vm, &e)) type = rio_builderOf(vm, rio_type(vm, type)->elem);
      else if (rio_isBig(rio_type(vm, type)) && !rio_isTemp(vm, &e))
        rio_FAIL(, rio_type(vm, type)->kind == rio_TypeKind_array
                       ? "this copies a big array (over 16 words): write its type to copy it (x: [N]T = a), or view it with a[:]"
                       : "this copies a big struct (over 16 words): write its type to copy it (x: T = s), or use &s");
    }
    rio_coerce(vm, &e, type);
  }
  ty = rio_type(vm, type);
  k = ty->kind;
  if (k == rio_TypeKind_void) rio_FAIL(, "variable has no type");
  if (c->curProc >= 0 && rio_isAggregate(k) && ty->hasRefs)
    rio_FAIL(, "structs/arrays/lists/unions holding slices or strings must be global");
  big = rio_isBig(ty);
  if (big) addr = rio_allocHigh(vm, ty->size);
  else {
    int isView = k == rio_TypeKind_slice || k == rio_TypeKind_builder;
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
      if (isView && c->curProc >= 0) rio_markLocalView(vm, (uint32_t)e.value, rio_typeWords(vm, type), localView);
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
  if ((k == rio_TypeKind_slice || k == rio_TypeKind_builder) && c->curProc >= 0 && !big) rio_markLocalView(vm, addr, rio_typeWords(vm, type), localView);
  rio_addSymbol(vm, name, nameLen, rio_SymKind_var, type, (int32_t)addr);
}

/* name := value, name: T = value, name: T, and name := &place */
void rio_declareVar(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  const char *name = c->tok.text;
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
  c->targetType = type;
  e = rio_parseExpr(vm);
  if (e.access & rio_Access_ref) { /* name := &place: bound once to that place, like a for-each element */
    int s, d;
    if (type >= 0) rio_FAIL(, "a reference takes its type from its place: write name := &x");
    if (c->exporting) rio_FAIL(, "a reference can't be exported");
    e.access = 0;
    s = rio_toAddrSlot(vm, &e);
    c->freeSlot = c->liveSlots;
    d = rio_allocSlots(vm, 1);
    c->liveSlots = c->freeSlot;
    if (s != d) rio_emit(vm, rio_Op_mov, d, s, 0);
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
    fields[n] = names[n] = c->tok.text;
    fieldLens[n] = nameLens[n] = c->tok.len;
    rio_next(vm);
    if (rio_tokIs(vm, "as", 2)) { /* 'as' only means something here */
      rio_next(vm);
      if (c->tok.kind != rio_Tok_id) rio_FAIL(, "name expected after 'as'");
      names[n] = c->tok.text;
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
      int s = k == rio_TypeKind_slice || k == rio_TypeKind_builder ? rio_toViewSlots(vm, &fe) : rio_toSlot(vm, &fe, 0);
      fe = rio_exprOf(rio_ExprKind_static, rio_valueType(f->type), s * 4);
    }
    rio_declare(vm, names[i], nameLens[i], -1, &fe, 0);
  }
}

#endif
