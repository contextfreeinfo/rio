#include "main.h"

namespace rio {

// TODO Rename this pass from "transform" to "express"?
struct TransformState {
  ModManager* mod;
};

// TODO Walk trees to find expressions that need to be statements.
// TODO Pull out the last one to be executed, then also pull out others that
// TODO would execute before it.

auto transform_block(
  TransformState* state, Node* node, bool can_return = false
) -> void;
auto transform_expr(
  TransformState* state, Node* node, bool can_return = false
) -> void;

auto transform(ModManager* mod) -> void {
  TransformState state;
  state.mod = mod;
  for (auto def: mod->global_defs) {
    transform_expr(&state, def->top);
  }
}

auto transform_block(
  TransformState* state, Node* node, bool can_return
) -> void {
  auto& items = node->Block.items;
  bool last = true;
  for (rint i = items.len - 1; i >= 0; i -= 1) {
    auto item = items[i];
    transform_expr(state, item, can_return && last);
    if (last) {
      if (can_return && !is_voidish(item->type.kind)) {
        switch (item->kind) {
          case Node::Kind::Block:
          case Node::Kind::Switch: {
            // Skip these.
            break;
          }
          default: {
            Node* ret_node = &state->mod->arena.alloc<Node>();
            ret_node->kind = Node::Kind::Return;
            ret_node->Return.expr = item;
            items[i] = ret_node;
          }
        }
      }
      last = false;
    }
  }
}

auto transform_expr(
  TransformState* state, Node* node, bool can_return
) -> void {
  switch (node->kind) {
    case Node::Kind::Array:
    case Node::Kind::Map:
    case Node::Kind::Tuple: {
      auto& items = node->Parent.items;
      for (rint i = items.len - 1; i >= 0; i -= 1) {
        transform_expr(state, items[i]);
      }
      break;
    }
    case Node::Kind::Block: {
      transform_block(state, node, can_return);
      break;
    }
    case Node::Kind::Call: {
      transform_expr(state, node->Call.callee);
      transform_expr(state, node->Call.args);
      break;
    }
    case Node::Kind::Case:
    case Node::Kind::For: {
      // TODO Transform if non-void.
      transform_expr(state, node->For.arg);
      transform_expr(state, node->For.expr);
      break;
    }
    case Node::Kind::Cast: {
      transform_expr(state, node->Cast.a);
      break;
    }
    case Node::Kind::Const: {
      transform_expr(state, node->Cast.b);
      break;
    }
    case Node::Kind::Equal:
    case Node::Kind::Less:
    case Node::Kind::LessOrEqual:
    case Node::Kind::More:
    case Node::Kind::MoreOrEqual:
    case Node::Kind::NotEqual: {
      transform_expr(state, node->Binary.a);
      transform_expr(state, node->Binary.b);
      break;
    }
    case Node::Kind::Fun:
    case Node::Kind::Proc: {
      transform_expr(state, node->Fun.expr, true);
      break;
    }
    case Node::Kind::Struct: {
      transform_expr(state, node->Fun.expr);
      break;
    }
    case Node::Kind::Switch: {
      // TODO Transform if non-voidish!!
      // TODO Forward can_return down each? Or just capture consistently and return after?
      fprintf(stderr, "switch: %d\n", (int)node->type.kind);
      break;
    }
    default: break;
  }
}

}
