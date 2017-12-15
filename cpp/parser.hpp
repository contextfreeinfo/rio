#pragma once

#include "tokenizer.hpp"
// #include <functional>
// #include <optional>
#include <sstream>

namespace rio {

enum struct NodeKind {
  Add,
  Assign,
  Block,
  Do,
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
};

auto name(NodeKind kind) -> std::string_view {
  switch (kind) {
    case NodeKind::Add: return "Add";
    case NodeKind::Assign: return "Assign";
    case NodeKind::Block: return "Block";
    case NodeKind::Do: return "Do";
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
    default: return "?";
  }
}

auto operator<<(std::ostream& stream, NodeKind kind) -> std::ostream& {
  return stream << name(kind);
}

auto token_to_node_kind(TokenState token_state) -> NodeKind {
  switch (token_state) {
    case TokenState::Assign: return NodeKind::Assign;
    case TokenState::HSpace: return NodeKind::Spaced;
    case TokenState::Plus: return NodeKind::Add;
    case TokenState::Times: return NodeKind::Multiply;
    case TokenState::VSpace: return NodeKind::Block;
    default: return NodeKind::Other;
  }
}

template<typename Item>
using optref = Item*;
// Alternative that seems perhaps overkill:
// using optref = std::optional<std::reference_wrapper<Item>>;

struct Node {

  // Fields.

  std::vector<Node> kids;

  NodeKind kind;

  optref<const Token> token;
  // optref<Token> token;
  
  // Functions.

  Node(NodeKind kind_): kind(kind_) {}

  Node(const Token& token_): kind(NodeKind::Token), token(&token_) {}
  // Node(Token& token_): kind(NodeKind::Token), token(&token_) {}
  
  auto format() -> std::string {
    return format_at("");
  }

  auto push(Node&& node) {
    kids.push_back(node);
  }

  auto push_token(const Token& token) {
    push(Node(token));
  }

  private:

  auto format_at(std::string_view prefix) -> std::string {
    // Might not be the most efficient, since it doesn't work on a single big
    // buffer, but it shouldn't be needed except for debugging.
    // TODO Pass down the string to append to?
    std::stringstream buffer;
    buffer << prefix << kind;
    if (token) {
      buffer << " " << *token;
    } else {
      std::string deeper{prefix};
      deeper += "  ";
      for (auto& kid: kids) {
        buffer << "\n" << kid.format_at(deeper);
      }
    }
    return buffer.str();
  }

};

struct Parser {

  // Fields.

  Index found_index;

  Index index;

  Index last_index;

  Index last_skipped;

  const std::vector<Token>& tokens;

  // Functions.

  Parser(const std::vector<Token>& tokens_):
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

  auto next(bool skipv = false) -> const Token& {
    // We never pass eof, so we'll have a token for sure.
    const Token* token;
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
    switch (peek().state) {
      case TokenState::End: {
        push_next(node);
        break;
      }  // else must be eof. Just move on.
    }
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
    auto has_int = false;
    switch (token->state) {
      case TokenState::Int: {
        push_token(number, *token);
        has_int = true;
        token = &next();
        break;
      }
    };
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
      }  // else must be end or eof. Just move on.
    }
    // println!("then at {:?}", self.peek());
    return node;
  }

  auto parse_prefix() -> Node {
    switch (peek().state) {
      case TokenState::Do: return parse_do();
      case TokenState::Dot: case TokenState::Int: return parse_number();
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

  auto peek() -> const Token& {
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

  void push_token(Node& parent, const Token& token) {
    push(parent, Node{token});
  }

};

}
