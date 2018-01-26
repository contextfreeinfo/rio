#pragma once

#include "tree.hpp"

namespace rio {

struct Parser {

  // Fields.

  USize found_index;

  USize index;

  USize last_index;

  USize last_skipped;

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

  void chew(Node& parent, USize index) {
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
    One<Node> current = &expr;
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
      auto nesting = token.nesting();
      auto node_kind = infix ?
        token_to_node_kind(token.state) :
        (
          nesting = Nesting::Flat,
          op_precedence = rio::precedence(TokenState::HSpace),
          NodeKind::Spaced
        );
      // We need a parent node for this expression.
      // Also, we have to build out manually when precedence falls.
      // If precedence rises, we'll go deeper into the call stack.
      // The precedence issue here is more for reset situations like new
      // blocks and such.
      // TODO ^ What does that ("reset situations") mean?
      if (
        first ||
        last_precedence > op_precedence ||
        nesting == Nesting::LeftToRight  // or RightToLeft changing curr?
      ) {
        // println!("Got one inside");
        // TODO Figure out types.
        Node outer{node_kind};
        // TODO Replace current, not expr, which is our outermost result.
        push(outer, std::move(expr));
        expr = std::move(outer);
        current = &expr;
        // Past first.
        first = false;
      }
      if (infix) {
        // TODO What for RightToLeft nesting?
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
      // TODO For RightToLeft, make current be the newest??
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

  auto parse_parens(TokenState end_state) -> Node {
    // Build the parent.
    Node node{NodeKind::Parens};
    push_next(node);
    // Go inside.
    auto content = parse_expr(precedence(end_state));
    push(node, std::move(content));
    // See where we ended.
    if (peek().state == end_state) {
      push_next(node);
    } // Else must be end or eof. Just move on.
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
      case TokenState::BracketOpen:
        return parse_parens(TokenState::BracketClose);
      case TokenState::ParenOpen: return parse_parens(TokenState::ParenClose);
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
