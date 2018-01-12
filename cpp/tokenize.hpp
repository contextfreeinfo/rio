#pragma once

#include <iostream>
#include <string_view>
#include <vector>

namespace rio {

using Index = size_t;

enum struct TokenState {
  Assign,
  BracketClose,
  BracketOpen,
  Colon,
  Comma,
  Comment,
  Def,
  Do,
  Dot,
  End,
  Eof,
  Error,
  EscapeStart,
  Escape,
  Extern,
  Fraction,
  HSpace,
  Id,
  Int,
  Let,
  Op,
  Op1,
  Op2,
  ParenOpen,
  ParenClose,
  Plus,
  Semi,
  Start,
  StringStart,
  StringStop,
  StringText,
  Times,
  Type,
  VSpace,
};

auto name(TokenState state) -> std::string_view {
  switch (state) {
    case TokenState::Assign: return "Assign";
    case TokenState::BracketOpen: return "BracketOpen";
    case TokenState::BracketClose: return "BracketClose";
    case TokenState::Colon: return "Colon";
    case TokenState::Comma: return "Comma";
    case TokenState::Comment: return "Comment";
    case TokenState::Def: return "Def";
    case TokenState::Do: return "Do";
    case TokenState::Dot: return "Dot";
    case TokenState::End: return "End";
    case TokenState::Eof: return "Eof";
    case TokenState::Error: return "Error";
    case TokenState::EscapeStart: return "EscapeStart";
    case TokenState::Escape: return "Escape";
    case TokenState::Extern: return "Extern";
    case TokenState::Fraction: return "Fraction";
    case TokenState::HSpace: return "HSpace";
    case TokenState::Id: return "Id";
    case TokenState::Let: return "Let";
    case TokenState::Int: return "Int";
    case TokenState::Op: return "Op";
    case TokenState::Op1: return "Op1";
    case TokenState::Op2: return "Op2";
    case TokenState::ParenOpen: return "ParenOpen";
    case TokenState::ParenClose: return "ParenClose";
    case TokenState::Plus: return "Plus";
    case TokenState::Semi: return "Semi";
    case TokenState::Start: return "Start";
    case TokenState::StringStart: return "StringStart";
    case TokenState::StringStop: return "StringStop";
    case TokenState::StringText: return "StringText";
    case TokenState::Times: return "Times";
    case TokenState::Type: return "Type";
    case TokenState::VSpace: return "VSpace";
    default: return "?";
  }
}

auto operator<<(std::ostream& stream, TokenState state) -> std::ostream& {
  return stream << name(state);
}

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

auto important(TokenState state) -> bool {
  switch (state) {
    case TokenState::Comment: case TokenState::HSpace: case TokenState::VSpace:
      return false;
    default: return true;
  }
}

auto infix(TokenState state) -> bool {
  switch (state) {
    case TokenState::Assign:
    case TokenState::Colon:
    case TokenState::Comma:
    case TokenState::Op:
    case TokenState::Op1:
    case TokenState::Op2:
    case TokenState::Plus:
    case TokenState::Semi:
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
    case TokenState::Comment: case TokenState::Semi: case TokenState::VSpace:
      return 10;
    case TokenState::Comma: return 20;
    case TokenState::Assign: return 22;
    case TokenState::HSpace: return 25;
    case TokenState::Colon: return 30;
    case TokenState::Plus: return 40;
    case TokenState::Times: return 50;
    default: return 25;
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

  auto closing() const -> bool {
    return ::rio::closing(state);
  }

  auto important() const -> bool {
    return ::rio::important(state);
  }

  auto infix() const -> bool {
    return ::rio::infix(state);
  }

  auto precedence() const -> int {
    return ::rio::precedence(state);
  }

};

auto operator<<(std::ostream& stream, const Token& token) -> std::ostream& {
  return stream
    << token.state << "["
    << token.line << ", " << token.col << "]("
    << token.text << ")";
}

struct Tokenizer {

  Tokenizer(std::string_view buffer) {
    this->buffer = buffer;
    char_index = 0;
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

  // This should be tracking utf8 code point positions.
  // TODO Actual utf8 handling.
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

  auto find_key(std::string_view text) -> TokenState {
    // Everything we care about is ascii here, so no code points needed.
    auto c = text.cbegin();
    auto end = text.cend();
    if (c < end) {
      switch (*c) {
        case 'd': {
          if (++c < end) {
            switch (*c) {
              case 'e': {
                if (++c < end) {
                  switch (*c) {
                    case 'f': {
                      if (++c >= end) return TokenState::Def;
                      break;
                    }
                  }
                }
                break;
              }
              case 'o': {
                if (++c >= end) return TokenState::Do;
                break;
              }
            }
          }
          break;
        }
        case 'e': {
          if (++c < end) {
            switch (*c) {
              case 'n': {
                if (++c < end) {
                  switch (*c) {
                    case 'd': {
                      if (++c >= end) return TokenState::End;
                      break;
                    }
                  }
                }
                break;
              }
              case 'x': {
                if (++c < end) {
                  switch (*c) {
                    case 't': {
                      // Without deeper tree, this means slightly slower parsing
                      // of all ids starting with "ext".
                      if (text == "extern") {
                        return TokenState::Extern;
                      }
                      break;
                    }
                  }
                }
                break;
              }
            }
          }
          break;
        }
        case 'l': {
          if (++c < end) {
            switch (*c) {
              case 'e': {
                if (++c < end) {
                  switch (*c) {
                    case 't': {
                      if (++c >= end) return TokenState::Let;
                      break;
                    }
                  }
                }
                break;
              }
            }
          }
          break;
        }
        case 't': {
          if (++c < end) {
            switch (*c) {
              case 'y': {
                if (++c < end) {
                  switch (*c) {
                    case 'p': {
                      if (++c < end) {
                        switch (*c) {
                          case 'e': {
                            if (++c >= end) return TokenState::Type;
                            break;
                          }
                        }
                      }
                      break;
                    }
                  }
                }
                break;
              }
            }
          }
          break;
        }
      }
    }
    return TokenState::Id;
  }

  auto find_op(std::string_view text) -> TokenState {
    auto c = text.cbegin();
    auto end = text.cend();
    if (c < end) {
      switch (*c) {
        case '=': {
          if (++c == end) return TokenState::Assign;
          break;
        }
        case '[': return TokenState::BracketOpen;
        case ']': return TokenState::BracketClose;
        case ':': return TokenState::Colon;
        case ',': return TokenState::Comma;
        case '(': return TokenState::ParenOpen;
        case ')': return TokenState::ParenClose;
        case '+': case '-': {
          if (++c == end) return TokenState::Plus;
          break;
        }
        case ';': return TokenState::Semi;
        case '*': case '/': {
          if (++c == end) return TokenState::Times;
          break;
        }
      }
    }
    return TokenState::Op;
  }

  auto find_new_state(char c) -> TokenState {
    switch (state) {
      case TokenState::Comment: {
        switch (c) {
          case '\n': case '\r': return TokenState::VSpace;
          default: return TokenState::Comment;
        }
      }
      // TODO Longer escape sequences.
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
    }
  }

  auto next() -> Token {
    Index stop_index;
    Index last_start = this->last_start;
    Index start_col = this->start_col;
    Index start_line = this->start_line;
    auto state = this->state;
    // std::cout << "Finding next at " << start_line << ", " << start_col << std::endl;
    while (true) {
      Index index = char_index;
      // std::cout << "Index " << index << std::endl;
      if (index >= buffer.size()) {
        stop_index = buffer.size();
        this->last_start = stop_index;
        // std::cout << "Done!" << std::endl;
        break;
      }
      char c = buffer[index];
      // std::cout << "At " << index << " found " << c << " (" << static_cast<int>(c) << ")" << std::endl;
      // TODO(tjp): Advance by utf8 code points.
      ++char_index;
      auto new_state = find_new_state(c);
      // std::cout << "On " << c << " change from " << state << " to " << new_state << std::endl;
      stop_index = index;
      state = this->state;
      auto state_changed = new_state != state || state == TokenState::Op1;
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
    switch (state) {
      case TokenState::Id: {
        state = find_key(text);
        break;
      }
      case TokenState::Op1: case TokenState::Op2: {
        state = find_op(text);
        break;
      }
      // Keep the current state.
      default: break;
    }
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
