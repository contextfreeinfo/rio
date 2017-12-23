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
    // See if we have any definitions.
    switch (kid.kind) {
      // TODO Eventually have class, for, and struct here, too?
      // TODO Consolidate class, struct, and/or type?
      case NodeKind::Def: case NodeKind::Let: case NodeKind::Type: {
        auto id = kid.token_at(1);
        if (id) {
          // std::cout << "Found " << id->text << std::endl;
          if (!node.define(id->text, kid)) {
            // TODO Store vector of errors in nodes and/or script?
            // std::cout << "Extra definition: " << *id << std::endl;
          }
        }
        break;
      }
      default: break;
    }
    // Recurse down.
    extract(kid);
  }
}

struct Context {
  //
};

struct Resolver {
  //
};

struct Script {
  // Holds all the resources for a script as well as providing access to script
  // info, such as the ast.

  // Fields.

  Node root;

  // Functions.

  Script(std::string_view source): root(NodeKind::Other) {
    // Tokenize.
    rio::Tokenizer tokenizer{source};
    tokens = tokenizer.collect();
    // Parse.
    rio::Parser parser{tokens};
    root = parser.parse();
    // Extract.
    extract(root);
  }

  private:

  std::vector<Token> tokens;

};

const Script std_script{std_source};

}
