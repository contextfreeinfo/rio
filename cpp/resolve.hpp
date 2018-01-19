#pragma once

#include "parse.hpp"

namespace rio {

auto extract_string_data(StringNode& node) {
  auto& data = node.data;
  for (auto& kid: node.kids) {
    auto token = kid.token();
    if (token) {
      switch (token->state) {
        case TokenState::Escape: {
          if (token->text.size() == 1) {
            switch (token->text[0]) {
              case '"': case '\'': case '\\': {
                data += token->text[0];
                break;
              }
              case 'n': {
                data += '\n';
                break;
              }
              default: {
                // TODO What others?
                break;
              }
            }
          } else {
            // TODO Fancier escapes.
          }
          break;
        }
        case TokenState::StringText: {
          data += token->text;
          break;
        }
        default: break;
      }
    }
  }
  // If unterminated, give newline.
  if (!node.kids.empty()) {
    auto& last = node.kids.back();
    auto token = last.token();
    if (token && token->state != TokenState::StringStop) {
      data += "\n";
    }
  }
}

template<typename Tracker>
auto extract(Node& node, Tracker&& tracker) -> void {
  if (node.kind == NodeKind::Token) return;
  auto& info = static_cast<ParentNode&>(*node.info);
  for (auto& kid: info.kids) {
    // See if we have any definitions.
    switch (kid.kind) {
      // TODO Eventually have class, for, params, and struct here, too.
      // TODO Consolidate class, struct, and/or type?
      case NodeKind::Def: case NodeKind::Let: case NodeKind::Type: {
        // TODO Infer types.
        // TODO Instead recurse manually to deal with lists, etc?
        auto id = kid.token_at(1);
        if (id) {
          // std::cout << "Found " << id->text << std::endl;
          auto& kid_info = kid.as<NamedNode>();
          kid_info.name = id->text;
          if (!node.define(id->text, kid)) {
            // TODO Store vector of errors in nodes and/or script?
            // std::cout << "Extra definition: " << *id << std::endl;
          }
          tracker(kid);
        }
        break;
      }
      case NodeKind::String: {
        extract_string_data(static_cast<StringNode&>(*kid.info));
        break;
      }
      default: break;
    }
    // Recurse down.
    extract(kid, tracker);
  }
}

struct Context {

  const Context* parent;

  const Node& scope;

  Context(const Node& scope_, Opt<const Context> parent_ = nullptr):
    parent(parent_), scope(scope_) {}

  auto get_def(std::string_view id) const -> Opt<Node> {
    auto node = scope.get_def(id);
    if (!node && parent) {
      node = parent->get_def(id);
    }
    return node;
  }

  auto resolve(Node& node) const -> void {
    if (node.kind == NodeKind::Token) return;
    auto& info = static_cast<ParentNode&>(*node.info);
    if (&node != &scope && info.symbols) {
      // Deeper context.
      Context inner{node, this};
      inner.resolve(node);
    } else {
      for (auto& kid: info.kids) {
        auto token = kid.token();
        if (token && token->state == TokenState::Id) {
          // Resolve this!
          auto& kid_info = static_cast<IdNode&>(*kid.info);
          // std::cout << "Resolve: " << kid.token->text << std::endl;
          kid_info.referent = get_def(kid_info.token->text);
          // std::cout << "Referent: " << kid.referent << std::endl;
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

  std::vector<Node*> defs;
  Node root;

  // Functions.

  Script(
    std::string_view source, Opt<const Context> env = nullptr
  ): root(NodeKind::Other) {
    // Tokenize.
    rio::Tokenizer tokenizer{source};
    tokens = tokenizer.collect();
    // Parse.
    rio::Parser parser{tokens};
    root = parser.parse();
    // Extract.
    extract(root, [this](Node& def) {
      defs.push_back(&def);
    });
    // std::cout << "Found defs: " << defs.size() << std::endl;
    // Resolve.
    Context context{root, env};
    context.resolve(root);
  }

  private:

  std::vector<Token> tokens;

};

}
