/* complete.c - rio_complete: the names that fit where the cursor is, for a REPL's Tab and an editor's or
   language server's completions. What's compiled (the session's tables) gives names their kinds and
   types; the text before the cursor gives the rest: whether it's after x., Type. or module., where a
   statement or an expression or a type goes, and names it declares that aren't compiled yet (an
   unfinished input's, a proc's parameters and locals). */
#include "compiler.h"
#ifndef rio_NO_COMPILER

#define rio_CPL_DETAIL 96 /* a detail's room: a type, a proc's signature */
#define rio_CPL_SEEN 64   /* names offered from the text, kept so each comes once */

typedef struct {
  rio_Vm *vm;
  rio_Compiler *c; /* (0: nothing compiled, just the text) */
  const char *text, *word;
  int wordLen;
  rio_CompleteFn fn;
  rio_LookupFn define; /* (rio_lookupAt's: the word exactly, and where it's declared) */
  void *userData;
  int file, line, textAt; /* where the name being offered is declared (rio_cplAt) */
  char detail[rio_CPL_DETAIL];
  int detailLen;
  int seenAt[rio_CPL_SEEN], seenLen[rio_CPL_SEEN], seenCount;
} rio_Cpl;

static int rio_cplWordChar(int ch) { return rio_isAlpha(ch) || rio_isDigit(ch); }

/* where the next name offered is declared: its file (a rio_Module's index; 1 the main one, 0 none), line
   (0: not known), or where in the text (-1: not there) */
static void rio_cplAt(rio_Cpl *k, int file, int line, int textAt) {
  k->file = file;
  k->line = line;
  k->textAt = textAt;
}

static void rio_cplOffer(rio_Cpl *k, const char *name, int n, int kind) {
  const char *file = 0;
  if (n < k->wordLen || memcmp(name, k->word, (size_t)k->wordLen)) return;
  if (!k->define) {
    k->fn(k->userData, name, n, kind, k->detail, k->detailLen);
    return;
  }
  if (n != k->wordLen) return; /* (rio_lookupAt: the word, exactly) */
  if (k->c && k->file >= 2 && k->file < k->c->moduleCount) file = k->c->names + k->c->modules[k->file].file;
  k->define(k->userData, name, n, kind, k->detail, k->detailLen, file, k->file == 1 || file ? k->line : 0, k->textAt);
}

/* ------------------------------------------------------------------ details: types as written */

static void rio_cplPut(rio_Cpl *k, const char *s, int n) {
  if (n < 0) n = (int)strlen(s);
  if (k->detailLen + n > rio_CPL_DETAIL - 4) n = rio_CPL_DETAIL - 4 - k->detailLen; /* (cut short: ...) */
  if (n <= 0) {
    if (k->detailLen <= rio_CPL_DETAIL - 4 && k->detailLen && k->detail[k->detailLen - 1] != '.') {
      memcpy(k->detail + k->detailLen, "...", 3);
      k->detailLen += 3;
    }
    return;
  }
  memcpy(k->detail + k->detailLen, s, (size_t)n);
  k->detailLen += n;
}

/* a type's name (a symbol for it), or 0 */
static int rio_cplTypeSym(rio_Cpl *k, int type) {
  int i;
  for (i = k->c->symCount - 1; i >= 0; i--)
    if (k->c->syms[i].kind == rio_SymKind_type && k->c->syms[i].type == type && k->c->syms[i].nameLen) return i + 1;
  return 0;
}

/* a type, as written, but a proc type in it just "proc(...)" */
static void rio_cplTypeFlat(rio_Cpl *k, int type) {
  int guard, sym;
  char n[16];
  for (guard = 0; guard < 16; guard++) { /* its [], [N], [..N], [..] and ?, outermost first */
    rio_Type *ty = rio_type(k->vm, type);
    if (type == rio_TypeId_string || type == rio_TypeId_blob) break;
    if (ty->elem == rio_TypeId_byte && (ty->kind == rio_TypeKind_array || ty->kind == rio_TypeKind_builder || ty->kind == rio_TypeKind_builderView)) {
      rio_cplPut(k, ty->kind == rio_TypeKind_array ? "Blob[" : "Blob[..", -1); /* Blob[N], Blob[..N], Blob[..] */
      if (ty->kind != rio_TypeKind_builderView) rio_cplPut(k, n, rio_formatInt(n, (int32_t)ty->count));
      rio_cplPut(k, "]", 1);
      return;
    }
    if (ty->kind == rio_TypeKind_slice) rio_cplPut(k, "[]", 2);
    else if (ty->kind == rio_TypeKind_builderView) rio_cplPut(k, "[..]", 4);
    else if (ty->kind == rio_TypeKind_array || ty->kind == rio_TypeKind_builder) {
      rio_cplPut(k, ty->kind == rio_TypeKind_builder ? "[.." : "[", -1);
      if (ty->kind == rio_TypeKind_array && ty->firstField && (sym = rio_cplTypeSym(k, ty->firstField)) > 0) /* [Enum]T */
        rio_cplPut(k, k->c->names + k->c->syms[sym - 1].name, k->c->syms[sym - 1].nameLen);
      else rio_cplPut(k, n, rio_formatInt(n, (int32_t)ty->count));
      rio_cplPut(k, "]", 1);
    } else if (ty->kind == rio_TypeKind_union && ty->elem && !rio_cplTypeSym(k, type)) rio_cplPut(k, "?", 1);
    else break;
    type = ty->elem;
  }
  switch (type) {
  case rio_TypeId_void: rio_cplPut(k, "nothing", -1); return;
  case rio_TypeId_int: rio_cplPut(k, "Int", -1); return;
  case rio_TypeId_float: rio_cplPut(k, "Float", -1); return;
  case rio_TypeId_byte: rio_cplPut(k, "Byte", -1); return;
  case rio_TypeId_string: rio_cplPut(k, "String", -1); return;
  case rio_TypeId_blob: rio_cplPut(k, "Blob", -1); return;
  case rio_TypeId_bool: rio_cplPut(k, "Bool", -1); return;
  }
  if ((sym = rio_cplTypeSym(k, type)) > 0) rio_cplPut(k, k->c->names + k->c->syms[sym - 1].name, k->c->syms[sym - 1].nameLen);
  else if (rio_type(k->vm, type)->kind == rio_TypeKind_proc) rio_cplPut(k, "proc(...)", -1);
  else rio_cplPut(k, "?", 1);
}

/* a proc's signature: proc(types) -> result, a parameter at a time from either a proc type or a ProcDef */
static void rio_cplSigEnd(rio_Cpl *k, int result) {
  rio_cplPut(k, ")", 1);
  if (result != rio_TypeId_void) {
    rio_cplPut(k, " -> ", 4);
    rio_cplTypeFlat(k, result);
  }
}
static void rio_cplParam(rio_Cpl *k, int j, int type, int ref, int optional) {
  if (j) rio_cplPut(k, ", ", 2);
  if (optional) rio_cplPut(k, "?", 1); /* (as Rio's name?: T has it, with no name) */
  if (ref) rio_cplPut(k, "&", 1);
  rio_cplTypeFlat(k, type);
}

/* a type as written, a proc type with its parameters */
static void rio_cplType(rio_Cpl *k, int type) {
  rio_Type *ty = rio_type(k->vm, type);
  int j;
  k->detailLen = 0;
  if (ty->kind != rio_TypeKind_proc || rio_cplTypeSym(k, type)) {
    rio_cplTypeFlat(k, type);
    return;
  }
  rio_cplPut(k, "proc(", 5);
  for (j = 0; j < ty->fieldCount; j++) {
    rio_Field *f = rio_field(k->vm, ty->firstField + j);
    rio_cplParam(k, j, f->type, f->offset == 1, j >= ty->fieldCount - (int)ty->count);
  }
  rio_cplSigEnd(k, ty->elem);
}

static void rio_cplProc(rio_Cpl *k, int proc, int skipSelf) {
  rio_ProcDef *p = &k->c->procDefs[proc];
  int j, from = skipSelf ? 1 : 0;
  k->detailLen = 0;
  rio_cplPut(k, "proc(", 5);
  for (j = from; j < p->paramCount; j++) {
    rio_Param *q = &k->c->params[p->firstParam + j];
    rio_cplParam(k, j - from, q->refType ? q->refType : q->type, q->refType != 0, j >= p->paramCount - p->optional);
  }
  rio_cplSigEnd(k, p->resultType);
}

/* a host proc's: as declared (its type), else as the host has it */
static void rio_cplFfi(rio_Cpl *k, const rio_Symbol *y) {
  rio_FfiDef *def;
  int j;
  if (y->type || y->value < 0) {
    rio_cplType(k, y->type);
    return;
  }
  def = &k->c->ffi[y->value];
  k->detailLen = 0;
  rio_cplPut(k, "proc(", 5);
  for (j = 0; j < def->paramCount; j++) {
    rio_Param *q = &k->c->params[def->firstParam + j];
    rio_cplParam(k, j, q->refType && q->refType != rio_REF_LAYOUT ? q->refType : q->type, q->refType != 0, j >= def->paramCount - def->optional);
  }
  rio_cplSigEnd(k, def->resultType);
}

/* ------------------------------------------------------------------ symbols */

/* a symbol's kind as an item's, and its detail */
static int rio_cplSymbol(rio_Cpl *k, const rio_Symbol *y) {
  k->detailLen = 0;
  switch (y->kind) {
  case rio_SymKind_const: rio_cplType(k, y->type); return rio_Item_const;
  case rio_SymKind_type:
    rio_cplPut(k, rio_type(k->vm, y->type)->kind == rio_TypeKind_enum ? "enum" : rio_type(k->vm, y->type)->kind == rio_TypeKind_union ? "union"
                  : rio_type(k->vm, y->type)->kind == rio_TypeKind_struct ? "struct" : "type", -1);
    return rio_Item_type;
  case rio_SymKind_proc: rio_cplProc(k, y->value, 0); return rio_Item_proc;
  case rio_SymKind_method: rio_cplProc(k, y->value, !k->c->procDefs[y->value].isStatic); return rio_Item_method;
  case rio_SymKind_ffi: rio_cplFfi(k, y); return rio_Item_proc;
  case rio_SymKind_builtin: rio_cplPut(k, "built in", -1); return rio_Item_proc;
  case rio_SymKind_module: rio_cplPut(k, "module", -1); return rio_Item_module;
  default: rio_cplType(k, y->type); return rio_Item_var;
  }
}

/* which symbols an offer takes: in scope (a module's own and the built-in ones), a module's exported
   ones, or a type's methods (static or not) */
enum { rio_CplSyms_scope, rio_CplSyms_exports, rio_CplSyms_methods, rio_CplSyms_statics, rio_CplSyms_types };

static int rio_cplTakes(rio_Cpl *k, const rio_Symbol *y, int which, int arg) {
  rio_Compiler *c = k->c;
  int dot;
  if (!y->nameLen) return 0; /* (hidden for now) */
  for (dot = 0; dot < y->nameLen && c->names[y->name + dot] != '.'; dot++) {}
  if (dot < y->nameLen) return 0; /* (a static host proc's Type.name: reached by Type.) */
  switch (which) {
  case rio_CplSyms_scope: return y->kind != rio_SymKind_method && (y->module == c->curModule || !y->module);
  case rio_CplSyms_types: return (y->kind == rio_SymKind_type || y->kind == rio_SymKind_module) && (y->module == c->curModule || !y->module);
  case rio_CplSyms_exports: return y->kind != rio_SymKind_method && (arg ? y->module == arg && y->exported : y < c->syms + c->coreSymCount);
  default:
    return y->kind == rio_SymKind_method && y->type == arg && (y->module == c->curModule || y->exported) &&
           (int)c->procDefs[y->value].isStatic == (which == rio_CplSyms_statics);
  }
}

/* each symbol an offer takes, the newest of each name */
static void rio_cplSymbols(rio_Cpl *k, int which, int arg) {
  rio_Compiler *c = k->c;
  int i, j;
  if (!c) return;
  for (i = c->symCount - 1; i >= 0; i--) {
    rio_Symbol *y = &c->syms[i];
    if (y->nameLen < k->wordLen || memcmp(c->names + y->name, k->word, (size_t)k->wordLen) || !rio_cplTakes(k, y, which, arg)) continue;
    for (j = i + 1; j < c->symCount; j++) /* (a newer one of the name hides it) */
      if (rio_nameIs(k->vm, c->syms[j].name, c->syms[j].nameLen, c->names + y->name, y->nameLen) && rio_cplTakes(k, &c->syms[j], which, arg)) break;
    if (j < c->symCount) continue;
    rio_cplAt(k, y->file, y->line, -1);
    rio_cplOffer(k, c->names + y->name, y->nameLen, rio_cplSymbol(k, y));
  }
}

/* ------------------------------------------------------------------ after a dot */

/* what the text before a dot is: a value of a type, a type, a module, a proc to call, or a slice of
   elements of a type (one the tables don't have yet) */
enum { rio_Is_nothing, rio_Is_value, rio_Is_type, rio_Is_module, rio_Is_call, rio_Is_slice };
typedef struct {
  int is, type;
} rio_CplIs;

static rio_CplIs rio_cplIs(int is, int type) {
  rio_CplIs r;
  r.is = is;
  r.type = type;
  return r;
}

/* the slice type of elem, if the tables have it (else 0) */
static int rio_cplSliceOf(rio_Cpl *k, int elem) {
  int i;
  if (elem == rio_TypeId_byte) return rio_TypeId_blob;
  for (i = rio_TypeId_builtinCount; i < k->c->typeCount; i++)
    if (rio_type(k->vm, i)->kind == rio_TypeKind_slice && rio_type(k->vm, i)->elem == elem) return i;
  return 0;
}

static rio_CplIs rio_cplOfSymbol(rio_Cpl *k, int i) {
  rio_Symbol *y = &k->c->syms[i];
  switch (y->kind) {
  case rio_SymKind_type: return rio_cplIs(rio_Is_type, y->type);
  case rio_SymKind_module: return rio_cplIs(rio_Is_module, y->value);
  case rio_SymKind_proc: case rio_SymKind_method: return rio_cplIs(rio_Is_call, k->c->procDefs[y->value].resultType);
  case rio_SymKind_ffi:
    return rio_cplIs(rio_Is_call, y->type ? rio_type(k->vm, y->type)->elem : y->value >= 0 ? k->c->ffi[y->value].resultType : 0);
  case rio_SymKind_builtin: return rio_cplIs(rio_Is_nothing, 0);
  default: return rio_cplIs(rio_Is_value, y->type);
  }
}

/* is [p, e) a declaration of name in the text: name := ..., name: ..., name :: ...; its type if the text
   says (name: T, name: &T, name := T{...}, name := T.make(...)) */
static int rio_cplSkipSpace(const char *p, const char *e) {
  const char *q = p;
  while (q < e && (*q == ' ' || *q == '\t')) q++;
  return (int)(q - p);
}

/* a type the tables have: of kind (slice, array, builder, builderView), with elements of elem, and count
   of them (arrays, builders); else one with the same members (another count; a builder for a view), or 0 */
static int rio_cplFind(rio_Cpl *k, int kind, int elem, uint32_t count) {
  int i, near = 0;
  if (kind == rio_TypeKind_slice && elem == rio_TypeId_byte) return rio_TypeId_blob;
  for (i = rio_TypeId_builtinCount; i < k->c->typeCount; i++) {
    rio_Type *t = rio_type(k->vm, i);
    int same = t->elem == elem && !(t->kind == rio_TypeKind_array && t->firstField);
    if (same && t->kind == kind && (kind == rio_TypeKind_slice || kind == rio_TypeKind_builderView || t->count == count)) return i;
    if (same && !near && (t->kind == kind || (kind == rio_TypeKind_builderView && t->kind == rio_TypeKind_builder))) near = i;
  }
  return near;
}

/* the brackets at p, [], [..], [..8] or [4]: their kind and count, and where they end */
static const char *rio_cplBrackets(const char *p, const char *end, int *kind, uint32_t *count) {
  int range = 0;
  *count = 0;
  p++;
  if (p + 1 < end && p[0] == '.' && p[1] == '.') {
    range = 1;
    p += 2;
  }
  while (p < end && rio_isDigit(*p)) *count = *count * 10 + (uint32_t)(*p++ - '0');
  *kind = range ? (*count ? rio_TypeKind_builder : rio_TypeKind_builderView) : *count ? rio_TypeKind_array : rio_TypeKind_slice;
  return p < end && *p == ']' ? p + 1 : end;
}

/* where the text declares name last before end, as a type it names (else 0; minus T, for a []T the
   tables don't have); self: the receiver of the
   last T.name :: proc */
static int rio_cplTextType(rio_Cpl *k, const char *name, int n, const char *end) {
  const char *p = k->text, *best = 0;
  int self = n == 4 && !memcmp(name, "self", 4), isSelf = 0, i, kinds[4], depth, type;
  uint32_t counts[4];
  for (; p + n <= end; p++) {
    const char *q;
    if ((p > k->text && (rio_cplWordChar(p[-1]) || p[-1] == '.')) || !rio_isAlpha(*p)) continue;
    if (self) { /* Name.name :: */
      for (q = p; q < end && rio_cplWordChar(*q); q++) {}
      if (q < end && *q == '.' && q + 1 < end && rio_isAlpha(q[1])) {
        const char *w = q + 1;
        while (w < end && rio_cplWordChar(*w)) w++;
        w += rio_cplSkipSpace(w, end);
        if (w + 1 < end && w[0] == ':' && w[1] == ':') {
          best = p;
          isSelf = 1;
        }
      }
      continue;
    }
    if (memcmp(p, name, (size_t)n) || (p + n < end && rio_cplWordChar(p[n]))) continue;
    q = p + n + rio_cplSkipSpace(p + n, end);
    if (q < end && *q == ':' && (q + 1 >= end || q[1] != ':')) best = q + 1 + (q + 1 < end && q[1] == '=');
  }
  if (!best) return 0;
  if (!isSelf) {
    best += rio_cplSkipSpace(best, end);
    if (best < end && *best == '&') best++;
  }
  for (depth = 0; depth < 4 && best < end && *best == '['; depth++) best = rio_cplBrackets(best, end, &kinds[depth], &counts[depth]); /* ([]T, [4]T, [..8]T) */
  for (p = best; p < end && rio_cplWordChar(*p); p++) {}
  if (p == best || !(*best >= 'A' && *best <= 'Z')) return 0;
  i = rio_lookup(k->vm, best, (int)(p - best));
  if (i < 0 || k->c->syms[i].kind != rio_SymKind_type) return 0;
  type = k->c->syms[i].type;
  if (type == rio_TypeId_blob && p < end && *p == '[') { /* Blob[4], Blob[..8], Blob[..]: of bytes */
    int kind;
    uint32_t count;
    p = rio_cplBrackets(p, end, &kind, &count);
    type = kind == rio_TypeKind_slice ? 0 : rio_cplFind(k, kind, rio_TypeId_byte, count);
  }
  if (!isSelf && p < end && *p != '{' && *p != '.' && *p != ')' && *p != ',' && *p != ' ' && *p != '\n' && *p != '\r' && *p != '=') return 0;
  while (depth-- > 0 && type) {
    int t = rio_cplFind(k, kinds[depth], type, counts[depth]);
    if (!t && !depth && kinds[0] == rio_TypeKind_slice) return -type; /* (a []T the tables don't have: T's, negated) */
    type = t;
  }
  return type;
}

/* the bracket closing the one at p (before end), or end */
static const char *rio_cplClose(const char *p, const char *end) {
  int depth = 0, str = 0;
  for (; p < end; p++) {
    if (str) {
      if (*p == '\\') p++;
      else if (*p == str) str = 0;
    } else if (*p == '"' || *p == '\'') str = *p;
    else if (*p == '(' || *p == '[' || *p == '{') depth++;
    else if ((*p == ')' || *p == ']' || *p == '}') && !--depth) return p;
  }
  return end;
}

/* what [from, dot) is: a name, then .name, (...) and [...] after it */
static rio_CplIs rio_cplChain(rio_Cpl *k, const char *from, const char *dot) {
  const char *p = from, *q;
  rio_CplIs r = rio_cplIs(rio_Is_nothing, 0);
  int i, steps;
  for (q = p; q < dot && rio_cplWordChar(*q); q++) {}
  if ((i = rio_lookup(k->vm, p, (int)(q - p))) >= 0) r = rio_cplOfSymbol(k, i);
  else if ((i = rio_cplTextType(k, p, (int)(q - p), from)) != 0) r = rio_cplIs(i > 0 ? rio_Is_value : rio_Is_slice, i > 0 ? i : -i); /* (not compiled yet) */
  for (p = q, steps = 0; p < dot && r.is && steps < 32; steps++) {
    rio_Type *ty = rio_type(k->vm, r.type);
    if (*p == '(') { /* a call: its result */
      p = rio_cplClose(p, dot) + 1;
      if (r.is == rio_Is_value && ty->kind == rio_TypeKind_proc) r = rio_cplIs(rio_Is_value, ty->elem);
      else if (r.is == rio_Is_call) r.is = r.type == rio_TypeId_void ? rio_Is_nothing : rio_Is_value;
      else r.is = rio_Is_nothing;
    } else if (*p == '[') { /* an element, or a slice */
      const char *e = rio_cplClose(p, dot), *s;
      int slice = e == p + 1;
      for (s = p + 1; s + 1 < e; s++) slice |= s[0] == '.' && s[1] == '.';
      p = e + 1;
      if (r.is == rio_Is_slice) r = slice ? r : rio_cplIs(rio_Is_value, rio_valueType(r.type));
      else if (r.is != rio_Is_value || (ty->kind != rio_TypeKind_array && ty->kind != rio_TypeKind_slice && ty->kind != rio_TypeKind_builder &&
                                        ty->kind != rio_TypeKind_builderView)) r.is = rio_Is_nothing;
      else if (!slice) r = rio_cplIs(rio_Is_value, rio_valueType(ty->elem));
      else if (ty->kind != rio_TypeKind_slice) r = rio_cplSliceOf(k, ty->elem) ? rio_cplIs(rio_Is_value, rio_cplSliceOf(k, ty->elem)) : rio_cplIs(rio_Is_slice, ty->elem);
    } else if (*p == '.' && p + 1 < dot && rio_isAlpha(p[1])) { /* .name */
      const char *s = p + 1;
      int n, f;
      int32_t v;
      for (q = s; q < dot && rio_cplWordChar(*q); q++) {}
      n = (int)(q - s);
      p = q;
      if (r.is == rio_Is_module) r = (i = rio_lookupExport(k->vm, r.type, s, n)) >= 0 ? rio_cplOfSymbol(k, i) : rio_cplIs(rio_Is_nothing, 0);
      else if (r.is == rio_Is_type && ty->kind == rio_TypeKind_enum && rio_findEnumValue(k->vm, r.type, s, n, &v)) r.is = rio_Is_value;
      else if (r.is == rio_Is_type) r = (i = rio_lookupMethod(k->vm, r.type, s, n)) >= 0 ? rio_cplOfSymbol(k, i) : rio_cplIs(rio_Is_nothing, 0);
      else if (r.is != rio_Is_value) r.is = rio_Is_nothing;
      else if ((f = rio_findField(k->vm, ty, s, n)) >= 0) r.type = rio_field(k->vm, ty->firstField + f)->type;
      else if ((i = rio_lookupMethod(k->vm, rio_valueType(r.type), s, n)) >= 0) r = rio_cplOfSymbol(k, i);
      else if ((n == 3 && (!memcmp(s, "len", 3) || !memcmp(s, "cap", 3))) || (n == 5 && !memcmp(s, "toInt", 5)) || (n == 6 && !memcmp(s, "getInt", 6)))
        r = rio_cplIs(rio_Is_call, rio_TypeId_int);
      else if (n == 7 && !memcmp(s, "toFloat", 7)) r = rio_cplIs(rio_Is_call, rio_TypeId_float);
      else if (n == 6 && !memcmp(s, "toBool", 6)) r = rio_cplIs(rio_Is_call, rio_TypeId_bool);
      else if (n == 8 && !memcmp(s, "asString", 8)) r = rio_cplIs(rio_Is_call, rio_TypeId_string);
      else if (n == 3 && !memcmp(s, "pop", 3)) r = rio_cplIs(rio_Is_call, rio_valueType(ty->elem));
      else r.is = rio_Is_nothing;
    } else r.is = rio_Is_nothing;
  }
  return p == dot ? r : rio_cplIs(rio_Is_nothing, 0);
}

/* where the chain before the dot at dot starts (a name, then .name, (...) and [...]), or 0 */
static const char *rio_cplChainStart(const char *text, const char *dot) {
  const char *p = dot;
  int steps;
  for (steps = 0; steps < 32 && p > text; steps++) {
    if (p[-1] == ')' || p[-1] == ']') { /* back to its opening bracket */
      int depth = 0;
      while (p > text) {
        p--;
        depth += *p == ')' || *p == ']' || *p == '}';
        depth -= *p == '(' || *p == '[' || *p == '{';
        if (!depth) break;
      }
      if (depth) return 0;
      continue;
    }
    if (!rio_cplWordChar(p[-1])) return 0;
    while (p > text && rio_cplWordChar(p[-1])) p--;
    if (rio_isDigit(*p)) return 0;
    if (p - text >= 1 && p[-1] == '.' && (p - text < 2 || p[-2] != '.')) {
      p--;
      continue;
    }
    return p;
  }
  return 0;
}

static void rio_cplNames(rio_Cpl *k, const char *const *names, int kind, const char *detail) {
  for (; *names; names++) {
    k->detailLen = 0;
    rio_cplPut(k, detail, -1);
    rio_cplAt(k, 0, 0, -1);
    rio_cplOffer(k, *names, (int)strlen(*names), kind);
  }
}

/* x.: its fields and methods, and the built-in ones its kind has */
static void rio_cplMembers(rio_Cpl *k, rio_CplIs r) {
  static const char *const lists[] = {"len", "cap", 0, "swap", 0, "push", "pop", "clear", "remove", "swapRemove", "pushAll", 0,
    "format", "print", 0, "pack", "unpack", "getInt", "setInt", 0, "toInt", 0, "toFloat", 0, "toBool", 0, "asString", 0};
  enum { len = 0, swap = 3, builder = 5, text = 12, bytes = 15, toInt = 20, toFloat = 22, toBool = 24, asString = 26 };
  rio_Type *ty;
  int kind, elem, j, isBuilder, isSeq, list[4] = {-1, -1, -1, -1};
  if (r.is == rio_Is_slice) { /* (a []T the tables don't have) */
    rio_cplNames(k, lists + len, rio_Item_method, "built in");
    rio_cplNames(k, lists + swap, rio_Item_method, "built in");
    if (r.type == rio_TypeId_byte) rio_cplNames(k, lists + bytes, rio_Item_method, "built in");
    return;
  }
  ty = rio_type(k->vm, r.type);
  kind = ty->kind;
  elem = ty->elem;
  isBuilder = kind == rio_TypeKind_builder || kind == rio_TypeKind_builderView;
  isSeq = isBuilder || kind == rio_TypeKind_array || kind == rio_TypeKind_slice;
  for (j = 0; kind == rio_TypeKind_struct && j < ty->fieldCount; j++) {
    rio_Field *f = rio_field(k->vm, ty->firstField + j);
    int sym = rio_cplTypeSym(k, r.type);
    rio_cplType(k, f->type);
    rio_cplAt(k, sym ? k->c->syms[sym - 1].file : 0, f->line, -1); /* (its struct's file) */
    rio_cplOffer(k, k->c->names + f->name, f->nameLen, rio_Item_field);
  }
  rio_cplSymbols(k, rio_CplSyms_methods, rio_valueType(r.type));
  if (kind == rio_TypeKind_struct && ty->elem) rio_cplSymbols(k, rio_CplSyms_methods, rio_genericOf(k->vm, ty->elem)); /* (a generic's) */
  if (r.type == rio_TypeId_range) {
    static const char *const range[] = {"len", "has", 0};
    rio_cplNames(k, range, rio_Item_method, "built in");
    return;
  }
  if (isSeq) {
    rio_cplNames(k, lists + len, rio_Item_method, "built in");
    rio_cplNames(k, lists + swap, rio_Item_method, "built in");
  }
  if (isBuilder) rio_cplNames(k, lists + builder, rio_Item_method, "built in");
  if (isSeq && elem == rio_TypeId_byte) { /* bytes: Blob, String, Blob[N], Blob[..N] */
    if (isBuilder) rio_cplNames(k, lists + text, rio_Item_method, "built in");
    rio_cplNames(k, lists + bytes, rio_Item_method, "built in");
    if (kind != rio_TypeKind_array) list[0] = toInt, list[1] = toFloat; /* (its text as a number) */
    list[2] = asString;
  } else if (kind == rio_TypeKind_array && (elem == rio_TypeId_int || elem == rio_TypeId_float)) list[0] = toInt, list[1] = toFloat;
  else if (kind == rio_TypeKind_int) list[0] = toInt, list[1] = toFloat, list[2] = toBool;
  else if (kind == rio_TypeKind_float) list[0] = toInt, list[1] = toFloat;
  else if (kind == rio_TypeKind_bool) list[0] = toInt, list[1] = toBool;
  else if (kind == rio_TypeKind_enum) list[0] = toInt;
  for (j = 0; j < 4; j++) if (list[j] >= 0) rio_cplNames(k, lists + list[j], rio_Item_method, "built in"); /* (the conversions) */
}

/* Type.: an enum's values, static procs (and static host procs, Type.name) */
static void rio_cplStatics(rio_Cpl *k, int type) {
  rio_Compiler *c = k->c;
  rio_Type *ty = rio_type(k->vm, type);
  int i, j, sym = rio_cplTypeSym(k, type);
  if (ty->kind == rio_TypeKind_enum) {
    static const char *const more[] = {"fromInt", "has", 0};
    for (j = 0; j < ty->fieldCount; j++) {
      rio_Field *f = rio_field(k->vm, ty->firstField + j);
      k->detailLen = 0;
      rio_cplTypeFlat(k, type);
      rio_cplAt(k, sym ? c->syms[sym - 1].file : 0, f->line, -1);
      rio_cplOffer(k, c->names + f->name, f->nameLen, rio_Item_value);
    }
    rio_cplNames(k, more, rio_Item_method, "built in");
  }
  rio_cplSymbols(k, rio_CplSyms_statics, type);
  for (i = c->symCount - 1; sym && i >= 0; i--) { /* Type.name, a host proc declared in Type's module */
    rio_Symbol *y = &c->syms[i], *t = &c->syms[sym - 1];
    if (y->kind != rio_SymKind_ffi || y->value < 0 || (y->module != c->curModule && !y->exported) || y->module != t->module) continue;
    if (y->nameLen <= t->nameLen + 1 || memcmp(c->names + y->name, c->names + t->name, t->nameLen) || c->names[y->name + t->nameLen] != '.') continue;
    rio_cplFfi(k, y);
    rio_cplAt(k, y->file, y->line, -1);
    rio_cplOffer(k, c->names + y->name + t->nameLen + 1, y->nameLen - t->nameLen - 1, rio_Item_proc);
  }
}

/* ------------------------------------------------------------------ keywords and the text's own names */

/* is [p, p + n) one of words? */
static int rio_cplIn(const char *p, int n, const char *const *words) {
  for (; *words; words++)
    if ((int)strlen(*words) == n && !memcmp(*words, p, (size_t)n)) return 1;
  return 0;
}

/* the names [text, end) declares that the tables don't have: name := ..., name: ..., name :: ...,
   for name, name in ..., and a proc's parameters (name: T) */
static void rio_cplTextNames(rio_Cpl *k, const char *end) {
  static const char *const words[] = {"proc", "struct", "if", "else", "for", "in", "end", "return", "break", "continue", "import",
    "include", "enum", "union", "switch", "case", "is", "nil", "and", "or", "not", "do", "by", 0};
  const char *p = k->text, *line = end;
  int str = 0, afterFor = 0;
  while (line > k->text && line[-1] != '\n') line--; /* (a name being declared on the cursor's line isn't one yet: x := x) */
  while (p < end) {
    const char *q, *w;
    int n, kind = -1, i;
    if (str) {
      if (*p == '\\') p++;
      else if (*p == str || *p == '\n') str = 0;
      p++;
      continue;
    }
    if (*p == '"' || *p == '\'') {
      str = *p++;
      continue;
    }
    if (*p == '#') {
      while (p < end && *p != '\n') p++;
      continue;
    }
    if (*p == '\n') afterFor = 0;
    if (!rio_isAlpha(*p) || (p > k->text && (rio_cplWordChar(p[-1]) || p[-1] == '.'))) {
      p++;
      continue;
    }
    for (q = p; q < end && rio_cplWordChar(*q); q++) {}
    n = (int)(q - p);
    w = q + rio_cplSkipSpace(q, end);
    if (n == 3 && !memcmp(p, "for", 3)) afterFor = 1;
    else if (n == 2 && !memcmp(p, "in", 2)) afterFor = 0;
    else if (q >= end) {
    } else if (rio_cplIn(p, n, words)) {
    } else if (afterFor) kind = rio_Item_var;
    else if (w + 1 < end && w[0] == ':' && w[1] == ':') {
      const char *v = w + 2 + rio_cplSkipSpace(w + 2, end);
      kind = (v + 4 <= end && !memcmp(v, "proc", 4)) ? rio_Item_proc
             : (v + 6 <= end && (!memcmp(v, "struct", 6) || !memcmp(v, "union(", 6))) || (v + 4 <= end && !memcmp(v, "enum", 4)) ? rio_Item_type
                                                                                                                                  : rio_Item_const;
    } else if (w < end && *w == ':') kind = rio_Item_var;
    else if (w < end && *w == ',') { /* a group, a, b: T (each name, then a :) */
      const char *g = w;
      while (g < end && *g == ',') {
        g++;
        g += rio_cplSkipSpace(g, end);
        while (g < end && rio_cplWordChar(*g)) g++;
        g += rio_cplSkipSpace(g, end);
      }
      if (g < end && *g == ':' && (g + 1 >= end || (g[1] != ':' && g[1] != '='))) kind = rio_Item_var;
    }
    else if (w + 1 < end && w[0] == '?' && w[1] == ':') kind = rio_Item_var; /* (an optional parameter, name?: T) */
    if (kind >= 0 && !afterFor && p >= line) kind = -1;
    if (kind >= 0 && k->c && rio_lookup(k->vm, p, n) >= 0) kind = -1; /* (compiled: offered as a symbol) */
    for (i = 0; kind >= 0 && i < k->seenCount; i++)
      if (k->seenLen[i] == n && !memcmp(k->text + k->seenAt[i], p, (size_t)n)) kind = -1;
    if (kind >= 0 && k->seenCount < rio_CPL_SEEN && !(p <= k->word && k->word < q)) {
      k->seenAt[k->seenCount] = (int)(p - k->text);
      k->seenLen[k->seenCount++] = n;
      k->detailLen = 0;
      rio_cplAt(k, 0, 0, (int)(p - k->text));
      rio_cplOffer(k, p, n, kind);
    }
    p = q;
  }
}

/* the text's state at end: in a string or comment (on end's line) */
static int rio_cplInText(const char *text, const char *end) {
  const char *p = end;
  int str = 0;
  while (p > text && p[-1] != '\n') p--;
  for (; p < end; p++) {
    if (str) {
      if (*p == '\\') p++;
      else if (*p == str) str = 0;
    } else if (*p == '"' || *p == '\'') str = *p;
    else if (*p == '#') return 1;
  }
  return str != 0;
}

/* is the innermost bracket open at end a [? (xs[end]) */
static int rio_cplInIndex(const char *text, const char *end) {
  const char *p = end;
  int depth = 0;
  while (p > text) {
    p--;
    if (*p == ')' || *p == ']' || *p == '}') depth++;
    else if (*p == '(' || *p == '[' || *p == '{') {
      if (!depth) return *p == '[';
      depth--;
    }
  }
  return 0;
}

/* is [p, p + n) the word w? */
static int rio_cplIs1(const char *p, int n, const char *w) { return (int)strlen(w) == n && !memcmp(p, w, (size_t)n); }

static int rio_cplRun(rio_Vm *vm, const char *text, uint32_t at, rio_CompleteFn fn, rio_LookupFn define, void *userData) {
  static const char *const statement[] = {"if", "for", "switch", "do", "return", 0};
  static const char *const inBlock[] = {"end", "else", "case", "break", "continue", 0};
  static const char *const topLevel[] = {"import", "include", 0};
  static const char *const value[] = {"nil", "not", "if", "switch", 0};
  static const char *const declaring[] = {"proc", "struct", "enum", "union", "static", "callback", "host", 0};
  static const char *const operand[] = {"and", "or", "is", 0};
  static const char *const headers[] = {"if", "switch", "case", "else", 0};
  static const char *const loop[] = {"in", "by", "do", 0};
  static const char *const notOperands[] = {"if", "else", "for", "in", "return", "import", "include", "switch", "case", "is", "and", "or",
    "not", "do", "by", "proc", "struct", "enum", "union", "static", "callback", "host", 0};
  rio_Cpl k;
  const char *end = text + at, *word = end, *b, *line, *prev, *first;
  int prevLen = 0, firstLen;
  memset(&k, 0, sizeof k);
  k.vm = vm;
  k.c = vm ? vm->compiler : 0;
  k.text = text;
  k.fn = fn;
  k.define = define;
  k.userData = userData;
  while (word > text && rio_cplWordChar(word[-1])) word--;
  k.word = word;
  k.wordLen = (int)(end - word);
  if ((k.wordLen && rio_isDigit(*word)) || rio_cplInText(text, word)) return (int)(word - text);
  if (word > text && word[-1] == '.' && (word - text < 2 || word[-2] != '.')) { /* x., Type., module.: what x is has */
    const char *from = k.c ? rio_cplChainStart(text, word - 1) : 0;
    rio_CplIs r = from ? rio_cplChain(&k, from, word - 1) : rio_cplIs(rio_Is_nothing, 0);
    if (r.is == rio_Is_module) rio_cplSymbols(&k, rio_CplSyms_exports, r.type);
    else if (r.is == rio_Is_type) rio_cplStatics(&k, r.type);
    else if (r.is == rio_Is_value || r.is == rio_Is_slice) rio_cplMembers(&k, r);
    return (int)(word - text);
  }
  for (b = word; b > text && (b[-1] == ' ' || b[-1] == '\t'); b--) {} /* the token before the word (a word: prev) */
  for (line = b; line > text && line[-1] != '\n'; line--) {}
  line += rio_cplSkipSpace(line, b);
  while (b - prevLen > text && rio_cplWordChar(b[-prevLen - 1])) prevLen++;
  prev = b - prevLen;
  for (first = line; first < b && rio_cplWordChar(*first); first++) {} /* the line's first word */
  firstLen = (int)(first - line);
  first = line;
  if (b == line || rio_cplIs1(prev, prevLen, "do") || rio_cplIs1(prev, prevLen, "else")) { /* a statement starts here */
    rio_cplNames(&k, statement, rio_Item_keyword, "");
    if (b == line) /* (the blocks open: the text's, and a check's, cut there) */
      rio_cplNames(&k, rio_replIndent(text, (uint32_t)(line - text)) || (k.c && k.c->blockCount) ? inBlock : topLevel, rio_Item_keyword, "");
    else rio_cplNames(&k, inBlock + 3, rio_Item_keyword, ""); /* (if x do break) */
    rio_cplSymbols(&k, rio_CplSyms_scope, 0);
    rio_cplTextNames(&k, word);
    return (int)(word - text);
  }
  if (b - text >= 2 && b[-1] == ':' && b[-2] == ':') { /* name :: proc, struct ..., or a constant */
    rio_cplNames(&k, declaring, rio_Item_keyword, "");
    rio_cplSymbols(&k, rio_CplSyms_scope, 0);
    return (int)(word - text);
  }
  if (rio_cplIn(prev, prevLen, declaring + 4)) { /* static proc, callback proc, host proc and host struct */
    static const char *const procs[] = {"proc", 0}, *const hosted[] = {"proc", "struct", 0}, *const statics[] = {"proc", "host", "callback", 0};
    rio_cplNames(&k, rio_cplIs1(prev, prevLen, "host") ? hosted : rio_cplIs1(prev, prevLen, "static") ? statics : procs, rio_Item_keyword, "");
    return (int)(word - text);
  }
  { /* a type goes here: x: T, x: &T, x?: T, -> T, and after []T, [4]T, [..]T there */
    const char *t = b;
    while (t > text && t[-1] == ']') { /* (back over [] [4] [..] [..8], with no value before them) */
      const char *o = t - 1;
      while (o > text && o[-1] != '[' && o[-1] != ']' && o[-1] != '\n') o--;
      if (o <= text || o[-1] != '[' || (o - 1 > text && (rio_cplWordChar(o[-2]) || o[-2] == ')' || o[-2] == ']'))) break;
      t = o - 1;
    }
    if (t > text && t[-1] == '&') t--;
    while (t > text && (t[-1] == ' ' || t[-1] == '\t')) t--;
    if ((t > text && t[-1] == ':' && (t - text < 2 || t[-2] != ':')) || (t - text >= 2 && t[-1] == '>' && t[-2] == '-')) {
      rio_cplSymbols(&k, rio_CplSyms_types, 0);
      return (int)(word - text);
    }
  }
  if ((prevLen && !rio_cplIn(prev, prevLen, notOperands)) ||
      (b > text && (b[-1] == ')' || b[-1] == ']' || b[-1] == '}' || b[-1] == '"' || b[-1] == '\''))) { /* after a value: an operator's word */
    rio_cplNames(&k, operand, rio_Item_keyword, "");
    if (rio_cplIs1(first, firstLen, "for")) rio_cplNames(&k, loop, rio_Item_keyword, "");
    else if (rio_cplIn(first, firstLen, headers)) rio_cplNames(&k, loop + 2, rio_Item_keyword, "");
    return (int)(word - text);
  }
  rio_cplNames(&k, value, rio_Item_keyword, ""); /* an expression goes here */
  if (rio_cplInIndex(text, word)) {
    static const char *const last[] = {"end", 0};
    rio_cplNames(&k, last, rio_Item_keyword, "");
  }
  rio_cplSymbols(&k, rio_CplSyms_scope, 0);
  rio_cplTextNames(&k, word);
  return (int)(word - text);
}

int rio_complete(rio_Vm *vm, const char *text, uint32_t at, rio_CompleteFn fn, void *userData) {
  return rio_cplRun(vm, text, at, fn, 0, userData);
}

int rio_lookupAt(rio_Vm *vm, const char *text, uint32_t at, rio_LookupFn fn, void *userData) {
  return rio_cplRun(vm, text, at, 0, fn, userData);
}

#endif
