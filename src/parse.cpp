#include "main.h"

namespace rio {

struct ParseState {
  const Token* tokens;
};

void advance_token(ParseState* state, bool skip_lines = false);
auto more_tokens(ParseState* state, Token::Kind end) -> bool;
void parse_atom(ParseState* state);
void parse_block(ParseState* state);
void parse_call(ParseState* state);
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

void parse_atom(ParseState* state) {
  // TODO Call parse_call, and have this in parse_atom
  auto tokens = state->tokens;
  switch (tokens->kind) {
    case Token::Kind::CurlyL: {
      parse_block(state);
      break;
    }
    case Token::Kind::Id: {
      printf("ref: %s\n", tokens->text);
      advance_token(state);
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
    case Token::Kind::String: {
      printf("string: %s\n", token_text(*tokens));
      advance_token(state);
      break;
    }
    default: {
      printf("junk: %s\n", token_name(*tokens));
      advance_token(state);
      break;
    }
  }
}

void parse_block(ParseState* state) {
  printf("begin block\n");
  advance_token(state, true);
  for (; more_tokens(state, Token::Kind::CurlyR); skip_comments(state, true)) {
    parse_expr(state);
  }
  advance_token(state);
  printf("end block\n");
}

void parse_call(ParseState* state) {
  parse_atom(state);
  if (state->tokens->kind == Token::Kind::RoundL) {
    printf("begin call\n");
    parse_tuple(state);
    printf("end call\n");
  }
}

void parse_expr(ParseState* state) {
  parse_call(state);
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
  for (; more_tokens(state, Token::Kind::RoundR); skip_comments(state, true)) {
    parse_expr(state);
    // TODO Accept comma, expect except after last.
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
