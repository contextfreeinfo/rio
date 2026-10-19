/* host.c - what the host provides, declared in Rio with @host, and checked against it (the rules:
   docs/spec/host.md):

     name* :: @host proc(a: Int, b: Float) -> Int   a host proc (rio_addFfi), or a builtin proc
     Type.name* :: @host proc(x: Int) -> Bool       a builtin method, on a named type
     ([..]Int).name* :: @host proc() -> Int         ... on any type
     Name* :: @host struct                          a builtin type

   A host proc's declared types must be exactly its sig's (an enum may stand for an Int). In a module, `name` is the host proc registered
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

/* a declared type for a host proc's sig type: the same, or an enum where the sig has an Int (i), which
   it is underneath */
static int rio_hostTypeOk(rio_Vm *vm, int declared, int sig) {
  return declared == sig || (sig == rio_TypeId_int && rio_type(vm, declared)->kind == rio_TypeKind_enum);
}

/* name :: @host proc(...) (procType, as written), for host proc i: the types must be its sig's (an enum
   may stand for an Int), and calls then take the declared ones */
static void rio_declareHostProc(rio_Vm *vm, const char *name, int nameLen, int i, int procType, int exporting) {
  rio_Compiler *c = vm->compiler;
  rio_FfiDef *def = &c->ffi[i];
  rio_Type *ty = rio_type(vm, procType);
  int same = ty->fieldCount == def->paramCount && rio_hostTypeOk(vm, ty->elem, def->resultType), j;
  for (j = 0; same && j < def->paramCount; j++) same = rio_hostTypeOk(vm, rio_field(vm, ty->firstField + j)->type, c->params[def->firstParam + j].type);
  if (!same) { /* say what the host has */
    char text[rio_ERROR_BUF_SIZE];
    int len = rio_appendText(text, 0, "doesn't match the host's ");
    len = rio_appendText(text, len, vm->ffi[i].name);
    len = rio_appendText(text, len, ": proc(");
    for (j = 0; j < def->paramCount; j++) {
      if (j) len = rio_appendText(text, len, ", ");
      len = rio_appendText(text, len, rio_typeName(c->params[def->firstParam + j].type));
    }
    len = rio_appendText(text, len, ")");
    if (def->resultType) {
      len = rio_appendText(text, len, " -> ");
      len = rio_appendText(text, len, rio_typeName(def->resultType));
    }
    text[len] = 0;
    rio_FAIL(, text);
  }
  for (j = 0; j < def->paramCount; j++) c->params[def->firstParam + j].type = rio_field(vm, ty->firstField + j)->type; /* (calls: as declared) */
  def->resultType = (uint16_t)ty->elem;
  vm->ffi[i].declared = 1;
  c->exporting = exporting;
  rio_addSymbol(vm, name, nameLen, rio_SymKind_ffi, 0, i);
}

/* ------------------------------------------------------------------ builtins */

/* parses an expression from text, as if it were at the given line of the current file */
static rio_Expr rio_parseExprFrom(rio_Vm *vm, const char *text, int len, int line) {
  rio_Compiler *c = vm->compiler;
  const char *src = c->src, *cursor = c->cursor, *srcEnd = c->srcEnd, *lineStart = c->lineStart;
  int savedLine = c->line, prevLine = c->prevLine, prevCol = c->prevCol, prevWidth = c->prevWidth;
  rio_Token tok = c->tok, peek = c->peek;
  rio_Expr e;
  c->src = c->cursor = c->lineStart = text;
  c->srcEnd = text + len;
  c->line = line;
  rio_lex(vm, &c->peek);
  rio_next(vm);
  e = rio_parseExpr(vm);
  if (!rio_failed(vm) && c->tok.kind != rio_Tok_eof) rio_fail(vm, "internal error: the check didn't read all of its call");
  c->src = src;
  c->cursor = cursor;
  c->srcEnd = srcEnd;
  c->lineStart = lineStart;
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

/* name :: @host proc(...) (procType), for a builtin (receiver: a method's type, or -1): a call with
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

/* at the @ of `name :: @host ...` (receiver: a method's type, or -1); the name's * was seen if
   c->exporting is set. line and col: the name's */
void rio_hostDecl(rio_Vm *vm, const char *name, int nameLen, int receiver, int line, int col) {
  rio_Compiler *c = vm->compiler;
  int exporting = c->exporting, i = -1, procType;
  c->exporting = 0;
  rio_next(vm);
  if (!rio_tokIs(vm, "host", 4)) rio_FAIL_AT_TOKEN(, "unknown annotation (@host declares what the host provides)");
  rio_next(vm);
  if (c->curProc >= 0 || c->blockCount) rio_FAIL(, "@host declarations go at the top level");
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
  if (c->tok.kind != rio_Tok_proc) rio_FAIL(, "'proc' or 'struct' expected after @host");
  procType = rio_parseType(vm); /* a proc type, as for proc values: proc(x: Int) -> Int */
  if (rio_failed(vm)) return;
  if (receiver < 0) { /* this module's host proc, a global one, or a builtin */
    char prefix[256];
    int n = rio_modulePrefix(vm, c->curModule, prefix);
    if (n) i = rio_findFfi(vm, prefix, n, name, nameLen);
    if (i < 0) i = rio_findFfi(vm, 0, 0, name, nameLen);
  }
  if (i >= 0) rio_declareHostProc(vm, name, nameLen, i, procType, exporting);
  else rio_declareBuiltin(vm, name, nameLen, receiver, procType, exporting, line, col);
}

/* does the current token start `(type).name ::`? (a method on a type without a name: @host only) */
int rio_atTypeMethod(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  const char *p = c->tok.text, *e = c->srcEnd;
  int depth = 0;
  if (c->tok.kind != '(') return 0;
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

/* (type).name* :: @host proc(...) */
void rio_typeMethodDecl(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  const char *name;
  int receiver, nameLen, line, col;
  if (c->curProc >= 0 || c->blockCount) rio_FAIL(, "@host declarations go at the top level");
  rio_next(vm);
  receiver = rio_parseType(vm);
  rio_expect(vm, ')', "')' expected");
  rio_expect(vm, '.', "'.' expected");
  if (c->tok.kind != rio_Tok_id) rio_FAIL(, "method name expected");
  name = c->tok.text;
  nameLen = c->tok.len;
  line = c->tok.line;
  col = c->tok.col;
  rio_next(vm);
  if (c->tok.kind == '*') {
    c->exporting = 1;
    rio_next(vm);
  }
  rio_expect(vm, rio_Tok_colonColon, "'::' expected");
  if (c->tok.kind != '@') rio_FAIL(, "a method on a type without a name is a builtin: write @host proc(...)");
  rio_hostDecl(vm, name, nameLen, receiver, line, col);
}

#endif
