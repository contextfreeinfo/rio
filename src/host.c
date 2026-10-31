/* host.c - what the host provides, declared in Rio with host, and checked against it (the rules:
   docs/spec/host.md):

     name* :: host proc(a: Int, b: Float) -> Int   a host proc (rio_addFfi), or a builtin proc
     Type.name* :: host proc(x: Int) -> Bool       a builtin method, on a named type
     ([..]Int).name* :: host proc() -> Int         ... on any type
     Name* :: host struct                          a builtin type

   A host proc's declared types must be exactly its sig's (an enum or a Bool may stand for an Int), and
   a struct parameter's layout its sig's {...} (rio_layout). In a module, `name` is the host proc registered
   as module.name (a module's key, with / as .), else the global one of that name. A module with host
   procs registered under its name must declare every one of them.

   The proc is written as a proc type, as for proc values (parameter names are for reading). A builtin's
   declaration is checked by compiling a call to it, with arguments of the declared types (a String
   argument as a literal, as hexBlob needs), in a proc of its own that's never called, and comparing the
   call's result with the declared result. The name then means the builtin.

   Host modules: a package import of a name the loader doesn't have, but that host procs are registered
   under (pc.cls, pc.pset), is made from those registrations: its procs, all exported. rio_autoImport
   makes a module visible everywhere, without an import: from the loader's source if it has one
   (compiled first: rio_openAutoImports), else from the registrations. */
#include "compiler.h"
#ifndef rio_NO_COMPILER

/* ------------------------------------------------------------------ host procs and modules */

/* the host proc registered under prefix.name (prefix NULL: just name), or -1 */
static int rio_findFfi(rio_Vm *vm, const char *prefix, int prefixLen, const char *name, int nameLen) {
  int i;
  for (i = 0; i < vm->ffiCount; i++) {
    const char *f = vm->ffi[i].name;
    if (prefix) {
      if (strncmp(f, prefix, (size_t)prefixLen) || f[prefixLen] != '.') continue;
      f += prefixLen + 1;
    }
    if ((int)strlen(f) == nameLen && !memcmp(f, name, (size_t)nameLen)) return i;
  }
  return -1;
}

/* is host proc i registered under prefix (prefix.name, name without a further '.')? */
static int rio_ffiUnder(rio_Vm *vm, int i, const char *prefix, int prefixLen) {
  const char *f = vm->ffi[i].name;
  return !strncmp(f, prefix, (size_t)prefixLen) && f[prefixLen] == '.' && !strchr(f + prefixLen + 1, '.');
}

/* a module's key as a host proc prefix: the path with / as . (into out, which holds 256) */
static int rio_modulePrefix(rio_Vm *vm, int module, char *out) {
  rio_Compiler *c = vm->compiler;
  rio_Module *m = &c->modules[module];
  int i, n = m->keyLen < 255 ? m->keyLen : 255;
  if (module < 2 || m->isInclude) return 0;
  for (i = 0; i < n; i++) out[i] = c->names[m->key + i] == '/' ? '.' : c->names[m->key + i];
  return n;
}

/* are any host procs registered under this module name? */
int rio_hasHostProcs(rio_Vm *vm, const char *key, int keyLen) {
  int i;
  for (i = 0; i < vm->ffiCount; i++)
    if (rio_ffiUnder(vm, i, key, keyLen)) return 1;
  return 0;
}

int rio_hostModule(rio_Vm *vm, const char *key, int keyLen) {
  rio_Compiler *c = vm->compiler;
  rio_Module *m;
  int i, index, n;
  char file[300];
  for (i = 2; i < c->moduleCount; i++) { /* made already */
    m = &c->modules[i];
    if (m->isPackage && !m->isInclude && rio_nameIs(vm, m->key, m->keyLen, key, keyLen)) return i;
  }
  if (!rio_hasHostProcs(vm, key, keyLen)) return -1;
  if (c->moduleCount >= (int)c->limits.modules) rio_FAIL(-1, "too many modules");
  index = c->moduleCount++;
  m = &c->modules[index];
  memset(m, 0, sizeof *m);
  m->key = (uint16_t)rio_addName(vm, key, keyLen);
  m->keyLen = (uint16_t)keyLen;
  m->isPackage = 1;
  m->state = rio_ModuleState_done;
  m->codeStart = m->codeEnd = (uint16_t)vm->codeLen; /* (no code) */
  n = keyLen < 200 ? keyLen : 200; /* a name for error messages */
  memcpy(file, key, (size_t)n);
  n = rio_appendText(file, n, " (host)");
  file[n] = 0;
  m->file = (uint16_t)rio_addName(vm, file, n + 1);
  for (i = 0; i < vm->ffiCount; i++) { /* its procs, all exported */
    if (!rio_ffiUnder(vm, i, key, keyLen)) continue;
    rio_addSymbol(vm, vm->ffi[i].name + keyLen + 1, (int)strlen(vm->ffi[i].name) - keyLen - 1, rio_SymKind_ffi, 0, i);
    c->syms[c->symCount - 1].module = (uint16_t)index;
    c->syms[c->symCount - 1].exported = 1;
    vm->ffi[i].declared = 1;
  }
  return index;
}

/* the end of a module's source: it declares every host proc registered under its name */
void rio_checkHostModule(rio_Vm *vm, int module) {
  char prefix[256], text[rio_ERROR_BUF_SIZE];
  int i, n = rio_modulePrefix(vm, module, prefix), len;
  if (!n) return;
  for (i = 0; i < vm->ffiCount; i++) {
    if (!rio_ffiUnder(vm, i, prefix, n) || vm->ffi[i].declared) continue;
    len = rio_appendText(text, 0, "this module doesn't declare the host proc ");
    len = rio_appendText(text, len, vm->ffi[i].name);
    text[len] = 0;
    rio_FAIL(, text);
  }
}

static const char *rio_typeName(int type) {
  switch (type) {
  case rio_TypeId_void: return "nothing";
  case rio_TypeId_int: return "Int";
  case rio_TypeId_float: return "Float";
  case rio_TypeId_string: return "String";
  case rio_TypeId_blob: return "Blob";
  case rio_TypeId_bool: return "Bool";
  default: return "a type of its own";
  }
}

/* a declared type for a host proc's sig type: the same, or an enum or Bool where the sig has an Int (i),
   which it is underneath */
static int rio_hostTypeOk(rio_Vm *vm, int declared, int sig) {
  int k = rio_type(vm, declared)->kind;
  return declared == sig || (sig == rio_TypeId_int && (k == rio_TypeKind_enum || k == rio_TypeKind_bool));
}

/* ------------------------------------------------------------------ structs, as hosts see them */

void rio_hostPut(rio_HostText *o, const char *s, int len) {
  if (len < 0) len = (int)strlen(s);
  if (o->want) o->differs |= o->n + (uint32_t)len > o->cap || memcmp(o->want + o->n, s, (size_t)len);
  else if (o->n + (uint32_t)len < o->cap) memcpy(o->out + o->n, s, (size_t)len);
  o->n += (uint32_t)len;
}
void rio_hostPutInt(rio_HostText *o, int32_t v) {
  char b[16];
  rio_hostPut(o, b, rio_formatInt(b, v));
}

/* A type's layout, as a host sees it: what each word is, by name. A struct's is {name:T,...}, with T as
   C reads it: Int (a Bool's too), Enum{a,b,c} for an enum (its values' names, in order: Enum(n){...} for
   a bitfield), Int(n) for a bitfield (negative n: signed), Float,
   String, Blob, []T, [..]T, [N]T, Blob[N], [..N]T, a struct's own layout, Proc, Union{T|...}. A host
   proc's sig has a struct parameter's in braces: a host reads it by its address with this layout, and
   an image made when it had fewer fields at its end still fits (rio_ffiFits). */
void rio_layoutOf(rio_Vm *vm, int type, int bits, rio_HostText *o) {
  rio_Compiler *c = vm->compiler;
  rio_Type *ty = rio_type(vm, type);
  int i, byte = ty->elem == rio_TypeId_byte && ty->kind != rio_TypeKind_struct && ty->kind != rio_TypeKind_union;
  if (ty->kind == rio_TypeKind_enum) { /* its values' names, so a reorder doesn't fit */
    rio_hostPut(o, bits ? "Enum(" : "Enum{", -1);
    if (bits) {
      rio_hostPutInt(o, bits);
      rio_hostPut(o, "){", 2);
    }
    for (i = 0; i < ty->fieldCount; i++) {
      rio_Field *f = rio_field(vm, ty->firstField + i);
      if (i) rio_hostPut(o, ",", 1);
      rio_hostPut(o, c->names + f->name, f->nameLen);
    }
    rio_hostPut(o, "}", 1);
    return;
  }
  if (bits) {
    rio_hostPut(o, "Int(", 4);
    rio_hostPutInt(o, bits);
    rio_hostPut(o, ")", 1);
    return;
  }
  switch (ty->kind) {
  case rio_TypeKind_float: rio_hostPut(o, "Float", 5); return;
  case rio_TypeKind_proc: rio_hostPut(o, "Proc", 4); return;
  case rio_TypeKind_void: rio_hostPut(o, "nil", 3); return;
  case rio_TypeKind_slice:
    if (type == rio_TypeId_string || type == rio_TypeId_blob) rio_hostPut(o, type == rio_TypeId_string ? "String" : "Blob", -1);
    else rio_hostPut(o, "[]", 2);
    break;
  case rio_TypeKind_builderView: rio_hostPut(o, byte ? "Blob[..]" : "[..]", -1); break;
  case rio_TypeKind_array: case rio_TypeKind_builder:
    rio_hostPut(o, byte ? "Blob[" : "[", -1);
    if (ty->kind == rio_TypeKind_builder) rio_hostPut(o, "..", 2);
    rio_hostPutInt(o, (int32_t)ty->count);
    rio_hostPut(o, "]", 1);
    break;
  case rio_TypeKind_struct: case rio_TypeKind_union:
    rio_hostPut(o, ty->kind == rio_TypeKind_union ? "Union{" : "{", -1);
    for (i = 0; i < ty->fieldCount; i++) {
      rio_Field *f = rio_field(vm, ty->firstField + i);
      if (i) rio_hostPut(o, ty->kind == rio_TypeKind_union ? "|" : ",", 1);
      if (ty->kind == rio_TypeKind_struct) {
        rio_hostPut(o, c->names + f->name, f->nameLen);
        rio_hostPut(o, ":", 1);
      }
      rio_layoutOf(vm, f->type, ty->kind == rio_TypeKind_struct ? f->bits : 0, o);
    }
    rio_hostPut(o, "}", 1);
    return;
  default: rio_hostPut(o, "Int", 3); return; /* (Int, Bool, an enum, a byte) */
  }
  if (!byte && ty->kind != rio_TypeKind_slice) rio_layoutOf(vm, ty->elem, 0, o);
  else if (ty->kind == rio_TypeKind_slice && type != rio_TypeId_string && type != rio_TypeId_blob) rio_layoutOf(vm, ty->elem, 0, o);
}

/* the length of a sig's {...} (or Union{...}) at g */
static int rio_groupLen(const char *g) {
  int depth = 0, n = 0;
  while (g[n] && g[n] != '{') n++;
  do {
    depth += g[n] == '{';
    depth -= g[n] == '}';
    n++;
  } while (g[n] && depth);
  return n;
}

/* is struct type's layout the sig's {...} at g? */
static int rio_layoutIs(rio_Vm *vm, int type, const char *g) {
  rio_HostText o;
  memset(&o, 0, sizeof o);
  o.want = g;
  o.cap = (uint32_t)rio_groupLen(g);
  rio_layoutOf(vm, type, 0, &o);
  return !o.differs && o.n == o.cap;
}

/* name :: host proc(...) (procType, as written), for host proc i: the types must be its sig's (an enum
   or Bool may stand for an Int, and a struct's layout must be the sig's {...}), and calls then take the
   declared ones */
static void rio_declareHostProc(rio_Vm *vm, const char *name, int nameLen, int i, int procType, int exporting) {
  rio_Compiler *c = vm->compiler;
  rio_FfiDef *def = &c->ffi[i];
  rio_Type *ty = rio_type(vm, procType);
  int same = ty->fieldCount == def->paramCount, j, rk = rio_type(vm, ty->elem)->kind;
  if (def->resultLayout) { /* (a struct or union of at most 16 words, as a Rio proc may return, but with no views) */
    same = same && (rk == rio_TypeKind_struct || rk == rio_TypeKind_union) && rio_layoutIs(vm, ty->elem, vm->ffi[i].sig + def->resultLayout - 1);
    if (same && (rio_type(vm, ty->elem)->hasRefs || rio_isBig(rio_type(vm, ty->elem))))
      rio_FAIL(, "a host proc returns what a proc may: no views in it, and at most 16 words");
  } else same = same && rio_hostTypeOk(vm, ty->elem, def->resultType);
  for (j = 0; same && j < def->paramCount; j++) {
    rio_Param *p = &c->params[def->firstParam + j];
    int type = rio_field(vm, ty->firstField + j)->type, ref = rio_field(vm, ty->firstField + j)->offset;
    if (ref != (p->refType != 0)) same = 0; /* (a &T for a &T only) */
    else if (p->type == rio_TypeId_void || p->refType == rio_REF_LAYOUT) /* (a struct's {...} or an enum's Enum{...}) */
      same = rio_layoutIs(vm, type, vm->ffi[i].sig + p->addr);
    else same = rio_hostTypeOk(vm, type, ref ? p->refType : p->type);
  }
  if (!same) { /* say what the host has */
    char text[rio_ERROR_BUF_SIZE];
    int len = rio_appendText(text, 0, "doesn't match the host's ");
    len = rio_appendText(text, len, vm->ffi[i].name);
    len = rio_appendText(text, len, ": proc(");
    for (j = 0; j < def->paramCount; j++) {
      rio_Param *p = &c->params[def->firstParam + j];
      if (j) len = rio_appendText(text, len, ", ");
      if (p->refType) len = rio_appendText(text, len, "&");
      if (p->type == rio_TypeId_void || p->refType == rio_REF_LAYOUT) { /* (a struct: its layout, or as much as fits) */
        const char *g = vm->ffi[i].sig + p->addr;
        int n = rio_groupLen(g), room = (int)sizeof text - len - 24;
        if (n > room) n = room > 0 ? room : 0;
        memcpy(text + len, g, (size_t)n);
        len += n;
      } else len = rio_appendText(text, len, rio_typeName(p->refType ? p->refType : p->type));
    }
    len = rio_appendText(text, len, ")");
    if (def->resultLayout) { /* (its layout, or as much as fits) */
      const char *g = vm->ffi[i].sig + def->resultLayout - 1;
      int n = rio_groupLen(g), room = (int)sizeof text - len - 8;
      if (n > room) n = room > 0 ? room : 0;
      len = rio_appendText(text, len, " -> ");
      memcpy(text + len, g, (size_t)n);
      len += n;
    } else if (def->resultType) {
      len = rio_appendText(text, len, " -> ");
      len = rio_appendText(text, len, rio_typeName(def->resultType));
    }
    text[len] = 0;
    rio_FAIL(, text);
  }
  for (j = 0; j < def->paramCount; j++) { /* (calls: as declared) */
    rio_Param *p = &c->params[def->firstParam + j];
    if (p->refType) p->refType = rio_field(vm, ty->firstField + j)->type; /* (and type stays the address word) */
    else p->type = rio_field(vm, ty->firstField + j)->type;
  }
  def->resultType = (uint16_t)ty->elem;
  def->optional = (uint16_t)ty->count; /* (its last ones, as declared: name?: T) */
  vm->ffi[i].declared = 1;
  c->exporting = exporting;
  rio_addSymbol(vm, name, nameLen, rio_SymKind_ffi, procType, i); /* (its type: as declared, for rio_hostHeader) */
}

/* ------------------------------------------------------------------ host procs as proc values */

static rio_Expr rio_parseExprFrom(rio_Vm *vm, const char *text, int len, int line);

/* host proc e, as a proc value of type `type` (or its own, when type < 0): the value of a proc that calls
   it, kept for it now and made at the end of compiling (rio_makeThunks), one for each host proc. A host
   proc can't call back into the program, so the proc never calls through a value and may be one */
void rio_ffiProcValue(rio_Vm *vm, rio_Expr *e, int type) {
  rio_Compiler *c = vm->compiler;
  rio_FfiDef *def = &c->ffi[e->value];
  int params[16], i, t, proc = -1;
  if (def->paramCount > 16) rio_FAIL(, "too many parameters for a proc value");
  for (i = 0; i < def->paramCount; i++) {
    rio_Param *p = &c->params[def->firstParam + i];
    if (p->refType) rio_FAIL(, "a proc with & parameters can't be a value");
    if (p->type == rio_TypeId_void) rio_FAIL(, "this host proc takes a struct: declare it, in its module's source, to say which");
    params[i] = p->type;
  }
  if (def->resultLayout && def->resultType == rio_TypeId_void) rio_FAIL(, "this host proc returns a struct: declare it, in its module's source, to say which");
  t = rio_procTypeOf(vm, params, def->paramCount, def->resultType, def->optional, 0);
  if (rio_failed(vm)) return;
  if (type >= 0 && t != type) rio_FAIL(, "this proc's parameters or result don't match");
  for (i = 0; i < c->thunkCount; i++)
    if (c->thunks[i].ffi == e->value) proc = c->thunks[i].proc;
  if (proc < 0) { /* (kept: its code comes at the end) */
    if (c->thunkCount >= (int)(sizeof c->thunks / sizeof c->thunks[0])) rio_FAIL(, "too many host procs used as proc values (16)");
    if (vm->procCount >= (int)c->limits.procs) rio_FAIL(, "too many procs");
    proc = vm->procCount++;
    memset(&c->procDefs[proc], 0, sizeof c->procDefs[proc]);
    c->procDefs[proc].defined = 1;
    c->procDefs[proc].usedAsValue = 1;
    c->thunks[c->thunkCount].proc = (uint16_t)proc;
    c->thunks[c->thunkCount++].ffi = (uint16_t)e->value;
  }
  *e = rio_exprOf(rio_ExprKind_const, t, proc + 1);
}

/* each host proc used as a value: its proc, (a0, a1, ...) -> host(a0, a1, ...), the host call as any other */
void rio_makeThunks(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  for (; c->thunksMade < c->thunkCount && !rio_failed(vm); c->thunksMade++) {
    int proc = c->thunks[c->thunksMade].proc, ffi = c->thunks[c->thunksMade].ffi, i, n;
    rio_FfiDef *def = &c->ffi[ffi];
    char call[256];
    rio_Expr e;
    c->exporting = 0;
    if (rio_beginProcAt(vm, proc, "", 0, -1, 0) < 0) return;
    for (i = 0; i < def->paramCount && !rio_failed(vm); i++) {
      char param[16];
      int len = rio_appendText(param, 0, "a");
      len += rio_formatInt(param + len, i);
      rio_procParam(vm, proc, param, len, c->params[def->firstParam + i].type, 0);
    }
    if (def->resultType != rio_TypeId_void && !rio_failed(vm)) rio_setProcResult(vm, proc, def->resultType);
    if (rio_failed(vm)) return;
    rio_beginBody(vm, proc);
    rio_addSymbol(vm, "host", 4, rio_SymKind_ffi, 0, ffi); /* (in its own scope only) */
    n = rio_appendText(call, 0, "host(");
    for (i = 0; i < def->paramCount; i++) {
      if (i) n = rio_appendText(call, n, ", ");
      call[n++] = 'a';
      n += rio_formatInt(call + n, i);
    }
    call[n++] = ')';
    e = rio_parseExprFrom(vm, call, n, c->line);
    if (rio_failed(vm)) return;
    if (def->resultType != rio_TypeId_void) {
      rio_Expr d = rio_exprOf(rio_ExprKind_static, def->resultType, (int32_t)c->procDefs[proc].resultAt);
      rio_store(vm, &d, &e);
    }
    rio_finishProc(vm);
    c->procDefs[proc].usedAsValue = 1;
  }
}

/* ------------------------------------------------------------------ builtins */

/* parses an expression from text, as if it were at the given line of the current file */
static rio_Expr rio_parseExprFrom(rio_Vm *vm, const char *text, int len, int line) {
  rio_Compiler *c = vm->compiler;
  const char *src = c->src, *cursor = c->cursor, *srcEnd = c->srcEnd;
  uint32_t srcAt = c->srcAt, lineAt = c->lineAt;
  int savedLine = c->line, prevLine = c->prevLine, prevCol = c->prevCol, prevWidth = c->prevWidth;
  rio_Text saved = c->text;
  rio_Token tok = c->tok, peek = c->peek;
  int textDone = c->textDone;
  rio_Expr e;
  c->text.src = text; /* (in memory, whatever the file's read as) */
  c->text.len = (uint32_t)len;
  c->text.read = 0;
  rio_readFrom(vm, 0, 0, line);
  rio_lex(vm, &c->peek);
  rio_next(vm);
  e = rio_parseExpr(vm);
  if (!rio_failed(vm) && c->tok.kind != rio_Tok_eof) rio_fail(vm, "internal error: the check didn't read all of its call");
  c->text = saved; /* (its own text read: what it was reading, as it was) */
  c->textDone = textDone;
  c->src = src;
  c->cursor = cursor;
  c->srcEnd = srcEnd;
  c->srcAt = srcAt;
  c->lineAt = lineAt;
  c->line = savedLine;
  c->prevLine = prevLine;
  c->prevCol = prevCol;
  c->prevWidth = prevWidth;
  c->tok = tok;
  c->peek = peek;
  if (rio_failed(vm)) { /* (the rest of the source reads as its end, as after any error) */
    c->tok.kind = c->peek.kind = rio_Tok_eof;
    c->tok.len = c->peek.len = 0;
  }
  return e;
}

/* name :: host proc(...) (procType), for a builtin (receiver: a method's type, or -1): a call with
   arguments of the declared types, in a proc that's never called, must compile and give the declared
   result. The proc's parameters are a0, a1...; a String argument is a literal instead */
static void rio_declareBuiltin(rio_Vm *vm, const char *name, int nameLen, int receiver, int procType, int exporting, int line, int col) {
  rio_Compiler *c = vm->compiler;
  rio_Type *ty = rio_type(vm, procType);
  char call[512], message[rio_ERROR_BUF_SIZE];
  int n = 0, i, proc, got, builtin = -1;
  rio_Expr e;
  if (receiver < 0) { /* a builtin proc: the name must mean one */
    i = rio_lookup(vm, name, nameLen);
    if (i < 0 || c->syms[i].kind != rio_SymKind_builtin) {
      rio_failAt(vm, line, col, nameLen, "the host has no proc by this name, and no builtin has it");
      return;
    }
    builtin = c->syms[i].value;
  }
  if (vm->procCount >= (int)c->limits.procs) rio_FAIL(, "too many procs");
  if ((proc = rio_beginProc(vm, "", 0, receiver)) < 0) return; /* (unnamed: the call inside finds the builtin) */
  for (i = 0; i < ty->fieldCount && !rio_failed(vm); i++) {
    char param[16];
    int len = rio_appendText(param, 0, "a");
    len += rio_formatInt(param + len, i);
    rio_procParam(vm, proc, param, len, rio_field(vm, ty->firstField + i)->type, 0);
  }
  if (ty->elem != rio_TypeId_void && !rio_failed(vm)) rio_setProcResult(vm, proc, ty->elem);
  if (rio_failed(vm)) return;
  rio_beginBody(vm, proc);
  /* the call: self.name(a0, a1), with each String argument a literal */
  if (receiver >= 0) n = rio_appendText(call, n, "self.");
  memcpy(call + n, name, (size_t)(nameLen < 200 ? nameLen : 200));
  n += nameLen < 200 ? nameLen : 200;
  call[n++] = '(';
  for (i = 0; i < ty->fieldCount; i++) {
    if (i) n = rio_appendText(call, n, ", ");
    if (rio_field(vm, ty->firstField + i)->type == rio_TypeId_string) n = rio_appendText(call, n, "\"\"");
    else {
      call[n++] = 'a';
      n += rio_formatInt(call + n, i);
    }
  }
  call[n++] = ')';
  e = rio_parseExprFrom(vm, call, n, line);
  if (rio_failed(vm)) { /* the builtin doesn't take these: say so, at the declaration */
    int len = rio_appendText(message, 0, "doesn't match the builtin: ");
    const char *near;
    len = rio_appendText(message, len, vm->errorText + vm->errorMsgAt);
    message[len] = 0;
    if ((near = strstr(message, " near '"))) message[near - message] = 0; /* (that's in the check's call, not the source) */
    c->failed = 0;
    rio_failAt(vm, line, col, nameLen, message);
    return;
  }
  got = e.kind == rio_ExprKind_void ? rio_TypeId_void : rio_valueType(e.type);
  if (got != rio_valueType(ty->elem)) {
    int len = rio_appendText(message, 0, "doesn't match the builtin: it gives ");
    len = rio_appendText(message, len, rio_typeName(got));
    message[len] = 0;
    rio_failAt(vm, line, col, nameLen, message);
    return;
  }
  rio_finishProc(vm);
  c->exporting = exporting;
  if (receiver < 0) rio_addSymbol(vm, name, nameLen, rio_SymKind_builtin, 0, builtin);
  else rio_addSymbol(vm, name, nameLen, rio_SymKind_method, receiver, proc); /* (the builtin method still wins) */
}

/* ------------------------------------------------------------------ declarations */

/* at the host of `name :: host ...` (receiver: a method's type, or -1); the name's * was seen if
   c->exporting is set. line and col: the name's */
void rio_hostDecl(rio_Vm *vm, const char *name, int nameLen, int receiver, int line, int col) {
  rio_Compiler *c = vm->compiler;
  int exporting = c->exporting, i = -1, procType;
  c->exporting = 0;
  rio_next(vm);
  if (c->curProc >= 0 || c->blockCount) rio_FAIL(, "host declarations go at the top level");
  if (c->tok.kind == rio_Tok_struct) { /* a builtin type (the host can't add types yet) */
    int sym = rio_lookup(vm, name, nameLen);
    if (receiver >= 0) rio_FAIL(, "'proc' expected");
    if (sym < 0 || c->syms[sym].kind != rio_SymKind_type || c->syms[sym].module != 0) {
      rio_failAt(vm, line, col, nameLen, "the host has no type by this name");
      return;
    }
    rio_next(vm);
    c->exporting = exporting;
    rio_addSymbol(vm, name, nameLen, rio_SymKind_type, c->syms[sym].type, 0);
    return;
  }
  if (c->tok.kind != rio_Tok_proc) rio_FAIL(, "'proc' or 'struct' expected after host");
  c->hostProcType = 1;
  procType = rio_parseType(vm); /* a proc type, as for proc values: proc(x: Int) -> Int (and &T parameters) */
  c->hostProcType = 0;
  if (rio_failed(vm)) return;
  if (receiver < 0) { /* this module's host proc, a global one, or a builtin */
    char prefix[256];
    int n = rio_modulePrefix(vm, c->curModule, prefix);
    if (n) i = rio_findFfi(vm, prefix, n, name, nameLen);
    if (i < 0) i = rio_findFfi(vm, 0, 0, name, nameLen);
  }
  if (i >= 0) rio_declareHostProc(vm, name, nameLen, i, procType, exporting);
  else if (vm->hostHeader && receiver < 0 && ((i = rio_lookup(vm, name, nameLen)) < 0 || c->syms[i].kind != rio_SymKind_builtin)) {
    c->exporting = exporting; /* (rio_hostHeader: recorded, for its header; calls can't be made) */
    rio_addSymbol(vm, name, nameLen, rio_SymKind_ffi, procType, -1);
  } else rio_declareBuiltin(vm, name, nameLen, receiver, procType, exporting, line, col);
}

/* does the current token start `(type).name ::`? (a method on a type without a name: host only) */
int rio_atTypeMethod(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  const char *p, *e;
  int depth = 0;
  if (c->tok.kind != '(') return 0;
  p = rio_lookAhead(vm);
  e = c->srcEnd;
  for (; p < e; p++) { /* (types have no strings or comments) */
    if (*p == '(' || *p == '[') depth++;
    else if (*p == ')' || *p == ']') {
      if (--depth == 0 && *p == ')') break;
    } else if (*p == '\n') return 0;
  }
  if (p >= e) return 0;
  for (p++; p < e && (*p == ' ' || *p == '\t'); p++) {}
  if (p >= e || *p != '.') return 0;
  p++;
  if (p >= e || !rio_isAlpha(*p)) return 0;
  while (p < e && (rio_isAlpha(*p) || rio_isDigit(*p))) p++;
  for (; p < e && (*p == ' ' || *p == '\t' || *p == '*'); p++) {}
  return p + 1 < e && p[0] == ':' && p[1] == ':'; /* (no expression goes on with ::) */
}

/* (type).name* :: host proc(...) */
void rio_typeMethodDecl(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  const char *name;
  int receiver, nameLen, line, col;
  if (c->curProc >= 0 || c->blockCount) rio_FAIL(, "host declarations go at the top level");
  rio_next(vm);
  receiver = rio_parseType(vm);
  rio_expect(vm, ')', "')' expected");
  rio_expect(vm, '.', "'.' expected");
  if (c->tok.kind != rio_Tok_id) rio_FAIL(, "method name expected");
  name = rio_keep(vm, c->tok.text, c->tok.len);
  nameLen = c->tok.len;
  line = c->tok.line;
  col = c->tok.col;
  rio_next(vm);
  if (c->tok.kind == '*') {
    c->exporting = 1;
    rio_next(vm);
  }
  rio_expect(vm, rio_Tok_colonColon, "'::' expected");
  if (!rio_atHost(vm)) rio_FAIL(, "a method on a type without a name is a builtin: write host proc(...)");
  rio_hostDecl(vm, name, nameLen, receiver, line, col);
}

#endif
