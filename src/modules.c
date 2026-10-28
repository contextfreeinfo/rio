/* modules.c - import and include. Another file is read without recursion: where to resume the current
   one goes on a stack (rio_Resume), the new file's tokens take over, and its end pops back. A module is
   compiled once, at its first import; the import statement then runs again, and finds it done. The
   rules: docs/spec/modules.md. */
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
/* a module's source: a host module's, registered (rio_addSource), else the loader's; 0, 1 (none), 2 (more
   than one) */
int rio_load(rio_Vm *vm, const char *key, int kind, rio_Source *s) {
  int i;
  if (kind & rio_LoadKind_package)
    for (i = 0; i < vm->sourceCount; i++)
      if (!strcmp(vm->sources[i].name, key)) {
        s->src = vm->sources[i].src;
        s->len = vm->sources[i].len;
        s->isDir = 0;
        s->name = 0;
        vm->sourcesUsed |= 1u << i;
        return 0;
      }
  return vm->loader ? vm->loader(vm->loaderData, key, kind, s) : 1;
}

static int rio_openFile(rio_Vm *vm, int isPackage, int isInclude, char *key, int keyLen, uint32_t at, uint32_t lineAt,
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
  found = rio_load(vm, key, (isPackage ? rio_LoadKind_package : 0) | (isInclude ? rio_LoadKind_file : 0), &s);
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
  resume->text = c->text;
  resume->at = at;
  resume->lineAt = lineAt;
  resume->line = line;
  resume->module = c->curModule;
  resume->file = c->curFile;
  resume->isInclude = isInclude;
  resume->autoImport = 0;
  c->curFile = c->moduleCount++;
  if (!isInclude) c->curModule = c->curFile; /* a module gets its own names; an included part shares them */
  c->text.src = s.src;
  c->text.len = s.len;
  c->text.read = s.src ? 0 : s.read;
  c->text.readData = s.readData;
  rio_readFrom(vm, 0, 0, 1);
  c->prevLine = 0;
  rio_lex(vm, &c->peek);
  rio_next(vm);
  c->tok.newline = 1; /* the file's first token starts a statement */
  return 0;
}

/* the auto-imports the loader has sources for, each opened at the start of the file before it (the last
   opened is compiled first, then the one before, then the main source): bound everywhere as they end */
void rio_openAutoImports(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  char key[256];
  int k;
  for (k = 0; k < c->autoSourceCount && !rio_failed(vm); k++) {
    const char *name = vm->autoImports[c->autoSources[k]];
    int n = (int)strlen(name);
    if (n >= (int)sizeof key) rio_FAIL(, "auto-import name too long");
    memcpy(key, name, (size_t)n + 1);
    if (rio_openFile(vm, 1, 0, key, n, 0, 0, 1, 0)) return;
    c->resume[c->resumeCount - 1].autoImport = 1 + c->autoSources[k];
  }
}

/* the end of a file: back to whoever imported or included it */
void rio_endFile(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  rio_Resume *resume;
  int ended = c->curFile;
  if (!c->modules[c->curFile].isInclude) rio_checkHostModule(vm, c->curFile); /* (it declares its host procs) */
  if (rio_failed(vm)) return;
  resume = &c->resume[--c->resumeCount];
  c->modules[c->curFile].codeEnd = (uint16_t)vm->codeLen;
  c->modules[c->curFile].state = rio_ModuleState_done;
  c->text = resume->text;
  rio_readFrom(vm, resume->at, resume->lineAt, resume->line);
  c->curModule = resume->module;
  c->curFile = resume->file;
  rio_lex(vm, &c->peek);
  rio_next(vm);
  if (resume->isInclude || resume->autoImport) c->tok.newline = 1; /* carry on after the include statement, or start */
  if (resume->autoImport) { /* an auto-import: visible everywhere, as the host's modules are */
    const char *name = vm->autoImports[resume->autoImport - 1];
    rio_addSymbol(vm, name, (int)strlen(name), rio_SymKind_module, 0, ended);
    if (rio_failed(vm)) return;
    c->syms[c->symCount - 1].module = 0;
  }
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
  *name = rio_keep(vm, c->tok.text, c->tok.len);
  *len = c->tok.len;
  rio_next(vm);
}

/* import .local.path or import package.path, then [as name][*] or .{a, b as c, ...}.
   A package import reaches its root only; deeper names go through what the root publishes. */
void rio_importStatement(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  rio_Module *from = &c->modules[c->curFile];
  uint32_t start = c->tok.at, startLineAt = c->tok.at - (uint32_t)(c->tok.col - 1);
  const char *segments[8];
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
    segments[n] = rio_keep(vm, c->tok.text, c->tok.len);
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
  if (!local && keyLen == 4 && !memcmp(key, "core", 4)) m = 0; /* (the builtins: always there) */
  else if ((m = rio_findFile(vm, isPackage, 0, key, keyLen)) < 0) { /* compile it first, then come back here */
    /* (a package the loader doesn't have may be a host module, made from the host procs registered
       under its name) */
    int hostProcs = !local && rio_hasHostProcs(vm, key, keyLen);
    if (!rio_openFile(vm, isPackage, 0, key, keyLen, start, startLineAt, startLine, hostProcs) || rio_failed(vm)) return;
    if ((m = rio_hostModule(vm, key, keyLen)) < 0) return;
  }
  if (m && c->modules[m].state != rio_ModuleState_done) rio_FAIL(, "import cycle"); /* (core, module 0: always done) */
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
      as = rio_keep(vm, c->tok.text, c->tok.len);
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
  rio_openFile(vm, from->isPackage, 1, key, keyLen, c->peek.at, c->peek.at - (uint32_t)(c->peek.col - 1), c->peek.line, 0); /* (back at the token after the path) */
}

#endif
