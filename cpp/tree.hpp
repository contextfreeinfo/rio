#pragma once

#include "tokenize.hpp"
#include "type.hpp"
#include "util.hpp"
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <sstream>

namespace rio {

enum struct NodeKind {
  Add,
  Arrow,
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
    case NodeKind::Arrow: return "Arrow";
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
    case TokenState::Arrow: return NodeKind::Arrow;
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

struct GenState {

  std::string_view prefix;

  std::ostream& stream;

  GenState(std::ostream& stream_, std::string_view prefix_ = ""):
    prefix(prefix_), stream(stream_) {}

  GenState(GenState& state, std::string_view prefix = ""):
    GenState(state.stream, prefix) {}

};

struct Node;

struct NodeFormatContext {

  std::string_view prefix;
  const Map<Node*, USize>& symbol_indices;

  NodeFormatContext(
    const Map<Node*, USize>& symbol_indices_, std::string_view prefix_
  ): prefix(prefix_), symbol_indices(symbol_indices_) {}

  NodeFormatContext(const NodeFormatContext& parent, std::string_view prefix_):
    NodeFormatContext(parent.symbol_indices, prefix_) {}

};

struct NodeInfo {
  virtual ~NodeInfo() {}
  virtual auto write(std::ostream& stream, const NodeFormatContext& context)
    const -> void = 0;
};

struct ParentNode: NodeInfo {

  // Never applies to tokens.
  std::vector<Node> kids;
  // Only applies scopes with symbols defined.
  Box<Map<std::string_view, Node*>> symbols;

  auto define(std::string_view id, Node& node) -> bool {
    if (!symbols) {
      symbols.reset(new Map<std::string_view, Node*>);
    }
    auto result = symbols->insert({id, &node});
    return result.second;
  }

  template<typename Select>
  auto find(Select&& select) -> Opt<Node>;

  auto get_def(std::string_view id) const -> Opt<Node> {
    if (!symbols) return nullptr;
    auto entry = symbols->find(id);
    return entry == symbols->end() ? nullptr : entry->second;
  }

  auto push(Node&& node) -> void {
    kids.push_back(std::move(node));
  }

  auto push_token(Token& token) -> void;

  auto write(std::ostream& stream, const NodeFormatContext& context) const
    -> void override;

};

struct NamedNode: ParentNode {
  // TODO Qualified names.
  std::string name;
};

struct DefNode: NamedNode {
  // TODO Mark and sweep to reduce output size for exes.
  // TODO Do we want custom generators????
  // std::optional<std::function<void(GenState& stream, Node& node)>> generate;
};

struct TypeNode: NamedNode {
  Type type;
};

struct NumberNode: ParentNode {

  union {
    F64 f64;
    I64 i64;
  } data;

  auto is_fraction() -> bool;

  // TODO Something to calculate the value.

};

struct StringNode: ParentNode {

  std::string data;

  auto write(std::ostream& stream, const NodeFormatContext& context) const
    -> void override
  {
    // TODO Escape all over again.
    stream << " \"" << data << "\"";
    ParentNode::write(stream, context);
  }

};

struct TokenNode: NodeInfo {

  Opt<Token> token;

  TokenNode(Token* token_ = nullptr): token(token_) {}

  auto write(std::ostream& stream, const NodeFormatContext& context) const
    -> void override
  {
    (void)&context;
    stream << " " << *token;
  }

};

struct IdNode: TokenNode {

  Opt<Node> referent;

  IdNode(Token* token = nullptr): TokenNode(token), referent(nullptr) {}

  auto write(std::ostream& stream, const NodeFormatContext& context) const
    -> void override
  {
    TokenNode::write(stream, context);
    if (referent) {
      stream << " @ " << context.symbol_indices.at(referent);
    }
  }

};

struct Node {

  // Fields.

  NodeKind kind;

  Box<NodeInfo> info;

  Opt<Type> type = nullptr;

  // Functions.

  Node(NodeKind kind_): kind(kind_) {
    switch (kind) {
      case NodeKind::Def: {
        info.reset(new DefNode);
        break;
      }
      case NodeKind::Let: {
        info.reset(new NamedNode);
        break;
      }
      case NodeKind::Number: {
        info.reset(new NumberNode);
        break;
      }
      case NodeKind::String: {
        info.reset(new StringNode);
        break;
      }
      case NodeKind::Type: {
        info.reset(new TypeNode);
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

  template<typename Type>
  auto as() -> Type& {
    return static_cast<Type&>(*info);
  }

  auto define(std::string_view id, Node& node) -> bool {
    return dynamic_cast<ParentNode&>(*info.get()).define(id, node);
  }

  template<typename Select>
  auto find(Select&& select) -> Opt<Node> {
    auto parent = dynamic_cast<ParentNode*>(info.get());
    if (parent) {
      return parent->find(select);
    } else {
      return nullptr;
    }
  }

  // TODO Change this to writing to an ostream!
  auto format(const Map<Node*, USize>& symbol_indices) const
    -> std::string
  {
    NodeFormatContext context{symbol_indices, ""};
    return format_at(context);
  }

  auto format_at(const NodeFormatContext& context) const -> std::string {
    // This wasn't just streaming because I had trouble working that out in the
    // Rust version.
    // TODO Go back to streams.
    std::stringstream buffer;
    buffer << context.prefix << kind;
    if (type && type->node) {
      buffer << " <" << static_cast<TypeNode&>(*type->node->info).name;
      auto pair = context.symbol_indices.find(type->node);
      if (pair != context.symbol_indices.end()) {
        buffer << " @ " << pair->second;
      }
      buffer << ">";
    }
    info->write(buffer, context);
    return buffer.str();
  }

  auto get_def(std::string_view id) const -> Opt<Node> {
    return dynamic_cast<ParentNode&>(*info).get_def(id);
  }

  auto push(Node&& node) -> void {
    dynamic_cast<ParentNode&>(*info).push(std::move(node));
  }

  auto push_token(Token& token) -> void {
    dynamic_cast<ParentNode&>(*info).push_token(token);
  }

  auto referent() -> Opt<Node> {
    auto token = this->token();
    if (token && token->state == TokenState::Id) {
      return static_cast<IdNode&>(*info).referent;
    }
    return nullptr;
  }

  auto token() -> Opt<Token> {
    if (kind == NodeKind::Token) {
      return static_cast<TokenNode&>(*info).token;
    }
    return nullptr;
  }

  auto token_at(USize index) -> Opt<Token> {
    USize count = 0;
    auto token_node = find([&count, index](Node& node) {
      if (node.token() && node.token()->important()) {
        if (count == index) return true;
        count += 1;
      }
      return false;
    });
    return token_node ? token_node->token() : nullptr;
  }

  auto type_defined() const -> Opt<Type> {
    if (kind == NodeKind::Type) {
      return &static_cast<TypeNode&>(*info).type;
    }
    return nullptr;
  }

};

inline auto NumberNode::is_fraction() -> bool {
  return find([](Node& node) {
    return node.token() && node.token()->state == TokenState::Dot;
  });
}

template<typename Select>
inline auto ParentNode::find(Select&& select) -> Opt<Node> {
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

inline auto ParentNode::push_token(Token& token) -> void {
  push(Node{token});
}

inline auto ParentNode::write(
  std::ostream& stream, const NodeFormatContext& context
) const -> void {
  // Symbols.
  if (symbols) {
    stream << " {\n";
    // We don't store order in the node, but we want to output by order.
    // First, get a stable vector of pairs.
    std::vector<Map<Str, Node*>::iterator> pairs;
    for (auto pair = symbols->begin(); pair != symbols->end(); ++pair) {
      pairs.push_back(pair);
    }
    // Figure out their indices.
    std::vector<USize> indices;
    for (auto pair = pairs.begin(); pair < pairs.end(); ++pair) {
      indices.push_back(context.symbol_indices.at((*pair)->second));
    }
    // Get the sorting order.
    std::vector<USize> order;
    order.reserve(indices.size());
    for (USize i = 0; i < indices.size(); ++i) {
      order.push_back(i);
    }
    std::sort(order.begin(), order.end(), [&indices](USize a, USize b) {
      return indices[a] < indices[b];
    });
    // Now output in order.
    for (auto i: order) {
      auto index = indices[i];
      auto& key = pairs[i]->first;
      stream << context.prefix << "  " << key << ": " << index << ",\n";
    }
    stream << context.prefix << "}";
  }
  // Kids.
  std::string deeper_prefix{context.prefix};
  deeper_prefix += "  ";
  NodeFormatContext deeper{context, deeper_prefix};
  for (auto& kid: kids) {
    stream << "\n" << kid.format_at(deeper);
  }
}

}
