/* header.c - rio_hostHeader: C for a host to write its host procs against, from the Rio module that
   declares them (rio -H): structs of the same layouts as Rio's, and the procs' sigs. Apart from the
   compiler (like aot.c), so a host that doesn't make headers (a board's) doesn't carry it. */
#include "compiler.h"

/* a struct type's name, or "" */
static const char *rio_structName(rio_Vm *vm, int type, int *len) {
  rio_Compiler *c = vm->compiler;
  int i;
  for (i = c->symCount - 1; i >= 0; i--)
    if (c->syms[i].kind == rio_SymKind_type && c->syms[i].type == type) {
      *len = c->syms[i].nameLen;
      return c->names + c->syms[i].name;
    }
  *len = 0;
  return "";
}

/* a field's name in C: a C keyword gets a _ */
static void rio_cName(rio_HostText *o, const char *s, int n) {
  static const char *const words[] = {"auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else",
    "enum", "extern", "float", "for", "goto", "if", "inline", "int", "long", "register", "restrict", "return", "short", "signed",
    "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", "bool"};
  size_t i;
  rio_hostPut(o, s, n);
  for (i = 0; i < sizeof words / sizeof words[0]; i++)
    if ((int)strlen(words[i]) == n && !memcmp(words[i], s, (size_t)n)) rio_hostPut(o, "_", 1);
}

/* a C declaration of name (a field's, with dims: "[2][3]") as type */
static void rio_cDecl(rio_Vm *vm, rio_HostText *o, const char *prefix, int type, const char *name, int nameLen, const char *dims) {
  rio_Type *ty = rio_type(vm, type);
  char more[64];
  int n;
  switch (ty->kind) {
  case rio_TypeKind_float: rio_hostPut(o, "float ", -1); break;
  case rio_TypeKind_slice: rio_hostPut(o, "rio_View ", -1); break;
  case rio_TypeKind_builderView: rio_hostPut(o, "uint32_t ", -1); break;
  case rio_TypeKind_struct: {
    const char *s = rio_structName(vm, type, &n);
    rio_hostPut(o, prefix, -1);
    rio_hostPut(o, "_", 1);
    rio_hostPut(o, s, n);
    rio_hostPut(o, " ", 1);
    break;
  }
  case rio_TypeKind_array:
    if (ty->elem != rio_TypeId_byte) { /* (its elements', with one more dim) */
      n = rio_appendText(more, 0, dims);
      more[n++] = '[';
      n += rio_formatInt(more + n, (int32_t)ty->count);
      more[n++] = ']';
      more[n] = 0;
      rio_cDecl(vm, o, prefix, ty->elem, name, nameLen, more);
      return;
    }
    rio_hostPut(o, "uint8_t ", -1);
    break;
  case rio_TypeKind_builder: case rio_TypeKind_union:
    rio_hostPut(o, ty->kind == rio_TypeKind_union ? "struct { int32_t tag; " : "struct { int32_t len; ", -1);
    if (ty->kind == rio_TypeKind_builder && ty->elem != rio_TypeId_byte) {
      n = rio_appendText(more, 0, "[");
      n += rio_formatInt(more + n, (int32_t)ty->count);
      more[n++] = ']';
      more[n] = 0;
      rio_cDecl(vm, o, prefix, ty->elem, "items", 5, more);
      rio_hostPut(o, "; ", 2);
    } else if (ty->size > 4) {
      rio_hostPut(o, ty->kind == rio_TypeKind_union ? "uint32_t words[" : "uint8_t items[", -1);
      rio_hostPutInt(o, (int32_t)(ty->kind == rio_TypeKind_union ? (ty->size - 4) / 4 : ty->size - 4));
      rio_hostPut(o, "]; ", 3);
    }
    rio_hostPut(o, "} ", 2);
    break;
  default: rio_hostPut(o, "int32_t ", -1); break; /* (Int, Bool, an enum, a proc value) */
  }
  rio_cName(o, name, nameLen);
  rio_hostPut(o, dims, -1);
  if (ty->kind == rio_TypeKind_builderView) rio_hostPut(o, "[3]", 3);
  else if (ty->kind == rio_TypeKind_array) { /* (Blob[N]: its bytes, padding too) */
    rio_hostPut(o, "[", 1);
    rio_hostPutInt(o, (int32_t)ty->size);
    rio_hostPut(o, "]", 1);
  }
}

/* a struct's C, after those of the structs inside it; done holds the ones written */
static void rio_cStruct(rio_Vm *vm, rio_HostText *o, const char *prefix, int type, uint16_t *done, int *doneCount) {
  rio_Compiler *c = vm->compiler;
  rio_Type *ty = rio_type(vm, type);
  const char *s;
  int i, n, lastBits = -1;
  if (ty->kind == rio_TypeKind_array || ty->kind == rio_TypeKind_builder || ty->kind == rio_TypeKind_slice || ty->kind == rio_TypeKind_builderView) {
    if (ty->elem != rio_TypeId_byte) rio_cStruct(vm, o, prefix, ty->elem, done, doneCount);
    return;
  }
  if (ty->kind != rio_TypeKind_struct && ty->kind != rio_TypeKind_union) return;
  for (i = 0; i < *doneCount; i++)
    if (done[i] == type) return;
  if (*doneCount >= 256) return;
  done[(*doneCount)++] = (uint16_t)type;
  for (i = 0; i < ty->fieldCount; i++) rio_cStruct(vm, o, prefix, rio_field(vm, ty->firstField + i)->type, done, doneCount);
  if (ty->kind == rio_TypeKind_union) return; /* (written where it's used) */
  s = rio_structName(vm, type, &n);
  rio_hostPut(o, "typedef struct {\n", -1);
  for (i = 0; i < ty->fieldCount; i++) {
    rio_Field *f = rio_field(vm, ty->firstField + i);
    if (f->bits) { /* (bitfields: their word, once; accessors below) */
      if (f->offset == lastBits) continue;
      lastBits = f->offset;
      rio_hostPut(o, "  uint32_t bits", -1);
      rio_hostPutInt(o, f->offset);
      rio_hostPut(o, ";\n", 2);
      continue;
    }
    rio_hostPut(o, "  ", 2);
    rio_cDecl(vm, o, prefix, f->type, c->names + f->name, f->nameLen, "");
    rio_hostPut(o, ";\n", 2);
  }
  rio_hostPut(o, "} ", 2);
  rio_hostPut(o, prefix, -1);
  rio_hostPut(o, "_", 1);
  rio_hostPut(o, s, n);
  rio_hostPut(o, ";\ntypedef char ", -1);
  rio_hostPut(o, prefix, -1);
  rio_hostPut(o, "_", 1);
  rio_hostPut(o, s, n);
  rio_hostPut(o, "_size[sizeof(", -1);
  rio_hostPut(o, prefix, -1);
  rio_hostPut(o, "_", 1);
  rio_hostPut(o, s, n);
  rio_hostPut(o, ") == ", -1);
  rio_hostPutInt(o, (int32_t)ty->size);
  rio_hostPut(o, " ? 1 : -1]; /* (Rio's size) */\n", -1);
  for (i = 0; i < ty->fieldCount; i++) { /* a bitfield: s's value, and set_name(s, v) */
    rio_Field *f = rio_field(vm, ty->firstField + i);
    int bits = f->bits < 0 ? -f->bits : f->bits, k;
    if (!f->bits) continue;
    for (k = 0; k < 2; k++) {
      rio_hostPut(o, "#define ", -1);
      rio_hostPut(o, prefix, -1);
      rio_hostPut(o, "_", 1);
      rio_hostPut(o, s, n);
      rio_hostPut(o, k ? "_set_" : "_", -1);
      rio_hostPut(o, c->names + f->name, f->nameLen);
      rio_hostPut(o, k ? "(s, v) ((s)->bits" : "(s) ((int32_t)", -1);
      if (k) { /* ((s)->bitsN = ((s)->bitsN & ~(mask << at)) | (((uint32_t)(v) & mask) << at)) */
        rio_hostPutInt(o, f->offset);
        rio_hostPut(o, " = ((s)->bits", -1);
        rio_hostPutInt(o, f->offset);
        rio_hostPut(o, " & ~(", -1);
        rio_hostPutInt(o, (int32_t)((1u << bits) - 1));
        rio_hostPut(o, "u << ", -1);
        rio_hostPutInt(o, f->bitAt);
        rio_hostPut(o, ")) | (((uint32_t)(v) & ", -1);
        rio_hostPutInt(o, (int32_t)((1u << bits) - 1));
        rio_hostPut(o, "u) << ", -1);
        rio_hostPutInt(o, f->bitAt);
        rio_hostPut(o, "))\n", -1);
      } else if (f->bits < 0) { /* ((int32_t)((s)->bitsN << (32 - at - n)) >> (32 - n)) */
        rio_hostPut(o, "((s)->bits", -1);
        rio_hostPutInt(o, f->offset);
        rio_hostPut(o, " << ", -1);
        rio_hostPutInt(o, 32 - f->bitAt - bits);
        rio_hostPut(o, ") >> ", -1);
        rio_hostPutInt(o, 32 - bits);
        rio_hostPut(o, ")\n", -1);
      } else { /* ((int32_t)((s)->bitsN >> at & mask)) */
        rio_hostPut(o, "((s)->bits", -1);
        rio_hostPutInt(o, f->offset);
        rio_hostPut(o, " >> ", -1);
        rio_hostPutInt(o, f->bitAt);
        rio_hostPut(o, " & ", -1);
        rio_hostPutInt(o, (int32_t)((1u << bits) - 1));
        rio_hostPut(o, "u))\n", -1);
      }
    }
  }
  rio_hostPut(o, "#define ", -1);
  rio_hostPut(o, prefix, -1);
  rio_hostPut(o, "_", 1);
  rio_hostPut(o, s, n);
  rio_hostPut(o, "_LAYOUT \"", -1);
  rio_layoutOf(vm, type, 0, o);
  rio_hostPut(o, "\"\n\n", -1);
}

/* a union or maybe result's C, prefix_proc_Result: a maybe's has (1: there, 0: nil) and value; another
   union's tag (which of its types, as listed) and words */
static void rio_cResult(rio_Vm *vm, rio_HostText *o, const char *prefix, const rio_Symbol *y, int type) {
  rio_Compiler *c = vm->compiler;
  rio_Type *ty = rio_type(vm, type);
  int maybe = ty->fieldCount == 2 && rio_field(vm, ty->firstField)->type == rio_TypeId_void && ty->elem, k;
  rio_hostPut(o, maybe ? "typedef struct {\n  int32_t has; /* 1: value is there, 0: nil */\n  " : "typedef struct {\n  int32_t tag; /* which of its types, as listed */\n  ", -1);
  if (maybe) rio_cDecl(vm, o, prefix, ty->elem, "value", 5, "");
  else {
    rio_hostPut(o, "uint32_t words[", -1);
    rio_hostPutInt(o, (int32_t)(ty->size > 4 ? (ty->size - 4) / 4 : 1));
    rio_hostPut(o, "]", 1);
  }
  rio_hostPut(o, ";\n} ", -1);
  for (k = 0; k < 2; k++) {
    rio_hostPut(o, prefix, -1);
    rio_hostPut(o, "_", 1);
    rio_hostPut(o, c->names + y->name, y->nameLen);
    rio_hostPut(o, k ? "_Result_size[sizeof(" : "_Result;\ntypedef char ", -1);
  }
  rio_hostPut(o, prefix, -1);
  rio_hostPut(o, "_", 1);
  rio_hostPut(o, c->names + y->name, y->nameLen);
  rio_hostPut(o, "_Result) == ", -1);
  rio_hostPutInt(o, (int32_t)ty->size);
  rio_hostPut(o, " ? 1 : -1]; /* (Rio's size) */\n\n", -1);
}

/* a host proc's sig, from its declared types, as C: string literals, with a struct's name_LAYOUT */
static void rio_cSig(rio_Vm *vm, rio_HostText *o, const char *prefix, int procType) {
  rio_Type *ty = rio_type(vm, procType);
  int i, open = 0, any = 0;
  for (i = 0; i <= ty->fieldCount; i++) {
    int type = i < ty->fieldCount ? rio_field(vm, ty->firstField + i)->type : ty->elem, k = rio_type(vm, type)->kind;
    if (i == ty->fieldCount && type == rio_TypeId_void) break;
    if (i == ty->fieldCount && k == rio_TypeKind_union) { /* (a union's or maybe's layout, written out) */
      rio_hostPut(o, open ? ">" : any ? " \">" : "\">", -1);
      rio_layoutOf(vm, type, 0, o);
      rio_hostPut(o, "\"", 1);
      return;
    }
    if (!open && (i == ty->fieldCount || k != rio_TypeKind_struct)) {
      rio_hostPut(o, any ? " \"" : "\"", -1);
      open = any = 1;
    }
    if (i == ty->fieldCount) rio_hostPut(o, ">", 1);
    if (i < ty->fieldCount && rio_field(vm, ty->firstField + i)->offset) { /* (a &T: &, then T as it'd be) */
      if (!open) {
        rio_hostPut(o, any ? " \"" : "\"", -1);
        open = any = 1;
      }
      rio_hostPut(o, "&", 1);
    }
    if (k == rio_TypeKind_struct) { /* " prefix_Name_LAYOUT */
      int n;
      const char *s = rio_structName(vm, type, &n);
      rio_hostPut(o, open ? "\" " : any ? " " : "", -1);
      rio_hostPut(o, prefix, -1);
      rio_hostPut(o, "_", 1);
      rio_hostPut(o, s, n);
      rio_hostPut(o, "_LAYOUT", -1);
      open = 0;
      any = 1;
    } else rio_hostPut(o, k == rio_TypeKind_float ? "f" : type == rio_TypeId_string ? "s" : type == rio_TypeId_blob ? "b" : "i", 1);
  }
  rio_hostPut(o, open ? "\"" : any ? "" : "\"\"", -1);
}

uint32_t rio_hostHeader(rio_Vm *vm, const char *src, uint32_t len, void *scratch, uint32_t scratchSize, const char *prefix,
                        char *out, uint32_t cap) {
  rio_Compiler *c;
  rio_HostText o;
  uint16_t done[256];
  int i, j, doneCount = 0, failed;
  vm->hostHeader = 1;
  failed = rio_compileScratch(vm, src, len, scratch, scratchSize);
  vm->hostHeader = 0;
  if (failed) return 0;
  c = vm->compiler;
  memset(&o, 0, sizeof o);
  o.out = out;
  o.cap = cap;
  rio_hostPut(&o, "/* Made by rio_hostHeader (rio -H) from the Rio module that declares these host procs: don't edit.\n"
              "   A struct parameter comes as 2 words: rio_getStruct(vm, a + i, &s, sizeof s) copies it into s.\n"
              "   Register the module's source with its procs (_SOURCE, below: rio_addSource), and each proc with\n"
              "   its _SIG (_HOST_PROCS lists them all). */\n"
              "#include <stdint.h>\n#include \"rio.h\"\n\n", -1);
  for (i = 0; i < c->symCount; i++) { /* the structs the host procs take and return */
    rio_Symbol *y = &c->syms[i];
    rio_Type *ty;
    if (y->kind != rio_SymKind_ffi || !y->type) continue;
    ty = rio_type(vm, y->type);
    for (j = 0; j < ty->fieldCount; j++) rio_cStruct(vm, &o, prefix, rio_field(vm, ty->firstField + j)->type, done, &doneCount);
    rio_cStruct(vm, &o, prefix, ty->elem, done, &doneCount);
    if (rio_type(vm, ty->elem)->kind == rio_TypeKind_union) rio_cResult(vm, &o, prefix, y, ty->elem);
  }
  for (i = 0; i < c->symCount; i++) { /* their sigs */
    rio_Symbol *y = &c->syms[i];
    if (y->kind != rio_SymKind_ffi || !y->type) continue;
    rio_hostPut(&o, "#define ", -1);
    rio_hostPut(&o, prefix, -1);
    rio_hostPut(&o, "_", 1);
    rio_hostPut(&o, c->names + y->name, y->nameLen);
    rio_hostPut(&o, "_SIG ", -1);
    rio_cSig(vm, &o, prefix, y->type);
    rio_hostPut(&o, "\n", 1);
  }
  rio_hostPut(&o, "\n#define ", -1); /* the host procs, for registering: X(name, sig) each */
  rio_hostPut(&o, prefix, -1);
  rio_hostPut(&o, "_HOST_PROCS(X)", -1);
  for (i = 0; i < c->symCount; i++) {
    rio_Symbol *y = &c->syms[i];
    if (y->kind != rio_SymKind_ffi || !y->type) continue;
    rio_hostPut(&o, " \\\n  X(", -1);
    rio_hostPut(&o, c->names + y->name, y->nameLen);
    rio_hostPut(&o, ", ", 2);
    rio_hostPut(&o, prefix, -1);
    rio_hostPut(&o, "_", 1);
    rio_hostPut(&o, c->names + y->name, y->nameLen);
    rio_hostPut(&o, "_SIG)", -1);
  }
  { /* the module's source, for rio_addSource(vm, "pc", pc_SOURCE, sizeof pc_SOURCE - 1), and its hash */
    uint32_t k, h = rio_hashText(src, len);
    char hex[11];
    rio_hostPut(&o, "\n\n#define ", -1);
    rio_hostPut(&o, prefix, -1);
    rio_hostPut(&o, "_HASH 0x", -1);
    for (k = 0; k < 8; k++) hex[k] = "0123456789abcdef"[h >> (28 - 4 * k) & 15];
    rio_hostPut(&o, hex, 8);
    rio_hostPut(&o, "u /* (rio_hashText of the source) */\n#define ", -1);
    rio_hostPut(&o, prefix, -1);
    rio_hostPut(&o, "_SOURCE \\\n  \"", -1);
    for (k = 0; k < len; k++) {
      unsigned char ch = (unsigned char)src[k];
      if (ch == '\n') rio_hostPut(&o, k + 1 < len ? "\\n\" \\\n  \"" : "\\n", -1);
      else if (ch == '\\' || ch == '"') {
        char e[2];
        e[0] = '\\';
        e[1] = (char)ch;
        rio_hostPut(&o, e, 2);
      } else if (ch == '\t') rio_hostPut(&o, "\\t", 2);
      else if (ch < 32 || ch >= 127) { /* (as octal: never run into a digit after it, as hex would) */
        char e[4];
        e[0] = '\\';
        e[1] = (char)('0' + (ch >> 6));
        e[2] = (char)('0' + (ch >> 3 & 7));
        e[3] = (char)('0' + (ch & 7));
        rio_hostPut(&o, e, 4);
      } else if (ch != '\r') rio_hostPut(&o, (const char *)&src[k], 1);
    }
    rio_hostPut(&o, "\"\n", -1);
  }
  if (o.n >= cap) {
    rio_setError(vm, rio_ErrorKind_compile, 0, 0, 0, "rio_hostHeader: too little room for the header");
    return 0;
  }
  out[o.n] = 0;
  return o.n;
}
