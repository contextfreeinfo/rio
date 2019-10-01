#include "main.h"

namespace rio {

struct ExpressTrackerState {
  rint buffer_index{-1};
  rint voidish_index{-1};
  BlockNode* voidish_parent{nullptr};
};

// TODO Rename this pass from "transform" to "express"?
struct TransformState {
  ModManager* mod;
  List<Node*> buffer;
  ExpressTrackerState tracker_state;
};

struct ExpressTracker {

  ExpressTracker(TransformState* state_): state{state_} {}

  ~ExpressTracker() {
    if (used) {
      // fprintf(stderr, "Restoring old state\n");
      state->tracker_state = old_tracker_state;
    }
  }

  auto track(BlockNode* voidish_parent, rint voidish_index) -> void {
    if (!used) {
      // fprintf(stderr, "Saving tracker state\n");
      old_tracker_state = state->tracker_state;
      used = true;
    }
    state->tracker_state.buffer_index = state->buffer.len;
    state->tracker_state.voidish_index = voidish_index;
    state->tracker_state.voidish_parent = voidish_parent;
  }

  TransformState* state;
  ExpressTrackerState old_tracker_state;
  bool used{false};
};

// TODO Walk trees to find expressions that need to be statements.
// TODO Pull out the last one to be executed, then also pull out others that
// TODO would execute before it.

auto transform_block(
  TransformState* state, Node** node, bool can_return = false
) -> void;
auto transform_expr(
  TransformState* state, Node** node, bool can_return = false
) -> void;

auto transform(ModManager* mod) -> void {
  TransformState state;
  state.mod = mod;
  for (auto def: mod->global_defs) {
    // Nobody should be swapping out top-level defs, but we need the pointer
    // here for consistency, anyway.
    transform_expr(&state, &def->top);
  }
}

auto transform_block(
  TransformState* state, Node** node_ref, bool can_return
) -> void {
  ExpressTracker tracker{state};
  Node* node = *node_ref;
  auto& items = node->Block.items;
  bool last = true;
  for (rint i = items.len - 1; i >= 0; i -= 1) {
    auto item_ref = &items[i];
    auto item = *item_ref;
    if (is_voidish(item->type.kind)) {
      // Update voidish index (and siblings for convenience?).
      tracker.track(&node->Block, i);
    }
    transform_expr(state, item_ref, can_return && last);
    if (last) {
      if (can_return && !is_voidish(item->type.kind)) {
        switch (item->kind) {
          case Node::Kind::Block:
          case Node::Kind::Switch: {
            // Skip these since we will have pushed returns into them.
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
  TransformState* state, Node** node_ref, bool can_return
) -> void {
  Node* node = *node_ref;
  switch (node->kind) {
    case Node::Kind::Array:
    case Node::Kind::Map:
    case Node::Kind::Tuple: {
      auto& items = node->Parent.items;
      for (rint i = items.len - 1; i >= 0; i -= 1) {
        transform_expr(state, &items[i]);
      }
      break;
    }
    case Node::Kind::Block: {
      transform_block(state, node_ref, can_return);
      break;
    }
    case Node::Kind::Call: {
      transform_expr(state, &node->Call.callee);
      transform_expr(state, &node->Call.args);
      break;
    }
    case Node::Kind::Case:
    case Node::Kind::For: {
      // TODO Transform if non-void.
      transform_expr(state, &node->For.arg);
      transform_expr(state, &node->For.expr, can_return);
      break;
    }
    case Node::Kind::Cast: {
      transform_expr(state, &node->Cast.a);
      break;
    }
    case Node::Kind::Const: {
      transform_expr(state, &node->Cast.b);
      break;
    }
    case Node::Kind::Else: {
      transform_expr(state, &node->Else.expr, can_return);
      break;
    }
    case Node::Kind::Equal:
    case Node::Kind::Less:
    case Node::Kind::LessOrEqual:
    case Node::Kind::More:
    case Node::Kind::MoreOrEqual:
    case Node::Kind::NotEqual: {
      transform_expr(state, &node->Binary.a);
      transform_expr(state, &node->Binary.b);
      break;
    }
    case Node::Kind::Fun:
    case Node::Kind::Proc: {
      transform_expr(state, &node->Fun.expr, true);
      break;
    }
    case Node::Kind::Struct: {
      transform_expr(state, &node->Fun.expr);
      break;
    }
    case Node::Kind::Switch: {
      // TODO Transform if non-voidish!
      // TODO We need to know the nearest voidish above, its parent, and its index in the parent.
      // TODO We need to replace it with at least 2 statements: a new var and block/statements assigning to the var.
      // TODO And we need to replace this switch with a ref to the new var.
      // TODO Forward can_return down each? Or just capture consistently and return after?
      if (is_voidish(node->type.kind) || can_return) {
        // Usage as return requires less transforming.
        if (node->Switch.arg) {
          transform_expr(state, &node->Switch.arg);
        }
        for (auto item: node->Switch.items) {
          // The address here isn't useful, but switch cases shouldn't be
          // swapped out, anyway.
          transform_expr(state, &item, can_return);
        }
      } else {
        auto voidish = state->tracker_state.voidish_parent ?
          state->tracker_state.voidish_parent->items[state->tracker_state.voidish_index] :
          nullptr;
        fprintf(stderr, "switch value type %d\n", (int)node->type.kind);
        if (voidish) {
          // TODO Start replacing children of voidish until we get to our ancestor. How to know when that is? Track parents in nodes?
          fprintf(stderr, "  under voidish %d\n", (int)voidish->kind);
        }
        // TODO Afterward, call back into transform_switch again?
      }
      break;
    }
    default: break;
  }
}

}
