#include "main.h"

namespace rio {

struct ResolveState {
  Engine* engine;
  ModManager* mod;
  // TODO A stack of locals.
};

Type choose_float_type(const Type& type);
Type choose_int_type(const Type& type);
auto ensure_global_refs(ModManager* mod) -> void;
void resolve_array(ResolveState* state, Node* node, const Type& type);
void resolve_block(ResolveState* state, Node* node, const Type& type);
void resolve_expr(ResolveState* state, Node* node, const Type& type);
auto resolve_ref(ResolveState* state, Node* node) -> void;
void resolve_tuple(ResolveState* state, Node* node, const Type& type);

void resolve(Engine* engine, ModManager* mod) {
  ResolveState state;
  state.engine = engine;
  state.mod = mod->root;
  // Ensure globals and resolve.
  ensure_global_refs(mod);
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
      // TODO Default to F32 or F64? C, Java, JS, Python, Rust are all F64.
      // TODO What are Nim, Zig, and Odin?
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

auto ensure_global_refs(ModManager* mod) -> void {
  auto& globals = mod->root->global_refs;
  // See if we haven't built the global refs for this multimod.
  // TODO Just force the root first so we don't need to bother checking?
  if (globals.len()) {
    return;
  }
  // First get all globals from this mod, since they have priority.
  for (auto global: mod->root->global_defs) {
    auto insert = globals.get_or_insert(global->name);
    if (insert.inserted) {
      // No conflict. Good.
      if (verbose) printf("internal global: %s\n", global->name);
      insert.pair->value = global;
    } else {
      // Conflict. Clear it out.
      if (verbose) printf("internal conflict: %s\n", global->name);
      insert.pair->value = nullptr;
    }
  }
  // Then get all globals from use imports.
  for (auto import: mod->root->uses) {
    for (auto& global: import->global_defs) {
      auto insert = globals.get_or_insert(global->name);
      if (insert.inserted) {
        // No conflict. Good.
        if (verbose) printf("external global: %s\n", global->name);
        insert.pair->value = global;
      } else {
        // Conflict. See if it's from the current mod or not.
        if (insert.pair->value->mod == mod) {
          if (verbose) printf("external ignored: %s\n", global->name);
        } else {
          // From somewhere else. Clear it.
          if (verbose) printf("external conflict: %s\n", global->name);
          insert.pair->value = nullptr;
        }
      }
    }
  }
}

void resolve_array(ResolveState* state, Node* node, const Type& type) {
  for (auto item: node->Array.items) {
    resolve_expr(state, item, {Type::Kind::None});
  }
  if (node->Array.items.len) {
    // TODO Fuse the types, etc.
    node->type = {Type::Kind::Array, &node->Array.items[0]->type};
  }
}

void resolve_block(ResolveState* state, Node* node, const Type& type) {
  // TODO The last item should expect the block type.
  for (auto item: node->Block.items) {
    resolve_expr(state, item, {Type::Kind::None});
  }
}

void resolve_expr(ResolveState* state, Node* node, const Type& type) {
  switch (node->kind) {
    case Node::Kind::Array: {
      resolve_array(state, node, type);
      break;
    }
    case Node::Kind::Block: {
      resolve_block(state, node, type);
      break;
    }
    case Node::Kind::Call: {
      // TODO The expected type should be some kind of function.
      // TODO Does this matter?
      // TODO And the return type should be the expected expr type.
      resolve_expr(state, node->Call.callee, {Type::Kind::None});
      // TODO Definitely should have expected types for the args at this point.
      resolve_expr(state, node->Call.args, {Type::Kind::None});
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
      resolve_ref(state, node);
      break;
    }
    case Node::Kind::String: {
      // TODO Be able to infer integer or slice types, too.
      // TODO Default to slice rather than *u8(*, 0)?
      // TODO Explicit suffixes on literals.
      node->type = {Type::Kind::String};
      break;
    }
    case Node::Kind::Tuple: {
      resolve_tuple(state, node, type);
      break;
    }
    default: {
      // printf("(!!! BROKEN %d !!!)", static_cast<int>(node.kind));
      break;
    }
  }
}

auto resolve_ref(ResolveState* state, Node* node) -> void {
  // TODO If we haven't resolved its subparts yet, jump to it???
  // TODO Leave a work queue of unresolved things???
  // TODO Look through local stack before globals.
  node->Ref.def = state->mod->global_refs.get(node->Ref.name);
}

void resolve_tuple(ResolveState* state, Node* node, const Type& type) {
  for (auto item: node->Tuple.items) {
    resolve_expr(state, item, {Type::Kind::None});
  }
}

}
