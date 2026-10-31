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
  if ((uint32_t)c->valueCount > c->high.exprDepth) c->high.exprDepth = (uint32_t)c->valueCount;
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
  if ((uint32_t)c->opCount > c->high.exprDepth) c->high.exprDepth = (uint32_t)c->opCount;
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

/* Binding strength, like Python's: ranges (a..<b, a..=b) and or loosest, then and, then not (3: not a == b
   is not (a == b), and not a and b is (not a) and b), then comparisons and is. 0: not a binary operator. */
static int rio_binaryPrecedence(int tok) {
  switch (tok) {
  case rio_Tok_rangeExcl: case rio_Tok_rangeIncl: return 1;
  case rio_Tok_or: return 1;
  case rio_Tok_and: return 2;
  case rio_Tok_eqEq: case rio_Tok_notEq: case '<': case '>': case rio_Tok_lessEq: case rio_Tok_greaterEq: return 4;
  case '+': case '-': case '|': case '^': return 5;
  case '*': case '/': case '%': case '&': case rio_Tok_shiftLeft: case rio_Tok_shiftRight: case rio_Tok_shiftRightU: return 6;
  case rio_Tok_is: return 4;
  }
  return 0;
}

/* The end of an and or or that gives a maybe, or a maybe's value, rather than a Bool, into a new place:
     x or y, x a ?T: x's value (kept whole at o->b), else y: a T, or with y a ?T, a ?T
     x and e, e not a Bool: e if x is true or there, else nil: a ?U (e's own type, if it's a maybe)
   The right side's value goes in and jumps to the end; the left side's jump lands on the other case. */
static void rio_maybeOpEnd(rio_Vm *vm, const rio_PendingOp *o, rio_Expr *r) {
  rio_Expr *l = rio_valueTop(vm), dst, from, z;
  int type, done, kept = o->b, tempStart = l->tempStart;
  rio_isTestUsed(vm, r, -1);
  rio_isTestUsed(vm, l, -1); /* (the result isn't the left side's Bool any more) */
  if (o->kind == rio_Pending_or) {
    int elem = rio_type(vm, o->punAt)->elem;
    if (rio_valueType(r->type) == o->punAt) type = o->punAt;
    else {
      if (rio_valueType(r->type) != elem && !(r->kind == rio_ExprKind_const && r->type == rio_TypeId_int && elem == rio_TypeId_float))
        r->type = rio_TypeId_void; /* (no conversion but a number literal's: the message below) */
      else rio_coerce(vm, r, elem);
      if (rio_valueType(r->type) != elem) rio_FAIL(, "type mismatch: or on a maybe takes its value type, or the same maybe");
      type = elem;
    }
  } else {
    type = rio_valueType(r->type);
    if (type == rio_TypeId_void) rio_FAIL(, "expected a value");
    if (!rio_isMaybe(vm, type)) type = rio_maybeOf(vm, type);
  }
  if (rio_failed(vm)) return;
  dst = rio_exprOf(rio_ExprKind_static, type, rio_allocSlots(vm, rio_typeWords(vm, type)) * 4);
  rio_store(vm, &dst, r);
  done = rio_emit(vm, rio_Op_jmp, 0, 0, rio_NONE);
  rio_patchJumps(vm, o->a, rio_label(vm));
  if (o->kind == rio_Pending_or) { /* x was there: its value (or all of it) */
    from = rio_exprOf(rio_ExprKind_static, type, (kept + (type == o->punAt ? 0 : 1)) * 4);
    rio_store(vm, &dst, &from);
  } else { /* x was false or nil: nil */
    z = rio_exprOf(rio_ExprKind_static, rio_TypeId_int, dst.value);
    from = rio_exprOf(rio_ExprKind_const, rio_TypeId_int, 0);
    rio_store(vm, &z, &from);
  }
  rio_patchJumps(vm, done, rio_label(vm));
  dst.tempStart = (uint16_t)tempStart;
  *l = dst;
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
  c->symCount = o.c; /* (the right side's narrowing ends) */
  c->nameBytes = (uint32_t)o.argCount;
  rio_needValue(vm, &r);
  if (o.punAt || (o.kind == rio_Pending_and && r.type != rio_TypeId_bool)) {
    rio_maybeOpEnd(vm, &o, &r);
    return;
  }
  rio_isTestCarry(vm, &r, d * 4, o.kind == rio_Pending_and ? rio_Tok_and : rio_Tok_or);
  if (r.type != rio_TypeId_bool) rio_FAIL(, "expected Bool (or, after and, any value)");
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
    rio_slotMarks(vm, &e);
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
    rio_slotMarks(vm, &e);
    return e;
  case rio_SymKind_ref:
  case rio_SymKind_readOnlyRef: /* value: the slot holding the receiver's (or loop element's) address */
    e = rio_exprOf(rio_ExprKind_indirect, y->type, y->value);
    e.access = y->kind == rio_SymKind_readOnlyRef;
    if (c->curProc >= 0) { /* (its slot's marks: what its place may view, and whether it's the proc's own memory) */
      e.localView = (uint8_t)rio_bitGet(c->localViews, (uint32_t)y->value);
      e.fromParam = (uint8_t)rio_bitGet(c->paramViews, (uint32_t)y->value);
    }
    return e;
  case rio_SymKind_ffi:
    if (y->value < 0) rio_FAIL(rio_voidExpr(), "the host doesn't have this proc (it's declared for rio_hostHeader)");
    return rio_exprOf(rio_ExprKind_ffi, 0, y->value);
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
  for (i = 0; i < n; i++) {
    int used, ch;
    if (s[i] == '\\' && i + 1 < n && (ch = rio_escapeAt(s + i + 1, s + n, &used)) >= 0) {
      if (s[i + 1] == 'u') len += rio_utf8(out + len, ch); /* (never longer than the escape) */
      else out[len++] = (uint8_t)ch;
      i += used;
    } else out[len++] = (uint8_t)s[i];
  }
  R[k].u = 0x80000000u | c->poolUsed;
  R[k + 1].u = (uint32_t)len;
  rio_bitSet(c->constIsString, k);
  vm->constCount += 2;
  c->poolUsed += (uint32_t)len;
  e = rio_exprOf(rio_ExprKind_static, rio_TypeId_string, (int32_t)k * 4);
  e.access = rio_Access_readOnly;
  return e;
}

/* a string constant whose bytes the lexer put in the pool already (a string literal's) */
rio_Expr rio_stringInPool(rio_Vm *vm, uint32_t at, int n) {
  rio_Compiler *c = vm->compiler;
  rio_Val *R = (rio_Val *)vm->mem;
  uint32_t k = vm->constCount;
  rio_Expr e;
  if (k + 2 > vm->constCap) rio_FAIL(rio_voidExpr(), "too many constants");
  R[k].u = 0x80000000u | at;
  R[k + 1].u = (uint32_t)n;
  rio_bitSet(c->constIsString, k);
  vm->constCount += 2;
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
  if (e->kind == rio_ExprKind_type && (i = rio_lookupMethod(vm, e->type, t->text, t->len)) >= 0) { /* T.make(...): a static proc */
    uint16_t tempStart = e->tempStart;
    if (!c->procDefs[c->syms[i].value].isStatic) rio_FAIL_AT_TOKEN(, "a method is called on a value (x.name()); one on its type is a static proc");
    if (!c->procDefs[c->syms[i].value].defined) rio_FAIL_AT_TOKEN(, "recursion is not allowed");
    *e = rio_exprOf(rio_ExprKind_proc, 0, c->syms[i].value);
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
    rio_FAIL_AT_TOKEN(, "a union's value is reachable inside switch u / case T, or if u is T (and after u is T and)");
  if (e->type == rio_TypeId_range && t->len == 3 && (!memcmp(t->text, "len", 3) || !memcmp(t->text, "has", 3))) {
    if (c->peek.kind != '(') rio_FAIL(, "method calls need ()");
    method = rio_exprOf(rio_ExprKind_builtin, 0, t->text[0] == 'l' ? rio_Builtin_rangeLen : rio_Builtin_rangeHas); /* r.len(), r.has(i) */
    method.access = rio_Access_boundMethod;
    rio_pushValue(vm, method);
    return;
  }
  if ((i = rio_findField(vm, ty, t->text, t->len)) >= 0) {
    rio_Field *f = rio_field(vm, ty->firstField + i);
    rio_offsetPlace(e, f->offset);
    e->type = f->type;
    if (e->kind == rio_ExprKind_static) rio_partMarks(vm, e);
    if (f->bits) { /* (a bitfield: its bits of the word there) */
      e->bitAt = f->bitAt;
      e->bits = (uint16_t)(f->bits < 0 ? -f->bits : f->bits);
      e->bitSigned = f->bits < 0;
    }
    return;
  }
  if (ty->kind == rio_TypeKind_array && rio_swizzle(vm, e)) return;
  if ((ty->kind == rio_TypeKind_array || ty->kind == rio_TypeKind_slice || ty->kind == rio_TypeKind_builder ||
       ty->kind == rio_TypeKind_builderView) && t->len == 3 && (!memcmp(t->text, "len", 3) || !memcmp(t->text, "cap", 3))) {
    method = rio_exprOf(rio_ExprKind_builtin, 0, t->text[0] == 'l' ? rio_Builtin_len : rio_Builtin_cap); /* x.len(), x.cap() */
  } else if (ty->kind != rio_TypeKind_struct && ty->kind != rio_TypeKind_builder && ty->kind != rio_TypeKind_builderView &&
             (k = rio_builtinNamed(t->text, t->len, rio_Builtin_toInt, rio_Builtin_asString)) >= 0) {
    method = rio_exprOf(rio_ExprKind_builtin, 0, k); /* conversions */
  } else if ((ty->kind == rio_TypeKind_builder || ty->kind == rio_TypeKind_builderView) && ty->elem == rio_TypeId_byte &&
             (k = rio_builtinNamed(t->text, t->len, rio_Builtin_toInt, rio_Builtin_toFloat)) >= 0) {
    method = rio_exprOf(rio_ExprKind_builtin, 0, k); /* a byte builder's text as a number */
  } else if (ty->kind == rio_TypeKind_builder || ty->kind == rio_TypeKind_builderView) {
    if ((k = rio_builtinNamed(t->text, t->len, rio_Builtin_push, rio_Builtin_setInt)) < 0) rio_FAIL(, "no such method");
    method = rio_exprOf(rio_ExprKind_builtin, 0, k);
  } else if ((ty->kind == rio_TypeKind_slice || ty->kind == rio_TypeKind_array) && t->len == 4 && !memcmp(t->text, "swap", 4)) {
    method = rio_exprOf(rio_ExprKind_builtin, 0, rio_Builtin_swap); /* xs.swap(i, j) */
  } else if ((ty->kind == rio_TypeKind_slice || ty->kind == rio_TypeKind_array) && ty->elem == rio_TypeId_byte &&
             (k = rio_builtinNamed(t->text, t->len, rio_Builtin_pack, rio_Builtin_setInt)) >= 0) {
    method = rio_exprOf(rio_ExprKind_builtin, 0, k); /* a Blob's (or String's) b.pack(at, x) ... */
  } else {
    if ((i = rio_lookupMethod(vm, rio_valueType(e->type), t->text, t->len)) < 0)
      rio_FAIL(, ty->kind == rio_TypeKind_struct ? "no such field or method"
                 : ty->kind == rio_TypeKind_array ? "no such method (swizzles like .x .zy .rgba need 1-4 letters, on arrays of 1-4 numbers)"
                                                   : "no such method");
    if (!c->procDefs[c->syms[i].value].defined) rio_FAIL(, "recursion is not allowed");
    if (c->procDefs[c->syms[i].value].isStatic) rio_FAIL_AT_TOKEN(, "a static proc is called on its type (T.name()), not a value");
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
  if (!(ty->kind == rio_TypeKind_array && rio_isViewKind(rio_type(vm, ty->elem)->kind))) /* (an array of views: its
                                                                                       elements view what they were given) */
    o->localView = (uint8_t)rio_isLocalView(vm, o);
  if (ty->kind != rio_TypeKind_array && ty->kind != rio_TypeKind_slice && ty->kind != rio_TypeKind_builder && ty->kind != rio_TypeKind_builderView)
    rio_FAIL(, "cannot index this");
  if (ty->kind == rio_TypeKind_array && ty->firstField) { /* [Enum]T: the enum's value is the element */
    if (rio_valueType(i->type) != ty->firstField) rio_FAIL(, "index this array with its enum");
    i->type = rio_TypeId_int;
  }
  if (rio_valueType(i->type) != rio_TypeId_int) rio_FAIL(, "index must be Int");
  elem = ty->elem;
  size = (int)rio_type(vm, elem)->size;
  readOnly = o->access || o->type == rio_TypeId_string;
  if (ty->kind == rio_TypeKind_builder || ty->kind == rio_TypeKind_builderView) { /* checked against the current length */
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
    rio_partMarks(vm, o);
    return;
  }
  s = rio_toSlot2(vm, o);
  ix = rio_toSlot(vm, i, 1);
  c->freeSlot = o->tempStart;
  d = rio_allocSlots(vm, 1);
  if (rio_type(vm, elem)->kind == rio_TypeKind_param) { /* a []T: its elements' size is a slot's */
    while (d == ix || d == s || d == s + 1) d = rio_allocSlots(vm, 1); /* (the size goes in first: not over them) */
    rio_emit(vm, rio_Op_mov, d, rio_typeSizeSlot(vm, elem), 0);
    rio_emit(vm, rio_Op_idxv, d, s, ix);
  } else {
    rio_emit(vm, rio_Op_idx, d, s, ix);
    rio_emitData(vm, 0, (uint32_t)size);
  }
  o->kind = rio_ExprKind_indirect;
  o->value = d;
  o->offset = 0;
  o->type = (uint16_t)elem;
  o->access = (uint8_t)readOnly;
}

/* is the innermost bracket an index not yet a slice (where a range is its bounds)? */
static int rio_inIndex(rio_Vm *vm, int opBase) {
  rio_Compiler *c = vm->compiler;
  int k;
  for (k = c->opCount - 1; k >= opBase; k--)
    if (c->ops[k].kind >= rio_Pending_paren) return c->ops[k].kind == rio_Pending_index && !c->ops[k].a;
  return 0;
}

/* o[lo..<hi], o[lo..=hi] (inclusive): o becomes the slice (hi of kind sliceEnd: left out, the rest) */
static void rio_slice(rio_Vm *vm, rio_Expr *o, rio_Expr *lo, rio_Expr *hi, int inclusive) {
  rio_Compiler *c = vm->compiler;
  rio_Type *ty;
  int s, l, h, d, localView, fromParam;
  rio_needValue(vm, o);
  ty = rio_type(vm, o->type);
  localView = rio_isLocalView(vm, o);
  fromParam = o->fromParam;
  if (ty->kind == rio_TypeKind_builder || ty->kind == rio_TypeKind_builderView) { /* a slice of the live elements */
    int builder = rio_toSlot3(vm, o), view = rio_allocSlots(vm, 2), tempStart = o->tempStart;
    rio_emit(vm, rio_Op_lview, view, builder, 0);
    *o = rio_exprOf(rio_ExprKind_static, rio_sliceOf(vm, ty->elem), view * 4);
    o->tempStart = (uint16_t)tempStart;
    ty = rio_type(vm, o->type);
  }
  if (ty->kind != rio_TypeKind_array && ty->kind != rio_TypeKind_slice) rio_FAIL(, "cannot slice this");
  if (ty->kind == rio_TypeKind_array && rio_isTemp(vm, o)) rio_FAIL(, "a computed array has no home to view: store it in a variable first");
  if (ty->kind == rio_TypeKind_array && ty->firstField) { /* counts[.east..<.west]: bounds are the enum's values, which are the positions */
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
  if (inclusive && hi->kind != rio_ExprKind_sliceEnd) { /* (its end is one more than its last) */
    int k = rio_allocSlots(vm, 1);
    rio_emit(vm, rio_Op_add, k, h, rio_constSlot(vm, 1));
    h = k;
  }
  c->freeSlot = o->tempStart;
  d = rio_allocSlots(vm, 2);
  if (rio_type(vm, ty->elem)->kind == rio_TypeKind_param) { /* a []T: bounds and length by slice, the start by T's size */
    int at = rio_allocSlots(vm, 1);
    while (at == s || at == s + 1 || at == l || at == h) at = rio_allocSlots(vm, 1); /* (written before slice reads them) */
    rio_emit(vm, rio_Op_mul, at, l, rio_typeSizeSlot(vm, ty->elem));
    rio_emit(vm, rio_Op_slice, d, s, l);
    rio_emitData(vm, h, 0);
    rio_emit(vm, rio_Op_add, d, d, at);
  } else {
    rio_emit(vm, rio_Op_slice, d, s, l);
    rio_emitData(vm, h, rio_type(vm, ty->elem)->size);
  }
  o->kind = rio_ExprKind_static;
  o->value = d * 4;
  o->offset = 0;
  o->localView = (uint8_t)localView;
  o->fromParam = (uint8_t)fromParam;
  if (ty->kind == rio_TypeKind_array) o->type = (uint16_t)rio_sliceOf(vm, ty->elem);
}

/* ------------------------------------------------------------------ struct and array literals */

/* an array or builder literal's type (lit->op is 1 for them, 0 for a struct's), its element's size, and
   where its elements start (after a builder's length word) */
static rio_Type *rio_literalArray(rio_Vm *vm, const rio_PendingOp *lit, int *size, int *base) {
  rio_Type *at = rio_type(vm, lit->b);
  *size = (int)rio_type(vm, at->elem)->size;
  *base = at->kind == rio_TypeKind_builder ? 4 : 0;
  return at;
}

/* the type an array literal's next value goes in as, or -1 (not known until its first value) */
static int rio_literalElem(rio_Vm *vm, const rio_PendingOp *lit) { return lit->op >= 2 ? lit->b : rio_type(vm, lit->b)->elem; }

/* inside Type{...}: the value that follows is for field `name` */
static void rio_setField(rio_Vm *vm, rio_PendingOp *lit, const char *name, int n) {
  int i = rio_findField(vm, rio_type(vm, lit->b), name, n);
  if (i < 0) rio_FAIL(, "no such field");
  if (i < 64 && (lit->fieldsSet >> i & 1)) rio_FAIL(, "field set twice");
  if (i < 64) lit->fieldsSet |= (uint64_t)1 << i;
  lit->a = i;
}

/* at the start of an array literal's next value: `.name =` (an element of an [Enum]T), or else the next
   in order. lit->punAt: 1 once values come in order, 2 once they're named; lit->a: the named one */
static void rio_arrayElement(rio_Vm *vm, rio_PendingOp *lit) {
  rio_Compiler *c = vm->compiler;
  rio_Type *at = rio_type(vm, lit->b);
  int32_t v;
  int named = c->tok.kind == '.' && c->peek.kind == rio_Tok_id && rio_peekAfterNext(vm) == '=';
  if (!named) {
    if (lit->punAt == 2) rio_FAIL_AT_TOKEN(, "name each value (.name = value), or none");
    lit->punAt = 1;
    return;
  }
  if (lit->op >= 2) rio_FAIL_AT_TOKEN(, "only an array indexed by an enum takes .name = value");
  if (at->kind != rio_TypeKind_array || !at->firstField) rio_FAIL_AT_TOKEN(, "only an array indexed by an enum takes .name = value");
  if (lit->punAt == 1) rio_FAIL_AT_TOKEN(, "name each value (.name = value), or none");
  if (!lit->punAt) /* (named values come in any order: the rest start as zero) */
    rio_emit(vm, rio_Op_zero, rio_addrConst(vm, (uint32_t)lit->c, at->size), (int)at->size, 0);
  lit->punAt = 2;
  rio_next(vm);
  if (!rio_findEnumValue(vm, at->firstField, c->tok.text, c->tok.len, &v)) rio_FAIL_AT_TOKEN(, "not a value of this enum");
  if (v < 64 && (lit->fieldsSet >> v & 1)) rio_FAIL_AT_TOKEN(, "element set twice");
  if (v < 64) lit->fieldsSet |= (uint64_t)1 << v;
  lit->a = v; /* (an [Enum]T's enum is 0, 1, 2 and so on: its value is the element) */
  rio_next(vm);
  rio_next(vm);
}

/* at the start of a literal's next value: `field =`, or a value standing for its field ({x}, {a.x}) */
static void rio_literalField(rio_Vm *vm, rio_PendingOp *lit) {
  rio_Compiler *c = vm->compiler;
  if (c->tok.kind == '}') return;
  if (lit->op) {
    rio_arrayElement(vm, lit);
    return;
  }
  if (c->tok.kind != rio_Tok_id) rio_FAIL(, "struct literal fields must be named: Type{field = value}");
  if (c->peek.kind == '=') {
    rio_setField(vm, lit, c->tok.text, c->tok.len);
    rio_next(vm);
    rio_next(vm);
    return;
  }
  lit->a = -1; /* named after the path's last name, once the value is read */
  lit->punAt = c->pathBreaks; /* (none more by then: just names and dots) */
  c->lastNameLen = 0;
}

/* a value standing for its field must be a name or a dotted path; its last name is the field */
static void rio_punField(rio_Vm *vm, rio_PendingOp *lit) {
  rio_Compiler *c = vm->compiler;
  if (c->pathBreaks != lit->punAt || c->lastNameLen <= 0) rio_FAIL(, "only a name or a.b path can stand for a field; write field = value");
  rio_setField(vm, lit, c->lastName, c->lastNameLen);
}

/* has struct type t bitfields (which share words, so a literal zeroes all of it first) */
static int rio_hasBitfields(rio_Vm *vm, int t) {
  const rio_Type *ty = rio_type(vm, t);
  int i;
  for (i = 0; ty->kind == rio_TypeKind_struct && i < ty->fieldCount; i++)
    if (rio_field(vm, ty->firstField + i)->bits) return 1;
  return 0;
}

/* a literal is complete: its value, in its temporaries, or kept with the constants when they were all
   it moved there (rio_constLiteral) */
static void rio_literalDone(rio_Vm *vm, rio_PendingOp *lit) {
  rio_Compiler *c = vm->compiler;
  rio_Expr e = rio_exprOf(rio_ExprKind_static, lit->b, lit->c);
  c->freeSlot = (uint32_t)(lit->c / 4 + rio_typeWords(vm, lit->b));
  rio_constLiteral(vm, &e);
  if (e.access & rio_Access_literal) c->freeSlot = lit->freeAtStart;
  rio_pushResult(vm, e, lit->freeAtStart);
}

/* [_]T{...} done: an [n]T of its n values. []T{...}: that array kept somewhere of its own, as a hidden
   variable is (big locals' memory, or a top-level variable's starting data), and the slice of it as the
   result: a view of the proc's own memory, in a proc */
static void rio_finishGrown(rio_Vm *vm, rio_PendingOp *lit) {
  rio_Compiler *c = vm->compiler;
  rio_Expr e, r;
  uint32_t v[2], size;
  int type, elem = lit->b;
  if (elem < 0) { /* no values: an empty slice, if it has an element type */
    if (lit->op == 2) rio_FAIL(, "[_]{} has no values to size it: write [0]T{}");
    rio_FAIL(, "[]{} has no values to type it: write []T{}");
  }
  if (!lit->argCount && lit->op == 2) rio_FAIL(, "[_]T{} has no values to size it: write [0]T{}");
  type = lit->argCount ? rio_arrayOf(vm, lit->b, (uint32_t)lit->argCount) : -1;
  if (lit->op == 2) {
    lit->b = type;
    rio_literalDone(vm, lit);
    return;
  }
  v[0] = v[1] = 0;
  if (lit->argCount) {
    lit->b = type;
    lit->op = 1;
    rio_literalDone(vm, lit);
    e = rio_popValue(vm);
    size = rio_type(vm, type)->size;
    if (!((e.access & rio_Access_literal) && c->curProc < 0 && !c->blockCount && !vm->inRepl && rio_literalData(vm, &e))) {
      rio_Expr home = rio_exprOf(rio_ExprKind_static, type, (int32_t)rio_allocHigh(vm, size));
      rio_store(vm, &home, &e);
      e = home;
    }
    v[0] = rio_expose(vm, (uint32_t)e.value, size);
    v[1] = (uint32_t)lit->argCount;
  }
  c->freeSlot = lit->freeAtStart;
  r = rio_exprOf(rio_ExprKind_static, rio_sliceOf(vm, elem), rio_findConsts(vm, v, 2, lit->argCount ? 1 : 0) * 4);
  r.access = rio_Access_readOnly;
  r.localView = (uint8_t)(c->curProc >= 0 && lit->argCount);
  rio_pushResult(vm, r, lit->freeAtStart);
}

/* the zeroing for fields a literal left out (an array: the elements after its values; a builder: its
   length too), then the struct or array as the result */
static void rio_finishLiteral(rio_Vm *vm, rio_PendingOp *lit) {
  if (lit->op >= 2) {
    rio_finishGrown(vm, lit);
    return;
  }
  if (rio_mentions(vm, lit->b, -2))
    rio_FAIL(, "can't tell all of this struct's types from its fields: give the fields they're in, or name them, Table[K, V]{...}");
  rio_Type *st = rio_type(vm, lit->b);
  rio_Field *fields = rio_field(vm, st->firstField);
  int i = 0, j, w, from, to;
  if (lit->op) {
    int size, base;
    rio_literalArray(vm, lit, &size, &base);
    if (base) rio_emit(vm, rio_Op_mov, lit->c / 4, rio_constSlot(vm, (uint32_t)lit->argCount), 0); /* the builder's length */
    from = base + lit->argCount * size;
    to = (int)st->size;
    if (size % 4 || lit->punAt == 2) from = to; /* (bytes and named elements: zeroed up front) */
    if (to - from <= 64)
      for (w = from / 4; w < to / 4; w++) rio_emit(vm, rio_Op_mov, lit->c / 4 + w, rio_constSlot(vm, 0), 0);
    else rio_emit(vm, rio_Op_zero, rio_addrConst(vm, (uint32_t)(lit->c + from), (uint32_t)(to - from)), to - from, 0);
    rio_literalDone(vm, lit);
    return;
  }
  while (st->fieldCount <= 64 && !rio_hasBitfields(vm, lit->b) && i < st->fieldCount) { /* (structs with more fields, or bitfields, were zeroed up front) */
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
  rio_literalDone(vm, lit);
}

/* ------------------------------------------------------------------ calls */

/* one argument of a call or struct literal is complete (on top of the value stack) */
/* a struct argument to a host proc, by address: where it is and its size, in the call's next 2 slots.
   It stays where it is until the call: in a variable, or in temporaries above the arguments, which
   the call keeps (rio_ffiRoom) */
static void rio_ffiStructArg(rio_Vm *vm, rio_PendingOp *m, rio_Expr *e, int type) {
  rio_Compiler *c = vm->compiler;
  int d = m->freeAtStart + m->c, size = (int)rio_type(vm, type)->size;
  rio_needValue(vm, e);
  if (rio_failed(vm)) return;
  if (e->kind == rio_ExprKind_static && (uint32_t)e->value >= c->liveSlots * 4 && (uint32_t)e->value < vm->top &&
      e->value / 4 + rio_typeWords(vm, type) > m->punAt) m->punAt = e->value / 4 + rio_typeWords(vm, type);
  if (e->kind == rio_ExprKind_static) {
    uint32_t v[2];
    v[0] = rio_expose(vm, (uint32_t)e->value, (uint32_t)size);
    v[1] = (uint32_t)size;
    rio_emit(vm, rio_Op_mov2, d, rio_findConsts(vm, v, 2, 1), 0);
  } else {
    rio_emit(vm, rio_Op_lea, d, e->value, e->offset);
    rio_emit(vm, rio_Op_mov, d + 1, rio_constSlot(vm, (uint32_t)size), 0);
  }
  m->c += 2;
  c->freeSlot = (uint32_t)m->punAt;
}

/* a host proc call taking a struct: the arguments' slots are kept from the start, and what's computed
   for them goes above (punAt: the first slot past both) */
static void rio_ffiRoom(rio_Vm *vm, rio_PendingOp *o, int ffi) {
  rio_Compiler *c = vm->compiler;
  rio_FfiDef *f = &c->ffi[ffi];
  int i, structs = 0;
  if (f->resultLayout) o->c = 2; /* (where a struct or union result goes comes first: rio_finishCall) */
  for (i = 0; i < f->paramCount; i++) structs |= rio_type(vm, c->params[f->firstParam + i].type)->kind == rio_TypeKind_struct;
  if (!structs) return;
  o->punAt = o->freeAtStart + vm->ffi[ffi].argWords;
  if ((int)c->freeSlot < o->punAt) rio_allocSlots(vm, o->punAt - (int)c->freeSlot);
}

static void rio_inferFields(rio_Vm *vm, rio_PendingOp *lit, rio_Expr *value);
static int rio_unify(rio_Vm *vm, int p, int pattern, int actual, int *bound);

static void rio_argDone(rio_Vm *vm, rio_PendingOp *m) {
  rio_Compiler *c = vm->compiler;
  rio_Expr *arg = rio_valueTop(vm), e;
  rio_Param *param = 0;
  rio_isTestUsed(vm, arg, -1);
  if (m->kind == rio_Pending_call && m->a == rio_ExprKind_proc && m->argCount < c->procDefs[m->b].paramCount)
    param = &c->params[c->procDefs[m->b].firstParam + m->argCount];
  if (arg->kind == rio_ExprKind_proc || arg->kind == rio_ExprKind_ffi) { /* a proc passed where a proc value goes */
    int type = -1;
    if (param) type = rio_mentions(vm, param->type, -1) ? -1 : param->type; /* (a generic's: once T is known) */
    else if (m->kind == rio_Pending_call && m->a == rio_ExprKind_static && m->argCount < rio_type(vm, m->c)->fieldCount)
      type = rio_field(vm, rio_type(vm, m->c)->firstField + m->argCount)->type;
    else if (m->kind == rio_Pending_literal && m->op) type = rio_literalElem(vm, m);
    else if (m->kind == rio_Pending_literal && m->a >= 0) {
      type = rio_field(vm, rio_type(vm, m->b)->firstField + m->a)->type;
      if (rio_mentions(vm, type, -2)) { /* (by its own signature: its T found from it, rio_inferFields) */
        rio_toProcValue(vm, arg, -1);
        type = -1;
      }
    }
    else if (m->kind == rio_Pending_call && m->a == rio_ExprKind_builtin && m->b == rio_Builtin_push && m->argCount == 1)
      type = rio_type(vm, c->values[m->valueBase].type)->elem; /* (xs.push(f): a builder of proc values) */
    if (type >= 0 && rio_type(vm, type)->kind == rio_TypeKind_proc) rio_toProcValue(vm, arg, type);
    else if (param && type < 0 && rio_type(vm, param->type)->kind == rio_TypeKind_proc) rio_toProcValue(vm, arg, -1); /* (its own type, matched as T is found) */
  }
  if (param && param->refType) { /* &place: checked here, its address taken when the call is made */
    if (!(arg->access & rio_Access_ref)) rio_FAIL(, "this parameter is a reference: pass &x");
    arg->access = 0;
    if (arg->type != param->refType && !rio_mentions(vm, param->refType, -1)) rio_FAIL(, "type mismatch");
    m->argCount++;
    return;
  }
  if (m->kind == rio_Pending_call && m->a == rio_ExprKind_static && m->argCount < rio_type(vm, m->c)->fieldCount &&
      rio_field(vm, rio_type(vm, m->c)->firstField + m->argCount)->offset) { /* &place, through a proc value: as to a proc */
    if (!(arg->access & rio_Access_ref)) rio_FAIL(, "this parameter is a reference: pass &x");
    arg->access = 0;
    if (arg->type != rio_field(vm, rio_type(vm, m->c)->firstField + m->argCount)->type) rio_FAIL(, "type mismatch");
    m->argCount++;
    return;
  }
  if (m->kind == rio_Pending_call && m->a == rio_ExprKind_ffi && m->argCount < c->ffi[m->b].paramCount &&
      c->params[c->ffi[m->b].firstParam + m->argCount].refType) { /* &place, to a host proc: its address and size */
    rio_Param *p = &c->params[c->ffi[m->b].firstParam + m->argCount];
    int s, d;
    if (!(arg->access & rio_Access_ref)) rio_FAIL(, "this parameter is a reference: pass &x");
    if (p->refType == rio_REF_LAYOUT) rio_FAIL(, "this host proc takes a struct: declare it, in its module's source, to say which");
    arg->access = 0;
    if (arg->type != p->refType) rio_FAIL(, "type mismatch");
    e = rio_popValue(vm);
    s = rio_toAddrSlot(vm, &e);
    c->freeSlot = (uint32_t)(m->freeAtStart + m->c);
    d = rio_allocSlots(vm, 2);
    if (s != d) rio_emit(vm, rio_Op_mov, d, s, 0);
    rio_emit(vm, rio_Op_mov, d + 1, rio_constSlot(vm, rio_type(vm, p->refType)->size), 0);
    if (c->freeSlot < (uint32_t)m->punAt) c->freeSlot = (uint32_t)m->punAt;
    m->c += 2;
    m->argCount++;
    return;
  }
  if (m->kind == rio_Pending_call && m->a == rio_ExprKind_builtin && m->b == rio_Builtin_unpack && m->argCount == 2 && (arg->access & rio_Access_ref)) {
    m->argCount++; /* b.unpack(at, &x): x's address taken when the call is made */
    return;
  }
  if (m->kind == rio_Pending_call && m->a == rio_ExprKind_builtin && m->b >= rio_Builtin_sizeOf && m->b <= rio_Builtin_bitOffsetOf &&
      m->argCount == 0 && arg->kind == rio_ExprKind_type) {
    m->argCount++; /* sizeOf(T), offsetOf(S, .f): a type */
    return;
  }
  rio_needValue(vm, arg);
  if (m->kind == rio_Pending_literal && m->op >= 2) { /* [_]{...}: right after the values before it */
    int words, k, at;
    rio_Expr dst;
    e = rio_popValue(vm);
    if (m->b < 0) { /* the first: its type is the elements' */
      m->b = rio_valueType(e.type);
      k = rio_type(vm, m->b)->kind;
      if (e.kind == rio_ExprKind_const && rio_type(vm, e.type)->kind == rio_TypeKind_union)
        rio_FAIL(, "nil has no type of its own to give the elements: write [_]T{...}");
      if (k == rio_TypeKind_void || k == rio_TypeKind_builder || k == rio_TypeKind_builderView || rio_type(vm, m->b)->size % 4)
        rio_FAIL(, "this can't be an element: write [_]T{...}");
    } else if (m->a && rio_valueType(e.type) != m->b && !(e.kind == rio_ExprKind_const && e.type == rio_TypeId_int && m->b == rio_TypeId_float))
      rio_FAIL(, "the elements take the first value's type, and this isn't it: write [_]T{...} to give their type");
    words = rio_typeWords(vm, m->b);
    at = m->c / 4 + m->argCount * words;
    if (at + words > 0xFFFF) rio_FAIL(, "more values than a literal holds");
    dst = rio_exprOf(rio_ExprKind_static, m->b, at * 4);
    rio_store(vm, &dst, &e);
    c->freeSlot = (uint32_t)(at + words);
    if (c->freeSlot > c->slotHigh) { /* (as rio_allocSlots, but keeping what the store marked) */
      c->slotHigh = c->freeSlot;
      if (c->slotHigh > c->limits.slots || c->slotHigh * 4 > vm->top) rio_FAIL(, "out of slots");
    }
    m->argCount++;
    return;
  }
  if (m->kind == rio_Pending_literal && m->op) { /* stored into the array at once, in order */
    int size, base;
    rio_Type *at = rio_literalArray(vm, m, &size, &base);
    int i = m->punAt == 2 ? m->a : m->argCount;
    rio_Expr dst;
    if ((uint32_t)i >= at->count) rio_FAIL(, at->kind == rio_TypeKind_builder ? "more values than the builder holds" : "more values than the array holds");
    e = rio_popValue(vm);
    dst = rio_exprOf(rio_ExprKind_static, at->elem, m->c + base + i * size);
    rio_store(vm, &dst, &e);
    c->freeSlot = (uint32_t)(m->c / 4 + rio_typeWords(vm, m->b));
  } else if (m->kind == rio_Pending_literal) { /* stored into the struct at once */
    rio_Type *st;
    rio_Field *f;
    rio_Expr dst;
    if (m->a < 0) rio_punField(vm, m);
    if (rio_failed(vm)) return;
    rio_inferFields(vm, m, rio_valueTop(vm));
    if (rio_failed(vm)) return;
    st = rio_type(vm, m->b);
    f = rio_field(vm, st->firstField + m->a);
    e = rio_popValue(vm);
    dst = rio_exprOf(rio_ExprKind_static, f->type, m->c + f->offset);
    if (f->bits) {
      dst.bitAt = f->bitAt;
      dst.bits = (uint16_t)(f->bits < 0 ? -f->bits : f->bits);
      dst.bitSigned = f->bits < 0;
    }
    rio_store(vm, &dst, &e);
    c->freeSlot = (uint32_t)(m->c / 4 + rio_typeWords(vm, m->b));
  } else if (m->a == rio_ExprKind_proc || m->a == rio_ExprKind_static) { /* kept on the stack until the call */
    int k = rio_type(vm, arg->type)->kind;
    int max = m->a == rio_ExprKind_proc ? c->procDefs[m->b].paramCount - c->procDefs[m->b].typeParams : rio_type(vm, m->c)->fieldCount;
    if (m->argCount >= max) rio_FAIL(, "too many arguments");
    if (arg->kind == rio_ExprKind_indirect && !rio_isAggregate(k) && k != rio_TypeKind_param) { /* read it now, before anything changes it */
      int s = k == rio_TypeKind_slice || k == rio_TypeKind_builderView ? rio_toViewSlots(vm, arg) : rio_toSlot(vm, arg, 1);
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
    if (type == rio_TypeId_void)
      rio_FAIL(, vm->ffi[m->b].sig[c->params[f->firstParam + m->argCount].addr] == 'E' ? "this host proc takes an enum: declare it, in its module's source, to say which"
                                                                                   : "this host proc takes a struct: declare it, in its module's source, to say which");
    rio_coerce(vm, &e, type);
    if (rio_type(vm, type)->kind == rio_TypeKind_struct) {
      if (e.type != type) rio_FAIL(, "type mismatch");
      rio_ffiStructArg(vm, m, &e, type);
      m->argCount++;
      return;
    }
    words = rio_typeWords(vm, type);
    s = words == 2 ? rio_toSlot2(vm, &e) : rio_toSlot(vm, &e, 1);
    c->freeSlot = (uint32_t)(m->freeAtStart + m->c);
    d = rio_allocSlots(vm, words);
    if (s != d && !(words == 1 && rio_retarget(vm, s, d))) rio_emit(vm, words == 2 ? rio_Op_mov2 : rio_Op_mov, d, s, 0);
    m->c += words;
    if (c->freeSlot < (uint32_t)m->punAt) c->freeSlot = (uint32_t)m->punAt; /* (structs' temporaries kept: rio_ffiRoom) */
  } else if (m->a == rio_ExprKind_builtin && (m->b == rio_Builtin_format || m->b == rio_Builtin_print)) rio_formatArg(vm, m);
  else if (m->a == rio_ExprKind_builtin && m->b == rio_Builtin_log) rio_logArg(vm, m);
  m->argCount++;
}

/* Table{keys = ks[], ...}: a generic struct's literal with its Ts left out. Each field's value finds the
   ones its type has, and the literal's type is the struct filled in with what's known so far. */
static void rio_inferFields(rio_Vm *vm, rio_PendingOp *lit, rio_Expr *value) {
  int ft = rio_field(vm, rio_type(vm, lit->b)->firstField + lit->a)->type, tmpl, g, bound[8], j;
  if (!rio_mentions(vm, ft, -2)) return;
  tmpl = rio_type(vm, lit->b)->elem ? rio_type(vm, lit->b)->elem : lit->b;
  g = rio_genericOf(vm, tmpl);
  for (j = 0; j < (int)rio_type(vm, g)->count; j++) bound[j] = -1;
  if (value->kind == rio_ExprKind_proc || value->kind == rio_ExprKind_ffi) rio_toProcValue(vm, value, -1);
  if (rio_failed(vm)) return;
  if (!rio_unify(vm, rio_NO_PROC, ft, rio_valueType(value->type), bound)) rio_FAIL(, "type mismatch");
  for (j = 0; j < (int)rio_type(vm, g)->count; j++)
    if (bound[j] < 0) bound[j] = (int)rio_type(vm, g)->size + j; /* (not known yet: stays a T) */
  lit->b = rio_bindType(vm, rio_NO_PROC, lit->b, bound);
}

/* zeros into slots [at, at + words): optional parameters a call left out */
static void rio_zeroSlots(rio_Vm *vm, int at, int words) {
  int z = rio_constSlot(vm, 0), w;
  for (w = 0; w < words; w++) rio_emit(vm, rio_Op_mov, at + w, z, 0);
}

/* the arguments a call has: all of its parameters', or all but some of the optional ones at the end */
static int rio_argsOk(const rio_PendingOp *m, int params, int optional) { return m->argCount <= params && m->argCount >= params - optional; }

/* ------------------------------------------------------------------ generic calls */

/* Matches a generic parameter's type (pattern, of proc p's) with an argument's (actual): each T it
   mentions is found, or must agree with what it was already found to be. 0 if they don't fit */
static int rio_unify(rio_Vm *vm, int p, int pattern, int actual, int *bound) {
  rio_Type *y = rio_type(vm, pattern), *a = rio_type(vm, actual);
  uint32_t i;
  if (!rio_mentions(vm, pattern, -1)) return 1; /* (checked as any argument is, as it's stored) */
  if (y->kind == rio_TypeKind_param && y->elem == p) {
    if (bound[y->count] < 0) bound[y->count] = actual;
    return bound[y->count] == actual;
  }
  if (y->kind == rio_TypeKind_slice) return (a->kind == rio_TypeKind_slice || a->kind == rio_TypeKind_array) && rio_unify(vm, p, y->elem, a->elem, bound);
  if (y->kind == rio_TypeKind_struct) { /* a generic struct filled in: the same one, field by field */
    if (a->kind != rio_TypeKind_struct || !y->elem || a->elem != y->elem) return 0;
    for (i = 0; i < y->fieldCount; i++)
      if (!rio_unify(vm, p, rio_field(vm, y->firstField + i)->type, rio_field(vm, a->firstField + i)->type, bound)) return 0;
    return 1;
  }
  if (y->kind != rio_TypeKind_proc || a->kind != rio_TypeKind_proc || a->fieldCount != y->fieldCount || a->count != y->count) return 0;
  for (i = 0; i < y->fieldCount; i++)
    if (rio_field(vm, y->firstField + i)->offset != rio_field(vm, a->firstField + i)->offset ||
        !rio_unify(vm, p, rio_field(vm, y->firstField + i)->type, rio_field(vm, a->firstField + i)->type, bound)) return 0;
  return rio_unify(vm, p, y->elem, a->elem, bound);
}

/* A generic struct (from: its template, or one filled in with a generic proc's Ts), filled in: made once
   for each set of field types, elem its template. Every field is the same size whatever T is, so it's
   laid out as the template is. */
static int rio_structOf(rio_Vm *vm, int p, int from, const int *bound) {
  rio_Compiler *c = vm->compiler;
  int types[64], tmpl = rio_type(vm, from)->elem ? rio_type(vm, from)->elem : from, n = rio_type(vm, from)->fieldCount, i, j, made;
  if (n > 64) rio_FAIL(0, "too many fields in a generic struct");
  for (j = 0; j < n; j++) types[j] = rio_bindType(vm, p, rio_field(vm, rio_type(vm, from)->firstField + j)->type, bound);
  for (i = rio_TypeId_builtinCount; i < c->typeCount; i++) {
    rio_Type *y = rio_type(vm, i);
    if (y->kind != rio_TypeKind_struct || y->elem != tmpl) continue;
    for (j = 0; j < n && rio_field(vm, y->firstField + j)->type == types[j]; j++) {}
    if (j == n) return i;
  }
  made = rio_newType(vm, rio_TypeKind_struct, tmpl, 0, rio_type(vm, tmpl)->size);
  rio_type(vm, made)->firstField = (uint16_t)c->fieldCount;
  for (j = 0; j < n; j++) {
    int f = rio_newField(vm);
    if (f < 0) return 0;
    *rio_field(vm, f) = *rio_field(vm, rio_type(vm, tmpl)->firstField + j);
    rio_field(vm, f)->type = (uint16_t)types[j];
  }
  rio_type(vm, made)->fieldCount = (uint16_t)n;
  rio_type(vm, made)->hasRefs = rio_type(vm, tmpl)->hasRefs;
  return made;
}

/* a generic parameter's type with what each T of p's (a proc, or rio_NO_PROC: a generic type's) was
   found to be */
int rio_bindType(rio_Vm *vm, int p, int t, const int *bound) {
  rio_Type *y = rio_type(vm, t);
  int params[16], refs = 0;
  uint32_t i;
  if (!rio_mentions(vm, t, -1)) return t;
  if (y->kind == rio_TypeKind_param) return y->elem == p ? bound[y->count] : t; /* (another's T stays) */
  if (y->kind == rio_TypeKind_slice) return rio_sliceOf(vm, rio_bindType(vm, p, y->elem, bound));
  if (y->kind == rio_TypeKind_struct) return rio_structOf(vm, p, t, bound);
  for (i = 0; i < y->fieldCount && i < 16; i++) {
    params[i] = rio_bindType(vm, p, rio_field(vm, y->firstField + i)->type, bound);
    refs |= rio_field(vm, y->firstField + i)->offset << i;
  }
  return rio_procTypeOf(vm, params, (int)y->fieldCount, rio_bindType(vm, p, y->elem, bound), (int)y->count, refs);
}

/* A call of generic proc p (the call m): what each T is, from the arguments, and checked against what
   the body does with it */
static void rio_bindCall(rio_Vm *vm, rio_PendingOp *m, int *bound) {
  rio_Compiler *c = vm->compiler;
  rio_ProcDef *p = &c->procDefs[m->b];
  int i, j, visible = p->paramCount - p->typeParams;
  for (j = 0; j < p->typeParams; j++) bound[j] = -1;
  for (i = 0; i < visible && i < m->argCount; i++) {
    rio_Expr *arg = &c->values[m->valueBase + i];
    if (arg->kind == rio_ExprKind_proc || arg->kind == rio_ExprKind_ffi) rio_toProcValue(vm, arg, -1);
    if (rio_failed(vm)) return;
    if (!rio_unify(vm, m->b, rio_paramPattern(vm, m->b, i), rio_valueType(arg->type), bound)) rio_FAIL(, "type mismatch");
  }
  for (j = 0; j < p->typeParams; j++) {
    rio_Type *t;
    if (bound[j] < 0) rio_FAIL(, "can't tell what T is: an argument it's in was left out");
    t = rio_type(vm, bound[j]);
    if (t->kind != rio_TypeKind_param && rio_mentions(vm, bound[j], -1)) rio_FAIL(, "a T made of another generic proc's T isn't ready yet");
    if ((p->viewTypes >> j & 1) && t->hasRefs)
      rio_FAIL(, "this proc copies its T into memory it was given, so a T holding views (a String) can't go there: it could keep a view past what it views");
    if (t->kind == rio_TypeKind_param) /* (this proc's own T, given on: what it does with it, its own does) */
      c->procDefs[t->elem].viewTypes |= (uint8_t)((p->viewTypes >> j & 1) << t->count);
  }
}

/* the hidden argument for a T that's type t: the address of the constant (size, descriptor) for it, or
   given on from this generic proc's own hidden parameter */
static int rio_typeArg(rio_Vm *vm, int t) {
  rio_Compiler *c = vm->compiler;
  rio_Type *y = rio_type(vm, t);
  uint32_t v[2];
  if (y->kind == rio_TypeKind_param) {
    rio_ProcDef *outer = &c->procDefs[y->elem];
    return (int)(c->params[outer->firstParam + outer->paramCount - outer->typeParams + y->count].addr / 4);
  }
  v[0] = y->size;
  v[1] = 0; /* (its descriptor, once a generic body prints or hashes a T) */
  return rio_constSlot(vm, (uint32_t)rio_findConsts(vm, v, 2, 0) * 4);
}

/* a call's arguments are all there: make it, and push its result */
static void rio_finishCall(rio_Vm *vm, rio_PendingOp *m) {
  rio_Compiler *c = vm->compiler;
  int i, d;
  if (m->a == rio_ExprKind_proc) { /* arguments into the proc's parameters, then call */
    rio_ProcDef *p = &c->procDefs[m->b];
    int resultLocal = 0, resultFrom = 0, visible = p->paramCount - p->typeParams, bound[8], resultType = p->resultType;
    if (!rio_argsOk(m, visible, p->optional)) rio_FAIL(, "wrong number of arguments");
    if (p->typeParams) {
      rio_bindCall(vm, m, bound);
      if (rio_failed(vm)) return;
      resultType = rio_bindType(vm, m->b, resultType, bound);
    }
    for (i = 0; i < visible; i++) {
      rio_Param *param = &c->params[p->firstParam + i];
      int type = p->typeParams ? rio_bindType(vm, m->b, param->type, bound) : param->type;
      rio_Expr dst = rio_exprOf(rio_ExprKind_static, type, (int32_t)param->addr), *arg = &c->values[m->valueBase + i];
      if (i >= m->argCount) { /* (left out: zero) */
        rio_zeroSlots(vm, (int)(param->addr / 4), rio_typeWords(vm, type));
        continue;
      }
      if (param->refType && p->typeParams && arg->type != rio_bindType(vm, m->b, param->refType, bound)) rio_FAIL(, "type mismatch");
      if ((!i && p->selfByRef) || param->refType) { /* the place's address (a view it returns may view that place) */
        if (arg->access & rio_Access_readOnly) { /* (a constant: the method has a copy, since it may change self) */
          rio_Expr copy = rio_exprOf(rio_ExprKind_static, rio_valueType(arg->type), rio_allocSlots(vm, rio_typeWords(vm, arg->type)) * 4);
          rio_store(vm, &copy, arg);
          *arg = copy;
        }
        rio_placeMarks(vm, arg, &resultLocal, &resultFrom);
        rio_emit(vm, rio_Op_mov, (int)(dst.value / 4), rio_toAddrSlot(vm, arg), 0);
        continue;
      }
      rio_slotMarks(vm, arg);
      resultLocal |= rio_isLocalView(vm, arg); /* (a view it returns may view what went in) */
      resultFrom |= arg->fromParam;
      c->storingArgs = 1;
      rio_store(vm, &dst, &c->values[m->valueBase + i]);
      c->storingArgs = 0;
    }
    for (i = 0; i < p->typeParams; i++) /* each T: where its size is */
      rio_emit(vm, rio_Op_mov, (int)(c->params[p->firstParam + visible + i].addr / 4), rio_typeArg(vm, bound[i]), 0);
    c->valueCount = m->valueBase;
    rio_emit(vm, rio_Op_call, 0, 0, p->entry);
    if (p->callsValues && c->curProc >= 0) c->procDefs[c->curProc].callsValues = 1;
    c->freeSlot = m->freeAtStart;
    if (p->resultType == rio_TypeId_void) {
      rio_pushResult(vm, rio_voidExpr(), m->freeAtStart);
      return;
    }
    { /* the result is copied out: the next call of the proc would overwrite it */
      rio_Expr src = rio_exprOf(rio_ExprKind_static, resultType, (int32_t)p->resultAt), dst;
      d = rio_allocSlots(vm, rio_typeWords(vm, resultType));
      dst = rio_exprOf(rio_ExprKind_static, resultType, d * 4);
      rio_store(vm, &dst, &src);
      rio_resultMarks(vm, &dst, resultLocal, resultFrom);
      rio_pushResult(vm, dst, m->freeAtStart);
    }
  } else if (m->a == rio_ExprKind_static) { /* through a proc value: the arguments go to a scratch area, the call copies them over */
    rio_Type *type = rio_type(vm, m->c);
    int words = 0, base, at = 0;
    if (!rio_argsOk(m, type->fieldCount, (int)type->count)) rio_FAIL(, "wrong number of arguments");
    for (i = 0; i < type->fieldCount; i++) {
      rio_Field *f = rio_field(vm, type->firstField + i);
      words += f->offset ? 1 : rio_typeWords(vm, f->type);
    }
    base = rio_allocSlots(vm, words ? words : 1);
    int resultLocal = 0, resultFrom = 0;
    for (i = 0; i < type->fieldCount; i++) {
      int paramType = rio_field(vm, type->firstField + i)->type;
      rio_Expr dst = rio_exprOf(rio_ExprKind_static, paramType, (base + at) * 4), *arg = &c->values[m->valueBase + i];
      if (rio_field(vm, type->firstField + i)->offset) { /* &place: its address, as to a proc */
        if (arg->access & rio_Access_readOnly) {
          rio_Expr copy = rio_exprOf(rio_ExprKind_static, rio_valueType(arg->type), rio_allocSlots(vm, rio_typeWords(vm, arg->type)) * 4);
          rio_store(vm, &copy, arg);
          *arg = copy;
        }
        rio_placeMarks(vm, arg, &resultLocal, &resultFrom);
        rio_emit(vm, rio_Op_mov, base + at, rio_toAddrSlot(vm, arg), 0);
        at++;
        continue;
      }
      if (i >= m->argCount) { /* (left out: zero) */
        rio_zeroSlots(vm, base + at, rio_typeWords(vm, paramType));
        at += rio_typeWords(vm, paramType);
        continue;
      }
      rio_slotMarks(vm, &c->values[m->valueBase + i]);
      resultLocal |= rio_isLocalView(vm, &c->values[m->valueBase + i]); /* (what it returns may view what went in) */
      resultFrom |= c->values[m->valueBase + i].fromParam;
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
    {
      rio_Expr r = rio_exprOf(rio_ExprKind_static, type->elem, d * 4);
      rio_resultMarks(vm, &r, resultLocal, resultFrom);
      rio_pushResult(vm, r, m->freeAtStart);
    }
  } else if (m->a == rio_ExprKind_ffi) { /* the arguments are in place; the result comes back in the first slot */
    rio_FfiDef *f = &c->ffi[m->b];
    if (!rio_argsOk(m, f->paramCount, f->optional)) rio_FAIL(, "wrong number of arguments");
    if (m->c < vm->ffi[m->b].argWords) { /* (the ones left out: zero) */
      c->freeSlot = (uint32_t)(m->freeAtStart + m->c);
      rio_zeroSlots(vm, rio_allocSlots(vm, vm->ffi[m->b].argWords - m->c), vm->ffi[m->b].argWords - m->c);
    }
    if (f->resultLayout) { /* a struct or union, zeroed above the arguments, and where it is first: the host writes it */
      int words = rio_typeWords(vm, f->resultType), size = (int)rio_type(vm, f->resultType)->size, dest;
      uint32_t v[2];
      c->freeSlot = (uint32_t)(m->freeAtStart + vm->ffi[m->b].argWords);
      if (c->freeSlot < (uint32_t)m->punAt) c->freeSlot = (uint32_t)m->punAt;
      dest = rio_allocSlots(vm, words);
      rio_zeroSlots(vm, dest, words);
      v[0] = rio_expose(vm, (uint32_t)dest * 4, (uint32_t)size);
      v[1] = (uint32_t)size;
      rio_emit(vm, rio_Op_mov2, m->freeAtStart, rio_findConsts(vm, v, 2, 1), 0);
      rio_emit(vm, rio_Op_ffi, m->freeAtStart, 0, m->b);
      rio_pushResult(vm, rio_exprOf(rio_ExprKind_static, f->resultType, dest * 4), m->freeAtStart);
      return;
    }
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
      rio_slice(vm, rio_valueTop(vm), &i, &hi, o.a == 2);
    } else {
      i = rio_popValue(vm);
      rio_needValue(vm, &i);
      if (i.type == rio_TypeId_range) { /* a[r]: its first to its last */
        hi = i;
        rio_offsetPlace(&hi, 4);
        hi.type = rio_TypeId_int;
        i.type = rio_TypeId_int;
        rio_slice(vm, rio_valueTop(vm), &i, &hi, 1);
      } else rio_index(vm, rio_valueTop(vm), &i);
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
  int bareBrace;         /* a { with its type from where it's going: its literal opens next (a line's first, too) */
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
  if (m->kind == rio_Pending_literal) return m->op ? rio_literalElem(vm, m) : rio_field(vm, rio_type(vm, m->b)->firstField + m->a)->type;
  if (m->kind == rio_Pending_index && m->valueBase > 0 && c->valueCount <= m->valueBase + 1) { /* counts[.east], counts[.a..<.b]: an [Enum]T takes its enum */
    rio_Type *array = rio_type(vm, c->values[m->valueBase - 1].type);
    if (array->kind == rio_TypeKind_array && array->firstField) return array->firstField;
  }
  if (m->kind == rio_Pending_call && m->a == rio_ExprKind_proc && m->argCount < c->procDefs[m->b].paramCount)
    return c->params[c->procDefs[m->b].firstParam + m->argCount].type;
  if (m->kind == rio_Pending_call && m->a == rio_ExprKind_ffi && m->argCount < c->ffi[m->b].paramCount) /* pc.text("hi", {...}), pc.pointer(.ui) */
    return c->params[c->ffi[m->b].firstParam + m->argCount].type;
  if (m->kind == rio_Pending_call && m->a == rio_ExprKind_static && m->argCount < rio_type(vm, m->c)->fieldCount)
    return rio_field(vm, rio_type(vm, m->c)->firstField + m->argCount)->type;
  if (m->kind == rio_Pending_call && m->a == rio_ExprKind_builtin && m->b == rio_Builtin_push && m->argCount == 1)
    return rio_type(vm, c->values[m->valueBase].type)->elem;
  if (m->kind == rio_Pending_call && m->a == rio_ExprKind_builtin && m->b == rio_Builtin_format && m->argCount == 1)
    return rio_TypeId_format; /* b.format({width = 4}, ...) */
  if (m->kind == rio_Pending_binary && (m->op == '+' || m->op == '-' || m->op == '*' || m->op == '/' || m->op == '%') &&
      c->valueCount > x->valueBase && rio_type(vm, c->values[c->valueCount - 1].type)->kind == rio_TypeKind_array)
    return c->values[c->valueCount - 1].type; /* p + {1, 0}: an array's partner, of its type */
  if (m->kind == rio_Pending_or && m->punAt) return rio_type(vm, m->punAt)->elem; /* x or {1, 0}: of x's value's type */
  return -1;
}

/* offsetOf(S, .a.b)'s second argument: the path's bit offset in S's type (or its value's), as an Int
   constant, and on the call (its c: the offset + 1, so the call knows it was a path) */
static int rio_fieldPath(rio_Vm *vm, int tempStart) {
  rio_Compiler *c = vm->compiler;
  rio_PendingOp *m = rio_opTop(vm);
  int type = c->values[m->valueBase].type;
  int32_t bit = 0;
  while (c->tok.kind == '.') {
    rio_Type *ty = rio_type(vm, type);
    rio_Field *f;
    int i;
    rio_next(vm);
    if (c->tok.kind != rio_Tok_id) rio_FAIL_AT_TOKEN(rio_Expect_failed, "expected a field's name");
    if ((i = rio_findField(vm, ty, c->tok.text, c->tok.len)) < 0)
      rio_FAIL_AT_TOKEN(rio_Expect_failed, ty->kind == rio_TypeKind_struct ? "no such field" : "only a struct has fields");
    f = rio_field(vm, ty->firstField + i);
    bit += (int32_t)f->offset * 8 + (f->bits ? (int32_t)f->bitAt : 0);
    type = f->type;
    rio_next(vm);
  }
  m->c = bit + 1;
  rio_pushResult(vm, rio_exprOf(rio_ExprKind_const, rio_TypeId_int, bit), tempStart);
  return rio_Expect_operator;
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

/* a string literal and any right after it, as one string: on its line, or on the lines after, inside
   brackets ("a" "b", or ("a" then "b" on the next line) */
static rio_Expr rio_stringLiterals(rio_Vm *vm, const rio_ExprState *x) {
  rio_Compiler *c = vm->compiler;
  uint32_t at = c->tok.val.u, n = (uint32_t)c->tok.len;
  while (c->peek.kind == rio_Tok_string && (!c->peek.newline || x->depth)) {
    uint32_t m = (uint32_t)c->peek.len;
    if (c->peek.val.u != at + n) { /* (not right after it in the pool: both again, at its end) */
      if (n + m > c->poolCap - c->poolUsed) rio_FAIL(rio_voidExpr(), "out of compiler memory");
      memcpy(c->pool + c->poolUsed, c->pool + at, n);
      memmove(c->pool + c->poolUsed + n, c->pool + c->peek.val.u, m);
      at = c->poolUsed;
      c->poolUsed += n + m;
    }
    n += m;
    rio_next(vm);
  }
  return rio_stringInPool(vm, at, (int)n);
}

/* end, inside index brackets: the last index of what they index (the innermost brackets must be theirs:
   xs[end], xs[end - 2..=end], ys[xs[end]]). An array's is a constant; a slice's, a String's, a builder's
   or a builder view's, its length less 1. Not an [E]T's: its values are named, and a new one would move
   what end means */
static rio_Expr rio_endIndex(rio_Vm *vm, const rio_ExprState *x) {
  rio_Compiler *c = vm->compiler;
  rio_PendingOp *m = 0;
  rio_Expr t, len;
  rio_Type *ty;
  int i, d;
  for (i = c->opCount - 1; i >= x->opBase && !m; i--)
    if (c->ops[i].kind >= rio_Pending_paren) m = &c->ops[i];
  if (!m || m->kind != rio_Pending_index || !m->valueBase)
    rio_FAIL_AT_TOKEN(rio_voidExpr(), "end means the last index only right inside an index's brackets: xs[end]");
  t = c->values[m->valueBase - 1];
  rio_needValue(vm, &t);
  if (rio_failed(vm)) return rio_voidExpr();
  ty = rio_type(vm, t.type);
  if (ty->kind == rio_TypeKind_array) {
    if (ty->firstField) rio_FAIL_AT_TOKEN(rio_voidExpr(), "end is for an array indexed by Int: name the enum's value");
    if (!ty->count) rio_FAIL_AT_TOKEN(rio_voidExpr(), "an empty array has no last index");
    return rio_exprOf(rio_ExprKind_const, rio_TypeId_int, (int32_t)ty->count - 1);
  }
  len = t; /* (its length word: a slice's second, a builder's first, through a builder view's first) */
  len.bits = 0;
  if (ty->kind == rio_TypeKind_slice) rio_offsetPlace(&len, 4);
  else if (ty->kind == rio_TypeKind_builderView) {
    len.type = rio_TypeId_int;
    len = rio_exprOf(rio_ExprKind_indirect, rio_TypeId_int, rio_toSlot(vm, &len, 0));
  } else if (ty->kind != rio_TypeKind_builder) rio_FAIL_AT_TOKEN(rio_voidExpr(), "end is the last index of an array, slice, String, Blob or builder");
  len.type = rio_TypeId_int;
  len.access = 0;
  d = rio_allocSlots(vm, 1);
  rio_emit(vm, rio_Op_sub, d, rio_toSlot(vm, &len, 0), rio_constSlot(vm, 1));
  return rio_exprOf(rio_ExprKind_static, rio_TypeId_int, d * 4);
}

/* where an operand goes: a value, a prefix operator, an opening bracket, or an empty call's ')' */
static int rio_parseOperand(rio_Vm *vm, rio_ExprState *x) {
  rio_Compiler *c = vm->compiler;
  int t = c->tok.kind, tempStart = (int)c->freeSlot;
  rio_Expr e;
  if (t == rio_Tok_id && c->opCount > x->opBase && rio_opTop(vm)->kind == rio_Pending_call && rio_opTop(vm)->a == rio_ExprKind_builtin &&
      rio_opTop(vm)->b == rio_Builtin_sizeOf && rio_opTop(vm)->argCount == 0) { /* sizeOf(Blob[4]): a whole type */
    int i = rio_lookup(vm, c->tok.text, c->tok.len);
    if (i >= 0 && c->syms[i].kind == rio_SymKind_type) {
      int type = rio_parseType(vm);
      if (rio_failed(vm)) return rio_Expect_failed;
      rio_pushResult(vm, rio_exprOf(rio_ExprKind_type, type, 0), tempStart);
      return rio_Expect_operator;
    }
  }
  if (t == rio_Tok_id && c->peek.kind == '[') { /* Map[Int, Point]: a generic type, filled in (Map.make: its static proc) */
    int i = rio_lookup(vm, c->tok.text, c->tok.len);
    if (i >= 0 && c->syms[i].kind == rio_SymKind_type && rio_type(vm, c->syms[i].type)->kind == rio_TypeKind_generic) {
      int type = rio_parseType(vm);
      if (rio_failed(vm)) return rio_Expect_failed;
      rio_pushResult(vm, rio_exprOf(rio_ExprKind_type, type, 0), tempStart);
      return rio_Expect_operator;
    }
  }
  if (t == rio_Tok_int || t == rio_Tok_float) e = rio_exprOf(rio_ExprKind_const, t == rio_Tok_int ? rio_TypeId_int : rio_TypeId_float, c->tok.val.i);
  else if (t == rio_Tok_string) e = rio_stringLiterals(vm, x);
  else if (t == rio_Tok_id) e = rio_identifier(vm);
  else if (t == rio_Tok_end) e = rio_endIndex(vm, x); /* (xs[end]) */
  else if (t == rio_Tok_proc) { /* a proc type, as a value: Less :: (proc(a, b: Int) -> Bool), an alias */
    int type = rio_parseType(vm);
    if (rio_failed(vm)) return rio_Expect_failed;
    rio_pushResult(vm, rio_exprOf(rio_ExprKind_type, type, 0), tempStart);
    return rio_Expect_operator;
  }
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
    rio_PendingOp *o = c->opCount > x->opBase ? rio_opTop(vm) : 0;
    if (t == '.' && o && o->kind == rio_Pending_call && o->a == rio_ExprKind_builtin && o->argCount == 1 &&
        (o->b == rio_Builtin_offsetOf || o->b == rio_Builtin_bitOffsetOf))
      return rio_fieldPath(vm, tempStart); /* offsetOf(S, .field) */
    return rio_impliedValue(vm, x, tempStart);
  } else if (t == '{') { /* {field = ...} or {a, b} takes its struct or array type from where the value is going */
    int type = rio_targetType(vm, x), k = type < 0 ? -1 : rio_type(vm, type)->kind;
    if (type >= 0 && rio_mentions(vm, type, -2))
      rio_FAIL_AT_TOKEN(rio_Expect_failed, "this field's type isn't known yet: give a field that settles it first, or name the struct's types");
    if (k == rio_TypeKind_slice) { /* (a slice: a literal of its elements, viewed) */
      rio_pushResult(vm, rio_exprOf(rio_ExprKind_type, rio_type(vm, type)->elem, rio_Infer_slice), tempStart);
      x->bareBrace = 1;
      return rio_Expect_operator;
    }
    if (k != rio_TypeKind_struct && k != rio_TypeKind_array && k != rio_TypeKind_builder)
      rio_FAIL_AT_TOKEN(rio_Expect_failed, "can't tell which struct or array this is; write Type{...} or [N]T{...}");
    rio_pushResult(vm, rio_exprOf(rio_ExprKind_type, type, 0), tempStart);
    x->bareBrace = 1;
    return rio_Expect_operator; /* the '{' is next: the literal itself */
  } else if (t == '[') { /* [N]T{...}, [..N]T{...}; or after is, any type: x is []Int */
    int type, k, infer = c->peek.kind == ']' ? rio_Infer_slice : 0;
    if (c->peek.kind == rio_Tok_id && c->peek.len == 1 && c->peek.text[0] == '_' && rio_peekAfterNext(vm) == ']') infer = rio_Infer_count;
    if (infer) { /* [_]T{...} and []T{...}, sized by their values; [_]{...} and []{...}, typed by the first too */
      rio_next(vm);
      if (infer == rio_Infer_count) rio_next(vm);
      rio_next(vm);
      type = c->tok.kind == '{' ? rio_TypeId_void : rio_parseType(vm);
      if (rio_failed(vm)) return rio_Expect_failed;
      if (c->tok.kind == '{') {
        rio_pushResult(vm, rio_exprOf(rio_ExprKind_type, type, infer), tempStart);
        return rio_Expect_operator;
      }
      if (infer == rio_Infer_count) rio_FAIL_AT_TOKEN(rio_Expect_failed, "[_] is for a literal, sized by its values: [_]T{a, b}");
      type = rio_sliceOf(vm, type); /* (a slice type, after is: x is []Int) */
    } else type = rio_parseType(vm);
    if (rio_failed(vm)) return rio_Expect_failed;
    k = rio_type(vm, type)->kind;
    if (c->opCount > x->opBase && c->tok.kind != '{' &&
        ((rio_opTop(vm)->kind == rio_Pending_binary && rio_opTop(vm)->op == rio_Tok_is) ||
         (rio_opTop(vm)->kind == rio_Pending_call && rio_opTop(vm)->a == rio_ExprKind_builtin && rio_opTop(vm)->b == rio_Builtin_sizeOf))) {
      rio_pushResult(vm, rio_exprOf(rio_ExprKind_type, type, 0), tempStart);
      return rio_Expect_operator;
    }
    if (c->tok.kind != '{' || (k != rio_TypeKind_array && k != rio_TypeKind_builder))
      rio_FAIL_AT_TOKEN(rio_Expect_failed, "only arrays and builders have literals here: [N]T{a, b} or [..N]T{a, b}");
    rio_pushResult(vm, rio_exprOf(rio_ExprKind_type, type, 0), tempStart);
    return rio_Expect_operator;
  } else if (t == rio_Tok_if || t == rio_Tok_switch)
    rio_FAIL_AT_TOKEN(rio_Expect_failed, "an if or switch is a value only as all of one, right after :=, = or return");
  else rio_FAIL_AT_TOKEN(rio_Expect_failed, rio_msgExpectedExpr);
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
    if (callee.kind == rio_ExprKind_builtin && (callee.value == rio_Builtin_format || callee.value == rio_Builtin_print)) rio_argDone(vm, o);
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
  if (callee.kind == rio_ExprKind_ffi) rio_ffiRoom(vm, o, callee.value);
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
    if (m->kind == rio_Pending_call && c->tok.kind == ')') { /* f(a, b,): a trailing comma */
      rio_closeBracket(vm, ')', 0);
      x->depth--;
      rio_next(vm);
      return rio_Expect_operator;
    }
    if (m->kind == rio_Pending_literal) rio_literalField(vm, m);
    return rio_Expect_operand;
  }
  if (t == ':') rio_FAIL(rio_Expect_failed, "slices take ranges: a[i..<j], a[i..=j], a[i..], a[..<j], a[]");
  if (t == rio_Tok_rangeExcl || t == rio_Tok_rangeIncl || t == '.') { /* xs[a..<b], xs[a..=b], xs[a..] */
    if (m->kind != rio_Pending_index || m->a) rio_FAIL(rio_Expect_failed, "unexpected range");
    m->a = t == rio_Tok_rangeIncl ? 2 : 1;
    rio_next(vm);
    if (t == '.') { /* the end left out: the rest */
      rio_next(vm);
      if (c->tok.kind != ']') rio_FAIL(rio_Expect_failed, "a[i..] is the rest: an end goes after ..< or ..=");
      rio_pushResult(vm, rio_exprOf(rio_ExprKind_sliceEnd, rio_TypeId_int, 0), (int)c->freeSlot);
      return rio_Expect_operator;
    }
    if (c->tok.kind == ']') rio_FAIL(rio_Expect_failed, "a range needs its end: a[i..<j], or a[i..] for the rest");
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
    if (t == rio_Tok_or && rio_isMaybe(vm, l->type)) { /* x or y: x's value if it's there (kept, whole), else y */
      rio_Expr kept;
      rio_isTestUsed(vm, l, -1);
      c->freeSlot = l->tempStart;
      kept = rio_exprOf(rio_ExprKind_static, l->type, rio_allocSlots(vm, rio_typeWords(vm, l->type)) * 4);
      rio_store(vm, &kept, l);
      o = rio_pushOp(vm, rio_Pending_or, prec, t);
      o->a = rio_emit(vm, rio_Op_jnz, kept.value / 4, 0, rio_NONE);
      o->b = kept.value / 4;
      o->c = c->symCount;
      o->argCount = (int32_t)c->nameBytes;
      o->punAt = l->type;
      kept.tempStart = l->tempStart;
      *l = kept;
      rio_next(vm);
      return rio_Expect_operand;
    }
    if (rio_isMaybe(vm, l->type)) rio_maybeTest(vm, l, t == rio_Tok_and); /* x and ...: whether x is there (x its value on the right) */
    if (l->type != rio_TypeId_bool)
      rio_FAIL(rio_Expect_failed, rio_narrowedSymbol(vm, l) >= 0 ? "this maybe is known to hold its value here (it was checked before): use it as one"
                                                                 : "expected Bool, or a maybe");
    s = rio_toSlot(vm, l, 1);
    c->freeSlot = l->tempStart;
    d = rio_allocSlots(vm, 1);
    if (s != d) rio_emit(vm, rio_Op_mov, d, s, 0);
    rio_isTestCarry(vm, l, d * 4, t);
    o = rio_pushOp(vm, t == rio_Tok_and ? rio_Pending_and : rio_Pending_or, prec, t);
    o->a = rio_emit(vm, t == rio_Tok_and ? rio_Op_jz : rio_Op_jnz, d, 0, rio_NONE);
    o->b = d;
    o->c = c->symCount;
    o->argCount = (int32_t)c->nameBytes;
    o->punAt = 0;
    rio_narrowRight(vm, d * 4, t == rio_Tok_or); /* (or: the right side runs when the left is false) */
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
  int t = c->tok.kind, prec, bare = x->bareBrace;
  x->bareBrace = 0;
  if (c->tok.newline && !x->depth && !(bare && t == '{')) return rio_Expect_done; /* a new line ends it, outside brackets */
  if (t == '(') return rio_openCall(vm, x);
  if (t == '[') {
    rio_pushOp(vm, rio_Pending_index, 0, 0);
    x->depth++;
    rio_next(vm);
    if (c->tok.kind == ':') rio_FAIL(rio_Expect_failed, "slices take ranges: a[i..<j], a[i..=j], a[i..], a[..<j], a[]");
    if (c->tok.kind == ']' || c->tok.kind == rio_Tok_rangeExcl || c->tok.kind == rio_Tok_rangeIncl) {
      /* a[], a[..<j]: the start left out, 0 (marked, so an [Enum]T takes it) */
      rio_Expr zero = rio_exprOf(rio_ExprKind_const, rio_TypeId_int, 0);
      zero.offset = 1;
      rio_pushResult(vm, zero, (int)c->freeSlot);
      if (c->tok.kind == ']') { /* and the end: all of it */
        rio_opTop(vm)->a = 1;
        rio_pushResult(vm, rio_exprOf(rio_ExprKind_sliceEnd, rio_TypeId_int, 0), (int)c->freeSlot);
      }
      return rio_Expect_operator;
    }
    return rio_Expect_operand;
  }
  if (t == rio_Tok_rangeExcl || t == rio_Tok_rangeIncl || (t == '.' && c->peek.kind == '.')) {
    if (!x->depth && c->rangeStops) return rio_Expect_done; /* (for x in a..<b: the loop's) */
    if (rio_inIndex(vm, x->opBase)) return rio_separator(vm, x, t);
    if (t == '.') rio_FAIL(rio_Expect_failed, "a range's end is left out only in a slice: a[i..]");
  }
  if (t == '.') {
    rio_next(vm);
    if (c->tok.kind != rio_Tok_id && c->tok.kind != rio_Tok_nil) rio_FAIL(rio_Expect_failed, "field name expected");
    rio_member(vm);
    rio_next(vm);
    return rio_Expect_operator;
  }
  if (t == '{' && rio_valueTop(vm)->kind == rio_ExprKind_type && rio_valueTop(vm)->value) { /* [_]T{...}, []T{...}: grows a value at a time */
    rio_Expr type = rio_popValue(vm);
    rio_PendingOp *o;
    rio_openLiteral(vm);
    o = rio_pushOp(vm, rio_Pending_literal, 0, 1 + type.value);
    o->b = type.type == rio_TypeId_void ? -1 : type.type;
    o->a = o->b < 0; /* (its first value gives the element type) */
    o->c = (int32_t)c->freeSlot * 4;
    x->depth++;
    rio_next(vm);
    rio_literalField(vm, o);
    return rio_Expect_operand;
  }
  if (t == '{') { /* Type{...}, [N]T{...} */
    rio_Expr type = *rio_valueTop(vm);
    rio_PendingOp *o;
    int k, isArray;
    if (type.kind == rio_ExprKind_type && rio_type(vm, type.type)->kind == rio_TypeKind_generic && rio_type(vm, rio_type(vm, type.type)->elem)->kind == rio_TypeKind_struct)
      type.type = rio_type(vm, type.type)->elem; /* Table{...}: laid out as its template (every one is), its Ts found from its fields */
    k = rio_type(vm, type.type)->kind;
    isArray = k == rio_TypeKind_array || k == rio_TypeKind_builder;
    if (type.kind != rio_ExprKind_type || (k != rio_TypeKind_struct && !isArray)) return rio_Expect_done;
    c->valueCount--;
    rio_openLiteral(vm);
    o = rio_pushOp(vm, rio_Pending_literal, 0, isArray);
    o->b = type.type;
    o->c = rio_allocSlots(vm, rio_typeWords(vm, type.type)) * 4;
    if (isArray ? rio_type(vm, rio_type(vm, type.type)->elem)->size % 4 != 0 : (rio_type(vm, type.type)->fieldCount > 64 || rio_hasBitfields(vm, type.type)))
      /* (bytes don't fill whole words, finishing tracks only 64 fields, and bitfields share words: zero all
         of it now) */
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
  x.bareBrace = 0;
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
