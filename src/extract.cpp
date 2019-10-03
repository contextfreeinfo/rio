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

auto def_slice_copy(ExtractState* state, rint buf_len_old) -> Slice<Def*>;
void extract_block(ExtractState* state, Node* node);
void extract_ref_names(
  ExtractState* state, Node* node, Node* top, bool explicit_only = false
);
void extract_expr(ExtractState* state, Node* node);
void extract_fun(ExtractState* state, Node* node);

struct Scoper {
  Scoper(ExtractState* state, Scope* scope) {
    buf_len_old = state->defs.len;
    this->scope = scope;
    this->state = state;
  }
  ~Scoper() {
    scope->defs = def_slice_copy(state, buf_len_old);
  }
  rint buf_len_old;
  Scope* scope;
  ExtractState* state;
};

// TODO Unify with node_slice_copy?
auto def_slice_copy(ExtractState* state, rint buf_len_old) -> Slice<Def*> {
  // Prep space.
  auto& buf = state->defs;
  rint len = buf.len - buf_len_old;
  if (!len) {
    return {nullptr, 0};
  }
  buf.len = buf_len_old;
  rint nbytes = len * sizeof(*buf.items);
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
  Scoper scoper{state, &node->Block.scope};
  auto top = node == state->mod->tree;
  for (auto item: node->Block.items) {
    extract_expr(state, item);
    // Top-level block is main.
    if (top && item->kind == Node::Kind::Block) {
      state->alloc_push("__main__", item);
    }
  }
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
    case Node::Kind::Case: {
      extract_expr(state, node->Case.arg);
      extract_expr(state, node->Case.expr);
      break;
    }
    case Node::Kind::Cast: {
      extract_ref_names(state, node->Const.a, node);
      // TODO Extract from ad hoc structural types?
      break;
    }
    case Node::Kind::Const:
    case Node::Kind::Update: {
      extract_ref_names(
        state, node->Const.a, node, node->kind == Node::Kind::Update
      );
      extract_expr(state, node->Const.b);
      break;
    }
    case Node::Kind::Else: {
      extract_expr(state, node->Else.expr);
      break;
    }
    case Node::Kind::Equal:
    case Node::Kind::Less:
    case Node::Kind::LessOrEqual:
    case Node::Kind::More:
    case Node::Kind::MoreOrEqual:
    case Node::Kind::NotEqual: {
      extract_expr(state, node->Binary.a);
      extract_expr(state, node->Binary.b);
      break;
    }
    case Node::Kind::For: {
      Scoper scoper(state, &node->For.scope);
      if (node->For.param) {
        extract_ref_names(state, node->For.param, node);
      }
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
    case Node::Kind::Switch: {
      if (node->Switch.arg) extract_expr(state, node->Switch.arg);
      // TODO Some kind of scope for switch? Make it into a block?
      for (auto item: node->Switch.items) {
        extract_expr(state, item);
      }
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
    if (verbose) fprintf(stderr, "fun: %s\n", node->Fun.name);
    state->alloc_push(node->Fun.name, node);
  }
  auto expr = node->Fun.expr;
  switch (node->kind) {
    case Node::Kind::Fun:
    case Node::Kind::Proc: {
      Scoper scoper{state, &node->Fun.scope};
      // First params.
      if (node->Fun.params && node->Fun.params->kind == Node::Kind::Tuple) {
        // For now, special case this.
        // TODO Will tuple instance literals only create symbols if const defs
        // TODO inside?
        for (auto item: node->Fun.params->Tuple.items) {
          switch (item->kind) {
            case Node::Kind::Cast: {
              extract_ref_names(state, item->Cast.a, node);
              break;
            }
            default: break;
          }
        }
      }
      // Now the body.
      extract_expr(state, expr);
      break;
    }
    case Node::Kind::Struct: {
      // No params here, and hoist up the block to the upper level for
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

void extract_ref_names(
  ExtractState* state, Node* node, Node* top, bool explicit_only
) {
  switch (node->kind) {
    case Node::Kind::Cast: {
      extract_ref_names(state, node->Cast.a, top, explicit_only);
      break;
    }
    case Node::Kind::Ref: {
      if (!explicit_only) {
        if (verbose) fprintf(stderr, "const/var: %s\n", node->Ref.name);
        node->Ref.def = state->alloc_push(node->Ref.name, node);
        node->Ref.def->top = top;
      }
      break;
    }
    case Node::Kind::Var: {
      // Been made explicit, so explicit lower not needed.
      extract_ref_names(state, node->Var.expr, top, false);
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
