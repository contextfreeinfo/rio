/* operators.c - unary and binary operators: constant folding, typed instructions, element-by-element
   arithmetic on arrays and structs of numbers, and unions' `is` with the narrowing it allows (see
   docs/spec/expressions.md). */
#include "compiler.h"
#ifndef rio_NO_COMPILER

static int rio_isCompare(int op) {
  return op == rio_Tok_eqEq || op == rio_Tok_notEq || op == '<' || op == '>' || op == rio_Tok_lessEq || op == rio_Tok_greaterEq;
}

/* l = l op r, both constants (Floats when isFloat), as the vm would compute it */
static void rio_fold(rio_Vm *vm, int op, rio_Expr *l, rio_Expr *r, int isFloat) {
  rio_Val a, b, result;
  int32_t x, y;
  a.i = l->value;
  b.i = r->value;
  x = a.i;
  y = b.i;
  if (isFloat) {
    switch (op) {
    case '+': result.f = a.f + b.f; break;
    case '-': result.f = a.f - b.f; break;
    case '*': result.f = a.f * b.f; break;
    case '/': result.f = a.f / b.f; break;
    case rio_Tok_eqEq: result.i = a.f == b.f; break;
    case rio_Tok_notEq: result.i = a.f != b.f; break;
    case '<': result.i = a.f < b.f; break;
    case '>': result.i = a.f > b.f; break;
    case rio_Tok_lessEq: result.i = a.f <= b.f; break;
    case rio_Tok_greaterEq: result.i = a.f >= b.f; break;
    default: rio_FAIL(, "integer operator on floats");
    }
  } else {
    switch (op) {
    case '+': result.u = a.u + b.u; break;
    case '-': result.u = a.u - b.u; break;
    case '*': result.u = a.u * b.u; break;
    case '/':
    case '%':
      if (!y) rio_FAIL(, "division by zero");
      if (y == -1) result.i = op == '/' ? (int32_t)(0u - a.u) : 0; /* (INT_MIN / -1 wraps) */
      else result.i = op == '/' ? x / y : x % y;
      break;
    case '&': result.u = a.u & b.u; break;
    case '|': result.u = a.u | b.u; break;
    case '^': result.u = a.u ^ b.u; break;
    case rio_Tok_shiftLeft: result.u = a.u << (b.u & 31); break;
    case rio_Tok_shiftRight: result.i = x >> (b.u & 31); break;
    case rio_Tok_eqEq: result.i = x == y; break;
    case rio_Tok_notEq: result.i = x != y; break;
    case '<': result.i = x < y; break;
    case '>': result.i = x > y; break;
    case rio_Tok_lessEq: result.i = x <= y; break;
    default: result.i = x >= y; break;
    }
  }
  l->value = result.i;
}

/* ------------------------------------------------------------------ unions and is */

/* Unions: a tag word (the index of the type held, in the order listed) and room for the biggest type.
   Which of the union's types e is (nil: a constant of the union type), or -1. */
int rio_unionMember(rio_Vm *vm, int unionType, const rio_Expr *e) {
  rio_Type *u = rio_type(vm, unionType);
  int i;
  if (e->kind == rio_ExprKind_const && e->type == unionType) return e->value;
  for (i = 0; i < u->fieldCount; i++) {
    int t = rio_field(vm, u->firstField + i)->type;
    if (t != rio_TypeId_void && t == rio_valueType(e->type)) return i;
  }
  if (u->elem == rio_TypeId_float && e->kind == rio_ExprKind_const && e->type == rio_TypeId_int) return 1; /* (?Float = 1: as Float = 1) */
  return -1;
}

/* stores a value of one of dst's types (or nil) into union dst: the value, then the tag. 0 if it isn't
   one (it may be the union itself) */
int rio_storeIntoUnion(rio_Vm *vm, rio_Expr *dst, rio_Expr *src) {
  int member = rio_unionMember(vm, dst->type, src);
  rio_Expr tag = *dst, tagValue;
  rio_Field *f;
  if (member < 0 || (src->type == dst->type && src->kind != rio_ExprKind_const)) return 0;
  /* two stores through dst's address: the first mustn't fuse with the instruction that computed it
     (xs[i]'s idx), or the second finds no address */
  if (dst->kind == rio_ExprKind_indirect) rio_label(vm);
  f = rio_field(vm, rio_type(vm, dst->type)->firstField + member);
  if (f->type != rio_TypeId_void) {
    rio_Expr payload = *dst;
    payload.type = f->type;
    rio_offsetPlace(&payload, 4);
    rio_store(vm, &payload, src);
  }
  tagValue = rio_exprOf(rio_ExprKind_const, rio_TypeId_int, member);
  tag.type = rio_TypeId_int;
  tag.access = 0;
  rio_store(vm, &tag, &tagValue);
  return 1;
}

/* Narrowing in an if: an `x is T` holds inside the block if it's one of the and-ed parts of the
   condition. Each one's result is followed: and carries it on to its own result (carry), anything else
   using it drops it (carry -1). */
void rio_isTestUsed(rio_Vm *vm, const rio_Expr *e, int carry) {
  rio_Compiler *c = vm->compiler;
  int i;
  if (e->kind != rio_ExprKind_static) return;
  for (i = 0; i < c->isTestCount; i++)
    if (c->isTests[i].resultAt == e->value) c->isTests[i].resultAt = carry;
}

/* x is T where x is narrowed already (if x is A and x is B): known while compiling, from the union it
   was narrowed from */
static int rio_isNarrowedOp(rio_Vm *vm, rio_Expr *l, const rio_Expr *r) {
  rio_Compiler *c = vm->compiler;
  rio_Symbol *y;
  int k = rio_narrowedSymbol(vm, l), u, member, tempStart = l->tempStart;
  if (k < 0) return 0;
  for (u = k - 1; u >= 0; u--) {
    y = &c->syms[u];
    if (y->kind != rio_SymKind_narrowed && y->nameLen == c->syms[k].nameLen &&
        !memcmp(c->names + y->name, c->names + c->syms[k].name, y->nameLen) && rio_type(vm, y->type)->kind == rio_TypeKind_union)
      break;
  }
  if (u < 0) return 0;
  u = c->syms[u].type;
  if (r->kind == rio_ExprKind_type || (r->kind == rio_ExprKind_const && r->type == u)) member = rio_unionMember(vm, u, r);
  else rio_FAIL(1, "is needs a type (or nil)");
  if (member < 0) rio_FAIL(1, "not one of this union's types");
  *l = rio_exprOf(rio_ExprKind_const, rio_TypeId_bool, rio_field(vm, rio_type(vm, u)->firstField + member)->type == l->type);
  l->tempStart = (uint16_t)tempStart;
  return 1;
}

/* u is T, u is nil: compare the tag */
static void rio_isOp(rio_Vm *vm, rio_Expr *l, rio_Expr *r) {
  rio_Compiler *c = vm->compiler;
  int member;
  int32_t varAt;
  rio_Expr tag, tagValue;
  rio_needValue(vm, l);
  if (rio_isNarrowedOp(vm, l, r)) return;
  if (rio_type(vm, l->type)->kind != rio_TypeKind_union) rio_FAIL(, "is tests which type a union holds");
  if (r->kind == rio_ExprKind_type || (r->kind == rio_ExprKind_const && r->type == l->type)) member = rio_unionMember(vm, l->type, r);
  else rio_FAIL(, "is needs a type (or nil)");
  if (member < 0) rio_FAIL(, "not one of this union's types");
  varAt = rio_narrowableAt(vm, l);
  tag = *l;
  tag.type = rio_TypeId_int;
  tag.access = 0;
  tagValue = rio_exprOf(rio_ExprKind_const, rio_TypeId_int, member);
  tagValue.tempStart = r->tempStart;
  rio_binaryOp(vm, rio_Tok_eqEq, &tag, &tagValue);
  *l = tag;
  if (varAt >= 0 && l->kind == rio_ExprKind_static && c->isTestCount < 8) { /* a variable: it can be narrowed */
    c->isTests[c->isTestCount].resultAt = l->value;
    c->isTests[c->isTestCount].varAt = varAt;
    c->isTests[c->isTestCount].member = member;
    c->isTestCount++;
  }
}

/* after the condition of an if: narrows each x is T its result still carries */
void rio_narrowIsTests(rio_Vm *vm, const rio_Expr *cond) {
  rio_Compiler *c = vm->compiler;
  int i;
  for (i = 0; i < c->isTestCount; i++)
    if (cond->kind == rio_ExprKind_static && c->isTests[i].resultAt == cond->value)
      rio_narrow(vm, c->isTests[i].varAt, c->isTests[i].member, 0);
  c->isTestCount = 0;
}

/* the right side of an and, which runs only when the left side (its result at resultAt) is true: each
   x is T the left side carries narrows x there, from a temporary copy (the and's end drops the names) */
void rio_narrowRight(rio_Vm *vm, int resultAt) {
  rio_Compiler *c = vm->compiler;
  int i;
  for (i = 0; i < c->isTestCount; i++)
    if (c->isTests[i].resultAt == resultAt) rio_narrow(vm, c->isTests[i].varAt, c->isTests[i].member, 1);
}

/* a union e that `is` or a switch can narrow, as rio_narrow knows it: a variable's address, or a
   reference's slot (a loop's element, a & parameter) with rio_REF_AT; or -1 */
int32_t rio_narrowableAt(rio_Vm *vm, const rio_Expr *e) {
  rio_Compiler *c = vm->compiler;
  int k;
  if (rio_type(vm, e->type)->kind != rio_TypeKind_union) return -1;
  if (e->kind == rio_ExprKind_static && !e->access && e->value < rio_SLOTS_END) return e->value;
  if (e->kind == rio_ExprKind_indirect && !e->offset && !(e->access & ~rio_Access_readOnly))
    for (k = c->symCount - 1; k >= 0; k--)
      if ((c->syms[k].kind == rio_SymKind_ref || c->syms[k].kind == rio_SymKind_readOnlyRef) && c->syms[k].value == e->value &&
          c->syms[k].type == e->type)
        return rio_REF_AT | e->value;
  return -1;
}

/* Inside a block that knows the union variable at varAt (or through the reference, with rio_REF_AT)
   holds its type `member`: the name now reads as that type, from a copy taken here (so it stays the
   right type whatever happens to the union). temp: in an expression, the copy in a temporary slot. */
void rio_narrow(rio_Vm *vm, int32_t varAt, int member, int temp) {
  rio_Compiler *c = vm->compiler;
  rio_Symbol *y = 0;
  rio_Field *f;
  rio_Expr src, dst;
  uint32_t at;
  int k, ref = (varAt & rio_REF_AT) != 0;
  for (k = c->symCount - 1; k >= 0; k--) {
    y = &c->syms[k];
    if (ref ? (y->kind == rio_SymKind_ref || y->kind == rio_SymKind_readOnlyRef) && y->value == (varAt & ~rio_REF_AT)
            : y->kind == rio_SymKind_var && y->value == varAt)
      if (rio_type(vm, y->type)->kind == rio_TypeKind_union) break;
  }
  if (k < 0) return;
  f = rio_field(vm, rio_type(vm, y->type)->firstField + member);
  if (f->type == rio_TypeId_void) return;
  if (rio_isBig(rio_type(vm, f->type))) at = rio_allocHigh(vm, rio_type(vm, f->type)->size);
  else if (temp) at = (uint32_t)rio_allocSlots(vm, rio_typeWords(vm, f->type)) * 4;
  else {
    c->freeSlot = c->liveSlots;
    at = (uint32_t)rio_allocSlots(vm, rio_typeWords(vm, f->type)) * 4;
    c->liveSlots = c->freeSlot;
  }
  if (ref) {
    src = rio_exprOf(rio_ExprKind_indirect, f->type, varAt & ~rio_REF_AT);
    rio_offsetPlace(&src, 4);
  } else src = rio_exprOf(rio_ExprKind_static, f->type, varAt + 4);
  dst = rio_exprOf(rio_ExprKind_static, f->type, (int32_t)at);
  rio_store(vm, &dst, &src);
  rio_addSymbol(vm, c->names + c->syms[k].name, c->syms[k].nameLen, rio_SymKind_narrowed, f->type, (int32_t)at);
}

/* the narrowed symbol e was read from, if e is exactly the narrowed name (not a field of it); or -1 */
int rio_narrowedSymbol(rio_Vm *vm, const rio_Expr *e) {
  rio_Compiler *c = vm->compiler;
  int k;
  for (k = c->symCount - 1; k >= 0; k--) {
    rio_Symbol *y = &c->syms[k];
    if (y->kind == rio_SymKind_narrowed && y->nameLen && y->value == e->value && y->type == e->type) return k;
  }
  return -1;
}

/* ------------------------------------------------------------------ binary and unary */

/* the instruction for binary op on Ints or Floats; *swap: the operands go the other way round (a > b is
   b < a). -1: not an operator for these */
static int rio_binaryInstr(rio_Vm *vm, int op, int isFloat, int *swap) {
  *swap = 0;
  switch (op) {
  case '+': return isFloat ? rio_Op_fadd : rio_Op_add;
  case '-': return isFloat ? rio_Op_fsub : rio_Op_sub;
  case '*': return isFloat ? rio_Op_fmul : rio_Op_mul;
  case '/': return isFloat ? rio_Op_fdiv : rio_Op_div;
  case rio_Tok_eqEq: return isFloat ? rio_Op_feq : rio_Op_eq;
  case rio_Tok_notEq: return isFloat ? rio_Op_fne : rio_Op_ne;
  case '>': *swap = 1; /* fall through */
  case '<': return isFloat ? rio_Op_flt : rio_Op_lt;
  case rio_Tok_greaterEq: *swap = 1; /* fall through */
  case rio_Tok_lessEq: return isFloat ? rio_Op_fle : rio_Op_le;
  case '%': if (!isFloat) return rio_Op_mod; break;
  case '&': if (!isFloat) return rio_Op_and; break;
  case '|': if (!isFloat) return rio_Op_or; break;
  case '^': if (!isFloat) return rio_Op_xor; break;
  case rio_Tok_shiftLeft: if (!isFloat) return rio_Op_shl; break;
  default: if (!isFloat) return rio_Op_shr; break;
  }
  rio_FAIL(-1, "integer operator on floats");
}

/* l = l op r */
/* l..<r, l..=r: a Range, its first and last (inclusive: l..<r ends at r - 1), in new slots (either side's
   value may be in the first of them) */
static void rio_rangeOp(rio_Vm *vm, int op, rio_Expr *l, rio_Expr *r) {
  int ls, rs, d, tempStart = l->tempStart < r->tempStart ? l->tempStart : r->tempStart;
  if (rio_valueType(l->type) != rio_TypeId_int || rio_valueType(r->type) != rio_TypeId_int) rio_FAIL(, "a range is of Ints: a..<b, a..=b");
  ls = rio_toSlot(vm, l, 1);
  rs = rio_toSlot(vm, r, 1);
  d = rio_allocSlots(vm, 2);
  rio_emit(vm, rio_Op_mov, d, ls, 0);
  if (op == rio_Tok_rangeExcl) rio_emit(vm, rio_Op_sub, d + 1, rs, rio_constSlot(vm, 1));
  else rio_emit(vm, rio_Op_mov, d + 1, rs, 0);
  *l = rio_exprOf(rio_ExprKind_static, rio_TypeId_range, d * 4);
  l->tempStart = (uint16_t)tempStart;
}

void rio_binaryOp(rio_Vm *vm, int op, rio_Expr *l, rio_Expr *r) {
  rio_Compiler *c = vm->compiler;
  int lt, rt, isCompare, instr, ls, rs, d;
  if (op == rio_Tok_is) {
    rio_isOp(vm, l, r);
    return;
  }
  rio_isTestUsed(vm, l, -1);
  rio_isTestUsed(vm, r, -1);
  rio_needValue(vm, l);
  rio_needValue(vm, r);
  if (op == rio_Tok_rangeExcl || op == rio_Tok_rangeIncl) {
    rio_rangeOp(vm, op, l, r);
    return;
  }
  {
    int lk = rio_type(vm, l->type)->kind, rk = rio_type(vm, r->type)->kind;
    if (lk == rio_TypeKind_array || lk == rio_TypeKind_struct || rk == rio_TypeKind_array || rk == rio_TypeKind_struct) {
      rio_elementwiseOp(vm, op, l, r, 0);
      return;
    }
  }
  lt = rio_valueType(l->type);
  rt = rio_valueType(r->type);
  if (l->kind == rio_ExprKind_const && lt == rio_TypeId_int && rt == rio_TypeId_float) { /* 1 + x: 1.0 + x */
    rio_coerce(vm, l, rio_TypeId_float);
    lt = rio_TypeId_float;
  }
  if (r->kind == rio_ExprKind_const && rt == rio_TypeId_int && lt == rio_TypeId_float) {
    rio_coerce(vm, r, rio_TypeId_float);
    rt = rio_TypeId_float;
  }
  if (lt != rt && (rio_type(vm, lt)->elem == rt || rio_type(vm, rt)->elem == lt) &&
      (rio_type(vm, lt)->kind == rio_TypeKind_union || rio_type(vm, rt)->kind == rio_TypeKind_union))
    rio_FAIL(, "this may be nil: check first (if x is T, then x is a T there)");
  if (lt != rt) rio_FAIL(, "type mismatch");
  isCompare = rio_isCompare(op);
  if ((lt == rio_TypeId_string || lt == rio_TypeId_blob) && (op == rio_Tok_eqEq || op == rio_Tok_notEq)) {
    ls = rio_toSlot2(vm, l);
    rs = rio_toSlot2(vm, r);
    instr = op == rio_Tok_eqEq ? rio_Op_seq : rio_Op_sne;
  } else {
    int isFloat = lt == rio_TypeId_float, isEnum = rio_type(vm, lt)->kind == rio_TypeKind_enum, swap;
    int equality = op == rio_Tok_eqEq || op == rio_Tok_notEq;
    if (isEnum && !equality) rio_FAIL(, "enums compare only with == and != (use .toInt() for order)");
    if (!isFloat && lt != rio_TypeId_int && !((lt == rio_TypeId_bool || isEnum) && equality)) rio_FAIL(, "bad operand types");
    if (l->kind == rio_ExprKind_const && r->kind == rio_ExprKind_const) {
      rio_fold(vm, op, l, r, isFloat);
      l->type = (uint16_t)(isCompare ? rio_TypeId_bool : lt);
      l->access = 0;
      return;
    }
    instr = rio_binaryInstr(vm, op, isFloat, &swap);
    if (instr < 0) return;
    ls = rio_toSlot(vm, l, 1);
    rs = rio_toSlot(vm, r, 1);
    if (swap) {
      d = ls;
      ls = rs;
      rs = d;
    }
  }
  /* the result goes in the first temporary of either side: both are done with */
  c->freeSlot = l->tempStart < r->tempStart ? l->tempStart : r->tempStart;
  d = rio_allocSlots(vm, 1);
  rio_emit(vm, instr, d, ls, rs);
  l->kind = rio_ExprKind_static;
  l->value = d * 4;
  l->type = (uint16_t)(isCompare ? rio_TypeId_bool : lt);
  l->access = 0;
  l->tempStart = (uint16_t)d;
}

/* An array or struct made of one number type (Int or Float, any array nesting): that type, and how many
   numbers in *count; 0 if type is anything else. */
int rio_numberArrayInfo(rio_Vm *vm, int type, int *count) {
  rio_Type *ty = rio_type(vm, type);
  uint32_t n = 1;
  int k, i;
  if (ty->kind != rio_TypeKind_array && ty->kind != rio_TypeKind_struct) return 0;
  while (ty->kind == rio_TypeKind_array) {
    n *= ty->count;
    ty = rio_type(vm, ty->elem);
  }
  k = ty->kind;
  if (k == rio_TypeKind_struct) {
    rio_Field *f = rio_field(vm, ty->firstField);
    if (!ty->fieldCount || ty->size != ty->fieldCount * 4u) return 0;
    k = rio_type(vm, f[0].type)->kind;
    for (i = 0; i < ty->fieldCount; i++)
      if (rio_type(vm, f[i].type)->kind != k || f[i].bits) return 0; /* (bitfields: not a vector of numbers) */
    n *= ty->fieldCount;
  }
  if (k != rio_TypeKind_int && k != rio_TypeKind_float) return 0;
  *count = (int)n;
  return k == rio_TypeKind_float ? rio_TypeId_float : rio_TypeId_int;
}

/* l op r element by element; either side may be a scalar (used for every element). The result goes into
   `into` (a op= b: then into is l), else into a temporary that replaces l. */
void rio_elementwiseOp(rio_Vm *vm, int op, rio_Expr *l, rio_Expr *r, rio_Expr *into) {
  rio_Compiler *c = vm->compiler;
  int lCount = 0, rCount = 0, lElem, rElem, elem, n, which, instr, ls, rs, dstAddr, d = 0, type, scalars;
  rio_needValue(vm, l);
  rio_needValue(vm, r);
  lElem = rio_numberArrayInfo(vm, l->type, &lCount);
  rElem = rio_numberArrayInfo(vm, r->type, &rCount);
  if (!lElem && !rElem) rio_FAIL(, "arithmetic needs numbers, or arrays and structs made of one number type");
  type = lElem ? l->type : r->type;
  elem = lElem ? lElem : rElem;
  n = lElem ? lCount : rCount;
  if (lElem && rElem) {
    if (l->type != r->type) rio_FAIL(, "type mismatch");
  } else {
    rio_Expr *scalar = lElem ? r : l;
    if (scalar->kind == rio_ExprKind_const && rio_valueType(scalar->type) == rio_TypeId_int && elem == rio_TypeId_float)
      rio_coerce(vm, scalar, rio_TypeId_float);
    if (rio_valueType(scalar->type) != elem) rio_FAIL(, "type mismatch");
  }
  which = op == '+' ? 0 : op == '-' ? 1 : op == '*' ? 2 : op == '/' ? 3 : op == '%' ? 4 : -1;
  if (which < 0) rio_FAIL(, "arrays and number structs only do + - * / %");
  if (which == 4 && elem == rio_TypeId_float) rio_FAIL(, "integer operator on floats");
  instr = (elem == rio_TypeId_float ? rio_Op_fvadd : rio_Op_vadd) + which;
  scalars = (lElem ? 0 : 16) | (rElem ? 0 : 32);
  ls = lElem ? rio_toAddrSlot(vm, l) : rio_toSlot(vm, l, 1);
  rs = rElem ? rio_toAddrSlot(vm, r) : rio_toSlot(vm, r, 1);
  if (into) dstAddr = into == l ? ls : rio_toAddrSlot(vm, into); /* a op= b: l is the target */
  else { /* at or below every operand's temporaries: safe, since elements are done in order */
    if (n > rio_SMALL_WORDS) rio_FAIL(, "too big for a temporary (over 16 words): change it in place with += -= *= /=");
    c->freeSlot = l->tempStart < r->tempStart ? l->tempStart : r->tempStart;
    d = rio_allocSlots(vm, n);
    dstAddr = rio_addrConst(vm, (uint32_t)d * 4, (uint32_t)n * 4);
  }
  rio_emit(vm, instr, dstAddr, ls, rs);
  vm->code[vm->codeLen - 1].x = (uint8_t)scalars;
  rio_emitData(vm, 0, (uint32_t)n);
  if (!into) {
    l->kind = rio_ExprKind_static;
    l->value = d * 4;
    l->offset = 0;
    l->type = (uint16_t)type;
    l->access = 0;
    l->tempStart = (uint16_t)d;
  }
}

/* e = op e (op: - ~ & or '!' for not) */
void rio_unaryOp(rio_Vm *vm, int op, rio_Expr *e) {
  rio_Compiler *c = vm->compiler;
  int t, s, d;
  rio_isTestUsed(vm, e, -1);
  rio_needValue(vm, e);
  if (op == '&') { /* a reference to a place, for a & parameter or name := &place */
    if (e->bits) rio_FAIL(, "a bitfield has no address of its own: & can't take it");
    if (e->kind == rio_ExprKind_const || e->access ||
        (e->kind == rio_ExprKind_static && (e->value < (int32_t)vm->constCap * 4 || rio_isTemp(vm, e))))
      rio_FAIL(, "& needs a variable, field or element that can be changed");
    e->access = rio_Access_ref;
    return;
  }
  if (op == '-' && (t = rio_numberArrayInfo(vm, e->type, &s)) != 0) { /* -v is 0 - v */
    rio_Expr zero = rio_exprOf(rio_ExprKind_const, t, 0);
    zero.tempStart = e->tempStart;
    rio_elementwiseOp(vm, '-', &zero, e, 0);
    *e = zero;
    return;
  }
  t = rio_valueType(e->type);
  if (op == '!' ? t != rio_TypeId_bool : t != rio_TypeId_int && !(t == rio_TypeId_float && op == '-'))
    rio_FAIL(, "bad operand for unary operator");
  if (e->kind == rio_ExprKind_const) {
    rio_Val v;
    v.i = e->value;
    if (t == rio_TypeId_float) v.f = -v.f;
    else v.i = op == '-' ? (int32_t)(0u - v.u) : op == '!' ? !v.i : ~v.i;
    e->value = v.i;
    e->access = 0;
    return;
  }
  s = rio_toSlot(vm, e, 1);
  c->freeSlot = e->tempStart;
  d = rio_allocSlots(vm, 1);
  rio_emit(vm, op == '-' ? (t == rio_TypeId_float ? rio_Op_fneg : rio_Op_neg) : op == '!' ? rio_Op_not : rio_Op_bnot, d, s, 0);
  e->kind = rio_ExprKind_static;
  e->value = d * 4;
  e->type = (uint16_t)t;
  e->access = 0;
}

#endif
