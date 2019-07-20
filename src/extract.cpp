#include "main.h"

namespace rio {

struct ExtractState {

  List<Def*> defs;
  ModManager* mod;

  auto alloc(string name, Node* node) -> Def* {
    Def* def = &mod->arena.alloc<Def>();
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
  void* items = state->mod->arena.alloc_bytes(nbytes);
  // Copy it in.
  memcpy(items, buf.items + buf_len_old, nbytes);
  return {static_cast<Def**>(items), len};
}

void extract(ModManager* mod) {
  assert(mod->tree->kind == Node::Kind::Block);
  ExtractState state;
  state.mod = mod;
  extract_block(&state, mod->tree);
  // TODO Put mod-level ids into a pointer-keyed Map.
}

void extract_block(ExtractState* state, Node* node) {
  auto buf_len_old = state->defs.len;
  for (auto item: node->Block.items) {
    extract_expr(state, item);
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
      for (auto item: node->Tuple.items) {
        extract_expr(state, item);
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
