#include "rio.h"

namespace rio {

auto advance_token(const Token* tokens) -> const Token*;
auto skip_comments(const Token* tokens) -> const Token*;

void parse(Engine* engine, const Token* tokens) {
  tokens = skip_comments(tokens);
  for (; tokens; tokens = advance_token(tokens)) {
    auto& token = *tokens;
    // Debug print.
    printf(
      "hey: %s (%zu, %zu)%s%s\n",
      token_name(token), token.begin.line, token.begin.col,
      has_text(token) ? ": " : "", token_text(token)
    );
  }
}

auto advance_token(const Token* tokens) -> const Token* {
  if (tokens->kind == Token::Kind::FileEnd) {
    tokens = nullptr;
  } else {
    tokens = skip_comments(tokens + 1);
  }
  return tokens;
}

auto skip_comments(const Token* tokens) -> const Token* {
  // TODO Store them somewhere automatically?
  for (; tokens->kind == Token::Kind::Comment; tokens += 1) {}
  return tokens;
}

}
