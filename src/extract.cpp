#include "main.h"

namespace rio {

struct ExtractState {

  List<Def*> defs;
  Engine* engine;

  auto alloc(string name, Node* node) -> Def* {
    Def* def = &engine->arena.alloc<Def>();
    def->name = name;
    def->node = node;
    def->value = nullptr;
    return def;
  }

  auto alloc_push(string name, Node* node) -> Def* {
    auto def = alloc(name, node);
    defs.push_val(def);
    return def;
  }

};

void extract_block(ExtractState* state, Node* node);
void extract_ref_names(ExtractState* state, Node* node, Node* value);
void extract_expr(ExtractState* state, Node* node);
void extract_fun(ExtractState* state, Node* node);

// TODO Unify with node_slice_copy?
auto def_slice_copy(ExtractState* state, usize buf_len_old) -> Slice<Def*> {
  // Prep space.
  auto& buf = state->defs;
  usize len = buf.len - buf_len_old;
  buf.len = buf_len_old;
  usize nbytes = len * sizeof(*buf.items);
  void* items = state->engine->arena.alloc_bytes(nbytes);
  // Copy it in.
  memcpy(items, buf.items + buf_len_old, nbytes);
  return {static_cast<Def**>(items), len};
}

void extract(Engine* engine, Node* tree) {
  assert(tree->kind == Node::Kind::Block);
  ExtractState state;
  state.engine = engine;
  extract_block(&state, tree);
  // TODO Put top level (only across all package files?) into a Map.
}

void extract_block(ExtractState* state, Node* node) {
  auto buf_len_old = state->defs.len;
  auto items = node->Block.items;
  for (usize i = 0; i < items.len; i += 1) {
    extract_expr(state, items[i]);
  }
  node->Block.scope.defs = def_slice_copy(state, buf_len_old);
}

void extract_expr(ExtractState* state, Node* node) {
  switch (node->kind) {
    case Node::Kind::Block: {
      extract_block(state, node);
      break;
    }
    case Node::Kind::Call: {
      extract_expr(state, node->Call.callee);
      extract_expr(state, node->Call.args);
      break;
    }
    case Node::Kind::Const: {
      extract_ref_names(state, node->Const.a, node->Const.b);
      extract_expr(state, node->Const.b);
      break;
    }
    case Node::Kind::Fun: {
      extract_fun(state, node);
      break;
    }
    case Node::Kind::Tuple: {
      // Could be blocks under here with their own scopes.
      auto items = node->Tuple.items;
      for (usize i = 0; i < items.len; i += 1) {
        extract_expr(state, items[i]);
      }
      break;
    }
    default: {
      // Nothing yet to do for others.
      break;
    }
  }
}

void extract_fun(ExtractState* state, Node* node) {
  if (*node->Fun.name) {
    if (verbose) printf("fun: %s\n", node->Fun.name);
    state->alloc_push(node->Fun.name, node);
  }
  // TODO Params in own scope.
  extract_expr(state, node->Fun.expr);
}

void extract_ref_names(ExtractState* state, Node* node, Node* value) {
  switch (node->kind) {
    case Node::Kind::Ref: {
      if (verbose) printf("const/var: %s\n", node->Ref.name);
      node->Ref.def = state->alloc_push(node->Ref.name, node);
      node->Ref.def->value = value;
      break;
    }
    default: {
      // TODO Destructuring.
      // TODO Add new expressions to reach inside for values.
      break;
    }
  }
}

}
