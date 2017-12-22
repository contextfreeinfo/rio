#pragma once

#include "parse.hpp"

namespace rio {

// TODO Define externally and include (preparsed?) during build.
const std::string_view std_source = R"std_source(

type String
type Void

def print(line: String): Void = extern

)std_source";

auto extract(Node& node) -> void {
  for (auto& kid: node.kids) {
    switch (kid.kind) {
      case NodeKind::Assign: case NodeKind::Comma: case NodeKind::Spaced: {
        // TODO See if we start with type, def, or let. Or class or struct?
        break;
      }
      default: break;
    }
  }
}

struct Context {
  //
};

struct Resolver {
  //
};

const Script std_script{std_source};

}
