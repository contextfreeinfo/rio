#include "main.h"

namespace rio { namespace c {

struct GenState {
  usize indent = 0;
  ModManager* mod = nullptr;
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
void gen_block(GenState* state, const Node& node);
auto gen_decl_expr(GenState* state, const Node& node) -> bool;
auto gen_decls(GenState* state, const Node& node) -> bool;
void gen_expr(GenState* state, const Node& node);
void gen_list_items(GenState* state, const Node& node);
void gen_indent(GenState* state);
void gen_param_items(GenState* state, const Node& node);
void gen_type(GenState* state, const Type& type);
void gen_statements(GenState* state, const Node& node);
auto needs_semi(const Node& node) -> bool;

void gen(Engine* engine) {
  GenState state;
  printf(
    "#include <stdint.h>\n"
    "#include <stdio.h>\n"
  );
  // Declarations.
  auto any = true;
  for (auto mod: engine->mods) {
    state.mod = mod;
    if (any) {
      printf("\n");
    }
    any = gen_decls(&state, *mod->tree);
  }
  // Definitions.
  for (auto mod: engine->mods) {
    state.mod = mod;
    printf("\n");
    gen_statements(&state, *mod->tree);
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
  printf("}\n");
}

auto gen_decl_expr(GenState* state, const Node& node) -> bool {
  switch (node.kind) {
    case Node::Kind::Fun: {
      gen_type(state, node.type);
      printf(" %s_%s();\n", state->mod->name, node.Fun.name);
      return true;
    }
    default: {
      // Nothing to do.
      return false;
    }
  }
}

auto gen_decls(GenState* state, const Node& node) -> bool {
  auto any = false;
  for (auto item: node.Block.items) {
    any |= gen_decl_expr(state, *item);
  }
  return any;
}

void gen_expr(GenState* state, const Node& node) {
  switch (node.kind) {
    case Node::Kind::Block: {
      if (!state->indent) {
        printf("int main() ");
      }
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
    case Node::Kind::Const: {
      gen_type(state, node.Const.a->type);
      printf(" const ");
      gen_expr(state, *node.Const.a);
      printf(" = ");
      gen_expr(state, *node.Const.b);
      break;
    }
    case Node::Kind::Float: {
      printf("%s", node.Float.text);
      break;
    }
    case Node::Kind::Fun: {
      // TODO When to mark static?
      bool prefix = !state->indent;
      gen_type(state, node.type);
      printf(
        " %s%s%s(",
        prefix ? state->mod->name : "",
        prefix ? "_" : "",
        node.Fun.name
      );
      gen_param_items(state, *node.Fun.params);
      printf(") ");
      // TODO Special handling of non-block exprs.
      if (node.Fun.expr->kind == Node::Kind::Block) {
        gen_block(state, *node.Fun.expr);
      }
      break;
    }
    case Node::Kind::Int: {
      printf("%s", node.Int.text);
      break;
    }
    case Node::Kind::Ref: {
      printf("%s", node.Ref.name);
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

void gen_indent(GenState* state) {
  for (usize i = 0; i < state->indent; i += 1) {
    printf("  ");
  }
}

void gen_list_items(GenState* state, const Node& node) {
  auto items = node.Tuple.items;
  for (usize i = 0; i < items.len; i += 1) {
    if (i) {
      printf(", ");
    }
    gen_expr(state, *items[i]);
  }
}

void gen_param_items(GenState* state, const Node& node) {
  auto items = node.Tuple.items;
  for (usize i = 0; i < items.len; i += 1) {
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
          gen_bad(state, node);
        }
        break;
      }
      case Node::Kind::Ref: {
        printf("(!!! TYPE !!!) %s", item.Ref.name);
        break;
      }
      default: {
        gen_bad(state, node);
        break;
      }
    }
  }
}

void gen_statements(GenState* state, const Node& node) {
  auto items = node.Block.items;
  bool last_none = false;
  for (usize i = 0; i < items.len; i += 1) {
    if (i && !state->indent && !last_none) {
      // Blank lines between at top level.
      printf("\n");
    }
    gen_indent(state);
    gen_expr(state, *items[i]);
    if (needs_semi(*items[i])) {
      printf(";\n");
      last_none = false;
    } else {
      // TODO Generalize last_none to a function or something.
      last_none = items[i]->kind == Node::Kind::Use;
    }
  }
}

void gen_type(GenState* state, const Type& type) {
  switch (type.kind) {
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
      printf("int");
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
    case Type::Kind::Void: {
      printf("void");
      break;
    }
    default: {
      printf("(!!! TYPE !!!)");
      break;
    }
  }
}

auto needs_semi(const Node& node) -> bool {
  switch (node.kind) {
    case Node::Kind::Block:
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
