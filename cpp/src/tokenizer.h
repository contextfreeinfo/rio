#include <iostream>
#include <string_view>
#include <vector>

namespace rio {

typedef size_t Index;

enum struct TokenState {
  Dot,
  Error,
  EscapeStart,
  Escape,
  Fraction,
  HSpace,
  Id,
  Int,
  Op1,
  Op2,
  Start,
  StringStart,
  StringStop,
  StringText,
  VSpace,
};

struct Token {
  Index col;
  Index row;
  Index index;
  TokenState state;
  std::string_view text;
};

struct Tokenizer {

  std::vector<Token> tokenize(const std::string& source) {
    std::vector<Token> tokens;
    char string_start;
    auto state = TokenState::Start;
    for (Index i = 0; i < source.length(); ++i) {
      auto c = source[i];
      TokenState new_state;
      switch (state) {
        case TokenState::Escape: {
          new_state = TokenState::StringText;
          break;
        }
        case TokenState::EscapeStart: {
          new_state = TokenState::Escape;
          break;
        }
        case TokenState::StringStart:
        case TokenState::StringText: {
          switch (c) {
            case '\\': {
              new_state = TokenState::EscapeStart;
              break;
            }
            case '\'':
            case '"': {
              if (c == string_start) {
                new_state = TokenState::StringStop;
              } else {
                new_state = TokenState::StringText;
              }
              break;
            }
            case '\r':
            case '\n': {
              new_state = TokenState::VSpace;
              break;
            }
            default: {
              new_state = TokenState::StringText;
              break;
            }
          }
          break;
        }
        default: {
          switch (c) {
            case ' ':
            case '\t': {
              new_state = TokenState::HSpace;
              break;
            }
            case '\'':
            case '"': {
              string_start = c;
              new_state = TokenState::StringStart;
              break;
            }
            case '\r':
            case '\n':
            {
              new_state = TokenState::VSpace;
              break;
            }
          }
        }
      }
      std::cout << c;
    }
    return tokens;
  }

};

}
