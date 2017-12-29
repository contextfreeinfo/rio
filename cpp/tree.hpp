#pragma once

#include "tokenize.hpp"
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <sstream>

namespace rio {

enum struct NodeKind {
  Add,
  Assign,
  Block,
  Colon,
  Comma,
  Def,
  Do,
  Let,
  Multiply,
  None,
  Number,
  Other,
  Parens,
  Row,
  Spaced,
  Stray,
  String,
  Token,
  Top,
  Type,
};

auto name(NodeKind kind) -> std::string_view {
  switch (kind) {
    case NodeKind::Add: return "Add";
    case NodeKind::Assign: return "Assign";
    case NodeKind::Block: return "Block";
    case NodeKind::Colon: return "Colon";
    case NodeKind::Comma: return "Comma";
    case NodeKind::Def: return "Def";
    case NodeKind::Do: return "Do";
    case NodeKind::Let: return "Let";
    case NodeKind::Multiply: return "Multiply";
    case NodeKind::None: return "None";
    case NodeKind::Number: return "Number";
    case NodeKind::Other: return "Other";
    case NodeKind::Parens: return "Parens";
    case NodeKind::Row: return "Row";
    case NodeKind::Spaced: return "Spaced";
    case NodeKind::Stray: return "Stray";
    case NodeKind::String: return "String";
    case NodeKind::Token: return "Token";
    case NodeKind::Top: return "Top";
    case NodeKind::Type: return "Type";
    default: return "?";
  }
}

auto operator<<(std::ostream& stream, NodeKind kind) -> std::ostream& {
  return stream << name(kind);
}

auto token_to_node_kind(TokenState token_state) -> NodeKind {
  switch (token_state) {
    case TokenState::Assign: return NodeKind::Assign;
    case TokenState::Colon: return NodeKind::Colon;
    case TokenState::Comma: return NodeKind::Comma;
    case TokenState::Def: return NodeKind::Def;
    case TokenState::HSpace: return NodeKind::Spaced;
    case TokenState::Let: return NodeKind::Let;
    case TokenState::Plus: return NodeKind::Add;
    case TokenState::Semi: case TokenState::VSpace: return NodeKind::Block;
    case TokenState::Times: return NodeKind::Multiply;
    case TokenState::Type: return NodeKind::Type;
    default: return NodeKind::Other;
  }
}

template<typename Item>
using optref = Item*;

struct Node;

struct NodeInfo {
  virtual ~NodeInfo() {}
  virtual auto write(std::ostream& stream, std::string_view prefix) const
    -> void = 0;
};

struct ParentNode: NodeInfo {

  // Never applies to tokens.
  std::vector<Node> kids;
  // Only applies scopes with symbols defined.
  std::unique_ptr<std::map<std::string_view, Node*>> symbols;

  auto define(std::string_view id, Node& node) -> bool {
    if (!symbols) {
      symbols.reset(new std::map<std::string_view, Node*>);
    }
    auto result = symbols->insert({id, &node});
    return result.second;
  }

  template<typename Select>
  auto find(Select&& select) -> optref<Node>;

  auto get_def(std::string_view id) const -> optref<Node> {
    if (!symbols) return nullptr;
    auto entry = symbols->find(id);
    return entry == symbols->end() ? nullptr : entry->second;
  }

  auto push(Node&& node) -> void {
    kids.push_back(std::move(node));
  }

  auto push_token(Token& token) -> void;

  auto write(std::ostream& stream, std::string_view prefix) const -> void
    override;

};

struct DefNode: ParentNode {
  std::optional<std::function<void(std::ostream& stream, Node& node)>> generate;
};

struct StringNode: ParentNode {

  std::string data;

  auto write(std::ostream& stream, std::string_view prefix) const -> void
    override
  {
    // TODO Escape all over again.
    stream << " \"" << data << "\"";
    ParentNode::write(stream, prefix);
  }

};

struct TokenNode: NodeInfo {

  optref<Token> token;

  TokenNode(Token* token_ = nullptr): token(token_) {}

  auto write(std::ostream& stream, std::string_view prefix) const -> void
    override
  {
    (void)prefix;
    stream << " " << *token;
  }

};

struct IdNode: TokenNode {

  optref<Node> referent;

  IdNode(Token* token = nullptr): TokenNode(token), referent(nullptr) {}

  auto write(std::ostream& stream, std::string_view prefix) const -> void
    override
  {
    TokenNode::write(stream, prefix);
    if (referent) {
      stream << " @ " << referent;
    }
  }

};

struct Node {

  // Fields.

  NodeKind kind;

  std::unique_ptr<NodeInfo> info;

  // Functions.

  Node(NodeKind kind_): kind(kind_) {
    switch (kind) {
      case NodeKind::Def: {
        info.reset(new DefNode);
        break;
      }
      case NodeKind::String: {
        info.reset(new StringNode);
        break;
      }
      default: {
        info.reset(new ParentNode);
        break;
      }
    }
  }

  Node(Token& token): kind(NodeKind::Token), info(
    token.state == TokenState::Id ? new IdNode{&token} : new TokenNode{&token}
  ) {}

  auto define(std::string_view id, Node& node) -> bool {
    return dynamic_cast<ParentNode&>(*info.get()).define(id, node);
  }

  template<typename Select>
  auto find(Select&& select) -> optref<Node> {
    auto parent = dynamic_cast<ParentNode*>(info.get());
    if (parent) {
      return parent->find(select);
    } else {
      return nullptr;
    }
  }

  // TODO Change this to writing to an ostream!
  auto format() const -> std::string {
    return format_at("");
  }

  auto format_at(std::string_view prefix) const -> std::string {
    // This wasn't just streaming because I had trouble working that out in the
    // Rust version.
    // TODO Go back to streams.
    std::stringstream buffer;
    buffer << prefix << kind;
    info->write(buffer, prefix);
    return buffer.str();
  }

  auto get_def(std::string_view id) const -> optref<Node> {
    return dynamic_cast<ParentNode&>(*info).get_def(id);
  }

  auto push(Node&& node) -> void {
    dynamic_cast<ParentNode&>(*info).push(std::move(node));
  }

  auto push_token(Token& token) -> void {
    dynamic_cast<ParentNode&>(*info).push_token(token);
  }

  auto referent() -> optref<Node> {
    auto token = this->token();
    if (token && token->state == TokenState::Id) {
      return static_cast<IdNode&>(*info).referent;
    }
    return nullptr;
  }

  auto token() -> optref<Token> {
    if (kind == NodeKind::Token) {
      return static_cast<TokenNode&>(*info).token;
    }
    return nullptr;
  }

  auto token_at(Index index) -> optref<Token> {
    Index count = 0;
    auto token_node = find([&count, index](Node& node) {
      if (node.token() && node.token()->important()) {
        if (count == index) return true;
        count += 1;
      }
      return false;
    });
    return token_node ? token_node->token() : nullptr;
  }

};

template<typename Select>
auto ParentNode::find(Select&& select) -> optref<Node> {
  for (auto& kid: kids) {
    if (select(kid)) {
      return &kid;
    } else {
      auto result = kid.find(select);
      if (result) {
        return result;
      }
    }
  }
  return nullptr;
}

auto ParentNode::push_token(Token& token) -> void {
  push(Node{token});
}

auto ParentNode::write(std::ostream& stream, std::string_view prefix) const
  -> void
{
  // Symbols.
  if (symbols) {
    stream << " {\n";
    for (auto& [key, ref]: *symbols) {
      stream << prefix << "  " << key << ": " << ref << ",\n";
    }
    stream << prefix << "}";
  }
  // Kids.
  std::string deeper{prefix};
  deeper += "  ";
  for (auto& kid: kids) {
    stream << "\n" << kid.format_at(deeper);
  }
}

}
