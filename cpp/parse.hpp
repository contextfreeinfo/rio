#pragma once

#include "tokenize.hpp"
#include <map>
#include <memory>
#include <sstream>
#include <variant>

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

struct ParentNode {

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

  auto write(std::ostream& stream, std::string_view prefix) const -> void;

};

struct TokenNode {

  optref<Node> referent;
  optref<Token> token;

  TokenNode(Token* token_ = nullptr): referent(nullptr), token(token_) {}

  auto write(std::ostream& stream) const -> void {
    stream << " " << *token;
    if (referent) {
      stream << " @ " << referent;
    }
  }

};

struct Node {

  // Fields.

  NodeKind kind;

  // union {
  //   ParentNode parent;
  //   TokenNode token;
  // };
  std::variant<ParentNode, TokenNode> info;

  // Functions.

  // Node(NodeKind kind_): kind(kind_), parent() {}
  Node(NodeKind kind_): kind(kind_), info(ParentNode{}) {}

  // Node(Token& token_): kind(NodeKind::Token), token(token_) {}
  Node(Token& token_): kind(NodeKind::Token), info(TokenNode{&token_}) {}

  // ~Node() {
  //   switch (kind) {
  //     case NodeKind::Token: {
  //       token.~TokenNode();
  //       break;
  //     }
  //     default: {
  //       parent.~ParentNode();
  //       break;
  //     }
  //   }
  // }

  auto define(std::string_view id, Node& node) -> bool {
    return std::get<ParentNode>(info).define(id, node);
  }

  template<typename Select>
  auto find(Select&& select) -> optref<Node> {
    if (std::holds_alternative<ParentNode>(info)) {
      return std::get<ParentNode>(info).find(select);
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
    if (kind == NodeKind::Token) {
      std::get<TokenNode>(info).write(buffer);
    } else {
      std::get<ParentNode>(info).write(buffer, prefix);
    }
    return buffer.str();
  }

  auto get_def(std::string_view id) const -> optref<Node> {
    return std::get<ParentNode>(info).get_def(id);
  }

  auto push(Node&& node) -> void {
    std::get<ParentNode>(info).push(std::move(node));
  }

  auto push_token(Token& token) -> void {
    std::get<ParentNode>(info).push_token(token);
  }

  auto token() -> optref<Token> {
    if (kind == NodeKind::Token) {
      return std::get<TokenNode>(info).token;
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

auto ParentNode::write(std::ostream& stream, std::string_view prefix) const -> void {
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

struct Parser {

  // Fields.

  Index found_index;

  Index index;

  Index last_index;

  Index last_skipped;

  std::vector<Token>& tokens;

  // Functions.

  Parser(std::vector<Token>& tokens_):
    found_index(0),
    index(0),
    last_index(0),
    last_skipped(0),
    tokens(tokens_)
  {}

  auto parse() -> Node {
    return parse_expr(0);
  }

  private:

  void chew(Node& parent, Index index) {
    // Push all skipped tokens.
    if (last_skipped < index) {
      auto end = tokens.begin() + index - 1;
      for (auto token = tokens.begin() + last_skipped; token < end; ++token) {
        parent.push_token(*token);
      }
    }
    last_skipped = index;
  }

  void focus(Node& parent) {
    index = found_index;
    chew(parent, index + 1);
  }

  auto next(bool skipv = false) -> Token& {
    // We never pass eof, so we'll have a token for sure.
    Token* token;
    auto index = this->index;
    last_index = index;
    auto done = false;
    while (!done) {
      token = &tokens[index];
      switch (token->state) {
        case TokenState::Comment: case TokenState::HSpace: {
          ++index;
          break;
        }
        case TokenState::VSpace: {
          if (skipv) {
            ++index;
          } else {
            done = true;
          }
          break;
        }
        default: {
          done = true;
          break;
        }
      }
    }
    found_index = index;
    if (index < tokens.size()) {
      ++index;
    }
    this->index = index;
    return *token;
  }

  auto parse_do() -> Node {
    // Build the do.
    // println!("start do");
    Node node{NodeKind::Do};
    push_next(node);
    // Go inside.
    auto content = parse_expr(precedence(TokenState::End));
    push(node, std::move(content));
    // See where we ended.
    // println!("after do at {:?}", self.peek());
    if (peek().state == TokenState::End) {
      push_next(node);
    }  // else must be eof. Just move on.
    // println!("then at {:?}", self.peek());
    return node;
  }

  auto parse_expr(int precedence) -> Node {
    auto expr = parse_prefix();
    auto first = true;
    auto last_precedence = precedence;
    while (true) {
      auto& token = peek();
      if (token.state == TokenState::Eof) {
        break;
      }
      // TODO Not infix if postfix instead (attached then space).
      auto infix = token.infix();
      auto op_precedence = token.precedence();
      // println!(
      //   "Peeked at {:?}, {} vs {}, infix: {}",
      //   token, precedence, op_precedence, infix,
      // );
      if (precedence >= op_precedence) {
        // Go to outer level.
        break;
      }
      auto node_kind = infix ?
        token_to_node_kind(token.state) :
        (
          op_precedence = rio::precedence(TokenState::HSpace),
          NodeKind::Spaced
        );
      // In https://github.com/KeepCalmAndLearnRust/pratt-parser they always
      // nest binary ops left to right, so there's no need for this, but I
      // want a flatter tree here, and I also want to keep skip tokens in
      // the tree, so I can't always just nest.
      // TODO Deep tree for binary ops even if not for lists/rows!!!!!
      // TODO The type could change and therefore the referent at every pair!!!
      // TODO Also right-to-left nesting for assignments!
      // So we have to build out manually when precedence falls.
      // If precedence rises, we'll go deeper into the call stack.
      // The precedence issue here is more for reset situations like new blocks
      // and such.
      if (first || last_precedence > op_precedence) {
        // println!("Got one inside");
        // TODO Figure out types.
        Node outer{node_kind};
        push(outer, std::move(expr));
        expr = std::move(outer);
        // Past first.
        first = false;
      }
      if (infix) {
        push_next(expr);
        // We can skip vertical after an infix operator.
        // This includes skipping other empty rows of vspace.
        next(true);
      } else if (token.closing()) {
        // We should have broken out on a close.
        // Must be a stray inside something larger, like a paren before end, or
        // end at top file level.
        // TODO Track stack of what opens we have, so we can break even better
        // TODO than just looking at precedence.
        Node stray{NodeKind::Stray};
        push_next(stray);
        push(expr, std::move(stray));
        // Try again to see what we can see.
        continue;
      }
      // Look at the next nonskippable.
      focus(expr);
      // TODO If infix (vs prefix +/-?), put a None instead of parsing.
      // println!("Post at {:?}", self.peek());
      auto kid = parse_expr(op_precedence);
      push(expr, std::move(kid));
      // Reset expectations for next time.
      last_precedence = op_precedence;
    }
    return expr;
  }

  auto parse_number() -> Node {
    Node number{NodeKind::Number};
    // Juggle our latest token, due to so many optional parts.
    auto* token = &next();
    if (token->state == TokenState::Plus) {
      push_token(number, *token);
      token = &next();
    }
    auto has_int = false;
    if (token->state == TokenState::Int) {
      push_token(number, *token);
      has_int = true;
      token = &next();
    }
    // Just a block for breaking from, not a loop.
    do {
      auto& dot = *token;
      if (token->state == TokenState::Dot) {
        push_token(number, *token);
        token = &next();
      } else {
        prev();
        break;
      }
      if (token->state == TokenState::Fraction) {
        push_token(number, *token);
      } else {
        prev();
        if (!has_int) {
          // TODO Parse this as member access even if there was preceding
          // TODO whitespace!
          return Node{dot};
        }
        break;
      }
      // TODO Exponent.
    } while (false);
    return number;
  }

  auto parse_parens() -> Node {
    // Build the parent.
    Node node{NodeKind::Parens};
    push_next(node);
    // Go inside.
    auto content = parse_expr(precedence(TokenState::ParenClose));
    push(node, std::move(content));
    // See where we ended.
    switch (peek().state) {
      case TokenState::ParenClose: {
        push_next(node);
        break;
      }
      // Must be end or eof. Just move on.
      default: break;
    }
    // println!("then at {:?}", self.peek());
    return node;
  }

  auto parse_prefix() -> Node {
    auto state = peek().state;
    switch (state) {
      case TokenState::Def: case TokenState::Let: case TokenState::Type: {
        // Later, we probably need to diversify here, but this is a start.
        Node node{token_to_node_kind(state)};
        push_next(node);
        push(node, parse_expr(precedence(TokenState::VSpace)));
        return node;
      }
      case TokenState::Do: return parse_do();
      case TokenState::Dot: case TokenState::Int: case TokenState::Plus:
        return parse_number();
      case TokenState::ParenClose: case TokenState::End:
      case TokenState::VSpace: {
        return Node{NodeKind::None};
      }
      case TokenState::Eof: return Node{peek()};
      case TokenState::ParenOpen: return parse_parens();
      case TokenState::StringStart: return parse_string();
      // TODO Add others that are always infix.
      // TODO Branch on paren, do, string, number, ...
      default: return Node{next()};
    }
  }

  auto parse_string() -> Node {
    Node node{NodeKind::String};
    // StringStart, then loop through contents.
    push_next(node);
    auto done = false;
    while (!done) {
      switch (peek().state) {
        case TokenState::StringStop: {
          push_next(node);
          done = true;
          break;
        }
        case TokenState::Eof: case TokenState::VSpace: {
          done = true;
          break;
        }
        default: {
          push_next(node);
          break;
        }
      }
    }
    return node;
  }

  auto peek() -> Token& {
    auto& token = next();
    prev();
    return token;
  }

  void prev() {
    // TODO This results in iterating over space multiple times.
    // TODO Instead remember where the next is.
    index = last_index;
  }

  void push(Node& parent, Node&& node) {
    chew(parent, index);
    if (node.kind != NodeKind::None) {
      // Now push the requested node.
      parent.push(std::move(node));
    }
  }

  void push_next(Node& parent) {
    push_token(parent, next());
  }

  void push_token(Node& parent, Token& token) {
    push(parent, Node{token});
  }

};

}
