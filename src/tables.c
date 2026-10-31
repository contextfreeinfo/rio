/* tables.c - the compiler's tables of names, symbols, types and fields. */
#include "compiler.h"
#ifndef rio_NO_COMPILER

/* ------------------------------------------------------------------ names and symbols */

/* copies a name into the names table; returns where */
int rio_addName(rio_Vm *vm, const char *s, int n) {
  rio_Compiler *c = vm->compiler;
  if (c->nameBytes + n > (int)c->limits.nameBytes) rio_FAIL(0, "out of name space");
  memcpy(c->names + c->nameBytes, s, (size_t)n);
  c->nameBytes += n;
  if ((uint32_t)c->nameBytes > c->high.nameBytes) c->high.nameBytes = (uint32_t)c->nameBytes;
  return c->nameBytes - n;
}

void rio_addSymbol(rio_Vm *vm, const char *s, int n, int kind, int type, int32_t value) {
  rio_Compiler *c = vm->compiler;
  rio_Symbol *y;
  if (c->symCount >= (int)c->limits.symbols) rio_FAIL(, "too many symbols");
  y = &c->syms[c->symCount++];
  if ((uint32_t)c->symCount > c->high.symbols) c->high.symbols = (uint32_t)c->symCount;
  y->name = (uint16_t)rio_addName(vm, s, n);
  y->nameLen = (uint16_t)n;
  y->kind = (uint8_t)kind;
  y->type = (uint16_t)type;
  y->value = value;
  y->module = (uint16_t)c->curModule;
  y->exported = 0;
  if (c->exporting && !c->blockCount && c->curProc < 0) { /* name* :: ... */
    y->exported = 1;
    c->exporting = 0;
  }
}

/* a name in scope: the newest of the current module's or the built-in ones, methods aside */
int rio_lookup(rio_Vm *vm, const char *s, int n) {
  rio_Compiler *c = vm->compiler;
  int i;
  for (i = c->symCount - 1; i >= 0; i--) {
    rio_Symbol *y = &c->syms[i];
    if (y->kind != rio_SymKind_method && (y->module == c->curModule || !y->module) && rio_nameIs(vm, y->name, y->nameLen, s, n))
      return i;
  }
  return -1;
}

/* an exported name of module m */
int rio_lookupExport(rio_Vm *vm, int module, const char *s, int n) {
  rio_Compiler *c = vm->compiler;
  int i;
  if (!module) { /* core.name: a builtin, even one a program's own name hides */
    for (i = c->coreSymCount - 1; i >= 0; i--)
      if (c->syms[i].kind != rio_SymKind_method && rio_nameIs(vm, c->syms[i].name, c->syms[i].nameLen, s, n)) return i;
    return -1;
  }
  for (i = c->symCount - 1; i >= 0; i--) {
    rio_Symbol *y = &c->syms[i];
    if (y->module == module && y->exported && y->kind != rio_SymKind_method && rio_nameIs(vm, y->name, y->nameLen, s, n))
      return i;
  }
  return -1;
}

/* a method of type t: this module's own, or an exported one (exported methods travel with their type) */
int rio_lookupMethod(rio_Vm *vm, int type, const char *s, int n) {
  rio_Compiler *c = vm->compiler;
  int i;
  for (i = c->symCount - 1; i >= 0; i--) {
    rio_Symbol *y = &c->syms[i];
    if (y->kind == rio_SymKind_method && y->type == type && (y->module == c->curModule || y->exported) &&
        rio_nameIs(vm, y->name, y->nameLen, s, n))
      return i;
  }
  if (rio_type(vm, type)->kind == rio_TypeKind_struct && rio_type(vm, type)->elem) /* (a generic struct filled in: its template's) */
    return rio_lookupMethod(vm, rio_genericOf(vm, rio_type(vm, type)->elem), s, n);
  return -1;
}

/* the generic type whose template is tmpl */
int rio_genericOf(rio_Vm *vm, int tmpl) {
  int i;
  for (i = rio_TypeId_builtinCount; i < vm->compiler->typeCount; i++)
    if (rio_type(vm, i)->kind == rio_TypeKind_generic && rio_type(vm, i)->elem == tmpl) return i;
  return 0;
}

/* ------------------------------------------------------------------ types */

/* a new field, at the end of the table; -1 if it's full */
int rio_newField(rio_Vm *vm) {
  rio_Compiler *c = vm->compiler;
  if (c->fieldCount >= (int)c->limits.fields) rio_FAIL(-1, "too many fields");
  return c->fieldCount++;
}

int rio_newType(rio_Vm *vm, int kind, int elem, uint32_t count, uint32_t size) {
  rio_Compiler *c = vm->compiler;
  rio_Type *y;
  if (c->typeCount >= (int)c->limits.types) rio_FAIL(0, "too many types");
  y = &c->types[c->typeCount];
  y->kind = (uint8_t)kind;
  y->elem = (uint16_t)elem;
  y->count = count;
  y->size = size;
  y->firstField = y->fieldCount = 0;
  y->hasRefs = (uint8_t)(kind == rio_TypeKind_slice || kind == rio_TypeKind_builderView ||
                         ((kind == rio_TypeKind_array || kind == rio_TypeKind_builder) && c->types[elem].hasRefs));
  return c->typeCount++;
}

/* Types are made once: each of these finds the type if it exists. The built-in ones come first, and none
   of them is made this way. */
#define rio_FOR_MADE_TYPES(i) for (i = rio_TypeId_builtinCount; i < vm->compiler->typeCount; i++)

int rio_sliceOf(rio_Vm *vm, int elem) {
  int i;
  if (elem == rio_TypeId_byte) return rio_TypeId_blob;
  rio_FOR_MADE_TYPES(i) {
    rio_Type *y = rio_type(vm, i);
    if (y->kind == rio_TypeKind_slice && y->elem == elem) return i;
  }
  return rio_newType(vm, rio_TypeKind_slice, elem, 0, 8);
}

int rio_arrayOf(rio_Vm *vm, int elem, uint32_t count) {
  uint32_t elemSize = rio_type(vm, elem)->size;
  int i;
  rio_FOR_MADE_TYPES(i) {
    rio_Type *y = rio_type(vm, i);
    if (y->kind == rio_TypeKind_array && y->elem == elem && y->count == count && !y->firstField) return i;
  }
  if (elemSize && count > 0x7FFFFFF0u / elemSize) rio_FAIL(0, "array too large");
  return rio_newType(vm, rio_TypeKind_array, elem, count, rio_align4(elemSize * count));
}

/* [Enum]T: one element per value of the enum, indexed by it (firstField: the enum). An enum's values
   are 0, 1, 2...: d.toInt() is d's element */
int rio_enumArrayOf(rio_Vm *vm, int elem, int enumType) {
  rio_Type *e = rio_type(vm, enumType);
  uint32_t elemSize = rio_type(vm, elem)->size;
  int i;
  rio_FOR_MADE_TYPES(i) {
    rio_Type *y = rio_type(vm, i);
    if (y->kind == rio_TypeKind_array && y->elem == elem && y->firstField == enumType) return i;
  }
  if (elemSize && e->fieldCount > 0x7FFFFFF0u / elemSize) rio_FAIL(0, "array too large");
  i = rio_newType(vm, rio_TypeKind_array, elem, e->fieldCount, rio_align4(elemSize * e->fieldCount));
  rio_type(vm, i)->firstField = (uint16_t)enumType;
  return i;
}

/* proc types: proc(T, U) -> R. Each parameter type is a field; elem is the result (void: none); count,
   how many of the last parameters are optional. A proc value is the proc's index + 1, so a value never
   set (0) is caught when called */
int rio_procTypeOf(rio_Vm *vm, const int *params, int n, int result, int optional, int refs) {
  rio_Compiler *c = vm->compiler;
  int i, j;
  rio_FOR_MADE_TYPES(i) {
    rio_Type *y = rio_type(vm, i);
    if (y->kind != rio_TypeKind_proc || y->fieldCount != n || y->elem != result || y->count != (uint32_t)optional) continue;
    for (j = 0; j < n && rio_field(vm, y->firstField + j)->type == params[j] && rio_field(vm, y->firstField + j)->offset == (refs >> j & 1); j++) {}
    if (j == n) return i;
  }
  i = rio_newType(vm, rio_TypeKind_proc, result, (uint32_t)optional, 4);
  rio_type(vm, i)->firstField = (uint16_t)c->fieldCount;
  for (j = 0; j < n; j++) {
    int f = rio_newField(vm);
    rio_Field *field;
    if (f < 0) return 0;
    field = rio_field(vm, f);
    field->name = 0;
    field->nameLen = 0;
    field->type = (uint16_t)params[j];
    field->offset = (uint16_t)(refs >> j & 1); /* (a &T parameter) */
  }
  rio_type(vm, i)->fieldCount = (uint16_t)n;
  return i;
}

/* [..N]T: a length word followed by N elements */
int rio_builderOf(rio_Vm *vm, int elem, uint32_t count) {
  uint32_t elemSize = rio_type(vm, elem)->size;
  int i;
  rio_FOR_MADE_TYPES(i) {
    rio_Type *y = rio_type(vm, i);
    if (y->kind == rio_TypeKind_builder && y->elem == elem && y->count == count) return i;
  }
  if (elemSize && count > 0x7FFFFFF0u / elemSize) rio_FAIL(0, "builder too large");
  return rio_newType(vm, rio_TypeKind_builder, elem, count, rio_align4(4 + elemSize * count));
}

/* [..]T: a builder view, a view of a length word and the memory it counts: (address of the length word,
   address of the data, capacity) */
int rio_builderViewOf(rio_Vm *vm, int elem) {
  int i;
  rio_FOR_MADE_TYPES(i) {
    rio_Type *y = rio_type(vm, i);
    if (y->kind == rio_TypeKind_builderView && y->elem == elem) return i;
  }
  return rio_newType(vm, rio_TypeKind_builderView, elem, 0, 12);
}

#undef rio_FOR_MADE_TYPES

/* a struct's field by name: its index among the struct's fields, or -1 */
int rio_findField(rio_Vm *vm, const rio_Type *type, const char *s, int n) {
  int i;
  for (i = 0; type->kind == rio_TypeKind_struct && i < type->fieldCount; i++) {
    rio_Field *f = rio_field(vm, type->firstField + i);
    if (rio_nameIs(vm, f->name, f->nameLen, s, n)) return i;
  }
  return -1;
}

/* an enum's value by name, into *value; 0 if there's none */
int rio_findEnumValue(rio_Vm *vm, int type, const char *s, int n, int32_t *value) {
  rio_Type *e = rio_type(vm, type);
  int i;
  for (i = 0; i < e->fieldCount; i++) {
    rio_Field *f = rio_field(vm, e->firstField + i);
    if (rio_nameIs(vm, f->name, f->nameLen, s, n)) {
      *value = rio_enumValue(f);
      return 1;
    }
  }
  return 0;
}

/* where a value is in its enum's list, or -1 */
int rio_enumIndex(rio_Vm *vm, int type, int32_t value) {
  rio_Type *e = rio_type(vm, type);
  int i;
  for (i = 0; i < e->fieldCount; i++)
    if (rio_enumValue(rio_field(vm, e->firstField + i)) == value) return i;
  return -1;
}

/* arrays, structs, builders and unions: values stored as a block of memory */
int rio_isViewKind(int kind) { return kind == rio_TypeKind_slice || kind == rio_TypeKind_builderView; }

int rio_isAggregate(int kind) {
  return kind == rio_TypeKind_array || kind == rio_TypeKind_struct || kind == rio_TypeKind_builder || kind == rio_TypeKind_union;
}

/* an aggregate too big to copy without being asked */
int rio_isBig(const rio_Type *type) { return rio_isAggregate(type->kind) && type->size > rio_SMALL_WORDS * 4; }

#endif
