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
    for (Index i = 0; i < source.length(); ++i) {
      auto c = source[i];
      std::cout << c;
    }
    return tokens;
  }

};

}
