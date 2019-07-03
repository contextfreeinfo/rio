#include "rio.h"

namespace rio {

struct ParseState {
  const Token* tokens;
};

void advance_token(ParseState* state, bool skip_lines = false);
auto more_tokens(ParseState* state, Token::Kind end) -> bool;
void parse_block(ParseState* state);
void parse_expr(ParseState* state);
void parse_fun(ParseState* state);
void parse_tuple(ParseState* state);
void skip_comments(ParseState* state, bool skip_lines = false);

void advance_token(ParseState* state, bool skip_lines) {
  auto& tokens = state->tokens;
  if (tokens->kind != Token::Kind::FileEnd) {
    tokens += 1;
    skip_comments(state, skip_lines);
  }
}

auto more_tokens(ParseState* state, Token::Kind end) -> bool {
  return
    state->tokens->kind != end &&
    state->tokens->kind != Token::Kind::FileEnd;
}

void parse(Engine* engine, const Token* tokens) {
  ParseState state = {tokens};
  skip_comments(&state, true);
  parse_expr(&state);
}

void parse_block(ParseState* state) {
  printf("begin block\n");
  advance_token(state, true);
  for (; more_tokens(state, Token::Kind::CurlyR); advance_token(state, true)) {
    parse_expr(state);
  }
  advance_token(state);
  printf("end block\n");
}

void parse_expr(ParseState* state) {
  auto tokens = state->tokens;
  switch (tokens->kind) {
    case Token::Kind::CurlyL: {
      parse_block(state);
      break;
    }
    case Token::Kind::Key: {
      switch (tokens->key) {
        case Key::Fun: {
          parse_fun(state);
          break;
        }
        default: break;
      } // switch
      break;
    }
    default: {
      printf("junk: %s\n", token_name(*tokens));
      break;
    }
  }
}

void parse_fun(ParseState* state) {
  printf("begin fun\n");
  advance_token(state);
  const char* name = "";
  if (state->tokens->kind == Token::Kind::Id) {
    name = state->tokens->text;
    printf("name %s\n", name);
    advance_token(state);
  }
  if (state->tokens->kind == Token::Kind::RoundL) {
    parse_tuple(state);
    printf("args\n");
  }
  parse_expr(state);
  printf("end fun\n");
}

void parse_tuple(ParseState* state) {
  printf("begin tuple\n");
  advance_token(state);
  for (; more_tokens(state, Token::Kind::RoundR); advance_token(state)) {
    // TODO Parse things.
  }
  advance_token(state);
  printf("end tuple\n");
}

void skip_comments(ParseState* state, bool skip_lines) {
  // TODO Store them somewhere automatically?
  auto& tokens = state->tokens;
  if (skip_lines) {
    for (;
      tokens->kind == Token::Kind::Comment ||
        tokens->kind == Token::Kind::LineEnd;
      tokens += 1
    ) {}
  } else {
    for (; tokens->kind == Token::Kind::Comment; tokens += 1) {}
  }
}

}
