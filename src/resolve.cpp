#include "main.h"

namespace rio {

struct ResolveState {

  Engine* engine;
  ModManager* mod;
  StrBuf str_buf;
  // A hack to deal with new defs appearing during resolve.
  List<Def*> new_defs;
  // TODO A stack of locals or of scopes.
  List<Scope*> scopes;

  Type& alloc_type(Type::Kind kind = Type::Kind::None) {
    Type& type = mod->arena.alloc<Type>();
    type.kind = kind;
    return type;
  }

};

struct Enter {
  Enter(ResolveState* state_, Scope& scope): state{state_} {
    used = scope.defs.len;
    if (used) {
      state->scopes.push_val(&scope);
    }
  }
  ~Enter() {
    if (used) {
      state->scopes.pop();
    }
  }
  ResolveState* state;
  bool used;
};

Type choose_float_type(const Type& type);
Type choose_int_type(const Type& type);
auto ensure_global_refs(ModManager* mod) -> void;
auto ensure_range_type(ResolveState* state, Node* node, Type* arg_type) -> void;
auto ensure_span_type(ResolveState* state, Node* node, Type* arg_type) -> void;
auto ensure_type_generic(
  ResolveState* state, Node* node, Type::Kind type_kind, Type* arg_type
) -> void;
auto infer_value_decl_type(Node* node, const Type& type) -> void;
auto resolve_array(ResolveState* state, Node* node, const Type& type) -> void;
auto resolve_block(ResolveState* state, Node* node, const Type& type) -> void;
auto resolve_cast(ResolveState* state, Node* node, const Type& type) -> void;
auto resolve_const(ResolveState* state, Node* node, const Type& type) -> void;
auto resolve_def(ResolveState* state, Def* def) -> void;
auto resolve_def_body(ResolveState* state, Def* def) -> void;
auto resolve_expr(ResolveState* state, Node* node, const Type& type) -> void;
auto resolve_map(ResolveState* state, Node* node, const Type& type) -> void;
auto resolve_member(ResolveState* state, Node* node, const Type& type) -> void;
auto resolve_proc(ResolveState* state, Node* node, const Type& type) -> void;
auto resolve_proc_sig(
  ResolveState* state, Node* node, const Type& type
) -> void;
auto resolve_range(ResolveState* state, Node* node, const Type& type) -> void;
auto resolve_ref(ResolveState* state, Node* node) -> void;
auto resolve_switch(ResolveState* state, Node* node, const Type& type) -> void;
auto resolve_tuple(ResolveState* state, Node* node, const Type& type) -> void;
auto resolve_type(ResolveState* state, Node* node) -> void;
auto struct_type_scope(const Type& type) -> Opt<const Scope>;

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
      return {Type::Kind::Float};
    }
  }
}

Type choose_int_type(const Type& type) {
  // TODO Explicit suffixes on literals.
  switch (type.kind) {
    // Support type promotion to floats on literals.
    case Type::Kind::F32:
    case Type::Kind::F64:
    case Type::Kind::Float:
    // And support int types, too, of course.
    case Type::Kind::I8:
    case Type::Kind::I16:
    case Type::Kind::I32:
    case Type::Kind::I64:
    case Type::Kind::Int:
    case Type::Kind::U8:
    case Type::Kind::U16:
    case Type::Kind::U32:
    case Type::Kind::U64:
    case Type::Kind::UInt: {
      return type;
    }
    default: {
      return {Type::Kind::Int};
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
      fprintf(stderr, "internal conflict: %s\n", global->name);
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
          // TODO For now, things like rio_Range_int cause this.
          // TODO Hoist them higher? Set their mod to the same thing? Mark something unique about them? Presume it never matters?
          // fprintf(stderr, "external conflict: %s\n", global->name);
          // insert.pair->value = nullptr;
        }
      }
    }
  }
}

auto ensure_range_type(
  ResolveState* state, Node* node, Type* arg_type
) -> void {
  ensure_type_generic(state, node, Type::Kind::Range, arg_type);
}

auto ensure_span_type(ResolveState* state, Node* node, Type* arg_type) -> void {
  ensure_type_generic(state, node, Type::Kind::Array, arg_type);
}

auto ensure_type_generic(
  ResolveState* state, Node* node, Type::Kind type_kind, Type* arg_type
) -> void {
  // TODO Generalize to all generics?
  node->type = {type_kind, arg_type};
  auto name = name_type(state->engine, &state->str_buf, node->type);
  auto insert = state->mod->global_refs.get_or_insert(name);
  if (insert.inserted) {
    Def* def = &state->mod->arena.alloc<Def>();
    def->name = name;
    // The namespace is from the original, but it appears here in resolution.
    def->mod = state->mod;
    // Make a fake node for now.
    // TODO Make a canonical single node for the type?
    Node* typedef_node = &state->mod->arena.alloc<Node>();
    typedef_node->kind = Node::Kind::None;
    def->top = typedef_node;
    def->type = &node->type;
    node->type.def = def;
    // Store the global.
    state->new_defs.push(def);
    insert.pair->value = def;
  } else {
    // Use what we already defined.
    node->type.def = insert.pair->value;
  }
}

auto infer_value_decl_type(Node* node, const Type& type) -> void {
  if (node->type.kind == Type::Kind::None) {
    // Infer from value to declaration.
    node->type = type;
    if (node->kind == Node::Kind::Var) {
      // TODO Make this more general. Seriously.
      node->Var.expr->type = node->type;
    }
  }
}

auto resolve_array(ResolveState* state, Node* node, const Type& type) -> void {
  const Type none_type = {Type::Kind::None};
  const Type* item_type = &none_type;
  if (type.kind == Type::Kind::Array && type.arg) {
    item_type = type.arg;
  }
  for (auto item: node->Array.items) {
    resolve_expr(state, item, *item_type);
  }
  Type* item_type_now = nullptr;
  if (node->Array.items.len) {
    // In case we didn't have an explicit type before, use the content type.
    item_type_now = &node->Array.items[0]->type;
  } else if (item_type->kind != Type::Kind::None) {
    // Abusively presume that non-none is actually a safe place in an arena.
    // TODO Reorg to push this explicitly as Opt<Type> instead of const Type& for the type parameter everywhere???
    item_type_now = const_cast<Type*>(item_type);
  }
  if (item_type_now) {
    ensure_span_type(state, node, item_type_now);
  }
}

auto is_voidish(Type::Kind kind) -> bool {
  return kind <= Type::Kind::Void;
}

auto resolve_block(ResolveState* state, Node* node, const Type& type) -> void {
  // TODO The last item should expect the block type.
  Enter scope{state, node->Block.scope};
  auto items = node->Block.items;
  for (rint i = 0; i < items.len - 1; i += 1) {
    auto item = items[i];
    // We know none of these values are being used at this level.
    // None means unspecified, and Void means expected void.
    // TODO Will we end up with no distinction between unknown and void???
    resolve_expr(state, item, {Type::Kind::Void});
  }
  if (items.len) {
    auto item = items[items.len - 1];
    resolve_expr(state, item, type);
    if (type.kind == Type::Kind::Void) {
      // TODO Apply ignored to all ignored cases?
      auto ignored_arg = &state->alloc_type();
      *ignored_arg = item->type;
      item->type.kind = Type::Kind::Ignored;
      item->type.arg = ignored_arg;
    } else {
      node->type = items[items.len - 1]->type;
    }
  }
}

auto resolve_cast(ResolveState* state, Node* node, const Type& type) -> void {
  resolve_type(state, node->Cast.b);
  node->type = node->Cast.b->type;
  resolve_expr(state, node->Cast.a, node->type);
  if (node->Cast.a->type.kind == Type::Kind::None) {
    // Infer from type to def.
    node->Cast.a->type = node->type;
  }
}

auto resolve_const(ResolveState* state, Node* node, const Type& type) -> void {
  resolve_expr(state, node->Const.a, type);
  auto* a_type = &node->Const.a->type;
  auto* expected_b_type = a_type->kind == Type::Kind::None ? &type : a_type;
  resolve_expr(state, node->Const.b, *expected_b_type);
  infer_value_decl_type(node->Const.a, node->Const.b->type);
  node->type = node->Const.a->type;
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
    case Node::Kind::Const:
    case Node::Kind::Update: {
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
      resolve_expr(state, node, *node->type.arg);
      break;
    }
    default: {
      break;
    }
  }
}

auto resolve_expr(ResolveState* state, Node* node, const Type& type) -> void {
  switch (node->kind) {
    case Node::Kind::Address:
    case Node::Kind::AddressMul: {
      // TODO If type is address/mul and arg is non-null, pass down arg type.
      resolve_expr(state, node->Address.expr, {Type::Kind::None});
      break;
    }
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
      switch (proc_type.kind) {
        case Type::Kind::AddressMul:
        case Type::Kind::Array: {
          // TODO Expect int/slice array for args?
          if (proc_type.arg) {
            node->type = *proc_type.arg;
          }
          break;
        }
        case Type::Kind::Proc: {
          node->type = proc_type.node->Fun.ret_type->type;
          if (proc_type.node) {
            args_type.node = proc_type.node->Fun.params;
            if (args_type.node) {
              args_type.kind = Type::Kind::Tuple;
            }
          }
          break;
        }
        default: break;
      }
      resolve_expr(state, node->Call.args, args_type);
      break;
    }
    case Node::Kind::Case: {
      // TODO Case might have particular expected arg types.
      resolve_expr(state, node->Case.arg, {Type::Kind::None});
      resolve_expr(state, node->Case.expr, type);
      node->type = node->Case.expr->type;
      break;
    }
    case Node::Kind::Cast: {
      resolve_cast(state, node, type);
      break;
    }
    case Node::Kind::Const:
    case Node::Kind::Update: {
      resolve_const(state, node, type);
      break;
    }
    case Node::Kind::Div:
    case Node::Kind::Mul:
    case Node::Kind::Minus:
    case Node::Kind::Plus: {
      resolve_expr(state, node->Binary.a, {Type::Kind::None});
      resolve_expr(state, node->Binary.b, {Type::Kind::None});
      // TODO General rules about result type, including built-in promotion.
      node->type = node->Binary.a->type;
      break;
    }
    case Node::Kind::Else:
    case Node::Kind::Unsafe: {
      resolve_expr(state, node->Unary.expr, type);
      node->type = node->Unary.expr->type;
      break;
    }
    case Node::Kind::Equal:
    case Node::Kind::Less:
    case Node::Kind::LessOrEqual:
    case Node::Kind::More:
    case Node::Kind::MoreOrEqual:
    case Node::Kind::NotEqual: {
      resolve_expr(state, node->Binary.a, {Type::Kind::None});
      resolve_expr(state, node->Binary.b, {Type::Kind::None});
      node->type = {Type::Kind::Bool};
      break;
    }
    case Node::Kind::Float: {
      node->type = choose_float_type(type);
      break;
    }
    case Node::Kind::For: {
      Enter scope{state, node->For.scope};
      // TODO Case might have particular expected arg types.
      resolve_expr(state, node->For.arg, {Type::Kind::None});
      if (node->For.param) {
        resolve_expr(state, node->For.arg, {Type::Kind::None});
        if (node->For.arg->type.arg) {
          infer_value_decl_type(node->For.param, *node->For.arg->type.arg);
        }
      }
      resolve_expr(state, node->For.expr, type);
      node->type = node->For.expr->type;
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
      resolve_map(state, node, type);
      break;
    }
    case Node::Kind::Member: {
      resolve_member(state, node, type);
      break;
    }
    case Node::Kind::Range: {
      resolve_range(state, node, type);
      break;
    }
    case Node::Kind::Ref: {
      resolve_ref(state, node);
      break;
    }
    case Node::Kind::Return: {
      resolve_expr(state, node->Return.expr, type);
      node->type.kind = Type::Kind::Never;
      break;
    }
    case Node::Kind::SizeOf: {
      resolve_type(state, node->SizeOf.expr);
      // TODO Types as any other expr.
      //~ resolve_expr(state, node->SizeOf.expr, {Type::Kind::None});
      node->type.kind = Type::Kind::Int;
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
    case Node::Kind::Switch: {
      resolve_switch(state, node, type);
      break;
    }
    case Node::Kind::Tuple: {
      resolve_tuple(state, node, type);
      break;
    }
    case Node::Kind::Var: {
      // TODO Does this do anything useful right now?
      resolve_expr(state, node->Var.expr, type);
      break;
    }
    default: {
      // printf("(!!! BROKEN %d !!!)", static_cast<int>(node.kind));
      break;
    }
  }
}

auto resolve_map(ResolveState* state, Node* node, const Type& type) -> void {
  node->type = type;
  auto* scope = struct_type_scope(type);
  if (!scope) return;
  Type none_type = {Type::Kind::None};
  for (auto item: node->Map.items) {
    // Get the name, so we can know the type.
    string name = nullptr;
    switch (item->kind) {
      case Node::Kind::Const: {
        if (item->Const.a->kind == Node::Kind::Ref) {
          name = item->Const.a->Ref.name;
        }
        break;
      }
      case Node::Kind::Ref: {
        name = item->Ref.name;
        break;
      }
      default: {
        // TODO Error.
        break;
      }
    }
    if (!name) {
      // TODO Error.
      continue;
    }
    // We have a name.
    auto item_def = scope->find(name);
    auto item_type =
      item_def && item_def->top ? &item_def->top->type : &none_type;
    resolve_expr(state, item, *item_type);
  }
}

auto resolve_member(ResolveState* state, Node* node, const Type& type) -> void {
  resolve_expr(state, node->Binary.a, {Type::Kind::None});
  auto scope = struct_type_scope(node->Binary.a->type);
  if (!scope) return;
  if (node->Binary.b->kind == Node::Kind::Ref) {
    auto item_def = scope->find(node->Binary.b->Ref.name);
    if (item_def && item_def->top) {
      node->type = item_def->top->type;
    }
  }
}

auto resolve_proc(ResolveState* state, Node* node, const Type& type) -> void {
  Enter scope{state, node->Fun.scope};
  resolve_proc_sig(state, node, type);
  resolve_expr(state, node->Fun.expr, *node->type.arg);
}

auto resolve_proc_sig(
  ResolveState* state, Node* node, const Type& type
) -> void {
  resolve_type(state, node->Fun.ret_type);
  node->type = {Type::Kind::Proc, &node->Fun.ret_type->type};
  node->type.node = node;
  // TODO If in a local expression, expected type might be given.
  if (node->Fun.params) {
    resolve_tuple(state, node->Fun.params, {Type::Kind::None});
  }
}

auto resolve_range(ResolveState* state, Node* node, const Type& type) -> void {
  // This none type is used temporarily at most, so it's ok to be local.
  const Type none_type = {Type::Kind::None};
  const Type* item_type = &none_type;
  if (type.kind == Type::Kind::Range && type.arg) {
    item_type = type.arg;
  }
  // Awkwardly step through from, to, and by.
  // TODO Generify kid list iteration to unify with arrays?
  // TODO Will we guarantee any of these?
  Type* item_type_now = nullptr;
  if (node->Range.from) {
    resolve_expr(state, node->Range.from, *item_type);
    item_type_now = &node->Range.from->type;
  }
  if (node->Range.to) {
    resolve_expr(state, node->Range.to, *item_type);
    if (!item_type_now) item_type_now = &node->Range.to->type;
  }
  if (node->Range.by) {
    resolve_expr(state, node->Range.by, *item_type);
    if (!item_type_now) item_type_now = &node->Range.by->type;
  }
  // Ensure type once we're done.
  if (item_type_now) {
    ensure_range_type(state, node, item_type_now);
  }
}

auto resolve_ref(ResolveState* state, Node* node) -> void {
  // Seems like checking against previous setting could help be efficient.
  // When I added the check, it made no difference in output, at least.
  if (!node->Ref.def) {
    // Look in local scopes first, from last (deepest) to first (outermost).
    for (int i = state->scopes.len - 1; i >= 0; i -= 1) {
      auto def = state->scopes[i]->find(node->Ref.name);
      if (def) {
        node->Ref.def = def;
        break;
      }
    }
    if (!node->Ref.def) {
      // Nothing in local scopes. Look in global.
      node->Ref.def = state->mod->global_refs.get(node->Ref.name);
    }
    if (node->Ref.def) {
      resolve_def(state, node->Ref.def);
      node->type = node->Ref.def->node->type;
    }
  }
}

auto resolve_switch(ResolveState* state, Node* node, const Type& type) -> void {
  if (node->Switch.arg) {
    resolve_expr(state, node->Switch.arg, {Type::Kind::None});
  }
  Type item_type = type;
  for (auto item: node->Switch.items) {
    // TODO Passing down some expectation of case arg type would be good.
    // TODO Also expectation of value type for case expressions.
    // TODO Does this mean expecting a procedure `proc(arg) expr`?
    resolve_expr(state, item, item_type);
    item_type = item->type;
  }
  node->type = item_type;
}

auto resolve_tuple(ResolveState* state, Node* node, const Type& type) -> void {
  auto params = type.kind == Type::Kind::Tuple ? &type.node->Tuple : nullptr;
  auto items = node->Tuple.items;
  for (rint i = 0; i < items.len; i += 1) {
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
  switch (node->kind) {
    case Node::Kind::Address: {
      resolve_type(state, node->Address.expr);
      node->type = {Type::Kind::Address};
      node->type.arg = &node->Address.expr->type;
      break;
    }
    case Node::Kind::AddressMul: {
      resolve_type(state, node->AddressMul.expr);
      node->type = {Type::Kind::AddressMul};
      node->type.arg = &node->AddressMul.expr->type;
      break;
    }
    case Node::Kind::Array: {
      for (auto item: node->Array.items) {
        resolve_type(state, item);
      }
      if (node->Array.items.len) {
        ensure_span_type(state, node, &node->Array.items[0]->type);
      } else {
        // TODO Error.
      }
      break;
    }
    case Node::Kind::Ref: {
      // TODO Make these basic defs in core mod, not crazy stuff here.
      string name = node->Ref.name;
      if (!strcmp(name, "bool")) {
        node->type = {Type::Kind::Bool};
      } else if (!strcmp(name, "float")) {
        node->type = {Type::Kind::Float};
      } else if (!strcmp(name, "i32")) {
        node->type = {Type::Kind::I32};
      } else if (!strcmp(name, "int")) {
        node->type = {Type::Kind::Int};
      } else if (!strcmp(name, "string")) {
        node->type = {Type::Kind::String};
      } else {
        // Custom type.
        node->type = {Type::Kind::User};
        node->type.def = state->mod->global_refs.get(name);
        // fprintf(stderr, "Yo dawg in %s: %s -> %p from %p\n", state->mod->name, name, (void*)node->type.def, (void*)&state->mod->global_refs);
      }
      break;
    }
    case Node::Kind::Void: {
      node->type = {Type::Kind::Void};
      break;
    }
    default: break;
  }
}

auto struct_type_scope(const Type& type) -> Opt<const Scope> {
  if (!(
    type.kind == Type::Kind::User && type.def && type.def->top &&
    type.def->top->kind == Node::Kind::Struct
  )) return nullptr;
  return &type.def->top->Fun.scope;
}

}
