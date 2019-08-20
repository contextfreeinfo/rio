#include "main.h"

namespace rio { namespace c {

struct GenState {
  rint indent = 0;
  ModManager* mod = nullptr;
  bool start_mod = false;
  bool start_section = false;
};

struct Indent {
  Indent(GenState* state_): state{state_} {
    state->indent += 1;
  }
  ~Indent() {
    state->indent -= 1;
  }
  GenState* state;
};

void gen_bad(GenState* state, const Node& node);
auto gen_block(GenState* state, const Node& node) -> void;
auto gen_const(GenState* state, const Node& node) -> void;
auto gen_decl_expr(GenState* state, const Node& node) -> void;
auto gen_decls(GenState* state) -> void;
auto gen_expr(GenState* state, const Node& node) -> void;
auto gen_for(GenState* state, const Node& node) -> void;
auto gen_function(GenState* state, const Node& node) -> void;
auto gen_function_def(GenState* state, const Node& node) -> void;
auto gen_function_defs(GenState* state) -> void;
auto gen_global_expr(GenState* state, const Node& node) -> void;
auto gen_globals(GenState* state) -> void;
void gen_list_items(GenState* state, const Node& node);
void gen_indent(GenState* state);
auto gen_map(GenState* state, const Node& node) -> void;
auto gen_mod(GenState* state, ModManager* mod) -> void;
auto gen_mod_header(GenState* state) -> void;
void gen_param_items(GenState* state, const Node* node);
void gen_ref(GenState* state, const Node& node);
auto gen_ref_def(GenState* state, const Def& def) -> void;
void gen_type(GenState* state, const Type& type);
auto gen_typedef(GenState* state, const Node& node) -> void;
auto gen_typedefs(GenState* state) -> void;
void gen_statements(GenState* state, const Node& node);
auto needs_semi(const Node& node) -> bool;

void gen(Engine* engine) {
  GenState state;
  // Common heading.
  // TODO Need to keep a tally of all external headers? Libs, too.
  printf(
    "#include <stdint.h>\n"
    "#include <stdio.h>\n"
  );
  // TODO Gen internal mod 0 here?
  // TODO This is where rio_Span_i32 is needed for current test case.
  // Now gen each mod.
  for (auto mod: engine->mods) {
    gen_mod(&state, mod);
  }
}

void gen_bad(GenState* state, const Node& node) {
  printf("(!!! BROKEN %d !!!)", static_cast<int>(node.kind));
}

void gen_block(GenState* state, const Node& node) {
  printf("{\n");
  {
    Indent _{state};
    gen_statements(state, node);
  }
  gen_indent(state);
  printf("}\n");
}

auto gen_const(GenState* state, const Node& node) -> void {
  gen_type(state, node.Const.a->type);
  printf(" const ");
  gen_expr(state, *node.Const.a);
  printf(" = ");
  gen_expr(state, *node.Const.b);
}

auto gen_decl_expr(GenState* state, const Node& node) -> void {
  switch (node.kind) {
    case Node::Kind::Fun:
    case Node::Kind::Proc: {
      gen_mod_header(state);
      gen_type(state, *node.type.arg);
      printf(" %s_%s(", state->mod->name, node.Fun.name);
      gen_param_items(state, node.Fun.params);
      printf(");\n");
      break;
    }
    default: {
      // Nothing to do.
      break;
    }
  }
}

auto gen_decls(GenState* state) -> void {
  state->start_section = true;
  for (auto def: state->mod->global_defs) {
    gen_decl_expr(state, *def->top);
  }
}

void gen_expr(GenState* state, const Node& node) {
  switch (node.kind) {
    case Node::Kind::Array: {
      // TODO Can nest this in c99, but c++ makes it temporary.
      // TODO And compiling under c++ is also very useful.
      // TODO Either way, before we can finish this, we need structs and maybe
      // TODO tuples.
      printf("{(");
      if (node.type.arg) {
        gen_type(state, *node.type.arg);
      } else {
        gen_type(state, {Type::Kind::None});
      }
      printf("[]){");
      gen_list_items(state, node);
      printf("}, %zu", node.Array.items.len);
      printf("}");
      break;
    }
    case Node::Kind::Block: {
      gen_block(state, node);
      break;
    }
    case Node::Kind::Call: {
      gen_expr(state, *node.Call.callee);
      printf("(");
      gen_list_items(state, *node.Call.args);
      printf(")");
      break;
    }
    case Node::Kind::Cast: {
      gen_type(state, node.type);
      if (node.Cast.a->kind == Node::Kind::Ref) {
        printf(" %s", node.Cast.a->Ref.name);
      }
      break;
    }
    case Node::Kind::Const: {
      gen_const(state, node);
      break;
    }
    case Node::Kind::Float: {
      printf("%s", node.Float.text);
      break;
    }
    case Node::Kind::For: {
      gen_for(state, node);
      break;
    }
    case Node::Kind::Fun: {
      gen_function(state, node);
      break;
    }
    case Node::Kind::Int: {
      printf("%s", node.Int.text);
      break;
    }
    case Node::Kind::Map: {
      gen_map(state, node);
      break;
    }
    case Node::Kind::Member: {
      gen_expr(state, *node.Member.a);
      // TODO If pointer, use ->. If pointer to pointer, deref first, etc?
      // TODO Just use deref for consistency?
      printf(".");
      gen_expr(state, *node.Member.b);
      break;
    }
    case Node::Kind::Ref: {
      gen_ref(state, node);
      break;
    }
    case Node::Kind::String: {
      printf("%s", node.String.text);
      break;
    }
    case Node::Kind::Use: {
      // Nothing to do here.
      break;
    }
    default: {
      gen_bad(state, node);
      break;
    }
  }
}

auto gen_for(GenState* state, const Node& node) -> void {
  printf("{\n");
  {
    Indent _{state};
    // Declare a single expr in case the arg is more than a simple var.
    // TODO Optimize this part away if it is just a ref.
    gen_indent(state);
    string list_name = "rio_span";
    // Generate list value as local.
    gen_type(state, node.For.arg->type);
    printf(" %s = ", list_name);
    gen_expr(state, *node.For.arg);
    printf(";\n");
    // The loop itself.
    gen_indent(state);
    string index_name = "rio_index";
    printf(
      "for (int_fast32_t %s = 0; %s < %s.len; %s += 1) {\n",
      index_name, index_name, list_name, index_name
    );
    {
      Indent _{state};
      if (node.For.expr->kind == Node::Kind::Block) {
        // Params.
        Node* params = node.For.expr->Block.params;
        if (params && params->Tuple.items.len) {
          auto param = params->Tuple.items[0];
          string name = "";
          // TODO Cast option.
          if (param->kind == Node::Kind::Ref) {
            name = param->Ref.name;
          }
          gen_indent(state);
          gen_type(state, *node.For.arg->type.arg);
          printf(" %s = %s.items[%s];\n", name, list_name, index_name);
          // TODO Index.
        }
        // Block contents.
        gen_statements(state, *node.For.expr);
      } else {
        // No param here.
        gen_expr(state, *node.For.expr);
      }
    }
    gen_indent(state);
    printf("}\n");
  }
  gen_indent(state);
  printf("}\n");
}

auto gen_function(GenState* state, const Node& node) -> void {
  // TODO When to mark static?
  bool prefix = !state->indent;
  gen_type(state, *node.type.arg);
  printf(
    " %s%s%s(",
    prefix ? state->mod->name : "",
    prefix ? "_" : "",
    node.Fun.name
  );
  gen_param_items(state, node.Fun.params);
  printf(") ");
  // TODO Special handling of non-block exprs.
  if (node.Fun.expr->kind == Node::Kind::Block) {
    gen_block(state, *node.Fun.expr);
  }
}

auto gen_function_def(GenState* state, const Node& node) -> void {
  switch (node.kind) {
    case Node::Kind::Block: {
      gen_mod_header(state);
      printf("\n");
      printf("int main() ");
      gen_block(state, node);
      break;
    }
    case Node::Kind::Fun:
    case Node::Kind::Proc: {
      gen_mod_header(state);
      printf("\n");
      gen_function(state, node);
      break;
    }
    default: break;
  }
}

auto gen_function_defs(GenState* state) -> void {
  state->start_section = false;
  for (auto def: state->mod->global_defs) {
    gen_function_def(state, *def->top);
  }
}

auto gen_global_expr(GenState* state, const Node& node) -> void {
  switch (node.kind) {
    case Node::Kind::Const: {
      gen_mod_header(state);
      gen_const(state, node);
      printf(";\n");
      break;
    }
    default: {
      // Nothing to do.
      break;
    }
  }
}

auto gen_globals(GenState* state) -> void {
  state->start_section = true;
  // TODO Dependency order.
  for (auto def: state->mod->global_defs) {
    gen_global_expr(state, *def->top);
  }
}

void gen_indent(GenState* state) {
  for (rint i = 0; i < state->indent; i += 1) {
    printf("  ");
  }
}

void gen_list_items(GenState* state, const Node& node) {
  auto items = node.Tuple.items;
  for (rint i = 0; i < items.len; i += 1) {
    if (i) {
      printf(", ");
    }
    gen_expr(state, *items[i]);
  }
}

auto gen_map(GenState* state, const Node& node) -> void {
  printf("(");
  gen_type(state, node.type);
  printf("){");
  auto items = node.Map.items;
  for (rint i = 0; i < items.len; i += 1) {
    auto& item = *items[i];
    if (i) {
      printf(", ");
    }
    switch (item.kind) {
      case Node::Kind::Const: {
        printf(".");
        gen_expr(state, *item.Const.a);
        printf(" = ");
        gen_expr(state, *item.Const.b);
        break;
      }
      case Node::Kind::Ref: {
        printf(".%s = %s", item.Ref.name, item.Ref.name);
        break;
      }
      default: {
        gen_bad(state, node);
        break;
      }
    }
  }
  printf("}");
}

auto gen_mod(GenState* state, ModManager* mod) -> void {
  // Now gen this one.
  state->mod = mod;
  state->start_mod = true;
  gen_typedefs(state);
  gen_decls(state);
  gen_globals(state);
  gen_function_defs(state);
}

auto gen_mod_header(GenState* state) -> void {
  if (state->start_mod) {
    printf("\n// %s\n", state->mod->file);
    state->start_mod = false;
  }
  if (state->start_section) {
    printf("\n");
    state->start_section = false;
  }
}

void gen_param_items(GenState* state, const Node* node) {
  if (!node) {
    return;
  }
  auto items = node->Tuple.items;
  for (rint i = 0; i < items.len; i += 1) {
    auto& item = *items[i];
    if (i) {
      printf(", ");
    }
    switch (item.kind) {
      case Node::Kind::Cast: {
        gen_type(state, item.type);
        if (item.Cast.a->kind == Node::Kind::Ref) {
          printf(" const %s", item.Cast.a->Ref.name);
        } else {
          printf(" ");
          gen_bad(state, *node);
        }
        break;
      }
      case Node::Kind::Ref: {
        printf("(!!! TYPE !!!) %s", item.Ref.name);
        break;
      }
      default: {
        gen_bad(state, *node);
        break;
      }
    }
  }
}

void gen_ref(GenState* state, const Node& node) {
  if (node.Ref.def) {
    gen_ref_def(state, *node.Ref.def);
  } else {
    printf("%s", node.Ref.name);
  }
}

auto gen_ref_def(GenState* state, const Def& def) -> void{
  if (def.mod) {
    printf("%s_", def.mod->name);
  }
  printf("%s", def.name);
}

void gen_statements(GenState* state, const Node& node) {
  // TODO Make a different version for top level?
  // TODO Just make a gen_function_defs thing for top?
  auto items = node.Block.items;
  for (rint i = 0; i < items.len; i += 1) {
    gen_indent(state);
    gen_expr(state, *items[i]);
    if (needs_semi(*items[i])) {
      printf(";\n");
    }
  }
}

auto gen_struct(GenState* state, const Node& node) -> void {
  bool prefix = !state->indent;
  printf(
    "typedef struct %s%s%s {\n",
    prefix ? state->mod->name : "",
    prefix ? "_" : "",
    node.Fun.name
  );
  if (node.Fun.expr->kind == Node::Kind::Block) {
    Indent _{state};
    auto& block = node.Fun.expr->Block;
    auto items = block.items;
    for (rint i = 0; i < items.len; i += 1) {
      gen_indent(state);
      gen_expr(state, *items[i]);
      if (needs_semi(*items[i])) {
        printf(";\n");
      }
    }
  }
  printf(
    "} %s%s%s;\n",
    prefix ? state->mod->name : "",
    prefix ? "_" : "",
    node.Fun.name
  );
}

void gen_type(GenState* state, const Type& type) {
  switch (type.kind) {
    case Type::Kind::Array: {
      // Resolve guarantees a def assigned. 
      printf("%s", type.def->name);
      break;
    }
    case Type::Kind::F32: {
      printf("float");
      break;
    }
    case Type::Kind::F64: {
      printf("double");
      break;
    }
    case Type::Kind::I8: {
      printf("int8_t");
      break;
    }
    case Type::Kind::I16: {
      printf("int16_t");
      break;
    }
    case Type::Kind::I32: {
      printf("int32_t");
      break;
    }
    case Type::Kind::I64: {
      printf("int64_t");
      break;
    }
    case Type::Kind::Int: {
      printf("int_fast32_t");
      break;
    }
    case Type::Kind::String: {
      printf("const char*");
      break;
    }
    case Type::Kind::U8: {
      printf("uint8_t");
      break;
    }
    case Type::Kind::U16: {
      printf("uint16_t");
      break;
    }
    case Type::Kind::U32: {
      printf("uint32_t");
      break;
    }
    case Type::Kind::U64: {
      printf("uint64_t");
      break;
    }
    case Type::Kind::User: {
      if (type.def) {
        gen_ref_def(state, *type.def);
      } else {
        printf("(!!! TYPE !!!)");
      }
      break;
    }
    case Type::Kind::Void: {
      printf("void");
      break;
    }
    default: {
      printf("(!!! TYPE %d !!!)", (int)type.kind);
      break;
    }
  }
}

auto gen_typedef(GenState* state, const Node& node) -> void {
  switch (node.kind) {
    case Node::Kind::Struct: {
      gen_mod_header(state);
      printf("\n");
      gen_struct(state, node);
      break;
    }
    default: break;
  }
}

auto gen_typedefs(GenState* state) -> void {
  state->start_section = false;
  for (auto def: state->mod->global_defs) {
    if (def->type && def->type->kind == Type::Kind::Array) {
      // TODO Wrap generic types in #ifndef #define things in case generated by
      // TODO different mod branches.
      auto& type = *def->type;
      // TODO Make this into a standard struct node, so it gets auto generated.
      gen_mod_header(state);
      printf("\n");
      printf("typedef struct %s {\n", def->name);
      {
        Indent _{state};
        gen_indent(state);
        if (type.arg) {
          gen_type(state, *type.arg);
        } else {
          gen_type(state, {Type::Kind::None});
        }
        printf("* items;\n");
        gen_indent(state);
        printf("int_fast32_t len;\n");
      }
      printf("} %s;\n", def->name);
    } else {
      gen_typedef(state, *def->top);
    }
  }
}

auto needs_semi(const Node& node) -> bool {
  switch (node.kind) {
    case Node::Kind::Block:
    case Node::Kind::For:
    case Node::Kind::Fun:
    case Node::Kind::Use: {
      return false;
    }
    default: {
      return true;
    }
  }
}

}}
