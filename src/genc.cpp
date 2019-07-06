#include "main.h"

namespace rio { namespace c {

void gen_expr(const Node& node);
void gen_statements(const Node& node);

void gen(Engine* engine, const Node& tree) {
  printf("#include <stdio.h>\n");
  printf("\n");
  gen_statements(tree);
}

void gen_expr(const Node& node) {
  switch (node.kind) {
    case Node::Kind::Block: {
      printf("{\n");
      gen_statements(node);
      printf("}\n");
      break;
    }
    case Node::Kind::Call: {
      gen_expr(*node.Call.callee);
      printf("(");
      printf(")");
      break;
    }
    case Node::Kind::Fun: {
      printf("void %s() ", node.Fun.name);
      // TODO Special handling of non-block exprs.
      gen_expr(*node.Fun.expr);
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
      printf("(!!! BROKEN !!!)");
      break;
    }
  }
}

void gen_statements(const Node& node) {
  auto items = node.Block.items;
  for (usize i = 0; i < items.len; i += 1) {
    gen_expr(*items[i]);
    printf(";\n");
  }
}

}}
