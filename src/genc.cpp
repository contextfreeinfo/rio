#include "main.h"

namespace rio { namespace c {

struct GenState {
  usize indent{0};
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

void gen_decl_expr(GenState* state, const Node& node);
void gen_decls(GenState* state, const Node& node);
void gen_expr(GenState* state, const Node& node);
void gen_indent(GenState* state);
void gen_tuple_items(GenState* state, const Node& node);
void gen_type(GenState* state, const Type& type);
void gen_statements(GenState* state, const Node& node);
auto needs_semi(const Node& node) -> bool;

void gen(Engine* engine, const Node& tree) {
  GenState state;
  printf(
    "#include <stdint.h>\n"
    "#include <stdio.h>\n"
  );
  printf("\n");
  assert(tree.kind == Node::Kind::Block);
  gen_decls(&state, tree);
  printf("\n");
  gen_statements(&state, tree);
}

void gen_decl_expr(GenState* state, const Node& node) {
  switch (node.kind) {
    case Node::Kind::Fun: {
      gen_type(state, node.type);
      printf(" %s();\n", node.Fun.name);
      break;
    }
    default: {
      // Nothing to do.
      break;
    }
  }
}

void gen_decls(GenState* state, const Node& node) {
  auto items = node.Block.items;
  for (usize i = 0; i < items.len; i += 1) {
    gen_indent(state);
    gen_decl_expr(state, *items[i]);
  }
}

void gen_expr(GenState* state, const Node& node) {
  switch (node.kind) {
    case Node::Kind::Block: {
      printf("{\n");
      {
        Indent _{state};
        gen_statements(state, node);
      }
      printf("}\n");
      break;
    }
    case Node::Kind::Call: {
      gen_expr(state, *node.Call.callee);
      printf("(");
      gen_tuple_items(state, *node.Call.args);
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
      gen_type(state, node.type);
      printf(" %s() ", node.Fun.name);
      // TODO Special handling of non-block exprs.
      gen_expr(state, *node.Fun.expr);
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
    default: {
      printf("(!!! BROKEN %d !!!)", static_cast<int>(node.kind));
      break;
    }
  }
}

void gen_indent(GenState* state) {
  for (usize i = 0; i < state->indent; i += 1) {
    printf("  ");
  }
}

void gen_statements(GenState* state, const Node& node) {
  auto items = node.Block.items;
  for (usize i = 0; i < items.len; i += 1) {
    if (i && !state->indent) {
      // Blank lines between at top level.
      printf("\n");
    }
    gen_indent(state);
    gen_expr(state, *items[i]);
    if (needs_semi(*items[i])) {
      printf(";\n");
    }
  }
}

void gen_tuple_items(GenState* state, const Node& node) {
  auto items = node.Tuple.items;
  for (usize i = 0; i < items.len; i += 1) {
    if (i) {
      printf(", ");
    }
    gen_expr(state, *items[i]);
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
    case Node::Kind::Fun: {
      return false;
    }
    default: {
      return true;
    }
  }
}

}}
