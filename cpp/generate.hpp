#pragma once

#include "resolve.hpp"
#include <stdexcept>

namespace rio {

struct GenState {
  std::string_view prefix;
  GenState(std::string_view prefix_ = "  "): prefix(prefix_) {}
};

struct CGenerator {

  std::ostream& stream;

  CGenerator(std::ostream& stream_): stream(stream_) {}

  auto generate(Script& main) -> void {
    GenState state;
    stream << "int main() {" << std::endl;
    generate_node(state, main.root);
    stream << state.prefix << "return 0;\n";
    stream << "}\n";
  }

  private:

  auto generate_block(GenState& state, ParentNode& node) -> void {
    for (auto& kid: node.kids) {
      generate_node(state, kid);
    }
  }

  auto generate_node(GenState& state, Node& node) -> void {
    switch (node.kind) {
      case NodeKind::Block: {
        generate_block(state, node.as<ParentNode>());
        break;
      }
      case NodeKind::Spaced: {
        generate_spaced(state, node.as<ParentNode>());
        break;
      }
      case NodeKind::String: {
        generate_string(state, node.as<StringNode>());
      }
      default: {
        // TODO Report skipping.
        break;
      }
    }
  }

  auto generate_spaced(GenState& state, ParentNode& node) -> void {
    Index count = 0;
    stream << state.prefix;
    for (auto& kid: node.kids) {
      if (kid.token() && !kid.token()->important()) continue;
      if (!count) {
        // We should validate before we get here, so the first should always be
        // a reference to some named function.
        auto referent = kid.referent();
        if (!referent) {
          // It's okay to fail fast at generation time.
          // TODO This shouldn't be needed once we have prior validation.
          throw std::runtime_error("unresolved call");
        }
        stream << referent->as<NamedNode>().name << "(";
      } else {
        // Args.
        if (count > 1) {
          stream << ", ";
        }
        generate_node(state, kid);
      }
      count += 1;
    }
    stream << ");\n";
  }

  auto generate_string(GenState& state, StringNode& node) -> void {
    (void)state;
    stream << "\"";
    for (auto c: node.data) {
      switch (c) {
        // TODO Other escapes.
        case '\\': case '"': {
          stream << "\\" << c;
          break;
        }
        default: {
          stream << c;
          break;
        }
      }
    }
    stream << "\"";
  }

};

}
