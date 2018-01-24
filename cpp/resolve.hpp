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

  const Context& std;

  Context(const Node& scope_, Opt<const Context> parent_ = nullptr):
    parent(parent_), scope(scope_), std(parent_ ? parent_->std : *this) {}

  auto get_def(std::string_view id) const -> Opt<Node> {
    auto node = scope.get_def(id);
    if (!node && parent) {
      node = parent->get_def(id);
    }
    return node;
  }

  auto infer_type(Node& node) const -> void {
    switch (node.kind) {
      case NodeKind::Assign: {
        // TODO If the thing in the left is untyped, infer from the right.
        break;
      }
      default: {
        // TODO Other cases.
        break;
      }
    }
  }

  auto resolve(Node& node) const -> void {
    // Some base and other special cases.
    switch (node.kind) {
      case NodeKind::Number: {
        if (static_cast<NumberNode&>(*node.info).is_fraction()) {
          node.type = std.get_def("F64")->type;
        } else {
          node.type = std.get_def("I64")->type;
        }
        return;
      }
      case NodeKind::String: {
        // TODO String/array type!
        return;
      }
      case NodeKind::Token: {
        auto token = node.token();
        if (token->state == TokenState::Id) {
          // Resolve this!
          // TODO Get the type of the referent.
          // TODO If this is member access, we'll need to look elsewhere.
          auto& info = static_cast<IdNode&>(*node.info);
          // std::cout << "Resolve: " << node.token->text << std::endl;
          info.referent = get_def(info.token->text);
          // std::cout << "Referent: " << node.referent << std::endl;
        }
        return;
      }
      default: {
        // First see if we have a new scope.
        auto& info = static_cast<ParentNode&>(*node.info);
        if (&node != &scope && info.symbols) {
          // Deeper context.
          Context inner{node, this};
          inner.resolve(node);
          return;
        } else {
          // Move on in the current context.
          break;
        }
      }
    }
    // Parent with arbitrary kids, so recurse down.
    auto& info = static_cast<ParentNode&>(*node.info);
    for (auto& kid: info.kids) {
      resolve(kid);
    }
    // TODO Also resolve overloaded operators.
    // Kids done, determine the type of this node.
    // TODO We also need to pass in expected type for some literal type like
    // TODO objects, anonymous function return types, ...
    infer_type(node);
  }

};

struct Script {
  // Holds all the resources for a script as well as providing access to script
  // info, such as the ast.

  // Fields.

  // We track these so far just so we can order them for nicer ast output.
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
