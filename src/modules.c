/* modules.c - import and include. Another file is read without recursion: where to resume the current
   one goes on a stack (rio_Resume), the new file's tokens take over, and its end pops back. A module is
   compiled once, at its first import; the import statement then runs again, and finds it done. */
#include "compiler.h"
#ifndef rio_NO_COMPILER

/* the file loaded as `key`, or -1 */
static int rio_findFile(rio_Vm *vm, int isPackage, int isInclude, const char *key, int n) {
  rio_Compiler *c = vm->compiler;
  int k;
  for (k = 2; k < c->moduleCount; k++) {
    rio_Module *m = &c->modules[k];
    if (m->isPackage == isPackage && m->isInclude == isInclude && rio_nameIs(vm, m->key, m->keyLen, key, n)) return k;
  }
  return -1;
}

/* Starts reading another file (a module to import, or a part to include), after saving where to resume:
   at cursor (whose line starts at lineStart). key: the path for the loader. Returns 0, or 1 if the
   loader doesn't have it and quietIfMissing (the caller has another way). */
static int rio_openFile(rio_Vm *vm, int isPackage, int isInclude, char *key, int keyLen, const char *cursor, const char *lineStart,
                        int line, int quietIfMissing) {
  rio_Compiler *c = vm->compiler;
  rio_Source s;
  rio_Module *m;
  rio_Resume *resume;
  int found, dirLen, n;
  char text[300];
  if (c->resumeCount >= rio_MAX_IMPORT_DEPTH) rio_FAIL(0, "imports and includes nested too deep");
  if (c->moduleCount >= (int)c->limits.modules) rio_FAIL(0, "too many modules");
  key[keyLen] = 0;
  memset(&s, 0, sizeof s);
  found = vm->loader ? vm->loader(vm->loaderData, key, (isPackage ? rio_LoadKind_package : 0) | (isInclude ? rio_LoadKind_file : 0), &s) : 1;
  if (found == 1 && quietIfMissing) return 1;
  if (found) {
    n = rio_appendText(text, 0, found == 2 ? "found in more than one library path: " : isInclude ? "file not found: " : "module not found: ");
    n = rio_appendText(text, n, key);
    text[n] = 0;
    rio_FAIL(0, text);
  }
  m = &c->modules[c->moduleCount];
  m->key = (uint16_t)rio_addName(vm, key, keyLen);
  m->keyLen = (uint16_t)keyLen;
  m->isPackage = (uint8_t)isPackage;
  m->isInclude = (uint8_t)isInclude;
  m->state = rio_ModuleState_compiling;
  /* its own imports and includes start in its directory: a directory module's own, else the file's */
  if (s.isDir && !isInclude) {
    memcpy(text, key, (size_t)keyLen);
    text[keyLen] = '/';
    dirLen = keyLen + 1;
  } else
    for (dirLen = keyLen; dirLen > 0 && key[dirLen - 1] != '/'; dirLen--) {}
  m->dir = (uint16_t)rio_addName(vm, s.isDir && !isInclude ? text : key, dirLen);
  m->dirLen = (uint16_t)dirLen;
  /* a name for error messages */
  if (s.name) n = rio_appendText(text, 0, s.name);
  else if (isInclude) n = rio_appendText(text, 0, key);
  else n = rio_appendText(text, rio_appendText(text, 0, key), ".rio");
  text[n] = 0;
  m->file = (uint16_t)rio_addName(vm, text, n + 1);
  m->codeStart = (uint16_t)vm->codeLen;
  resume = &c->resume[c->resumeCount++];
  resume->src = c->src;
  resume->srcEnd = c->srcEnd;
  resume->lineStart = lineStart;
  resume->cursor = cursor;
  resume->line = line;
  resume->module = c->curModule;
  resume->file = c->curFile;
  resume->isInclude = isInclude;
  c->curFile = c->moduleCount++;
  if (!isInclude) c->curModule = c->curFile; /* a module gets its own names; an included part shares them */
  c->src = c->cursor = c->lineStart = s.src;
  c->srcEnd = s.src + s.len;
  c->line = 1;
  c->prevLine = 0;
  rio_lex(vm, &c->peek);
  rio_next(vm);
  c->tok.newline = 1; /* the file's first token starts a statement */
  return 0;
}

/* the end of a file: back to whoever imported or included it */
void rio_endFile(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  rio_Resume *resume;
  if (!c->modules[c->curFile].isInclude) rio_checkHostModule(vm, c->curFile); /* (it declares its host procs) */
  if (rio_failed(vm)) return;
  resume = &c->resume[--c->resumeCount];
  c->modules[c->curFile].codeEnd = (uint16_t)vm->codeLen;
  c->modules[c->curFile].state = rio_ModuleState_done;
  c->src = resume->src;
  c->srcEnd = resume->srcEnd;
  c->lineStart = resume->lineStart;
  c->cursor = resume->cursor;
  c->line = resume->line;
  c->curModule = resume->module;
  c->curFile = resume->file;
  rio_lex(vm, &c->peek);
  rio_next(vm);
  if (resume->isInclude) c->tok.newline = 1; /* carry on after the include statement */
  c->lastLabel = vm->codeLen;
}

/* the current token is `as`, a name only special in some places */
static int rio_atAs(rio_Vm *vm) { return rio_tokIs(vm, "as", 2); }

/* `as name` if it's there, into *name and *len */
static void rio_parseAs(rio_Vm *vm, const char **name, int *len) {
  rio_Compiler *c = vm->compiler;
  if (!rio_atAs(vm)) return;
  rio_next(vm);
  if (c->tok.kind != rio_Tok_id) rio_FAIL(, "name expected after 'as'");
  *name = c->tok.text;
  *len = c->tok.len;
  rio_next(vm);
}

/* import .local.path or import package.path, then [as name][*] or .{a, b as c, ...}.
   A package import reaches its root only; deeper names go through what the root publishes. */
void rio_importStatement(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  rio_Module *from = &c->modules[c->curFile];
  const char *start = c->tok.text, *startLineStart = c->tok.text - (c->tok.col - 1), *segments[8];
  char key[256];
  int startLine = c->tok.line, local, isPackage, keyLen = 0, n = 0, segmentLens[8], m, k;
  rio_next(vm);
  if (c->curProc >= 0 || c->blockCount) rio_FAIL(, "imports must be at the top level");
  local = c->tok.kind == '.';
  if (local) rio_next(vm);
  isPackage = local ? from->isPackage : 1;
  if (local) { /* relative to this file's directory */
    memcpy(key, c->names + from->dir, from->dirLen);
    keyLen = from->dirLen;
  }
  for (;;) { /* the path: a.b.c */
    if (c->tok.kind != rio_Tok_id) rio_FAIL(, "module name expected");
    if (n >= 8) rio_FAIL(, "module path too long");
    segments[n] = c->tok.text;
    segmentLens[n] = c->tok.len;
    if (local || !n) { /* a package import names just its root */
      if (keyLen + c->tok.len + 2 > (int)sizeof key) rio_FAIL(, "module path too long");
      if (local && n) key[keyLen++] = '/';
      memcpy(key + keyLen, c->tok.text, (size_t)c->tok.len);
      keyLen += c->tok.len;
    }
    n++;
    rio_next(vm);
    if (c->tok.kind == '.' && c->peek.kind == rio_Tok_id) {
      rio_next(vm);
      continue;
    }
    break;
  }
  if ((m = rio_findFile(vm, isPackage, 0, key, keyLen)) < 0) { /* compile it first, then come back here */
    /* (a package the loader doesn't have may be a host module, made from the host procs registered
       under its name) */
    int hostProcs = !local && rio_hasHostProcs(vm, key, keyLen);
    if (!rio_openFile(vm, isPackage, 0, key, keyLen, start, startLineStart, startLine, hostProcs) || rio_failed(vm)) return;
    if ((m = rio_hostModule(vm, key, keyLen)) < 0) return;
  }
  if (c->modules[m].state != rio_ModuleState_done) rio_FAIL(, "import cycle");
  for (k = 1; !local && k < n; k++) { /* tween.easing: only if tween publishes easing */
    int i = rio_lookupExport(vm, m, segments[k], segmentLens[k]);
    if (i < 0 || c->syms[i].kind != rio_SymKind_module) {
      char text[200];
      int len = rio_appendText(text, 0, "that package doesn't publish "), segmentLen = segmentLens[k] < 64 ? segmentLens[k] : 64;
      memcpy(text + len, segments[k], (size_t)segmentLen);
      text[len + segmentLen] = 0;
      rio_FAIL(, text);
    }
    m = c->syms[i].value;
  }
  if (c->tok.kind == '.' && c->peek.kind == '{') { /* .{a, b as c} */
    rio_next(vm);
    rio_next(vm);
    while (c->tok.kind != '}' && !rio_failed(vm)) {
      const char *as;
      int asLen, i;
      rio_Symbol y;
      if (c->tok.kind != rio_Tok_id) rio_FAIL(, "name expected");
      if ((i = rio_lookupExport(vm, m, c->tok.text, c->tok.len)) < 0) rio_FAIL_AT_TOKEN(, "not exported by that module");
      as = c->tok.text;
      asLen = c->tok.len;
      rio_next(vm);
      if (rio_atAs(vm)) {
        rio_parseAs(vm, &as, &asLen);
        if (rio_failed(vm)) return;
      }
      if (c->tok.kind == '*') rio_FAIL_AT_TOKEN(, "to export an imported name, write Name* :: mod.Name");
      y = c->syms[i];
      rio_addSymbol(vm, as, asLen, y.kind, y.type, y.value);
      if (c->tok.kind != ',') break;
      rio_next(vm);
    }
    rio_expect(vm, '}', "'}' expected");
  } else { /* the module itself, under its last name or another */
    const char *name = segments[n - 1];
    int nameLen = segmentLens[n - 1];
    if (rio_atAs(vm)) {
      rio_parseAs(vm, &name, &nameLen);
      if (rio_failed(vm)) return;
    }
    rio_addSymbol(vm, name, nameLen, rio_SymKind_module, 0, m);
    if (c->tok.kind == '*') { /* publish your own submodule */
      if (!local) rio_FAIL_AT_TOKEN(, "only your own submodules can be published (import .name*)");
      c->syms[c->symCount - 1].exported = 1;
      rio_next(vm);
    }
  }
}

/* include "parts/a.rio": that file becomes part of this module (same names, private ones too). Always
   relative to this file and downward; each file once. */
void rio_includeStatement(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  rio_Module *from = &c->modules[c->curFile];
  rio_Token *t = &c->tok;
  char key[256];
  int keyLen, i;
  const char *resumeAt;
  rio_next(vm);
  if (c->curProc >= 0 || c->blockCount) rio_FAIL(, "includes must be at the top level");
  if (t->kind != rio_Tok_string) rio_FAIL(, "include needs a file path in quotes");
  if (t->len < 5 || memcmp(t->text + t->len - 4, ".rio", 4)) rio_FAIL_AT_TOKEN(, "include paths name a .rio file");
  for (i = 0; i < t->len; i++) { /* plain names separated by '/': no "", ".", "..", absolute or drive paths */
    char ch = t->text[i];
    int segmentStart = !i || t->text[i - 1] == '/', segmentEnd = i + 1 == t->len || t->text[i + 1] == '/';
    if (ch == '\\' || ch == ':' || (ch == '/' && segmentStart) || (ch == '.' && segmentStart && (segmentEnd || t->text[i + 1] == '.')))
      rio_FAIL_AT_TOKEN(, "include paths stay inside this file's directory");
  }
  if (from->dirLen + t->len + 1 > (int)sizeof key) rio_FAIL_AT_TOKEN(, "include path too long");
  memcpy(key, c->names + from->dir, from->dirLen);
  memcpy(key + from->dirLen, t->text, (size_t)t->len);
  keyLen = from->dirLen + t->len;
  if (rio_findFile(vm, from->isPackage, 1, key, keyLen) >= 0) rio_FAIL_AT_TOKEN(, "file already included");
  if (!c->peek.newline && c->peek.kind != rio_Tok_eof && c->peek.kind != ';') {
    rio_next(vm);
    rio_FAIL(, "expected end of statement");
  }
  resumeAt = c->peek.kind == rio_Tok_string ? c->peek.text - 1 : c->peek.text; /* the token after the path */
  rio_openFile(vm, from->isPackage, 1, key, keyLen, resumeAt, resumeAt - (c->peek.col - 1), c->peek.line, 0);
}

#endif
