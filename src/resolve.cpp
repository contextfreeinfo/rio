#include "main.h"

namespace rio {

struct ResolveState {
  Engine* engine;
  ModManager* mod;
};

Type choose_float_type(const Type& type);
Type choose_int_type(const Type& type);
void resolve_block(ResolveState* state, Node* node, const Type& type);
void resolve_expr(ResolveState* state, Node* node, const Type& type);
void resolve_tuple(ResolveState* state, Node* node, const Type& type);

void resolve(Engine* engine, ModManager* mod) {
  ResolveState state;
  state.engine = engine;
  state.mod = mod;
  // TODO Build symbol map (with uses) at each root for faster resolution?
  if (mod->root) {
    if (verbose) printf("root: %s\n", mod->root->file);
    for (auto part: mod->root->parts) {
      if (verbose) printf("part: %s\n", part->file);
    }
  }
  resolve_block(&state, mod->tree, {Type::Kind::None});
}

Type choose_float_type(const Type& type) {
  // TODO Explicit suffixes on literals.
  switch (type.kind) {
    case Type::Kind::F32:
    case Type::Kind::F64: {
      return type;
    }
    default: {
      // TODO Default to F32 or F64? C, Java, Python, JS are all F64.
      // TODO What are Rust, Zig, and Odin?
      return {Type::Kind::F64};
    }
  }
}

Type choose_int_type(const Type& type) {
  // TODO Explicit suffixes on literals.
  switch (type.kind) {
    case Type::Kind::I8:
    case Type::Kind::I16:
    case Type::Kind::I32:
    case Type::Kind::I64:
    case Type::Kind::ISize:
    case Type::Kind::Int:
    case Type::Kind::U8:
    case Type::Kind::U16:
    case Type::Kind::U32:
    case Type::Kind::U64:
    case Type::Kind::USize: {
      return type;
    }
    default: {
      return {Type::Kind::I32};
    }
  }
}

void resolve_block(ResolveState* state, Node* node, const Type& type) {
  for (auto item: node->Block.items) {
    resolve_expr(state, item, {Type::Kind::None});
  }
}

void resolve_expr(ResolveState* state, Node* node, const Type& type) {
  switch (node->kind) {
    case Node::Kind::Block: {
      resolve_block(state, node, {Type::Kind::None});
      break;
    }
    case Node::Kind::Call: {
      // Left over from gen.
      // gen_expr(state, *node.Call.callee);
      // gen_tuple_items(state, *node.Call.args);
      break;
    }
    case Node::Kind::Cast: {
      Node* type_node = node->Cast.b;
      // TODO Actual resolution of type names.
      // TODO Global outer map on interned pointers to types.
      if (type_node->kind == Node::Kind::Ref) {
        string name = type_node->Ref.name;
        if (!strcmp(name, "i32")) {
          node->type = {Type::Kind::I32};
        } else if (!strcmp(name, "string")) {
          node->type = {Type::Kind::String};
        }
      }
      break;
    }
    case Node::Kind::Const: {
      // TODO First see if side `a` declares an explicit type.
      if (node->Const.a->type.kind == Type::Kind::None) {
        // Infer from value to def.
        resolve_expr(state, node->Const.b, {Type::Kind::None});
        node->Const.a->type = node->Const.b->type;
      }
      break;
    }
    case Node::Kind::Float: {
      node->type = choose_float_type(type);
      break;
    }
    case Node::Kind::Fun: {
      if (!strcmp(node->Fun.name, "main")) {
        // TODO Instead have a global main call a package main.
        // TODO Package main can return void or other ints.
        node->type = {Type::Kind::Int};
      } else {
        // TODO Actual types on functions.
        node->type = {Type::Kind::Void};
      }
      // TODO If in a local expression, expected type might be given.
      resolve_tuple(state, node->Fun.params, {Type::Kind::None});
      resolve_expr(state, node->Fun.expr, {Type::Kind::None});
      break;
    }
    case Node::Kind::Int: {
      node->type = choose_int_type(type);
      break;
    }
    case Node::Kind::Ref: {
      // TODO Look it up.
      // TODO If we haven't resolved it yet, jump to it???
      // TODO Leave a work queue of unresolved things???
      break;
    }
    case Node::Kind::String: {
      // TODO Be able to infer integer or slice types, too.
      // TODO Default to slice rather than *u8(*, 0)?
      // TODO Explicit suffixes on literals.
      node->type = {Type::Kind::String};
      break;
    }
    default: {
      // printf("(!!! BROKEN %d !!!)", static_cast<int>(node.kind));
      break;
    }
  }
}

void resolve_tuple(ResolveState* state, Node* node, const Type& type) {
  for (auto item: node->Tuple.items) {
    resolve_expr(state, item, {Type::Kind::None});
  }
}

}
