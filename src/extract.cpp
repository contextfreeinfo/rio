#include "main.h"

namespace rio {

struct ExtractState {

  List<Def*> defs;
  ModManager* mod;

  auto alloc(string name, Node* node) -> Def* {
    Def* def = &mod->arena.alloc<Def>();
    def->name = name;
    def->node = node;
    def->top = node;
    return def;
  }

  auto alloc_push(string name, Node* node) -> Def* {
    auto def = alloc(name, node);
    defs.push_val(def);
    return def;
  }

};

void extract_block(ExtractState* state, Node* node);
void extract_ref_names(ExtractState* state, Node* node, Node* top);
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
  // Put mod globals into the mod root globals list.
  auto root = mod->root;
  for (auto def: mod->tree->Block.scope.defs) {
    // fprintf(stderr, "Global def in %s: %s\n", root->name, def->name);
    def->mod = root;
    root->global_defs.push(def);
  }
}

void extract_block(ExtractState* state, Node* node) {
  auto top = node == state->mod->tree;
  auto buf_len_old = state->defs.len;
  for (auto item: node->Block.items) {
    extract_expr(state, item);
    // Top-level block is main.
    if (top && item->kind == Node::Kind::Block) {
      state->alloc_push("__main__", item);
    }
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
      extract_ref_names(state, node->Const.a, node);
      extract_expr(state, node->Const.b);
      break;
    }
    case Node::Kind::For: {
      extract_expr(state, node->For.arg);
      extract_expr(state, node->For.expr);
      break;
    }
    case Node::Kind::Fun:
    case Node::Kind::Proc:
    case Node::Kind::Struct: {
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
    default: break;
  }
}

void extract_fun(ExtractState* state, Node* node) {
  if (*node->Fun.name) {
    if (verbose) printf("fun: %s\n", node->Fun.name);
    state->alloc_push(node->Fun.name, node);
  }
  auto expr = node->Fun.expr;
  switch (node->kind) {
    case Node::Kind::Fun:
    case Node::Kind::Proc: {
      // TODO Params in own scope.
      extract_expr(state, expr);
      break;
    }
    case Node::Kind::Struct: {
      // No params here, and host up the block to the upper level for
      // convenience.
      if (node->Fun.expr->kind == Node::Kind::Block) {
        extract_block(state, expr);
        node->Fun.scope = expr->Block.scope;
        expr->Block.scope.forget();
      }
      break;
    }
    default: break;
  }
}

void extract_ref_names(ExtractState* state, Node* node, Node* top) {
  switch (node->kind) {
    case Node::Kind::Ref: {
      if (verbose) printf("const/var: %s\n", node->Ref.name);
      node->Ref.def = state->alloc_push(node->Ref.name, node);
      node->Ref.def->top = top;
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
