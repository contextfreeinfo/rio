#pragma once

#include "parse.hpp"

namespace rio {

auto extract(Node& node) -> void {
  for (auto& kid: node.kids) {
    // See if we have any definitions.
    switch (kid.kind) {
      // TODO Eventually have class, for, params, and struct here, too.
      // TODO Consolidate class, struct, and/or type?
      case NodeKind::Def: case NodeKind::Let: case NodeKind::Type: {
        // TODO Instead recurse manually to deal with lists, etc?
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

  Context* parent;

  Node& scope;

  Context(Node& scope_, Context* parent_ = nullptr):
    parent(parent_), scope(scope_) {}

  auto resolve(Node& node) -> void {
    if (&node != &scope && node.symbols) {
      // Deeper context.
      Context inner{node, this};
      inner.resolve(node);
    } else {
      for (auto& kid: node.kids) {
        if (kid.token) {
          if (kid.token->state == TokenState::Id) {
            // Resolve this!
            // std::cout << "Resolve: " << kid.token->text << std::endl;
          }
        } else {
          // Deeper node.
          // Tokens shouldn't have kids, so `else` is okay.
          resolve(kid);
        }
      }
    }
  }

};

struct Script {
  // Holds all the resources for a script as well as providing access to script
  // info, such as the ast.

  // Fields.

  Node root;

  // Functions.

  Script(
    std::string_view source, Context* env = nullptr
  ): root(NodeKind::Other) {
    // Tokenize.
    rio::Tokenizer tokenizer{source};
    tokens = tokenizer.collect();
    // Parse.
    rio::Parser parser{tokens};
    root = parser.parse();
    // Extract.
    extract(root);
    // Resolve.
    Context context{root, env};
    context.resolve(root);
  }

  private:

  std::vector<Token> tokens;

};

}
