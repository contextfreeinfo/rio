#include "main.h"

namespace rio {

struct ResolveState {

  Engine* engine;
  ModManager* mod;
  StrBuf str_buf;
  // A hack to deal with new defs appearing during resolve.
  List<Def*> new_defs;
  // TODO A stack of locals or of scopes.

  Type& alloc_type(Type::Kind kind = Type::Kind::None) {
    Type& type = mod->arena.alloc<Type>();
    type.kind = kind;
    return type;
  }

};

Type choose_float_type(const Type& type);
Type choose_int_type(const Type& type);
auto ensure_global_refs(ModManager* mod) -> void;
void resolve_array(ResolveState* state, Node* node, const Type& type);
void resolve_block(ResolveState* state, Node* node, const Type& type);
auto resolve_cast(ResolveState* state, Node* node, const Type& type) -> void;
auto resolve_const(ResolveState* state, Node* node, const Type& type) -> void;
auto resolve_def(ResolveState* state, Def* def) -> void;
auto resolve_def_body(ResolveState* state, Def* def) -> void;
void resolve_expr(ResolveState* state, Node* node, const Type& type);
auto resolve_proc(ResolveState* state, Node* node, const Type& type) -> void;
auto resolve_proc_sig(
  ResolveState* state, Node* node, const Type& type
) -> void;
auto resolve_ref(ResolveState* state, Node* node) -> void;
void resolve_tuple(ResolveState* state, Node* node, const Type& type);
auto resolve_type(ResolveState* state, Node* node) -> void;

auto resolve(Engine* engine) -> void {
  // The root mods should be sorted before this point.
  for (auto mod: engine->mods) {
    // TODO Presume future parallel and keep the state separate?
    // TODO Or unify to prevent allocations?
    ResolveState state;
    state.engine = engine;
    state.mod = mod;
    // Prepare globals and resolve top-level defs.
    // Note that new globals can be defined in each mod as we go, and that's ok.
    ensure_global_refs(mod);
    for (auto def: mod->global_defs) {
      resolve_def(&state, def);
    }
    for (auto def: mod->global_defs) {
      resolve_def_body(&state, def);
    }
    // Add the new defs on to the old.
    // TODO Do they still need resolved?
    for (auto def: state.new_defs) {
      mod->global_defs.push(def);
    }
  }
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
  auto& globals = mod->global_refs;
  // See if we haven't built the global refs for this multimod.
  // TODO Just force the root first so we don't need to bother checking?
  if (globals.len()) {
    return;
  }
  // First get all globals from this mod, since they have priority.
  // fprintf(stderr, "mod: %s with %p, at %zu/%zu\n", mod->name, (void*)&globals, globals.len(), globals.capacity());
  // blab = true;
  for (auto global: mod->global_defs) {
    auto insert = globals.get_or_insert(global->name);
    if (insert.inserted) {
      // No conflict. Good.
      // fprintf(stderr, "internal global: %s -> %p at %p, now %zu/%zu\n", global->name, (void*)global, (void*)insert.pair, globals.len(), globals.capacity());
      insert.pair->value = global;
    } else {
      // Conflict. Clear it out.
      // fprintf(stderr, "internal conflict: %s\n", global->name);
      insert.pair->value = nullptr;
    }
  }
  // Then get all globals from use imports.
  for (auto import: mod->uses) {
    for (auto& global: import->global_defs) {
      auto insert = globals.get_or_insert(global->name);
      if (insert.inserted) {
        // No conflict. Good.
        // fprintf(stderr, "external global: %s, now %zu/%zu\n", global->name, globals.len(), globals.capacity());
        insert.pair->value = global;
      } else {
        // Conflict. See if it's from the current mod or not.
        if (insert.pair->value->mod == mod) {
          // fprintf(stderr, "external ignored: %s\n", global->name);
        } else {
          // From somewhere else. Clear it.
          // fprintf(stderr, "external conflict: %s\n", global->name);
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
    auto name = name_type(state->engine, &state->str_buf, node->type);
    auto insert = state->mod->global_refs.get_or_insert(name);
    if (insert.inserted) {
      Def* def = &state->mod->arena.alloc<Def>();
      def->name = name;
      // Make a fake node for now.
      // TODO Make a canonical single node for Span.
      Node* typedef_node = &state->mod->arena.alloc<Node>();
      typedef_node->kind = Node::Kind::None;
      def->top = typedef_node;
      def->type = &node->type;
      node->type.def = def;
      // Store the global.
      state->new_defs.push(def);
      insert.pair->value = def;
    }
  }
}

void resolve_block(ResolveState* state, Node* node, const Type& type) {
  // TODO The last item should expect the block type.
  for (auto item: node->Block.items) {
    resolve_expr(state, item, {Type::Kind::None});
  }
}

auto resolve_cast(ResolveState* state, Node* node, const Type& type) -> void {
  resolve_type(state, node->Cast.b);
  node->type = node->Cast.b->type;
  resolve_expr(state, node->Cast.a, node->Cast.b->type);
}

auto resolve_const(ResolveState* state, Node* node, const Type& type) -> void {
  resolve_expr(state, node->Const.a, {Type::Kind::None});
  if (node->Const.a->type.kind == Type::Kind::None) {
    // Infer from value to def.
    resolve_expr(state, node->Const.b, {Type::Kind::None});
    node->Const.a->type = node->Const.b->type;
  }
}

auto resolve_def(ResolveState* state, Def* def) -> void {
  if (def->resolve_started) {
    // TODO Error if not finished.
    return;
  }
  if (def->mod && def->mod != state->mod) {
    // TODO Error.
    return;
  }
  def->resolve_started = true;
  auto node = def->top;
  // TODO Rewind locals stack to def scope.
  switch (node->kind) {
    case Node::Kind::Cast: {
      resolve_cast(state, node, {Type::Kind::None});
      break;
    }
    case Node::Kind::Const: {
      resolve_const(state, node, {Type::Kind::None});
      break;
    }
    case Node::Kind::Fun:
    case Node::Kind::Proc: {
      resolve_proc_sig(state, node, {Type::Kind::None});
      break;
    }
    case Node::Kind::Struct: {
      // TODO Instead resolve nested defs in resolve_def mode.
      resolve_expr(state, node->Fun.expr, {Type::Kind::None});
      break;
    }
    default: {
      // printf("(!!! BROKEN %d !!!)", static_cast<int>(node.kind));
      break;
    }
  }
  def->resolved = true;
}

auto resolve_def_body(ResolveState* state, Def* def) -> void {
  auto node = def->top;
  switch (node->kind) {
    case Node::Kind::Block: {
      // TODO Expect return type.
      resolve_block(state, node, {Type::Kind::None});
      break;
    }
    case Node::Kind::Fun:
    case Node::Kind::Proc: {
      // TODO Expect return type.
      resolve_expr(state, node->Fun.expr, {Type::Kind::None});
      break;
    }
    default: {
      break;
    }
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
      Type proc_type = node->Call.callee->type;
      Type args_type = {Type::Kind::None};
      if (proc_type.kind == Type::Kind::Proc && proc_type.node) {
        args_type.node = proc_type.node->Fun.params;
        if (args_type.node) {
          args_type.kind = Type::Kind::Tuple;
        }
      }
      resolve_expr(state, node->Call.args, args_type);
      break;
    }
    case Node::Kind::Cast: {
      resolve_cast(state, node, type);
      break;
    }
    case Node::Kind::Const: {
      resolve_const(state, node, type);
      break;
    }
    case Node::Kind::Float: {
      node->type = choose_float_type(type);
      break;
    }
    case Node::Kind::Fun:
    case Node::Kind::Proc: {
      resolve_proc(state, node, type);
      break;
    }
    case Node::Kind::Int: {
      node->type = choose_int_type(type);
      break;
    }
    case Node::Kind::Map: {
      node->type = type;
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
    case Node::Kind::Struct: {
      resolve_expr(state, node->Fun.expr, {Type::Kind::None});
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

auto resolve_proc(ResolveState* state, Node* node, const Type& type) -> void {
  resolve_proc_sig(state, node, type);
  // TODO Expect proc return type.
  resolve_expr(state, node->Fun.expr, {Type::Kind::None});
}

auto resolve_proc_sig(
  ResolveState* state, Node* node, const Type& type
) -> void {
  auto ret_type = &state->alloc_type();
  node->type = {Type::Kind::Proc, ret_type};
  node->type.node = node;
  // TODO Actual types on functions.
  ret_type->kind = Type::Kind::Void;
  // TODO If in a local expression, expected type might be given.
  if (node->Fun.params) {
    resolve_tuple(state, node->Fun.params, {Type::Kind::None});
  }
}

auto resolve_ref(ResolveState* state, Node* node) -> void {
  // TODO If not yet resolved, call resolve_def on it.
  node->Ref.def = state->mod->global_refs.get(node->Ref.name);
  if (node->Ref.def) {
    resolve_def(state, node->Ref.def);
    node->type = node->Ref.def->node->type;
  }
}

void resolve_tuple(ResolveState* state, Node* node, const Type& type) {
  auto params = type.kind == Type::Kind::Tuple ? &type.node->Tuple : nullptr;
  auto items = node->Tuple.items;
  for (usize i = 0; i < items.len; i += 1) {
    Type item_type = {Type::Kind::None};
    if (params) {
      item_type = params->items[i]->type;
    }
    resolve_expr(state, items[i], item_type);
  }
}

auto resolve_type(ResolveState* state, Node* node) -> void {
  // TODO Types are actually of type type(id?), but is this ok for casts?
  // TODO Global outer map on interned pointers to types.
  if (node->kind == Node::Kind::Ref) {
    string name = node->Ref.name;
    if (!strcmp(name, "i32")) {
      node->type = {Type::Kind::I32};
    } else if (!strcmp(name, "string")) {
      node->type = {Type::Kind::String};
    } else {
      // Custom type.
      node->type = {Type::Kind::User};
      node->type.def = state->mod->global_refs.get(name);
      // fprintf(stderr, "Yo dawg in %s: %s -> %p from %p\n", state->mod->name, name, (void*)node->type.def, (void*)&state->mod->global_refs);
    }
  }
}

}
