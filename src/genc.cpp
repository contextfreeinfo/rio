#include "main.h"

namespace rio { namespace c {

struct GenState {
  rint indent = 0;
  ModManager* mod = nullptr;
  Type* parent_type = nullptr;
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

auto gen_bad(GenState* state, const Node& node) -> void;
auto gen_binary(GenState* state, const Node& node, string op) -> void;
auto gen_block(GenState* state, const Node& node) -> void;
auto gen_case(GenState* state, const Node& node) -> void;
auto gen_const(GenState* state, const Node& node) -> void;
auto gen_decl_expr(GenState* state, const Node& node) -> void;
auto gen_decls(GenState* state) -> void;
auto gen_expr(GenState* state, const Node& node) -> void;
auto gen_for(GenState* state, const Node& node) -> void;
auto gen_for_range(GenState* state, const Node& node) -> void;
auto gen_for_span(GenState* state, const Node& node) -> void;
auto gen_function(GenState* state, const Node& node) -> void;
auto gen_function_def(GenState* state, const Node& node) -> void;
auto gen_function_defs(GenState* state) -> void;
auto gen_global_expr(GenState* state, const Node& node) -> void;
auto gen_globals(GenState* state) -> void;
auto gen_if(GenState* state, const Node& node) -> void;
auto gen_list_items(GenState* state, const Node& node) -> void;
auto gen_indent(GenState* state) -> void;
auto gen_map(GenState* state, const Node& node) -> void;
auto gen_mod(GenState* state, ModManager* mod) -> void;
auto gen_mod_header(GenState* state) -> void;
auto gen_param_items(GenState* state, const Node* node) -> void;
auto gen_range(GenState* state, const Node& node) -> void;
auto gen_ref(GenState* state, const Node& node) -> void;
auto gen_ref_def(GenState* state, const Def& def) -> void;
auto gen_switch(GenState* state, const Node& node) -> void;
auto gen_switch_if(GenState* state, const Node& node) -> void;
auto gen_type(GenState* state, const Type& type) -> void;
auto gen_type_opt(GenState* state, Opt<const Type> type) -> void;
auto gen_typedef(GenState* state, const Node& node) -> void;
auto gen_typedef_array(GenState* state, const Def* def) -> void;
auto gen_typedef_range(GenState* state, const Def* def) -> void;
auto gen_typedefs(GenState* state) -> void;
auto gen_statements(GenState* state, const Node& node) -> void;
auto gen_val_decl(
  GenState* state, const Node& node, bool is_const = false
) -> void;
auto needs_semi(const Node& node) -> bool;

auto gen(Engine* engine) -> void {
  GenState state;
  // Common heading.
  // TODO Need to keep a tally of all external headers? Libs, too.
  printf(
    "#include <stdbool.h>\n"
    "#include <stddef.h>\n"
    "#include <stdint.h>\n"
    // TODO Include stdio, stdlib only if `use "c"`.
    "#include <stdio.h>\n"
    "#include <stdlib.h>\n"
    "\n"
    // Use typedefs because we want the generated code to be the same on all
    // platforms, but in case we need preprocessor junk, define in advance.
    "typedef double rio_float;\n"
    "typedef ptrdiff_t rio_int;\n"
    // TODO Perhaps include span len on strings, too.
    // TODO If so, we'll need something for raw strings, too, so we can make good c libraries in rio.
    "typedef const char* rio_string;\n"
  );
  // TODO Gen internal mod 0 here?
  // TODO This is where rio_Span_i32 is needed for current test case.
  // Now gen each mod.
  for (auto mod: engine->mods) {
    gen_mod(&state, mod);
  }
}

auto gen_bad(GenState* state, const Node& node) -> void {
  printf("(!!! BROKEN %d !!!)", static_cast<int>(node.kind));
}

auto gen_binary(GenState* state, const Node& node, string op) -> void {
  gen_expr(state, *node.Binary.a);
  printf("%s", op);
  gen_expr(state, *node.Binary.b);
}

auto gen_block(GenState* state, const Node& node) -> void {
  printf("{\n");
  {
    Indent _{state};
    gen_statements(state, node);
  }
  gen_indent(state);
  printf("}\n");
}

auto gen_case(GenState* state, const Node& node) -> void {
  // Build a makeshift if switch.
  Node switch_node = {Node::Kind::Switch};
  switch_node.Switch = {0};
  const Node* items[] = {&node};
  switch_node.Switch.items.items = const_cast<Node**>(items);
  switch_node.Switch.items.len = 1;
  gen_switch_if(state, switch_node);
}

auto gen_const(GenState* state, const Node& node) -> void {
  gen_val_decl(state, *node.Const.a, true);
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

auto gen_expr(GenState* state, const Node& node) -> void {
  switch (node.kind) {
    case Node::Kind::Array: {
      // TODO Can nest this in c99, but c++ makes it temporary.
      // TODO And compiling under c++ is also very useful.
      // TODO Either way, before we can finish this, we need structs and maybe
      // TODO tuples.
      printf("(");
      gen_type(state, node.type);
      printf("){");
      if (node.Array.items.len) {
        printf("(");
        if (node.type.arg) {
          gen_type(state, *node.type.arg);
        } else {
          gen_type(state, {Type::Kind::None});
        }
        // Include the size because tcc sometimes needs it.
        printf("[%td]){", node.Array.items.len);
        gen_list_items(state, node);
        printf("}, %td", node.Array.items.len);
      } else {
        // Null pointer and 0 length.
        printf("0");
      }
      printf("}");
      break;
    }
    case Node::Kind::Block: {
      gen_block(state, node);
      break;
    }
    case Node::Kind::Call: {
      gen_expr(state, *node.Call.callee);
      string ends;
      switch (node.Call.callee->type.kind) {
        case Type::Kind::AddressMul: {
          ends = "[]";
          break;
        }
        case Type::Kind::Array: {
          // TODO Generate bounds assert, perhaps via rio_Span_..._get or such.
          printf(".items");
          ends = "[]";
          break;
        }
        default: {
          ends = "()";
          break;
        }
      }
      printf("%c", ends[0]);
      gen_list_items(state, *node.Call.args);
      printf("%c", ends[1]);
      break;
    }
    case Node::Kind::Case: {
      gen_case(state, node);
      break;
    }
    case Node::Kind::Cast: {
      printf("(");
      gen_type(state, node.type);
      printf(")(");
      gen_expr(state, *node.Cast.a);
      printf(")");
      break;
    }
    case Node::Kind::Const: {
      gen_const(state, node);
      break;
    }
    case Node::Kind::Div: {
      gen_binary(state, node, " / ");
      break;
    }
    case Node::Kind::Equal: {
      gen_binary(state, node, " == ");
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
    case Node::Kind::Less: {
      gen_binary(state, node, " < ");
      break;
    }
    case Node::Kind::LessOrEqual: {
      gen_binary(state, node, " <= ");
      break;
    }
    case Node::Kind::Map: {
      gen_map(state, node);
      break;
    }
    case Node::Kind::Member: {
      // TODO If pointer, use ->. If pointer to pointer, deref first, etc?
      // TODO Just use deref for consistency?
      gen_binary(state, node, ".");
      break;
    }
    case Node::Kind::Minus: {
      gen_binary(state, node, " - ");
      break;
    }
    case Node::Kind::More: {
      gen_binary(state, node, " > ");
      break;
    }
    case Node::Kind::MoreOrEqual: {
      gen_binary(state, node, " >= ");
      break;
    }
    case Node::Kind::Mul: {
      gen_binary(state, node, " * ");
      break;
    }
    case Node::Kind::NotEqual: {
      gen_binary(state, node, " != ");
      break;
    }
    case Node::Kind::Plus: {
      gen_binary(state, node, " + ");
      break;
    }
    case Node::Kind::Range: {
      gen_range(state, node);
      break;
    }
    case Node::Kind::Ref: {
      gen_ref(state, node);
      break;
    }
    case Node::Kind::Return: {
      printf("return ");
      gen_expr(state, *node.Return.expr);
      break;
    }
    case Node::Kind::SizeOf: {
      printf("sizeof(");
      gen_type(state, node.SizeOf.expr->type);
      // TODO Types as any other expr.
      //~ gen_expr(state, *node.SizeOf.expr);
      printf(")");
      break;
    }
    case Node::Kind::String: {
      printf("%s", node.String.text);
      break;
    }
    case Node::Kind::Switch: {
      if (node.Switch.arg) {
        gen_switch(state, node);
      } else {
        gen_switch_if(state, node);
      }
      break;
    }
    case Node::Kind::Unsafe: {
      gen_expr(state, *node.Unsafe.expr);
      break;
    }
    case Node::Kind::Use: {
      // Nothing to do here.
      break;
    }
    case Node::Kind::Update: {
      gen_binary(state, node, " = ");
      break;
    }
    case Node::Kind::Var: {
      gen_val_decl(state, *node.Var.expr);
      break;
    }
    default: {
      gen_bad(state, node);
      break;
    }
  }
}

auto gen_for(GenState* state, const Node& node) -> void {
  switch (node.For.arg->type.kind) {
    case Type::Kind::Array: {
      gen_for_span(state, node);
      break;
    }
    case Type::Kind::Range: {
      gen_for_range(state, node);
      break;
    }
    default: {
      printf("(!!! for what !!!)");
      break;
    }
  }
}

auto gen_for_range(GenState* state, const Node& node) -> void {
  printf("{\n");
  {
    Indent _{state};
    // Declare a single expr in case the arg is more than a simple var.
    // TODO Optimize this part away if it is just a ref.
    gen_indent(state);
    string list_name = "rio_list";
    // Generate list value as local.
    gen_type(state, node.For.arg->type);
    printf(" %s = ", list_name);
    gen_expr(state, *node.For.arg);
    printf(";\n");
    // Params.
    // TODO Index.
    auto param = node.For.param;
    auto is_block = node.For.expr->kind == Node::Kind::Block;
    if (!param && is_block) {
      Node* params = node.For.expr->Block.params;
      if (params && params->Tuple.items.len) {
        param = params->Tuple.items[0];
      }
    }
    // The loop itself.
    gen_indent(state);
    auto name =
      param && param->kind == Node::Kind::Ref ? param->Ref.name : "rio_value";
    // TODO Support index var that always starts at 0.
    printf("for (");
    gen_type_opt(state, node.For.arg->type.arg);
    printf(
      // Here, we depend on the C compiler to optimize as appropriate.
      " %s = %s.from; %s.inclusive ? %s <= %s.to : %s < %s.to; %s += %s.by"
      ") {\n",
      name, list_name, list_name, name, list_name, name, list_name, name,
      list_name
    );
    {
      Indent _{state};
      if (is_block) {
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

auto gen_for_span(GenState* state, const Node& node) -> void {
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
      "for (rio_int %s = 0; %s < %s.length; %s += 1) {\n",
      index_name, index_name, list_name, index_name
    );
    {
      Indent _{state};
      if (node.For.expr->kind == Node::Kind::Block) {
        // Params.
        auto param = node.For.param;
        if (!param) {
          Node* params = node.For.expr->Block.params;
          if (params && params->Tuple.items.len) {
            param = params->Tuple.items[0];
          }
        }
        if (param) {
          string name = "";
          // TODO Cast option.
          if (param->kind == Node::Kind::Ref) {
            name = param->Ref.name;
          }
          gen_indent(state);
          gen_type_opt(state, node.For.arg->type.arg);
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

auto gen_if(GenState* state, const Node& node) -> void {
  printf("(!!! if !!!)\n");
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

auto gen_range(GenState* state, const Node& node) -> void {
  // TODO Transform to a struct instance node and generate that way???
  // TODO gen_type(state, node.type); ???
  printf("(");
  gen_type(state, node.type);
  printf("){");
  if (node.Range.from) {
    gen_expr(state, *node.Range.from);
  } else {
    printf("0");
  }
  if (node.Range.to) {
    printf(", ");
    gen_expr(state, *node.Range.to);
  } else {
    // Through the end if unspecified.
    printf(", -1");
  }
  if (node.Range.by) {
    printf(", ");
    gen_expr(state, *node.Range.by);
  } else {
    // Explicit default to step by 1.
    printf(", 1");
  }
  // Implicit end is always inclusive.
  printf(", %s", node.Range.inclusive || !node.Range.to ? "true" : "false");
  printf("}");
}

auto gen_ref(GenState* state, const Node& node) -> void {
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
    for (auto item: block.items) {
      switch (item->kind) {
        case Node::Kind::Fun:
        case Node::Kind::Proc: {
          // Generate these outside of struct.
          break;
        }
        default: {
          gen_indent(state);
          if (item->kind == Node::Kind::Cast) {
            gen_val_decl(state, *item);
          } else {
            gen_bad(state, *item);
          }
          printf(";\n");
          break;
        }
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

auto gen_switch(GenState* state, const Node& node) -> void {
  printf("(!!! switch !!!)\n");
}

auto gen_maybe_block_expr(GenState* state, const Node& node) -> void {
  if (node.kind == Node::Kind::Block) {
    // In expr mode, separate by commas. TODO Nice indentation?
    auto items = node.Block.items;
    for (rint i = 0; i < items.len; i += 1) {
      if (i) {
        printf(", ");
      }
      gen_expr(state, *items[i]);
    }
  } else {
    gen_expr(state, node);
  }
}

auto gen_switch_if(GenState* state, const Node& node) -> void {
  auto first = true;
  if (is_voidish(node.type.kind)) {
    // Statements.
    for (auto item: node.Switch.items) {
      switch (item->kind) {
        case Node::Kind::Case: {
          if (first) {
            printf("if (");
            first = false;
          } else {
            gen_indent(state);
            printf("else if (");
          }
          gen_expr(state, *item->Case.arg);
          printf(") ");
          gen_expr(state, *item->Case.expr);
          break;
        }
        case Node::Kind::Else: {
          gen_indent(state);
          printf("else ");
          gen_expr(state, *item->Else.expr);
          break;
        }
        default: {
          gen_bad(state, node);
          break;
        }
      }
    }
  } else {
    // Expression. If it gets this far, presume ternary `?:` works.
    for (auto item: node.Switch.items) {
      switch (item->kind) {
        case Node::Kind::Case: {
          printf("(");
          gen_expr(state, *item->Case.arg);
          printf(") ? (");
          gen_maybe_block_expr(state, *item->Case.expr);
          printf(") : ");
          break;
        }
        case Node::Kind::Else: {
          printf("(");
          gen_maybe_block_expr(state, *item->Else.expr);
          printf(")");
          break;
        }
        default: {
          gen_bad(state, node);
          break;
        }
      }
    }
  }
}

auto gen_type(GenState* state, const Type& type) -> void {
  switch (type.kind) {
    case Type::Kind::Address:
    case Type::Kind::AddressMul: {
      gen_type(state, *type.arg);
      printf("*");
      break;
    }
    case Type::Kind::Array:
    case Type::Kind::Range: {
      // Resolve (usually!) guarantees a def assigned.
      printf("%s", type.def ? type.def->name : "(!!! GENERIC_TYPE !!!)");
      break;
    }
    case Type::Kind::Bool: {
      printf("bool");
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
    case Type::Kind::Float: {
      printf("rio_float");
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
      printf("rio_int");
      break;
    }
    case Type::Kind::String: {
      printf("rio_string");
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

auto gen_type_opt(GenState* state, Opt<const Type> type) -> void {
  gen_type(state, type ? *type : Type {Type::Kind::None});
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

auto gen_typedef_array(GenState* state, const Def* def) -> void {
  // TODO Wrap generic types in #ifndef #define things in case generated by
  // TODO different mod branches.
  auto& type = *def->type;
  // TODO Make this into a standard struct node, so it gets auto generated.
  gen_mod_header(state);
  printf("\n");
  // These could be generated through different dependency paths, so guard the
  // generation.
  // TODO(@tjp): Be more careful about when this is really needed.
  printf("#ifndef typedef_%s\n", def->name);
  printf("#define typedef_%s\n", def->name);
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
    printf("rio_int length;\n");
  }
  printf("} %s;\n", def->name);
  printf("#endif  // typedef_%s\n", def->name);
}

auto gen_typedef_range(GenState* state, const Def* def) -> void {
  // TODO Wrap generic types in #ifndef #define things in case generated by
  // TODO different mod branches.
  auto& type = *def->type;
  // TODO Make this into a standard struct node, so it gets auto generated.
  gen_mod_header(state);
  printf("\n");
  // TODO(@tjp): Unify guard handling with gen_typedef_array.
  printf("#ifndef typedef_%s\n", def->name);
  printf("#define typedef_%s\n", def->name);
  printf("typedef struct %s {\n", def->name);
  {
    Indent _{state};
    gen_indent(state);
    gen_type(state, *type.arg);
    printf(" from;\n");
    gen_indent(state);
    gen_type(state, *type.arg);
    printf(" to;\n");
    gen_indent(state);
    gen_type(state, *type.arg);
    printf(" by;\n");
    gen_indent(state);
    printf("bool inclusive;\n");
  }
  printf("} %s;\n", def->name);
  printf("#endif  // typedef_%s\n", def->name);
}

auto gen_typedefs(GenState* state) -> void {
  state->start_section = false;
  for (auto def: state->mod->global_defs) {
    // TODO Generate struct definitions for arrays and ranges earlier?
    switch (def->type ? def->type->kind : Type::Kind::None) {
      case Type::Kind::Array: {
        gen_typedef_array(state, def);
        break;
      }
      case Type::Kind::Range: {
        gen_typedef_range(state, def);
        break;
      }
      default: {
        gen_typedef(state, *def->top);
        break;
      }
    }
  }
}

auto gen_val_decl(GenState* state, const Node& node, bool is_const) -> void {
  gen_type(state, node.type);
  printf(is_const ? " const " : " ");
  switch (node.kind) {
    case Node::Kind::Cast: {
      // We already generated the type above, so skip the repeat.
      gen_expr(state, *node.Cast.a);
      break;
    }
    default: {
      gen_expr(state, node);
      break;
    }
  }
}

auto needs_semi(const Node& node) -> bool {
  switch (node.kind) {
    case Node::Kind::Block:
    case Node::Kind::Case:
    case Node::Kind::For:
    case Node::Kind::Fun:
    case Node::Kind::Switch:
    case Node::Kind::Unsafe:
    case Node::Kind::Use: {
      return false;
    }
    default: {
      return true;
    }
  }
}

}}
