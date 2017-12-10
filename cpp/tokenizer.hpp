#pragma once

#include <string_view>
#include <vector>

namespace rio {

using Index = size_t;

enum struct TokenState {
  Assign,
  Comment,
  Do,
  Dot,
  End,
  Eof,
  Error,
  EscapeStart,
  Escape,
  Fraction,
  HSpace,
  Id,
  Int,
  Op,
  Op1,
  Op2,
  ParenOpen,
  ParenClose,
  Plus,
  Start,
  StringStart,
  StringStop,
  StringText,
  Times,
  VSpace,
};

auto closing(TokenState state) -> bool {
  switch (state) {
    case TokenState::End: {
      return true;
    }
    default: {
      return false;
    }
  }
}

auto infix(TokenState state) -> bool {
  switch (state) {
    case TokenState::Assign:
    case TokenState::Op:
    case TokenState::Op1:
    case TokenState::Op2:
    case TokenState::Plus:
    case TokenState::Times:
    case TokenState::VSpace: {
      return true;
    }
    default: {
      return false;
    }
  }
}

auto precedence(TokenState state) -> int {
  switch (state) {
    case TokenState::Eof: return 0;
    case TokenState::End: return 5;
    // TODO Same precedence as End but with open-token stack tracking?
    case TokenState::ParenClose: return 7;
    case TokenState::Comment:
    case TokenState::VSpace: return 10;
    case TokenState::HSpace: return 20;
    case TokenState::Plus: return 30;
    case TokenState::Times: return 40;
    default: return 20;
  }
}

struct Token {

  // Fields.

  Index line;

  // Col in code points.
  Index col;

  // Index in bytes.
  Index index;

  TokenState state;

  std::string_view text;

  // Functions.

  auto closing() -> bool {
    return ::rio::closing(state);
  }

  auto infix() -> bool {
    return ::rio::infix(state);
  }

  auto precedence() -> bool {
    return ::rio::precedence(state);
  }

};

struct Tokenizer {

  Tokenizer(std::string_view buffer) {
    this->buffer = buffer;
    col_index = 0;
    gave_eof = false;
    last_start = 0;
    line_index = 0;
    start_line = 0;
    start_col = 0;
    state = TokenState::Start;
    string_start = '?';
  }

  auto collect() -> std::vector<Token> {
    std::vector<Token> tokens;
    while (true) {
      auto token = next();
      tokens.push_back(token);
      if (token.state == TokenState::Eof) {
        break;
      }
    }
    return tokens;
  }

  private:

  // Fields.

  std::string_view buffer;

  Index char_index;

  Index col_index;

  bool gave_eof;

  Index last_start;

  Index line_index;

  Index start_col;

  Index start_line;

  TokenState state;

  // TODO Code point type?
  char string_start;

  // Functions.

# if 0
  
  impl<'a> Tokenizer<'a> {
  
    fn find_key(&self, text: &str) -> TokenState {
      let mut chars = text.chars();
      match chars.next() {
        Some('d') => match chars.next() {
          Some('o') => match chars.next() {
            None => TokenState::Do,
            _ => TokenState::Id,
          }
          _ => TokenState::Id,
        }
        Some('e') => match chars.next() {
          Some('n') => match chars.next() {
            Some('d') => match chars.next() {
              None => TokenState::End,
              _ => TokenState::Id,
            }
            _ => TokenState::Id,
          }
          _ => TokenState::Id,
        }
        _ => TokenState::Id,
      }
    }
  
    fn find_op(&self, text: &str) -> TokenState {
      let mut chars = text.chars();
      match chars.next() {
        Some('=') => match chars.next() {
          None => TokenState::Assign,
          _ => TokenState::Op,
        }
        Some('(') => TokenState::ParenOpen,
        Some(')') => TokenState::ParenClose,
        Some('+') | Some('-') => match chars.next() {
          None => TokenState::Plus,
          _ => TokenState::Op,
        }
        Some('*') | Some('/') => match chars.next() {
          None => TokenState::Times,
          _ => TokenState::Op,
        }
        _ => TokenState::Op,
      }
    }
  
  }

#endif

  auto find_new_state(char c) -> TokenState {
    switch (state) {
      case TokenState::Comment: {
        switch (c) {
          case '\n': case '\r': return TokenState::VSpace;
          default: return TokenState::Comment;
        }
      }
      case TokenState::Escape: return TokenState::StringText;
      case TokenState::EscapeStart: return TokenState::Escape;
      case TokenState::StringStart: case TokenState::StringText: {
        switch (c) {
          case '\\': return TokenState::EscapeStart;
          case '\'': case '"': {
            if (c == string_start) {
              return TokenState::StringStop;
            } else {
              return TokenState::StringText;
            }
          }
          case '\n': case '\r': return TokenState::VSpace;
          default: return TokenState::StringText;
        }
      }
      default: {
        switch (c) {
          case '#': return TokenState::Comment;
          case ' ': case '\t': return TokenState::HSpace;
          case '.': return TokenState::Dot;
          // TODO '_' as digits separator.
          case '_': return TokenState::Id;
          case ',': case ';': case ':': case '(': case ')': case '[': case ']':
          case '{': case '}': {
            return TokenState::Op1;
          }
          case '+': case '-': case '*': case '/': case '=': {
            // TODO Deal with compound operators on these.
            return TokenState::Op2;
          }
          case '\'': case '"': {
            // Side effect!
            string_start = c;
            return TokenState::StringStart;
          }
          case '\n': case '\r': return TokenState::VSpace;
          default: {
            if (c >= '0' && c <= '9') {
              switch (this->state) {
                case TokenState::Dot: return TokenState::Fraction;
                case TokenState::Fraction: return TokenState::Fraction;
                case TokenState::Id: return TokenState::Id;
                default: return TokenState::Int;
              }
            } else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
              return TokenState::Id;
            } else {
              return TokenState::Error;
            }
          }
        }
      }
    };
}

  auto next() -> Token {
    Index stop_index;
    Index last_start = this->last_start;
    Index start_col = this->start_col;
    Index start_line = this->start_line;
    auto state = this->state;
    while (true) {
      Index index = char_index;
      if (index >= buffer.size()) {
        stop_index = buffer.size();
        this->last_start = stop_index;
        break;
      }
      char c = buffer[index];
      // TODO(tjp): Advance by utf8 code points.
      ++char_index;
      auto new_state = find_new_state(c);
      stop_index = index;
      state = this->state;
      auto state_changed = new_state != state;
      if (state_changed) {
        this->state = new_state;
        this->last_start = index;
        this->start_line = this->line_index;
        this->start_col = this->col_index;
      }
      // Count rows and cols.
      this->col_index += 1;
      if (c == '\n') {
        this->line_index += 1;
        this->col_index = 0;
      }
      // If the first char, we might need to keep going.
      if (state_changed && index > last_start) {
        break;
      }
    }
    if (stop_index == last_start) {
      // if (this->gave_eof) {
      // TODO Fail out somehow?
      //   return None;
      // }
      // Eof token lets parser ignore None case better.
      state = TokenState::Eof;
      gave_eof = true;
    }
    // We have a token to give.
    auto text = buffer.substr(last_start, stop_index - last_start);
    // state = match state {
    //   TokenState::Id => self.find_key(text),
    //   TokenState::Op1 | TokenState::Op2 => self.find_op(text),
    //   _ => state,
    // };
    auto token = Token{};
    token.line = start_line + 1;
    token.col = start_col + 1;
    token.index = last_start;
    token.state = state;
    token.text = text;
    return token;
  }
  
};

}
