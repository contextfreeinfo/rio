/* expr.c - the expression parser: shunting-yard, with a value stack (rio_Expr) and an operator stack
   (rio_PendingOp) that also holds open brackets: calls, struct literals and indexing. Code is emitted
   as operators reduce, so values are usually already in slots by the time they're used. What they
   mean: docs/spec/expressions.md. */
#include "compiler.h"
#ifndef rio_NO_COMPILER

/* ------------------------------------------------------------------ the stacks */

/* after an error the stacks can come up short: the empty cases only happen then, and give harmless values */
rio_Expr *rio_valueTop(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  return &c->values[c->valueCount > 0 ? c->valueCount - 1 : 0];
}

rio_Expr rio_popValue(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  return c->valueCount > 0 ? c->values[--c->valueCount] : rio_voidExpr();
}

static void rio_pushValue(rio_Vm *vm, rio_Expr e) {
  rio_Compiler *c = vm->compiler;
  if (c->valueCount >= (int)c->limits.exprDepth) rio_FAIL(, "expression too complex");
  c->values[c->valueCount++] = e;
}

/* pushes a result whose temporaries start at slot tempStart */
void rio_pushResult(rio_Vm *vm, rio_Expr e, int tempStart) {
  e.tempStart = (uint16_t)tempStart;
  rio_pushValue(vm, e);
}

static rio_PendingOp *rio_pushOp(rio_Vm *vm, int kind, int prec, int op) {
  rio_Compiler *c = vm->compiler;
  rio_PendingOp *o;
  if (c->opCount >= (int)c->limits.exprDepth) rio_FAIL(&c->ops[0], "expression too complex"); /* (somewhere harmless to write) */
  o = &c->ops[c->opCount++];
  o->kind = (uint8_t)kind;
  o->prec = (uint8_t)prec;
  o->op = (int16_t)op;
  o->a = o->b = o->c = o->argCount = o->punAt = 0;
  o->fieldsSet = 0;
  o->valueBase = (uint16_t)c->valueCount;
  o->freeAtStart = (uint16_t)c->freeSlot;
  return o;
}

static rio_PendingOp *rio_opTop(rio_Vm *vm) { return &vm->compiler->ops[vm->compiler->opCount - 1]; }

/* Binding strength, like Python's: or loosest, then and, then not (3: not a == b is not (a == b), and
   not a and b is (not a) and b), then comparisons and is. 0: not a binary operator. */
static int rio_binaryPrecedence(int tok) {
  switch (tok) {
  case rio_Tok_or: return 1;
  case rio_Tok_and: return 2;
  case rio_Tok_eqEq: case rio_Tok_notEq: case '<': case '>': case rio_Tok_lessEq: case rio_Tok_greaterEq: return 4;
  case '+': case '-': case '|': case '^': return 5;
  case '*': case '/': case '%': case '&': case rio_Tok_shiftLeft: case rio_Tok_shiftRight: return 6;
  case rio_Tok_is: return 4;
  }
  return 0;
}

/* applies the operator on top of the stack */
static void rio_reduceOne(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  rio_PendingOp o;
  rio_Expr r;
  int s, d;
  if (c->opCount <= 0) return;
  o = c->ops[--c->opCount];
  if (o.kind == rio_Pending_unary) {
    rio_unaryOp(vm, o.op, rio_valueTop(vm));
    return;
  }
  r = rio_popValue(vm);
  if (o.kind == rio_Pending_binary) {
    rio_binaryOp(vm, o.op, rio_valueTop(vm), &r);
    return;
  }
  /* and / or: the right side goes into the result slot too, where the left side's jump lands */
  d = o.b;
  rio_isTestUsed(vm, &r, o.kind == rio_Pending_and ? d * 4 : -1);
  rio_needValue(vm, &r);
  if (r.type != rio_TypeId_bool) rio_FAIL(, "expected Bool");
  s = rio_toSlot(vm, &r, 1);
  c->freeSlot = (uint32_t)d + 1;
  if (s != d && !rio_retarget(vm, s, d)) rio_emit(vm, rio_Op_mov, d, s, 0);
  rio_patchJumps(vm, o.a, rio_label(vm));
}

/* ------------------------------------------------------------------ names and literals */

rio_Expr rio_symbolExpr(rio_Vm *vm, int sym) {
  rio_Compiler *c = vm->compiler;
  rio_Symbol *y = &c->syms[sym];
  rio_Expr e;
  switch (y->kind) {
  case rio_SymKind_module: return rio_exprOf(rio_ExprKind_module, 0, y->value);
  case rio_SymKind_var:
    e = rio_exprOf(rio_ExprKind_static, y->type, y->value);
    if (rio_isLocalMemory(vm, &e) && (uint32_t)e.value < vm->top) e.localView = (uint8_t)rio_bitGet(c->localViews, (uint32_t)e.value / 4);
    return e;
  case rio_SymKind_const: { /* scalars are the value itself; a String, its constant */
    int k = rio_type(vm, y->type)->kind;
    int scalar = k <= rio_TypeKind_float || k == rio_TypeKind_bool || k == rio_TypeKind_enum;
    e = rio_exprOf(scalar ? rio_ExprKind_const : rio_ExprKind_static, y->type, y->value);
    e.access = rio_Access_readOnly;
    return e;
  }
  case rio_SymKind_proc:
    if (!c->procDefs[y->value].defined) rio_FAIL(rio_voidExpr(), "recursion is not allowed");
    return rio_exprOf(rio_ExprKind_proc, 0, y->value);
  case rio_SymKind_narrowed:
    e = rio_exprOf(rio_ExprKind_static, y->type, y->value);
    e.access = rio_Access_narrowed;
    return e;
  case rio_SymKind_ref:
  case rio_SymKind_readOnlyRef: /* value: the slot holding the receiver's (or loop element's) address */
    e = rio_exprOf(rio_ExprKind_indirect, y->type, y->value);
    e.access = y->kind == rio_SymKind_readOnlyRef;
    return e;
  case rio_SymKind_ffi: return rio_exprOf(rio_ExprKind_ffi, 0, y->value);
  case rio_SymKind_builtin:
    if (y->value == rio_Builtin_len || y->value == rio_Builtin_cap)
      rio_FAIL(rio_voidExpr(), y->value == rio_Builtin_len ? "len is a method: write x.len()" : "cap is a method: write x.cap()");
    return rio_exprOf(rio_ExprKind_builtin, 0, y->value);
  default: return rio_exprOf(rio_ExprKind_type, y->type, 0);
  }
}

static rio_Expr rio_identifier(rio_Vm *vm) {
  rio_Token *t = &vm->compiler->tok;
  int i = rio_lookup(vm, t->text, t->len);
  if (i < 0) rio_FAIL_AT_TOKEN(rio_voidExpr(), "undefined name");
  return rio_symbolExpr(vm, i);
}

/* A string literal's text (escapes still in it) into the pool, and its (address, length) constants. The
   address is a pool offset until the strings move to their final place (constIsString). */
rio_Expr rio_stringConst(rio_Vm *vm, const char *s, int n) {
  rio_Compiler *c = vm->compiler;
  rio_Val *R = (rio_Val *)vm->mem;
  uint8_t *out = c->pool + c->poolUsed;
  uint32_t k = vm->constCount;
  int i, len = 0;
  rio_Expr e;
  if ((uint32_t)n > c->poolCap - c->poolUsed) rio_FAIL(rio_voidExpr(), "out of compiler memory");
  if (k + 2 > vm->constCap) rio_FAIL(rio_voidExpr(), "too many constants");
  for (i = 0; i < n; i++) out[len++] = (uint8_t)(s[i] == '\\' && i + 1 < n ? rio_escapeChar(s[++i]) : s[i]);
  R[k].u = 0x80000000u | c->poolUsed;
  R[k + 1].u = (uint32_t)len;
  rio_bitSet(c->constIsString, k);
  vm->constCount += 2;
  c->poolUsed += (uint32_t)len;
  e = rio_exprOf(rio_ExprKind_static, rio_TypeId_string, (int32_t)k * 4);
  e.access = rio_Access_readOnly;
  return e;
}

/* the constant for -D define `define` as type `type` (type < 0: inferred from the text) */
rio_Expr rio_defineValue(rio_Vm *vm, int define, int type) {
  const char *name = vm->defines[define].name, *v = vm->defines[define].value ? vm->defines[define].value : "";
  int n = (int)strlen(v);
  rio_Token k;
  if (type < 0) {
    if (!n || !strcmp(v, "true") || !strcmp(v, "false")) type = rio_TypeId_bool;
    else if (!rio_lexNumber(vm, v, &k)) type = rio_TypeId_string;
    else type = k.kind == rio_Tok_int ? rio_TypeId_int : rio_TypeId_float;
  }
  if (type == rio_TypeId_string) /* surrounding quotes are optional */
    return n >= 2 && v[0] == '"' && v[n - 1] == '"' ? rio_stringConst(vm, v + 1, n - 2) : rio_stringConst(vm, v, n);
  if (type == rio_TypeId_bool) {
    if (!n || !strcmp(v, "true")) return rio_exprOf(rio_ExprKind_const, rio_TypeId_bool, 1);
    if (!strcmp(v, "false")) return rio_exprOf(rio_ExprKind_const, rio_TypeId_bool, 0);
    if (rio_lexNumber(vm, v, &k) && k.kind == rio_Tok_int) return rio_exprOf(rio_ExprKind_const, rio_TypeId_bool, k.val.i != 0);
  } else if (rio_lexNumber(vm, v, &k) && (type == rio_TypeId_float || k.kind == rio_Tok_int)) {
    if (type == rio_TypeId_float && k.kind == rio_Tok_int) k.val.f = (float)k.val.i;
    return rio_exprOf(rio_ExprKind_const, type, k.val.i);
  }
  { /* -D N: value doesn't fit Int */
    char text[rio_ERROR_BUF_SIZE];
    int len = rio_appendText(text, 0, "-D ");
    len = rio_appendText(text, len, name);
    len = rio_appendText(text, len, ": value doesn't fit ");
    len = rio_appendText(text, len, type == rio_TypeId_int ? "Int" : type == rio_TypeId_float ? "Float" : "Bool");
    text[len] = 0;
    rio_failAt(vm, 0, 0, 0, text);
    vm->compiler->failMessage = 0;
    return rio_voidExpr();
  }
}

/* ------------------------------------------------------------------ members */

/* v.x, v.zy, v.rgba on an array of 1-4 Ints, Floats or Bools: one letter is that element (it can be
   assigned), more build a new read-only array of those elements. 0: the name isn't a swizzle */
static int rio_swizzle(rio_Vm *vm, rio_Expr *e) {
  static const char letters[] = "xyzwrgba";
  rio_Token *t = &vm->compiler->tok;
  rio_Type *ty = rio_type(vm, e->type);
  int index[4], j, n = t->len, elem = ty->elem, elemKind = rio_type(vm, elem)->kind, d, sets = 0;
  if (ty->kind != rio_TypeKind_array || ty->firstField || ty->count > 4 ||
      (elemKind != rio_TypeKind_int && elemKind != rio_TypeKind_float && elemKind != rio_TypeKind_bool) || n > 4)
    return 0;
  for (j = 0; j < n; j++) {
    const char *p = memchr(letters, t->text[j], 8);
    if (!p) return 0;
    index[j] = (int)(p - letters);
    sets |= index[j] < 4 ? 1 : 2; /* xyzw or rgba */
    index[j] &= 3;
  }
  if (sets == 3) rio_FAIL_AT_TOKEN(0, "use xyzw or rgba, not both");
  for (j = 0; j < n; j++)
    if ((uint32_t)index[j] >= ty->count) rio_FAIL_AT_TOKEN(0, "no such element: the array is shorter");
  if (n == 1) {
    rio_offsetPlace(e, index[0] * 4);
    e->type = (uint16_t)elem;
    return 1;
  }
  d = rio_allocSlots(vm, n);
  for (j = 0; j < n; j++) {
    rio_Expr src = *e, dst = rio_exprOf(rio_ExprKind_static, elem, (d + j) * 4);
    rio_offsetPlace(&src, index[j] * 4);
    src.type = (uint16_t)elem;
    rio_store(vm, &dst, &src);
  }
  e->kind = rio_ExprKind_static;
  e->value = d * 4;
  e->offset = 0;
  e->type = (uint16_t)rio_arrayOf(vm, elem, (uint32_t)n);
  e->access = rio_Access_readOnly;
  return 1;
}

/* x.name, with name the current token: a field, a module's name, an enum value, or a method (pushed
   bound to x, which stays below it on the stack) */
static void rio_member(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  rio_Token *t = &c->tok;
  rio_Expr *e = rio_valueTop(vm), method;
  rio_Type *ty;
  int i, k;
  if (e->kind == rio_ExprKind_module) { /* mod.name: one of the module's exported names */
    uint16_t tempStart = e->tempStart;
    if ((i = rio_lookupExport(vm, e->value, t->text, t->len)) < 0) rio_FAIL(, "not exported by that module");
    *e = rio_symbolExpr(vm, i);
    e->tempStart = tempStart;
    return;
  }
  if (e->kind == rio_ExprKind_type && rio_type(vm, e->type)->kind == rio_TypeKind_enum) { /* Enum.value, Enum.fromInt, Enum.has */
    int32_t v;
    uint16_t tempStart = e->tempStart;
    if (rio_findEnumValue(vm, e->type, t->text, t->len, &v)) {
      *e = rio_exprOf(rio_ExprKind_const, e->type, v);
      e->tempStart = tempStart;
      return;
    }
    if (rio_tokIs(vm, "fromInt", 7) || rio_tokIs(vm, "has", 3)) {
      if (c->peek.kind != '(') rio_FAIL(, "call it: Enum.fromInt(n), Enum.has(n)");
      *e = rio_exprOf(rio_ExprKind_builtin, e->type, t->len == 3 ? rio_Builtin_has : rio_Builtin_fromInt);
      e->tempStart = tempStart;
      return;
    }
    rio_FAIL_AT_TOKEN(, "not a value of this enum");
  }
  rio_needValue(vm, e);
  ty = rio_type(vm, e->type);
  if (ty->kind == rio_TypeKind_union && rio_lookupMethod(vm, e->type, t->text, t->len) < 0)
    rio_FAIL_AT_TOKEN(, "a union's value is reachable inside switch u / case T, or if u is T");
  if ((i = rio_findField(vm, ty, t->text, t->len)) >= 0) {
    rio_Field *f = rio_field(vm, ty->firstField + i);
    rio_offsetPlace(e, f->offset);
    e->type = f->type;
    return;
  }
  if (ty->kind == rio_TypeKind_array && rio_swizzle(vm, e)) return;
  if ((ty->kind == rio_TypeKind_array || ty->kind == rio_TypeKind_slice || ty->kind == rio_TypeKind_list ||
       ty->kind == rio_TypeKind_builder) && t->len == 3 && (!memcmp(t->text, "len", 3) || !memcmp(t->text, "cap", 3))) {
    method = rio_exprOf(rio_ExprKind_builtin, 0, t->text[0] == 'l' ? rio_Builtin_len : rio_Builtin_cap); /* x.len(), x.cap() */
  } else if (ty->kind != rio_TypeKind_struct && ty->kind != rio_TypeKind_list && ty->kind != rio_TypeKind_builder &&
             (k = rio_builtinNamed(t->text, t->len, rio_Builtin_toInt, rio_Builtin_asString)) >= 0) {
    method = rio_exprOf(rio_ExprKind_builtin, 0, k); /* conversions */
  } else if (ty->kind == rio_TypeKind_list || ty->kind == rio_TypeKind_builder) {
    if ((k = rio_builtinNamed(t->text, t->len, rio_Builtin_push, rio_Builtin_format)) < 0) rio_FAIL(, "no such method");
    method = rio_exprOf(rio_ExprKind_builtin, 0, k);
  } else {
    if ((i = rio_lookupMethod(vm, rio_valueType(e->type), t->text, t->len)) < 0)
      rio_FAIL(, ty->kind == rio_TypeKind_struct ? "no such field or method"
                 : ty->kind == rio_TypeKind_array ? "no such method (swizzles like .x .zy .rgba need 1-4 letters, on arrays of 1-4 numbers)"
                                                   : "no such method");
    if (!c->procDefs[c->syms[i].value].defined) rio_FAIL(, "recursion is not allowed");
    method = rio_exprOf(rio_ExprKind_proc, 0, c->syms[i].value);
  }
  if (c->peek.kind != '(') rio_FAIL(, "method calls need ()");
  method.access = rio_Access_boundMethod;
  rio_pushValue(vm, method);
}

/* ------------------------------------------------------------------ indexing and slicing */

/* o[i]: o becomes the element */
static void rio_index(rio_Vm *vm, rio_Expr *o, rio_Expr *i) {
  rio_Compiler *c = vm->compiler;
  rio_Type *ty;
  int elem, size, s, ix, d, readOnly;
  rio_needValue(vm, o);
  rio_needValue(vm, i);
  ty = rio_type(vm, o->type);
  o->localView = (uint8_t)rio_isLocalView(vm, o);
  if (ty->kind != rio_TypeKind_array && ty->kind != rio_TypeKind_slice && ty->kind != rio_TypeKind_list && ty->kind != rio_TypeKind_builder)
    rio_FAIL(, "cannot index this");
  if (ty->kind == rio_TypeKind_array && ty->firstField) { /* [Enum]T: the enum's value is the element */
    if (rio_valueType(i->type) != ty->firstField) rio_FAIL(, "index this array with its enum");
    i->type = rio_TypeId_int;
  }
  if (rio_valueType(i->type) != rio_TypeId_int) rio_FAIL(, "index must be Int");
  elem = ty->elem;
  size = (int)rio_type(vm, elem)->size;
  readOnly = o->access || o->type == rio_TypeId_string;
  if (ty->kind == rio_TypeKind_list || ty->kind == rio_TypeKind_builder) { /* checked against the current length */
    s = rio_toSlot3(vm, o);
    ix = rio_toSlot(vm, i, 1);
    c->freeSlot = o->tempStart;
    d = rio_allocSlots(vm, 1);
    rio_emit(vm, rio_Op_lidx, d, s, ix);
    rio_emitData(vm, 0, (uint32_t)size);
    o->kind = rio_ExprKind_indirect;
    o->value = d;
    o->offset = 0;
    o->type = (uint16_t)elem;
    o->access = 0;
    return;
  }
  if (ty->kind == rio_TypeKind_array && o->kind == rio_ExprKind_static && i->kind == rio_ExprKind_const) { /* known now */
    if ((uint32_t)i->value >= ty->count) rio_FAIL(, "index out of bounds");
    o->value += i->value * size;
    o->type = (uint16_t)elem;
    return;
  }
  s = rio_toSlot2(vm, o);
  ix = rio_toSlot(vm, i, 1);
  c->freeSlot = o->tempStart;
  d = rio_allocSlots(vm, 1);
  rio_emit(vm, rio_Op_idx, d, s, ix);
  rio_emitData(vm, 0, (uint32_t)size);
  o->kind = rio_ExprKind_indirect;
  o->value = d;
  o->offset = 0;
  o->type = (uint16_t)elem;
  o->access = (uint8_t)readOnly;
}

/* o[lo:hi]: o becomes the slice (hi of kind sliceEnd: left out) */
static void rio_slice(rio_Vm *vm, rio_Expr *o, rio_Expr *lo, rio_Expr *hi) {
  rio_Compiler *c = vm->compiler;
  rio_Type *ty;
  int s, l, h, d, localView;
  rio_needValue(vm, o);
  ty = rio_type(vm, o->type);
  localView = rio_isLocalView(vm, o);
  if (ty->kind == rio_TypeKind_list || ty->kind == rio_TypeKind_builder) { /* a slice of the live elements */
    int builder = rio_toSlot3(vm, o), view = rio_allocSlots(vm, 2), tempStart = o->tempStart;
    rio_emit(vm, rio_Op_lview, view, builder, 0);
    *o = rio_exprOf(rio_ExprKind_static, rio_sliceOf(vm, ty->elem), view * 4);
    o->tempStart = (uint16_t)tempStart;
    ty = rio_type(vm, o->type);
  }
  if (ty->kind != rio_TypeKind_array && ty->kind != rio_TypeKind_slice) rio_FAIL(, "cannot slice this");
  if (ty->kind == rio_TypeKind_array && rio_isTemp(vm, o)) rio_FAIL(, "a computed array has no home to view: store it in a variable first");
  if (ty->kind == rio_TypeKind_array && ty->firstField) { /* counts[.east:.west]: bounds are the enum's values, which are the positions */
    int loLeftOut = lo->kind == rio_ExprKind_const && lo->type == rio_TypeId_int && lo->offset == 1;
    if ((!loLeftOut && rio_valueType(lo->type) != ty->firstField) ||
        (hi->kind != rio_ExprKind_sliceEnd && rio_valueType(hi->type) != ty->firstField))
      rio_FAIL(, "slice this array with its enum's values");
    lo->type = rio_TypeId_int;
    if (hi->kind != rio_ExprKind_sliceEnd) hi->type = rio_TypeId_int;
  }
  if (rio_valueType(lo->type) != rio_TypeId_int || (hi->kind != rio_ExprKind_sliceEnd && rio_valueType(hi->type) != rio_TypeId_int))
    rio_FAIL(, "slice bounds must be Int");
  s = rio_toSlot2(vm, o);
  l = rio_toSlot(vm, lo, 1);
  h = hi->kind == rio_ExprKind_sliceEnd ? s + 1 : rio_toSlot(vm, hi, 1);
  c->freeSlot = o->tempStart;
  d = rio_allocSlots(vm, 2);
  rio_emit(vm, rio_Op_slice, d, s, l);
  rio_emitData(vm, h, rio_type(vm, ty->elem)->size);
  o->kind = rio_ExprKind_static;
  o->value = d * 4;
  o->offset = 0;
  o->localView = (uint8_t)localView;
  if (ty->kind == rio_TypeKind_array) o->type = (uint16_t)rio_sliceOf(vm, ty->elem);
}

/* ------------------------------------------------------------------ struct literals */

/* inside Type{...}: the value that follows is for field `name` */
static void rio_setField(rio_Vm *vm, rio_PendingOp *lit, const char *name, int n) {
  int i = rio_findField(vm, rio_type(vm, lit->b), name, n);
  if (i < 0) rio_FAIL(, "no such field");
  if (i < 64 && (lit->fieldsSet >> i & 1)) rio_FAIL(, "field set twice");
  if (i < 64) lit->fieldsSet |= (uint64_t)1 << i;
  lit->a = i;
}

/* at the start of a literal's next value: `field =`, or a value standing for its field ({x}, {a.x}) */
static void rio_literalField(rio_Vm *vm, rio_PendingOp *lit) {
  rio_Compiler *c = vm->compiler;
  if (c->tok.kind == '}') return;
  if (c->tok.kind != rio_Tok_id) rio_FAIL(, "struct literal fields must be named: Type{field = value}");
  if (c->peek.kind == '=') {
    rio_setField(vm, lit, c->tok.text, c->tok.len);
    rio_next(vm);
    rio_next(vm);
    return;
  }
  lit->a = -1; /* named after the path's last name, once the value is read */
  lit->punAt = (int32_t)(c->tok.text - c->src);
}

/* a value standing for its field must be a name or a dotted path; its last name is the field */
static void rio_punField(rio_Vm *vm, rio_PendingOp *lit) {
  rio_Compiler *c = vm->compiler;
  const char *p = c->src + lit->punAt, *e = c->tok.text, *name = 0;
  int nameLen = 0;
  while (e > p && rio_isSpace(e[-1])) e--;
  while (p < e) {
    if (rio_isAlpha(*p)) {
      name = p;
      while (p < e && (rio_isAlpha(*p) || rio_isDigit(*p))) p++;
      nameLen = (int)(p - name);
    } else if (*p == '.' || rio_isSpace(*p)) p++;
    else if (*p == '#') {
      while (p < e && *p != '\n') p++;
    } else rio_FAIL(, "only a name or a.b path can stand for a field; write field = value");
  }
  rio_setField(vm, lit, name, nameLen);
}

/* the zeroing for fields a literal left out, then the struct as the result */
static void rio_finishLiteral(rio_Vm *vm, rio_PendingOp *lit) {
  rio_Compiler *c = vm->compiler;
  rio_Type *st = rio_type(vm, lit->b);
  rio_Field *fields = rio_field(vm, st->firstField);
  int i = 0, j, w, from, to;
  while (st->fieldCount <= 64 && i < st->fieldCount) { /* (structs with more fields were zeroed up front) */
    if (lit->fieldsSet >> i & 1) {
      i++;
      continue;
    }
    for (j = i; j < st->fieldCount && !(lit->fieldsSet >> j & 1); j++) {}
    from = fields[i].offset; /* fields [i, j) are unset: bytes [from, to) */
    to = j < st->fieldCount ? fields[j].offset : (int)st->size;
    if (to - from <= 64)
      for (w = from / 4; w < to / 4; w++) rio_emit(vm, rio_Op_mov, lit->c / 4 + w, rio_constSlot(vm, 0), 0);
    else rio_emit(vm, rio_Op_zero, rio_addrConst(vm, (uint32_t)(lit->c + from), (uint32_t)(to - from)), to - from, 0);
    i = j;
  }
  c->freeSlot = (uint32_t)(lit->c / 4 + rio_typeWords(vm, lit->b));
  rio_pushResult(vm, rio_exprOf(rio_ExprKind_static, lit->b, lit->c), lit->freeAtStart);
}

/* ------------------------------------------------------------------ calls */

/* one argument of a call or struct literal is complete (on top of the value stack) */
static void rio_argDone(rio_Vm *vm, rio_PendingOp *m) {
  rio_Compiler *c = vm->compiler;
  rio_Expr *arg = rio_valueTop(vm), e;
  rio_Param *param = 0;
  rio_isTestUsed(vm, arg, -1);
  if (m->kind == rio_Pending_call && m->a == rio_ExprKind_proc && m->argCount < c->procDefs[m->b].paramCount)
    param = &c->params[c->procDefs[m->b].firstParam + m->argCount];
  if (arg->kind == rio_ExprKind_proc) { /* a proc passed where a proc value goes */
    int type = -1;
    if (param) type = param->type;
    else if (m->kind == rio_Pending_call && m->a == rio_ExprKind_static && m->argCount < rio_type(vm, m->c)->fieldCount)
      type = rio_field(vm, rio_type(vm, m->c)->firstField + m->argCount)->type;
    else if (m->kind == rio_Pending_literal && m->a >= 0) type = rio_field(vm, rio_type(vm, m->b)->firstField + m->a)->type;
    if (type >= 0 && rio_type(vm, type)->kind == rio_TypeKind_proc) rio_toProcValue(vm, arg, type);
  }
  if (param && param->refType) { /* &place: checked here, its address taken when the call is made */
    if (!(arg->access & rio_Access_ref)) rio_FAIL(, "this parameter is a reference: pass &x");
    arg->access = 0;
    if (arg->type != param->refType) rio_FAIL(, "type mismatch");
    m->argCount++;
    return;
  }
  rio_needValue(vm, arg);
  if (m->kind == rio_Pending_literal) { /* stored into the struct at once */
    rio_Type *st = rio_type(vm, m->b);
    rio_Field *f;
    rio_Expr dst;
    if (m->a < 0) rio_punField(vm, m);
    f = rio_field(vm, st->firstField + m->a);
    e = rio_popValue(vm);
    dst = rio_exprOf(rio_ExprKind_static, f->type, m->c + f->offset);
    rio_store(vm, &dst, &e);
    c->freeSlot = (uint32_t)(m->c / 4 + rio_typeWords(vm, m->b));
  } else if (m->a == rio_ExprKind_proc || m->a == rio_ExprKind_static) { /* kept on the stack until the call */
    int k = rio_type(vm, arg->type)->kind;
    int max = m->a == rio_ExprKind_proc ? c->procDefs[m->b].paramCount : rio_type(vm, m->c)->fieldCount;
    if (m->argCount >= max) rio_FAIL(, "too many arguments");
    if (arg->kind == rio_ExprKind_indirect && !rio_isAggregate(k)) { /* read it now, before anything changes it */
      int s = k == rio_TypeKind_slice || k == rio_TypeKind_builder ? rio_toViewSlots(vm, arg) : rio_toSlot(vm, arg, 1);
      arg->kind = rio_ExprKind_static;
      arg->value = s * 4;
      arg->type = (uint16_t)rio_valueType(arg->type);
    }
  } else if (m->a == rio_ExprKind_ffi) { /* into consecutive slots after the call's start */
    rio_FfiDef *f = &c->ffi[m->b];
    int type, words, s, d;
    if (m->argCount >= f->paramCount) rio_FAIL(, "too many arguments");
    e = rio_popValue(vm);
    type = c->params[f->firstParam + m->argCount].type;
    rio_coerce(vm, &e, type);
    words = rio_typeWords(vm, type);
    s = words == 2 ? rio_toSlot2(vm, &e) : rio_toSlot(vm, &e, 1);
    c->freeSlot = (uint32_t)(m->freeAtStart + m->c);
    d = rio_allocSlots(vm, words);
    if (s != d && !(words == 1 && rio_retarget(vm, s, d))) rio_emit(vm, words == 2 ? rio_Op_mov2 : rio_Op_mov, d, s, 0);
    m->c += words;
  } else if (m->a == rio_ExprKind_builtin && m->b == rio_Builtin_format) rio_formatArg(vm, m);
  else if (m->a == rio_ExprKind_builtin && m->b == rio_Builtin_log) rio_logArg(vm, m);
  m->argCount++;
}

/* a call's arguments are all there: make it, and push its result */
static void rio_finishCall(rio_Vm *vm, rio_PendingOp *m) {
  rio_Compiler *c = vm->compiler;
  int i, d;
  if (m->a == rio_ExprKind_proc) { /* arguments into the proc's parameters, then call */
    rio_ProcDef *p = &c->procDefs[m->b];
    if (m->argCount != p->paramCount) rio_FAIL(, "wrong number of arguments");
    for (i = 0; i < p->paramCount; i++) {
      rio_Param *param = &c->params[p->firstParam + i];
      rio_Expr dst = rio_exprOf(rio_ExprKind_static, param->type, (int32_t)param->addr);
      if ((!i && p->selfByRef) || param->refType) { /* the place's address */
        rio_emit(vm, rio_Op_mov, (int)(dst.value / 4), rio_toAddrSlot(vm, &c->values[m->valueBase + i]), 0);
        continue;
      }
      c->storingArgs = 1;
      rio_store(vm, &dst, &c->values[m->valueBase + i]);
      c->storingArgs = 0;
    }
    c->valueCount = m->valueBase;
    rio_emit(vm, rio_Op_call, 0, 0, p->entry);
    if (p->callsValues && c->curProc >= 0) c->procDefs[c->curProc].callsValues = 1;
    c->freeSlot = m->freeAtStart;
    if (p->resultType == rio_TypeId_void) {
      rio_pushResult(vm, rio_voidExpr(), m->freeAtStart);
      return;
    }
    { /* the result is copied out: the next call of the proc would overwrite it */
      rio_Expr src = rio_exprOf(rio_ExprKind_static, p->resultType, (int32_t)p->resultAt), dst;
      d = rio_allocSlots(vm, rio_typeWords(vm, p->resultType));
      dst = rio_exprOf(rio_ExprKind_static, p->resultType, d * 4);
      rio_store(vm, &dst, &src);
      rio_pushResult(vm, dst, m->freeAtStart);
    }
  } else if (m->a == rio_ExprKind_static) { /* through a proc value: the arguments go to a scratch area, the call copies them over */
    rio_Type *type = rio_type(vm, m->c);
    int words = 0, base, at = 0;
    if (m->argCount != type->fieldCount) rio_FAIL(, "wrong number of arguments");
    for (i = 0; i < type->fieldCount; i++) words += rio_typeWords(vm, rio_field(vm, type->firstField + i)->type);
    base = rio_allocSlots(vm, words ? words : 1);
    for (i = 0; i < type->fieldCount; i++) {
      int paramType = rio_field(vm, type->firstField + i)->type;
      rio_Expr dst = rio_exprOf(rio_ExprKind_static, paramType, (base + at) * 4);
      c->storingArgs = 1;
      rio_store(vm, &dst, &c->values[m->valueBase + i]);
      c->storingArgs = 0;
      at += rio_typeWords(vm, paramType);
    }
    c->valueCount = m->valueBase;
    rio_emit(vm, rio_Op_calli, m->b, base, words);
    if (c->curProc >= 0) c->procDefs[c->curProc].callsValues = 1;
    c->freeSlot = m->freeAtStart;
    if (type->elem == rio_TypeId_void) {
      rio_pushResult(vm, rio_voidExpr(), m->freeAtStart);
      return;
    }
    d = rio_allocSlots(vm, rio_typeWords(vm, type->elem));
    rio_emit(vm, rio_Op_getret, d, m->b, rio_typeWords(vm, type->elem));
    rio_pushResult(vm, rio_exprOf(rio_ExprKind_static, type->elem, d * 4), m->freeAtStart);
  } else if (m->a == rio_ExprKind_ffi) { /* the arguments are in place; the result comes back in the first slot */
    rio_FfiDef *f = &c->ffi[m->b];
    if (m->argCount != f->paramCount) rio_FAIL(, "wrong number of arguments");
    rio_emit(vm, rio_Op_ffi, m->freeAtStart, 0, m->b);
    c->freeSlot = m->freeAtStart;
    if (f->resultType == rio_TypeId_void) rio_pushResult(vm, rio_voidExpr(), m->freeAtStart);
    else rio_pushResult(vm, rio_exprOf(rio_ExprKind_static, f->resultType, rio_allocSlots(vm, rio_typeWords(vm, f->resultType)) * 4), m->freeAtStart);
  } else if (m->a == rio_ExprKind_builtin) rio_finishBuiltinCall(vm, m);
}

/* a closing bracket t (hasArg: a value before it) */
static void rio_closeBracket(rio_Vm *vm, int t, int hasArg) {
  rio_Compiler *c = vm->compiler;
  rio_PendingOp *m = rio_opTop(vm), o;
  if (t == ')' && m->kind == rio_Pending_paren) {
    c->opCount--;
    return;
  }
  if (t == ']') {
    rio_Expr i, hi;
    if (m->kind != rio_Pending_index) rio_FAIL(, "unexpected ']'");
    o = c->ops[--c->opCount];
    if (o.a) {
      hi = rio_popValue(vm);
      i = rio_popValue(vm);
      rio_slice(vm, rio_valueTop(vm), &i, &hi);
    } else {
      i = rio_popValue(vm);
      rio_index(vm, rio_valueTop(vm), &i);
    }
    return;
  }
  if (m->kind != (t == ')' ? rio_Pending_call : rio_Pending_literal)) rio_FAIL(, t == ')' ? "unexpected ')'" : "unexpected '}'");
  if (hasArg) rio_argDone(vm, m);
  o = c->ops[--c->opCount];
  if (t == ')') rio_finishCall(vm, &o);
  else rio_finishLiteral(vm, &o);
}

/* ------------------------------------------------------------------ expressions */

typedef struct {
  int opBase, valueBase; /* the stacks' heights when this expression started */
  int depth;             /* its brackets still open */
  int whole;             /* the type the whole expression is for, or -1 */
} rio_ExprState;

/* what a step of the parser expects next */
enum { rio_Expect_operand, rio_Expect_operator, rio_Expect_done, rio_Expect_failed };

/* The type a value is headed for right now, or -1: what the whole expression is for, or the argument or
   field being filled in by the innermost call or struct literal. */
static int rio_targetType(rio_Vm *vm, const rio_ExprState *x) {
  rio_Compiler *c = vm->compiler;
  rio_PendingOp *m;
  if (c->opCount == x->opBase) return x->whole;
  m = rio_opTop(vm);
  if (m->kind == rio_Pending_literal) return rio_field(vm, rio_type(vm, m->b)->firstField + m->a)->type;
  if (m->kind == rio_Pending_index && m->valueBase > 0 && c->valueCount <= m->valueBase + 1) { /* counts[.east], counts[.a:.b]: an [Enum]T takes its enum */
    rio_Type *array = rio_type(vm, c->values[m->valueBase - 1].type);
    if (array->kind == rio_TypeKind_array && array->firstField) return array->firstField;
  }
  if (m->kind == rio_Pending_call && m->a == rio_ExprKind_proc && m->argCount < c->procDefs[m->b].paramCount)
    return c->params[c->procDefs[m->b].firstParam + m->argCount].type;
  if (m->kind == rio_Pending_call && m->a == rio_ExprKind_static && m->argCount < rio_type(vm, m->c)->fieldCount)
    return rio_field(vm, rio_type(vm, m->c)->firstField + m->argCount)->type;
  if (m->kind == rio_Pending_call && m->a == rio_ExprKind_builtin && m->b == rio_Builtin_push && m->argCount == 1)
    return rio_type(vm, c->values[m->valueBase].type)->elem;
  return -1;
}

/* .value or nil, whose type is where it's going: an enum value, or a union's nil */
static int rio_impliedValue(rio_Vm *vm, rio_ExprState *x, int tempStart) {
  rio_Compiler *c = vm->compiler;
  int t = c->tok.kind, type = rio_targetType(vm, x);
  int32_t v;
  if (type < 0 && c->opCount > x->opBase && rio_opTop(vm)->kind == rio_Pending_binary && c->valueCount > x->valueBase) { /* x == .value */
    int op = rio_opTop(vm)->op;
    if (op == rio_Tok_eqEq || op == rio_Tok_notEq || op == '<' || op == '>' || op == rio_Tok_lessEq || op == rio_Tok_greaterEq || op == rio_Tok_is)
      type = rio_valueTop(vm)->type;
  }
  if (t == '.') rio_next(vm);
  if (t == rio_Tok_nil && type >= 0 && rio_type(vm, type)->kind == rio_TypeKind_union) { /* a union's nil: its tag, as a constant */
    rio_Type *u = rio_type(vm, type);
    int j;
    for (j = 0; j < u->fieldCount && rio_field(vm, u->firstField + j)->type != rio_TypeId_void; j++) {}
    if (j == u->fieldCount) rio_FAIL_AT_TOKEN(rio_Expect_failed, "this union doesn't list nil");
    rio_pushResult(vm, rio_exprOf(rio_ExprKind_const, type, j), tempStart);
    rio_next(vm);
    return rio_Expect_operator;
  }
  if (type < 0 || rio_type(vm, type)->kind != rio_TypeKind_enum)
    rio_FAIL_AT_TOKEN(rio_Expect_failed, t == rio_Tok_nil ? "nil needs an enum or union that lists nil first" : "can't tell which enum this is; write Enum.value");
  if (!rio_findEnumValue(vm, type, c->tok.text, c->tok.len, &v))
    rio_FAIL_AT_TOKEN(rio_Expect_failed, t == rio_Tok_nil ? "this enum doesn't list nil" : "not a value of this enum");
  rio_pushResult(vm, rio_exprOf(rio_ExprKind_const, type, v), tempStart);
  rio_next(vm);
  return rio_Expect_operator;
}

/* where an operand goes: a value, a prefix operator, an opening bracket, or an empty call's ')' */
static int rio_parseOperand(rio_Vm *vm, rio_ExprState *x) {
  rio_Compiler *c = vm->compiler;
  int t = c->tok.kind, tempStart = (int)c->freeSlot;
  rio_Expr e;
  if (t == rio_Tok_int || t == rio_Tok_float) e = rio_exprOf(rio_ExprKind_const, t == rio_Tok_int ? rio_TypeId_int : rio_TypeId_float, c->tok.val.i);
  else if (t == rio_Tok_string) e = rio_stringConst(vm, c->tok.text, c->tok.len);
  else if (t == rio_Tok_id) e = rio_identifier(vm);
  else if (t == '(') {
    rio_pushOp(vm, rio_Pending_paren, 0, 0);
    x->depth++;
    rio_next(vm);
    return rio_Expect_operand;
  } else if (t == '-' || t == '~' || t == '&') {
    rio_pushOp(vm, rio_Pending_unary, 7, t);
    rio_next(vm);
    return rio_Expect_operand;
  } else if (t == rio_Tok_not) {
    rio_pushOp(vm, rio_Pending_unary, 3, '!');
    rio_next(vm);
    return rio_Expect_operand;
  } else if ((t == ')' || t == '}') && c->opCount > x->opBase &&
             rio_opTop(vm)->kind == (t == ')' ? rio_Pending_call : rio_Pending_literal) &&
             (t == '}' || rio_opTop(vm)->argCount == 0) && c->valueCount == rio_opTop(vm)->valueBase) { /* f(), T{} */
    rio_closeBracket(vm, t, 0);
    x->depth--;
    rio_next(vm);
    return rio_Expect_operator;
  } else if ((t == '.' && c->peek.kind == rio_Tok_id) || t == rio_Tok_nil) {
    return rio_impliedValue(vm, x, tempStart);
  } else if (t == '{') { /* {field = ...} takes its struct type from where the value is going */
    int type = rio_targetType(vm, x);
    if (type < 0 || rio_type(vm, type)->kind != rio_TypeKind_struct)
      rio_FAIL_AT_TOKEN(rio_Expect_failed, "can't tell which struct this is; write Type{...}");
    rio_pushResult(vm, rio_exprOf(rio_ExprKind_type, type, 0), tempStart);
    return rio_Expect_operator; /* the '{' is next: the literal itself */
  } else rio_FAIL_AT_TOKEN(rio_Expect_failed, rio_msgExpectedExpr);
  rio_pushResult(vm, e, tempStart);
  rio_next(vm);
  return rio_Expect_operator;
}

/* f( after a value f: a method on its receiver, a proc value, a proc, ffi function or builtin */
static int rio_openCall(rio_Vm *vm, rio_ExprState *x) {
  rio_Compiler *c = vm->compiler;
  rio_Expr callee = *rio_valueTop(vm);
  rio_PendingOp *o;
  if (callee.access == rio_Access_boundMethod && (callee.kind == rio_ExprKind_proc || callee.kind == rio_ExprKind_builtin)) {
    /* receiver.method(: the receiver is the first argument */
    c->valueCount--;
    o = rio_pushOp(vm, rio_Pending_call, 0, 0);
    o->a = callee.kind;
    o->b = callee.value;
    o->valueBase = (uint16_t)(c->valueCount - 1);
    o->freeAtStart = rio_valueTop(vm)->tempStart;
    if (callee.kind == rio_ExprKind_builtin && callee.value == rio_Builtin_format) rio_argDone(vm, o);
    else o->argCount = 1;
    x->depth++;
    rio_next(vm);
    if (c->tok.kind == ')') {
      rio_closeBracket(vm, ')', 0);
      x->depth--;
      rio_next(vm);
      return rio_Expect_operator;
    }
    return rio_Expect_operand;
  }
  if ((callee.kind == rio_ExprKind_static || callee.kind == rio_ExprKind_indirect || callee.kind == rio_ExprKind_const) &&
      rio_type(vm, callee.type)->kind == rio_TypeKind_proc) { /* f(...) through a proc value */
    int s = rio_toSlot(vm, &callee, 1);
    c->valueCount--;
    o = rio_pushOp(vm, rio_Pending_call, 0, 0);
    o->a = rio_ExprKind_static;
    o->b = s;
    o->c = callee.type;
    x->depth++;
    rio_next(vm);
    return rio_Expect_operand;
  }
  if (callee.kind < rio_ExprKind_proc || callee.kind > rio_ExprKind_type) rio_FAIL(rio_Expect_failed, "not callable");
  if (callee.kind == rio_ExprKind_type)
    rio_FAIL(rio_Expect_failed, rio_type(vm, callee.type)->kind == rio_TypeKind_struct
                                    ? "use Struct{...} to build a struct"
                                    : "convert with x.toInt(), x.toFloat(), x.toBool() or b.asString()");
  c->valueCount--;
  o = rio_pushOp(vm, rio_Pending_call, 0, 0);
  o->a = callee.kind;
  o->b = callee.kind == rio_ExprKind_type ? callee.type : callee.value;
  if (callee.kind == rio_ExprKind_builtin && callee.value >= rio_Builtin_fromInt) o->c = callee.type; /* Enum.fromInt(...): which enum */
  x->depth++;
  rio_next(vm);
  return rio_Expect_operand;
}

/* a , : or closing bracket inside the expression's brackets */
static int rio_separator(rio_Vm *vm, rio_ExprState *x, int t) {
  rio_Compiler *c = vm->compiler;
  rio_PendingOp *m;
  while (c->opCount > 0 && rio_opTop(vm)->kind < rio_Pending_paren) rio_reduceOne(vm);
  m = rio_opTop(vm);
  if (t == ',') {
    if (m->kind != rio_Pending_call && m->kind != rio_Pending_literal) rio_FAIL(rio_Expect_failed, "unexpected ','");
    rio_argDone(vm, m);
    rio_next(vm);
    if (m->kind == rio_Pending_literal) rio_literalField(vm, m);
    return rio_Expect_operand;
  }
  if (t == ':') { /* xs[a:b] */
    if (m->kind != rio_Pending_index || m->a) rio_FAIL(rio_Expect_failed, "unexpected ':'");
    m->a = 1;
    rio_next(vm);
    if (c->tok.kind == ']') { /* the end left out */
      rio_pushResult(vm, rio_exprOf(rio_ExprKind_sliceEnd, rio_TypeId_int, 0), (int)c->freeSlot);
      return rio_Expect_operator;
    }
    return rio_Expect_operand;
  }
  rio_closeBracket(vm, t, 1);
  x->depth--;
  rio_next(vm);
  return rio_Expect_operator;
}

/* a binary operator, after reducing what binds at least as tightly */
static int rio_binaryOperator(rio_Vm *vm, int t, int prec) {
  rio_Compiler *c = vm->compiler;
  if (t == rio_Tok_and || t == rio_Tok_or) { /* the left side into a result slot, then a jump over the right side */
    rio_Expr *l = rio_valueTop(vm);
    rio_PendingOp *o;
    int s, d;
    rio_needValue(vm, l);
    if (l->type != rio_TypeId_bool) rio_FAIL(rio_Expect_failed, "expected Bool");
    s = rio_toSlot(vm, l, 1);
    c->freeSlot = l->tempStart;
    d = rio_allocSlots(vm, 1);
    if (s != d) rio_emit(vm, rio_Op_mov, d, s, 0);
    rio_isTestUsed(vm, l, t == rio_Tok_and ? d * 4 : -1);
    o = rio_pushOp(vm, t == rio_Tok_and ? rio_Pending_and : rio_Pending_or, prec, t);
    o->a = rio_emit(vm, t == rio_Tok_and ? rio_Op_jz : rio_Op_jnz, d, 0, rio_NONE);
    o->b = d;
    l->kind = rio_ExprKind_static;
    l->value = d * 4;
    l->type = rio_TypeId_bool;
    l->access = 0;
  } else rio_pushOp(vm, rio_Pending_binary, prec, t);
  rio_next(vm);
  return rio_Expect_operand;
}

/* after an operand: postfix (calls, indexing, members, literals), separators, binary operators */
static int rio_parseOperator(rio_Vm *vm, rio_ExprState *x) {
  rio_Compiler *c = vm->compiler;
  int t = c->tok.kind, prec;
  if (c->tok.newline && !x->depth) return rio_Expect_done; /* a new line ends it, outside brackets */
  if (t == '(') return rio_openCall(vm, x);
  if (t == '[') {
    rio_pushOp(vm, rio_Pending_index, 0, 0);
    x->depth++;
    rio_next(vm);
    if (c->tok.kind == ':') { /* the start left out: 0, marked so an [Enum]T accepts it */
      rio_Expr zero = rio_exprOf(rio_ExprKind_const, rio_TypeId_int, 0);
      zero.offset = 1;
      rio_pushResult(vm, zero, (int)c->freeSlot);
      return rio_Expect_operator;
    }
    return rio_Expect_operand;
  }
  if (t == '.') {
    rio_next(vm);
    if (c->tok.kind != rio_Tok_id && c->tok.kind != rio_Tok_nil) rio_FAIL(rio_Expect_failed, "field name expected");
    rio_member(vm);
    rio_next(vm);
    return rio_Expect_operator;
  }
  if (t == '{') { /* Type{...} */
    rio_Expr type = *rio_valueTop(vm);
    rio_PendingOp *o;
    if (type.kind != rio_ExprKind_type || rio_type(vm, type.type)->kind != rio_TypeKind_struct) return rio_Expect_done;
    c->valueCount--;
    o = rio_pushOp(vm, rio_Pending_literal, 0, 0);
    o->b = type.type;
    o->c = rio_allocSlots(vm, rio_typeWords(vm, type.type)) * 4;
    if (rio_type(vm, type.type)->fieldCount > 64) /* (finishing tracks only 64 fields: zero all of it now) */
      rio_emit(vm, rio_Op_zero, rio_addrConst(vm, (uint32_t)o->c, rio_type(vm, type.type)->size), (int)rio_type(vm, type.type)->size, 0);
    x->depth++;
    rio_next(vm);
    rio_literalField(vm, o);
    return rio_Expect_operand;
  }
  if (t == ',' || t == ')' || t == ']' || t == '}' || t == ':') return x->depth ? rio_separator(vm, x, t) : rio_Expect_done;
  if (!(prec = rio_binaryPrecedence(t))) return rio_Expect_done;
  while (c->opCount > x->opBase && rio_opTop(vm)->kind < rio_Pending_paren && rio_opTop(vm)->prec >= prec) rio_reduceOne(vm);
  return rio_binaryOperator(vm, t, prec);
}

/* parses an expression, emitting its code, and returns its value */
rio_Expr rio_parseExpr(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  rio_ExprState x;
  int expect = rio_Expect_operand;
  x.opBase = c->opCount;
  x.valueBase = c->valueCount;
  x.depth = 0;
  x.whole = c->targetType;
  c->targetType = -1;
  for (;;) {
    expect = expect == rio_Expect_operand ? rio_parseOperand(vm, &x) : rio_parseOperator(vm, &x);
    if (expect == rio_Expect_failed) return rio_voidExpr();
    if (expect == rio_Expect_done) break;
  }
  while (c->opCount > x.opBase) {
    if (rio_opTop(vm)->kind >= rio_Pending_paren) rio_FAIL(rio_voidExpr(), rio_msgUnclosedBracket);
    rio_reduceOne(vm);
  }
  if (c->valueCount != x.valueBase + 1) rio_FAIL(rio_voidExpr(), "bad expression");
  return rio_popValue(vm);
}

int32_t rio_parsePositiveConst(rio_Vm *vm) {
  rio_Expr e = rio_parseExpr(vm);
  if (e.kind != rio_ExprKind_const || e.type != rio_TypeId_int || e.value <= 0) rio_FAIL(0, "positive integer constant expected");
  return e.value;
}

#endif
