/* statements.c - statements, and the blocks they open: procs, if/else, loops and switches, each closed
   by its end. Blocks live on a stack (rio_Block) instead of in recursive calls. The rules:
   docs/spec/statements.md, and for procs, docs/spec/declarations.md. */
#include "compiler.h"
#ifndef rio_NO_COMPILER

/* ------------------------------------------------------------------ blocks and scopes */

rio_Block *rio_pushBlock(rio_Vm *vm, int kind) {
  rio_Compiler *c = vm->compiler;
  rio_Block *b;
  if (c->blockCount >= (int)c->limits.blocks) rio_FAIL(&c->blocks[0], "blocks nested too deep");
  b = &c->blocks[c->blockCount++];
  if ((uint32_t)c->blockCount > c->high.blocks) c->high.blocks = (uint32_t)c->blockCount;
  b->kind = (uint8_t)kind;
  b->flow = (uint8_t)((c->unreachable ? rio_Flow_startUnreachable : 0) | rio_Flow_branchesEnd);
  b->symCount = (uint16_t)c->symCount;
  b->nameBytes = (uint16_t)c->nameBytes;
  b->liveSlots = (uint16_t)c->liveSlots;
  b->a = b->b = b->breaks = b->continues = b->condJump = rio_NONE;
  b->labelLen = 0;
  return b;
}

static rio_Block *rio_topBlock(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  return c->blockCount ? &c->blocks[c->blockCount - 1] : 0;
}

/* A block's names end and its slots get reused, except any whose address was taken: a view of them may
   outlive the block, so they keep their memory (and their one type) for good. */
void rio_endScope(rio_Vm *vm, const rio_Block *b) {
  rio_Compiler *c = vm->compiler;
  uint32_t s, top = b->liveSlots;
  for (s = b->liveSlots; s < c->liveSlots; s++)
    if (rio_bitGet(c->exposed, s)) top = s + 1;
  c->symCount = b->symCount;
  c->nameBytes = b->nameBytes;
  c->liveSlots = c->freeSlot = top;
}

/* Reaching ends, for missing returns: the end of a branch (if, else, case), after which the next one
   starts as the block did */
static void rio_endBranch(rio_Compiler *c, rio_Block *b) {
  if (!c->unreachable) b->flow &= (uint8_t)~rio_Flow_branchesEnd;
  c->unreachable = (int16_t)(b->flow & rio_Flow_startUnreachable);
}

/* after a block with branches: its end can't be reached if one of them always runs and none gets to
   its end */
static void rio_endBranches(rio_Compiler *c, rio_Block *b) {
  rio_endBranch(c, b);
  if ((b->flow & rio_Flow_complete) && (b->flow & rio_Flow_branchesEnd)) c->unreachable = 1;
}

/* the open loop labeled name, innermost first, within this proc; -1 if none */
static int rio_findLabel(rio_Vm *vm, const char *name, int n) {
  rio_Compiler *c = vm->compiler;
  int i;
  for (i = c->blockCount - 1; i >= 0 && c->blocks[i].kind != rio_Block_proc; i--) {
    rio_Block *b = &c->blocks[i];
    if (b->kind >= rio_Block_loop && b->labelLen == n && !memcmp(b->label, name, (size_t)n)) return i;
  }
  return -1;
}

/* ------------------------------------------------------------------ if, case and else */

/* `if a` is short for a switch on conditions, `switch` then `case a`: the block is one (an if block,
   whose counter is 1 until its first case), and each case is tested in turn, the first true one run */

/* a condition's test: a jump past the case when it's false, and any `is` narrowing for inside it */
static void rio_condition(rio_Vm *vm, rio_Block *b) {
  rio_Compiler *c = vm->compiler;
  rio_Expr e, cond;
  c->isTestCount = 0;
  e = rio_parseExpr(vm);
  cond = e;
  b->a = (uint16_t)rio_jumpIfFalse(vm, &e);
  rio_narrowIsTests(vm, &cond);
}

/* the branch before ends: a jump from it to the end, and its failed test lands here */
static void rio_nextBranch(rio_Vm *vm, rio_Block *b) {
  rio_Compiler *c = vm->compiler;
  rio_next(vm);
  rio_endScope(vm, b);
  rio_endBranch(c, b);
  b->b = (uint16_t)rio_jumpListAdd(vm, b->b, rio_emit(vm, rio_Op_jmp, 0, 0, rio_NONE));
  rio_patchJumps(vm, b->a, rio_label(vm));
}

static void rio_ifStatement(rio_Vm *vm) {
  rio_Block *b;
  rio_next(vm);
  b = rio_pushBlock(vm, rio_Block_if);
  b->counter = 0;
  rio_condition(vm, b);
}

/* switch with nothing after it: its cases are conditions */
static void rio_conditionSwitch(rio_Vm *vm) {
  rio_Block *b = rio_pushBlock(vm, rio_Block_if);
  b->counter = 1;
}

/* case cond, in an if or a switch on conditions: tested when the cases before weren't true */
static void rio_conditionCase(rio_Vm *vm, rio_Block *b) {
  if (b->counter == 1) { /* (the first: nothing before it) */
    rio_next(vm);
    b->counter = 0;
  } else rio_nextBranch(vm, b);
  rio_condition(vm, b);
}

/* a switch's else: what no case matched */
static void rio_switchElse(rio_Vm *vm, rio_Block *b);

/* else: what no condition before was true for */
static void rio_elseStatement(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  rio_Block *b = rio_topBlock(vm);
  if (b && b->kind == rio_Block_switch) {
    rio_switchElse(vm, b);
    return;
  }
  if (b && b->kind == rio_Block_else) rio_FAIL_AT_TOKEN(, "an if has one else, which comes last");
  if (!b || b->kind != rio_Block_if) rio_FAIL_AT_TOKEN(, "'else' without 'if'");
  if (b->counter == 1) rio_FAIL_AT_TOKEN(, "a switch's body starts with case");
  if (c->peek.kind == rio_Tok_if && !c->peek.newline) rio_FAIL_AT_TOKEN(, "write case for another condition: if a ... case b ... else ... end");
  rio_nextBranch(vm, b);
  b->kind = rio_Block_else;
  b->flow |= rio_Flow_complete;
  b->a = rio_NONE;
}

/* ------------------------------------------------------------------ loops */

/* for x in xs, for x, i in xs: walks an element pointer up to the end, by the element's size (lim+1) */
static void rio_forEach(rio_Vm *vm, rio_Block *b, rio_Expr *e, const char *name, int nameLen, const char *indexName, int indexLen,
                        int pointer, int end, int index) {
  rio_Compiler *c = vm->compiler;
  rio_Type *ty;
  int s, size, readOnly, indexType = rio_TypeId_int;
  rio_needValue(vm, e);
  ty = rio_type(vm, e->type);
  if (ty->kind == rio_TypeKind_list || ty->kind == rio_TypeKind_builder) { /* the elements there when the loop starts */
    int builder = rio_toSlot3(vm, e), view = rio_allocSlots(vm, 2);
    rio_emit(vm, rio_Op_lview, view, builder, 0);
    *e = rio_exprOf(rio_ExprKind_static, rio_sliceOf(vm, ty->elem), view * 4);
    ty = rio_type(vm, e->type);
  }
  if (ty->kind != rio_TypeKind_array && ty->kind != rio_TypeKind_slice) rio_FAIL(, "for needs a range, array, slice or list");
  if (ty->kind == rio_TypeKind_array && rio_isTemp(vm, e)) rio_FAIL(, "a computed array has no home to view: store it in a variable first");
  size = (int)rio_type(vm, ty->elem)->size;
  readOnly = e->access || e->type == rio_TypeId_string;
  if (ty->kind == rio_TypeKind_array && ty->firstField) indexType = ty->firstField; /* for v, d in counts: d is the enum */
  s = rio_toSlot2(vm, e);
  rio_emit(vm, rio_Op_mov, pointer, s, 0);
  if (size != 1) { /* end = address + length * size */
    rio_emit(vm, rio_Op_mul, end, s + 1, rio_constSlot(vm, size));
    rio_emit(vm, rio_Op_add, end, s, end);
  } else rio_emit(vm, rio_Op_add, end, s, s + 1);
  rio_emit(vm, rio_Op_mov, end + 1, rio_constSlot(vm, size), 0);
  if (indexName) rio_emit(vm, rio_Op_mov, index, rio_constSlot(vm, 0), 0);
  c->freeSlot = c->liveSlots;
  rio_addSymbol(vm, name, nameLen, readOnly ? rio_SymKind_readOnlyRef : rio_SymKind_ref, ty->elem, pointer);
  if (indexName) rio_addSymbol(vm, indexName, indexLen, rio_SymKind_var, indexType, index * 4);
  b->kind = rio_Block_each;
  b->counter = (uint16_t)pointer;
  b->limit = (uint16_t)end;
  b->b = (uint16_t)index;
  b->breaks = (uint16_t)rio_emit(vm, rio_Op_jle, end, pointer, rio_NONE); /* (bottom-tested: skip it all when empty) */
  b->a = (uint16_t)rio_label(vm);
}

/* for x in a..<b, for x in a..=b */
static void rio_forRange(rio_Vm *vm, rio_Block *b, rio_Expr *e, const char *name, int nameLen, int counter, int limit) {
  rio_Compiler *c = vm->compiler;
  rio_Expr d = rio_exprOf(rio_ExprKind_static, rio_TypeId_int, counter * 4);
  int inclusive;
  rio_store(vm, &d, e);
  c->freeSlot = c->liveSlots;
  inclusive = c->tok.kind == rio_Tok_rangeIncl;
  rio_next(vm);
  *e = rio_parseExpr(vm);
  if (inclusive) { /* a..=b is a..<b + 1 */
    rio_Expr one = rio_exprOf(rio_ExprKind_const, rio_TypeId_int, 1);
    one.tempStart = (uint16_t)c->freeSlot;
    rio_binaryOp(vm, '+', e, &one);
  }
  d = rio_exprOf(rio_ExprKind_static, rio_TypeId_int, limit * 4);
  rio_store(vm, &d, e);
  c->freeSlot = c->liveSlots;
  rio_addSymbol(vm, name, nameLen, rio_SymKind_var, rio_TypeId_int, counter * 4);
  b->counter = (uint16_t)counter;
  b->limit = (uint16_t)limit;
  b->breaks = (uint16_t)rio_emit(vm, rio_Op_jle, limit, counter, rio_NONE);
  b->a = (uint16_t)rio_label(vm);
}

/* for x in ..., for x, i in ...: a range, or the elements of a sequence (or an enum's values) */
static void rio_forIn(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  const char *name = rio_keep(vm, c->tok.text, c->tok.len), *indexName = 0;
  int nameLen = c->tok.len, indexLen = 0, counter, limit, index = rio_NONE;
  rio_Block *b;
  rio_Expr e;
  rio_next(vm);
  if (c->tok.kind == ',') {
    rio_next(vm);
    if (c->tok.kind != rio_Tok_id) rio_FAIL_AT_TOKEN(, "index name expected");
    indexName = rio_keep(vm, c->tok.text, c->tok.len);
    indexLen = c->tok.len;
    rio_next(vm);
  }
  if (c->tok.kind != rio_Tok_in) rio_FAIL_AT_TOKEN(, "'in' expected");
  rio_next(vm);
  b = rio_pushBlock(vm, rio_Block_for);
  c->freeSlot = c->liveSlots;
  counter = rio_allocSlots(vm, 1);
  limit = rio_allocSlots(vm, 2);
  if (indexName) index = rio_allocSlots(vm, 1);
  c->liveSlots = c->freeSlot;
  e = rio_parseExpr(vm);
  if (e.kind == rio_ExprKind_type && rio_type(vm, e.type)->kind == rio_TypeKind_enum) { /* for d in Enum: its values, in the order listed */
    rio_Type *et = rio_type(vm, e.type);
    int at = (int)et->count * 4, n = et->fieldCount, elem = e.type;
    e = rio_exprOf(rio_ExprKind_static, rio_arrayOf(vm, elem, (uint32_t)n), at);
    e.access = rio_Access_readOnly;
  }
  if (c->tok.kind != rio_Tok_rangeExcl && c->tok.kind != rio_Tok_rangeIncl) {
    rio_forEach(vm, b, &e, name, nameLen, indexName, indexLen, counter, limit, index);
    return;
  }
  if (indexName) rio_FAIL(, "a range loop has no index: its variable is the index");
  rio_forRange(vm, b, &e, name, nameLen, counter, limit);
}

/* for (forever), for cond (while), or for ... in */
static void rio_forStatement(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  rio_Block *b;
  rio_next(vm);
  if (c->tok.newline || c->tok.kind == rio_Tok_end) {
    b = rio_pushBlock(vm, rio_Block_loop);
    b->a = (uint16_t)rio_label(vm);
  } else if (c->tok.kind == rio_Tok_id && (c->peek.kind == rio_Tok_in || c->peek.kind == ',')) rio_forIn(vm);
  else { /* the condition is tested at the top; its end may move it to the bottom (rio_endLoop) */
    int top = rio_label(vm), jump;
    rio_Expr e = rio_parseExpr(vm);
    jump = rio_jumpIfFalse(vm, &e);
    b = rio_pushBlock(vm, rio_Block_loop);
    b->a = (uint16_t)top;
    b->breaks = b->condJump = (uint16_t)jump;
  }
}

/* The end of a for or while loop: jump back to the top. A while loop whose condition is short and simple
   is rotated instead: the condition is repeated at the bottom, inverted, jumping back into the body, so
   each iteration takes one branch instead of two. */
static void rio_endLoop(rio_Vm *vm, rio_Block *b) {
  rio_Compiler *c = vm->compiler;
  int n, condJump = b->condJump;
  int rotate = condJump != rio_NONE && vm->code[condJump].op != rio_Op_jmp && condJump - b->a <= 24;
  for (n = b->a; rotate && n < condJump; n++)
    if (rio_isJump(vm->code[n].op)) rotate = 0;
  if (rotate) {
    rio_patchJumps(vm, b->continues, rio_label(vm));
    c->lineOverride = rio_compiledLine(vm, b->a); /* (the copy keeps the condition's line) */
    for (n = b->a; n <= condJump; n++) {
      rio_Instr in = vm->code[n];
      rio_emit(vm, in.op, in.a, in.b, in.c);
      vm->code[vm->codeLen - 1].x = in.x;
    }
    c->lineOverride = 0;
    rio_invertJump(&vm->code[vm->codeLen - 1]);
    vm->code[vm->codeLen - 1].c = (uint16_t)(condJump + 1);
  } else {
    rio_patchJumps(vm, b->continues, b->a);
    rio_emit(vm, rio_Op_jmp, 0, 0, b->a);
  }
}

/* the end of a proc's body */
static void rio_endProc(rio_Vm *vm, rio_Block *b) {
  rio_Compiler *c = vm->compiler;
  rio_ProcDef *p = &c->procDefs[b->b];
  c->unreachable = 0;
  rio_emit(vm, rio_Op_ret, 0, 0, 0);
  rio_patchJumps(vm, b->a, rio_label(vm));
  p->defined = 1;
  p->end = (uint16_t)vm->codeLen;
  p->frameEnd = (uint16_t)c->slotHigh;
  c->curProc = -1;
  c->symCount = b->symCount;
  c->nameBytes = b->nameBytes;
  c->liveSlots = c->freeSlot = c->slotHigh;
  c->blockCount--;
}

/* the end of the proc being compiled, which host.c's checks make without an `end` */
void rio_finishProc(rio_Vm *vm) { rio_endProc(vm, rio_topBlock(vm)); }

static void rio_endSwitch(rio_Vm *vm, rio_Block *b);

static void rio_endStatement(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  rio_Block *b = rio_topBlock(vm);
  int line = c->tok.line, col = c->tok.col;
  if (!b) rio_FAIL_AT_TOKEN(, "'end' without block");
  rio_next(vm);
  if (b->kind == rio_Block_switch) {
    rio_endSwitch(vm, b);
    rio_endScope(vm, b);
    c->blockCount--;
    return;
  }
  if (b->kind == rio_Block_proc) {
    if (c->procDefs[b->b].resultType != rio_TypeId_void && !c->unreachable) {
      rio_failAt(vm, line, col, 3, "missing return: a proc with a result returns on every path");
      return;
    }
    rio_endProc(vm, b);
    return;
  }
  if (b->kind == rio_Block_loop) rio_endLoop(vm, b);
  else if (b->kind == rio_Block_for) {
    rio_patchJumps(vm, b->continues, rio_label(vm));
    rio_emit(vm, rio_Op_fori, b->counter, b->limit, b->a);
  } else if (b->kind == rio_Block_each) {
    rio_patchJumps(vm, b->continues, rio_label(vm));
    if (b->b != rio_NONE) rio_emit(vm, rio_Op_add, b->b, b->b, rio_constSlot(vm, 1)); /* the index */
    rio_emit(vm, rio_Op_each, b->counter, b->limit, b->a);
  } else rio_patchJumps(vm, b->b, rio_label(vm)); /* if/else: the earlier branches' jumps to the end */
  rio_patchJumps(vm, b->kind >= rio_Block_loop ? b->breaks : b->a, rio_label(vm));
  if (b->kind < rio_Block_loop) rio_endBranches(c, b);
  else c->unreachable = (int16_t)((b->flow & rio_Flow_startUnreachable) || /* (a loop with no condition, never broken) */
                        (b->kind == rio_Block_loop && b->condJump == rio_NONE && !(b->flow & rio_Flow_broken)));
  rio_endScope(vm, b);
  c->blockCount--;
}

/* break / break outer, continue / continue outer: the innermost loop, or the one labeled outer */
static void rio_breakStatement(rio_Vm *vm, int isBreak) {
  rio_Compiler *c = vm->compiler;
  rio_Block *b;
  int i;
  for (i = c->blockCount - 1; i >= 0 && c->blocks[i].kind != rio_Block_proc; i--)
    if (c->blocks[i].kind >= rio_Block_loop) break;
  if (i < 0 || c->blocks[i].kind == rio_Block_proc) rio_FAIL_AT_TOKEN(, "not inside a loop");
  rio_next(vm);
  if (c->tok.kind == rio_Tok_id && !c->tok.newline) {
    if ((i = rio_findLabel(vm, c->tok.text, c->tok.len)) < 0) rio_FAIL_AT_TOKEN(, "no loop with this label around here");
    rio_next(vm);
  }
  b = &c->blocks[i];
  c->unreachable = 1;
  if (isBreak) b->flow |= rio_Flow_broken;
  if (isBreak) b->breaks = (uint16_t)rio_jumpListAdd(vm, b->breaks, rio_emit(vm, rio_Op_jmp, 0, 0, rio_NONE));
  else b->continues = (uint16_t)rio_jumpListAdd(vm, b->continues, rio_emit(vm, rio_Op_jmp, 0, 0, rio_NONE));
}

static void rio_returnStatement(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  rio_ProcDef *p;
  if (c->curProc < 0) rio_FAIL_AT_TOKEN(, "return outside proc");
  rio_next(vm);
  p = &c->procDefs[c->curProc];
  if (p->resultType != rio_TypeId_void) {
    rio_Expr d = rio_exprOf(rio_ExprKind_static, p->resultType, (int32_t)p->resultAt), e;
    int k = rio_type(vm, p->resultType)->kind;
    c->targetType = p->resultType;
    e = rio_parseExpr(vm);
    if ((k == rio_TypeKind_slice || k == rio_TypeKind_builder) && rio_isLocalView(vm, &e))
      rio_FAIL(, "this returns a view of the proc's own memory, which the next call rewrites: make that memory global, or have the caller pass it in");
    rio_store(vm, &d, &e);
  }
  rio_emit(vm, rio_Op_ret, 0, 0, 0);
  c->unreachable = 1;
}

/* ------------------------------------------------------------------ switch */

/* switch x / switch name := x: the subject goes in a slot of its own, for each case to compare */
static void rio_switchStatement(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  rio_Block *b;
  rio_Expr e;
  int type, k, s, subject, narrowable;
  rio_next(vm);
  if (c->tok.newline || c->tok.kind == rio_Tok_eof || c->tok.kind == ';') {
    rio_conditionSwitch(vm);
    return;
  }
  rio_pushBlock(vm, rio_Block_switch);
  if (c->tok.kind == rio_Tok_id && c->peek.kind == rio_Tok_colonEq) { /* the name lives in the switch */
    rio_declareVar(vm);
    e = rio_symbolExpr(vm, c->symCount - 1);
  } else e = rio_parseExpr(vm);
  rio_needValue(vm, &e);
  type = rio_valueType(e.type);
  k = rio_type(vm, type)->kind;
  if (k != rio_TypeKind_int && k != rio_TypeKind_enum && k != rio_TypeKind_union && type != rio_TypeId_string)
    rio_FAIL(, "switch needs an Int, an enum, a union or a String");
  /* a union variable: cases can narrow it */
  narrowable = k == rio_TypeKind_union && e.kind == rio_ExprKind_static && !e.access && e.value < rio_SLOTS_END ? e.value / 4 : rio_NONE;
  if (k == rio_TypeKind_union) { /* its tag */
    rio_Expr tag = e;
    tag.type = rio_TypeId_int;
    tag.access = 0;
    subject = rio_toSlot(vm, &tag, 1);
  } else if (type == rio_TypeId_string) subject = rio_toSlot2(vm, &e); /* (a String: where it is, and its length) */
  else subject = rio_toSlot(vm, &e, 1);
  c->freeSlot = c->liveSlots;
  s = rio_allocSlots(vm, type == rio_TypeId_string ? 2 : 1);
  c->liveSlots = c->freeSlot;
  if (s != subject) rio_emit(vm, rio_Op_mov, s, subject, 0);
  if (type == rio_TypeId_string && s != subject) rio_emit(vm, rio_Op_mov, s + 1, subject + 1, 0);
  if (c->switchCount >= 16) rio_FAIL(, "switches nested too deep");
  b = rio_topBlock(vm);
  b->counter = (uint16_t)s;
  b->limit = (uint16_t)type;
  b->condJump = b->breaks = rio_NONE;
  b->b = 0;
  b->a = (uint16_t)narrowable;
  b->caseSymCount = (uint16_t)c->symCount;
  b->caseNameBytes = (uint16_t)c->nameBytes;
  b->caseLiveSlots = (uint16_t)c->liveSlots;
  b->switchIndex = (uint16_t)c->switchCount++;
  memset(c->switchCases[b->switchIndex], 0, sizeof c->switchCases[0]);
}

/* the next case (or else) of a switch starts from the scope right after the subject, after the case
   before it jumps to the end, and the previous case's failed test lands here */
static void rio_nextCase(rio_Vm *vm, rio_Block *b) {
  rio_Block scope;
  if (b->b & 1) rio_endBranch(vm->compiler, b);
  if (b->b & 1) b->breaks = (uint16_t)rio_jumpListAdd(vm, b->breaks, rio_emit(vm, rio_Op_jmp, 0, 0, rio_NONE));
  rio_patchJumps(vm, b->condJump, rio_label(vm));
  b->condJump = rio_NONE;
  scope = *b;
  scope.symCount = b->caseSymCount;
  scope.nameBytes = b->caseNameBytes;
  scope.liveSlots = b->caseLiveSlots;
  rio_endScope(vm, &scope);
  b->caseLiveSlots = (uint16_t)vm->compiler->liveSlots; /* (lifted above any slot whose address was taken) */
}

/* case v, v...: one jump per value; the last one skips to the next case */
static void rio_caseStatement(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  rio_Block *b = rio_topBlock(vm);
  int body = rio_NONE;
  if (b && b->kind == rio_Block_if) {
    rio_conditionCase(vm, b);
    return;
  }
  if (b && b->kind == rio_Block_else) rio_FAIL_AT_TOKEN(, "the else comes last");
  if (!b || b->kind != rio_Block_switch) rio_FAIL_AT_TOKEN(, "'case' outside an if or a switch");
  if (b->b & 2) rio_FAIL_AT_TOKEN(, "the else of a switch comes last");
  rio_next(vm);
  rio_nextCase(vm, b);
  while (!rio_failed(vm)) {
    rio_Expr v;
    int valueSlot, isUnion = rio_type(vm, b->limit)->kind == rio_TypeKind_union;
    c->targetType = b->limit;
    v = rio_parseExpr(vm);
    if (isUnion) { /* case T: the index of the type */
      int j = v.kind == rio_ExprKind_type || (v.kind == rio_ExprKind_const && v.type == b->limit) ? rio_unionMember(vm, b->limit, &v) : -1;
      if (j < 0) rio_FAIL(, "not one of this union's types");
      v = rio_exprOf(rio_ExprKind_const, b->limit, j);
    } else rio_coerce(vm, &v, b->limit);
    if (rio_failed(vm)) return;
    if (v.kind != rio_ExprKind_const && b->limit != rio_TypeId_string) rio_FAIL(, "case values are constants");
    if (rio_type(vm, b->limit)->kind == rio_TypeKind_enum || isUnion) { /* coverage: each value once */
      int j = isUnion ? v.value : rio_enumIndex(vm, b->limit, v.value);
      if (j >= 0 && j < 256) {
        uint64_t *row = &c->switchCases[b->switchIndex][j >> 6];
        if (*row >> (j & 63) & 1) rio_FAIL(, "this value already has a case");
        *row |= (uint64_t)1 << (j & 63);
      }
    }
    if (b->limit == rio_TypeId_string) { /* a String: compared, the result in a slot of its own */
      int d;
      if (!(v.kind == rio_ExprKind_static && (uint32_t)v.value / 4 < vm->constCount && rio_bitGet(c->constIsString, (uint32_t)v.value / 4)))
        rio_FAIL(, "case values are constants");
      c->freeSlot = c->liveSlots;
      d = rio_allocSlots(vm, 1);
      rio_emit(vm, rio_Op_seq, d, b->counter, v.value / 4);
      if (c->tok.kind != ',') {
        b->condJump = (uint16_t)rio_emit(vm, rio_Op_jz, d, 0, rio_NONE);
        break;
      }
      body = rio_jumpListAdd(vm, body, rio_emit(vm, rio_Op_jnz, d, 0, rio_NONE));
      rio_next(vm);
      continue;
    }
    valueSlot = rio_constSlot(vm, (uint32_t)v.value);
    if (c->tok.kind != ',') { /* the last value: not it, to the next case */
      b->condJump = (uint16_t)rio_emit(vm, rio_Op_jne, b->counter, valueSlot, rio_NONE);
      if (isUnion && body == rio_NONE && b->a != rio_NONE) { /* one type: the variable is that type inside */
        rio_patchJumps(vm, body, rio_label(vm));
        b->b |= 1;
        rio_narrow(vm, (int32_t)b->a * 4, v.value);
        return;
      }
      break;
    }
    body = rio_jumpListAdd(vm, body, rio_emit(vm, rio_Op_jeq, b->counter, valueSlot, rio_NONE));
    rio_next(vm);
  }
  rio_patchJumps(vm, body, rio_label(vm));
  b->b |= 1;
}

static void rio_switchElse(rio_Vm *vm, rio_Block *b) {
  if (b->b & 2) rio_FAIL_AT_TOKEN(, "a switch has one else");
  rio_next(vm);
  rio_nextCase(vm, b);
  b->b |= 3;
  b->flow |= rio_Flow_complete;
}

/* no case matched: fall out; with no else, every enum value (or union type) needs a case */
static void rio_endSwitch(rio_Vm *vm, rio_Block *b) {
  rio_Compiler *c = vm->compiler;
  int i;
  rio_patchJumps(vm, b->condJump, rio_label(vm));
  rio_patchJumps(vm, b->breaks, rio_label(vm));
  if (!(b->b & 2) && (rio_type(vm, b->limit)->kind == rio_TypeKind_enum || rio_type(vm, b->limit)->kind == rio_TypeKind_union)) {
    rio_Type *et = rio_type(vm, b->limit);
    if (et->fieldCount > 256) rio_FAIL(, "a switch on an enum this big needs an else");
    for (i = 0; i < et->fieldCount; i++)
      if (!(c->switchCases[b->switchIndex][i >> 6] >> (i & 63) & 1)) {
        rio_Field *f = rio_field(vm, et->firstField + i);
        char text[rio_ERROR_BUF_SIZE];
        int n = rio_appendText(text, 0, et->kind == rio_TypeKind_union ? "this switch has no case for " : "this switch has no case for ."), j;
        for (j = 0; j < f->nameLen && n < 80; j++) text[n++] = c->names[f->name + j];
        n = rio_appendText(text, n, " (add one, or an else)");
        text[n] = 0;
        rio_FAIL(, text);
      }
    b->flow |= rio_Flow_complete; /* (every value has a case) */
  }
  if (b->b & 1) rio_endBranches(c, b);
  c->switchCount--;
}

/* ------------------------------------------------------------------ expression statements */

/* in the REPL, a bare expression prints its value */
static void rio_autoPrint(rio_Vm *vm, rio_Expr *e) {
  rio_Type *ty = rio_type(vm, e->type);
  int k = ty->kind;
  if (k == rio_TypeKind_slice && ty->elem == rio_TypeId_byte) rio_emitX(vm, rio_Op_logs, 0, rio_toSlot2(vm, e));
  else if (k == rio_TypeKind_float) rio_emitX(vm, rio_Op_logf, 0, rio_toSlot(vm, e, 1));
  else if (k == rio_TypeKind_bool) rio_emitX(vm, rio_Op_logb, 0, rio_toSlot(vm, e, 1));
  else if (k == rio_TypeKind_enum) rio_logEnumName(vm, e, 0);
  else if (k == rio_TypeKind_int || k == rio_TypeKind_byte) rio_emitX(vm, rio_Op_logi, 0, rio_toSlot(vm, e, 1));
  else if (k == rio_TypeKind_struct || k == rio_TypeKind_array || k == rio_TypeKind_slice || k == rio_TypeKind_list ||
           k == rio_TypeKind_builder || k == rio_TypeKind_union) { /* by descriptor */
    int s = rio_printOperand(vm, e), d = rio_descriptor(vm, e->type);
    if (rio_failed(vm)) return;
    rio_emit(vm, rio_Op_logv, s, d, 0);
  } else return;
  rio_emit(vm, rio_Op_loge, 0, 0, 0);
}

/* x = ... on a narrowed x assigns the whole union, and x is the union again from here on (the value
   is read with x still narrowed, so x = Circle{r = x.w} works) */
static void rio_assignNarrowed(rio_Vm *vm, rio_Expr *l) {
  rio_Compiler *c = vm->compiler;
  int k = c->tok.kind == '=' ? rio_narrowedSymbol(vm, l) : -1, i, n;
  rio_Expr r;
  if (k < 0) rio_FAIL(, "this is a copy of what the union holds: assign the whole union to change it");
  n = c->syms[k].nameLen;
  c->syms[k].nameLen = 0; /* hidden for a moment: the name finds the union */
  i = rio_lookup(vm, c->names + c->syms[k].name, n);
  c->syms[k].nameLen = (uint16_t)n;
  if (i < 0) rio_FAIL(, "undefined name");
  *l = rio_symbolExpr(vm, i);
  rio_next(vm);
  c->targetType = l->type;
  r = rio_parseExpr(vm);
  rio_store(vm, l, &r);
  c->syms[k].nameLen = 0; /* hidden for good */
}

/* an expression, maybe assigned to: x = v, x op= v */
static void rio_expressionStatement(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  rio_Expr l = rio_parseExpr(vm), r, current;
  int op, k;
  if (c->tok.kind != '=' && c->tok.kind != rio_Tok_opAssign) {
    if (vm->inRepl && c->curProc < 0 && !c->blockCount && l.kind <= rio_ExprKind_indirect) rio_autoPrint(vm, &l);
    return;
  }
  if (l.access & rio_Access_narrowed) {
    rio_assignNarrowed(vm, &l);
    return;
  }
  if ((l.kind != rio_ExprKind_static && l.kind != rio_ExprKind_indirect) || l.access ||
      (l.kind == rio_ExprKind_static && l.value < (int32_t)vm->constCap * 4))
    rio_FAIL(, "cannot assign to this");
  if (c->tok.kind == '=') {
    rio_next(vm);
    c->targetType = l.type;
    r = rio_parseExpr(vm);
    rio_store(vm, &l, &r);
    return;
  }
  op = c->tok.op; /* x op= v */
  k = rio_type(vm, l.type)->kind;
  rio_next(vm);
  if (k == rio_TypeKind_array || k == rio_TypeKind_struct) { /* element by element, in place */
    r = rio_parseExpr(vm);
    rio_elementwiseOp(vm, op, &l, &r, &l);
    return;
  }
  current = l;
  if (!(l.kind == rio_ExprKind_static && (k == rio_TypeKind_int || k == rio_TypeKind_float) && l.value < rio_SLOTS_END)) {
    int s = rio_toSlot(vm, &current, 0); /* read it once (an element: its address stays put) */
    current = rio_exprOf(rio_ExprKind_static, rio_valueType(l.type), s * 4);
    current.tempStart = (uint16_t)s;
  }
  r = rio_parseExpr(vm);
  rio_binaryOp(vm, op, &current, &r);
  rio_store(vm, &l, &current);
}

/* ------------------------------------------------------------------ declarations */

/* Name :: proc / struct / enum / union, or Name :: a constant expression (a type, proc or module is an
   alias). A top-level constant a -D define names takes the define's value. */
static void rio_constantDecl(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  const char *name = rio_keep(vm, c->tok.text, c->tok.len);
  int nameLen = c->tok.len, line = c->tok.line, col = c->tok.col, k;
  rio_Expr e;
  rio_next(vm);
  rio_next(vm);
  switch (c->tok.kind) {
  case '@': rio_hostDecl(vm, name, nameLen, -1, line, col); return;
  case rio_Tok_proc: rio_procDef(vm, name, nameLen, -1); return;
  case rio_Tok_struct: rio_structDef(vm, name, nameLen); return;
  case rio_Tok_enum: rio_enumDef(vm, name, nameLen); return;
  case rio_Tok_union: rio_unionDef(vm, name, nameLen); return;
  }
  if (c->tok.kind == '[' || (c->tok.kind == rio_Tok_id && c->peek.kind == '[' && (k = rio_lookup(vm, c->tok.text, c->tok.len)) >= 0 &&
                             c->syms[k].kind == rio_SymKind_type)) { /* another name for a type written out: [2]Float, Blob[64] */
    k = rio_parseType(vm);
    if (!rio_failed(vm)) rio_addSymbol(vm, name, nameLen, rio_SymKind_type, k, 0);
    return;
  }
  e = rio_parseExpr(vm);
  if (c->curProc < 0 && !c->blockCount && (e.kind == rio_ExprKind_const || e.type == rio_TypeId_string)) {
    int define = rio_lookup(vm, name, nameLen) - c->firstDefine;
    if (define >= 0 && define < vm->defineCount) e = rio_defineValue(vm, define, e.type);
  }
  if (e.kind == rio_ExprKind_type) rio_addSymbol(vm, name, nameLen, rio_SymKind_type, e.type, 0);
  else if (e.kind == rio_ExprKind_proc) rio_addSymbol(vm, name, nameLen, rio_SymKind_proc, 0, e.value);
  else if (e.kind == rio_ExprKind_module) rio_addSymbol(vm, name, nameLen, rio_SymKind_module, 0, e.value);
  else if (e.kind == rio_ExprKind_const) rio_addSymbol(vm, name, nameLen, rio_SymKind_const, e.type, e.value);
  else if (e.kind == rio_ExprKind_static && e.access && e.type == rio_TypeId_string)
    rio_addSymbol(vm, name, nameLen, rio_SymKind_const, rio_TypeId_string, e.value);
  else rio_FAIL(, "constant expression expected");
}

/* name* :: / name* := / name*: exports the name: the * is dropped, and the next symbol is exported */
static void rio_exportMark(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  int after = rio_peekAfterNext(vm);
  if (after == rio_Tok_colonColon || after == rio_Tok_colonEq || after == ':') {
    rio_Token name = c->tok;
    if (c->curProc >= 0 || c->blockCount) rio_FAIL_AT_TOKEN(, "only top-level names can be exported");
    rio_next(vm);
    c->tok = name;
    c->exporting = 1;
  }
}

/* ------------------------------------------------------------------ statements */

static void rio_statementAfterLabel(rio_Vm *vm, int t) {
  rio_Compiler *c = vm->compiler;
  int i;
  if (t == rio_Tok_id && c->peek.kind == '*') rio_exportMark(vm);
  if (t == '(' && rio_atTypeMethod(vm)) {
    rio_typeMethodDecl(vm);
    return;
  }
  if (t == rio_Tok_id && c->peek.kind == '.' && (i = rio_lookup(vm, c->tok.text, c->tok.len)) >= 0 && c->syms[i].kind == rio_SymKind_type)
    rio_methodDef(vm, c->syms[i].type);
  else if (t == rio_Tok_id && c->peek.kind == rio_Tok_colonColon) rio_constantDecl(vm);
  else if (t == rio_Tok_id && (c->peek.kind == ':' || c->peek.kind == rio_Tok_colonEq)) rio_declareVar(vm);
  else switch (t) {
    case rio_Tok_switch: rio_switchStatement(vm); break;
    case rio_Tok_case: rio_caseStatement(vm); break;
    case rio_Tok_if: rio_ifStatement(vm); break;
    case rio_Tok_else: rio_elseStatement(vm); break;
    case rio_Tok_for: rio_forStatement(vm); break;
    case rio_Tok_end: rio_endStatement(vm); break;
    case rio_Tok_return: rio_returnStatement(vm); break;
    case rio_Tok_break: rio_breakStatement(vm, 1); break;
    case rio_Tok_continue: rio_breakStatement(vm, 0); break;
    case '{': rio_destructure(vm); break;
    default: rio_expressionStatement(vm);
  }
}

void rio_statement(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  rio_Block *top = rio_topBlock(vm);
  int t = c->tok.kind, blocksBefore = c->blockCount, labelLen = 0;
  const char *label = 0;
  c->freeSlot = c->liveSlots;
  if (t == rio_Tok_id && c->peek.kind == ':' && !c->peek.newline && rio_peekAfterNext(vm) == rio_Tok_for) { /* outer: for ... */
    if (rio_findLabel(vm, c->tok.text, c->tok.len) >= 0) rio_FAIL_AT_TOKEN(, "a loop with this label is already open here");
    if (c->tok.len > (int)sizeof c->blocks[0].label) rio_FAIL_AT_TOKEN(, "label too long");
    label = rio_keep(vm, c->tok.text, c->tok.len);
    labelLen = c->tok.len;
    rio_next(vm);
    rio_next(vm);
    t = c->tok.kind;
  }
  if (top && ((top->kind == rio_Block_switch && !(top->b & 3)) || (top->kind == rio_Block_if && top->counter == 1)) &&
      t != rio_Tok_case && t != rio_Tok_else && t != rio_Tok_end)
    rio_FAIL_AT_TOKEN(, "a switch's body starts with case");
  if (t == ';') {
    rio_next(vm);
    return;
  }
  if (t == rio_Tok_import) rio_importStatement(vm);
  else if (t == rio_Tok_include) rio_includeStatement(vm);
  else {
    rio_statementAfterLabel(vm, t);
    if (label && c->blockCount > blocksBefore) {
      rio_Block *loop = rio_topBlock(vm);
      memcpy(loop->label, label, (size_t)labelLen); /* (its own copy: it's needed through the loop) */
      loop->labelLen = (uint16_t)labelLen;
    }
  }
  if (c->exporting) {
    c->exporting = 0;
    rio_FAIL(, "nothing here to export");
  }
  c->freeSlot = c->liveSlots;
  if (c->tok.kind != rio_Tok_eof && c->tok.kind != ';' && !c->tok.newline) rio_FAIL(, "expected end of statement");
}

#endif
