#include "rio.h"

namespace rio {

auto is_id_part(char c) -> bool;
auto is_id_start(char c) -> bool;
auto is_space(char c) -> bool;
auto is_vspace(char c) -> bool;
auto next_token_comment(const char* buf) -> Token;
auto next_token_id(const char* buf) -> Token;
auto next_token(const char* buf) -> Token;

struct KeyId {
  Key key;
  const char* id;
};

const KeyId key_ids[] = {
  {Key::Fun, "fun"},
};

auto is_id_part(char c) -> bool {
  return is_id_start(c) || ('0' <= c && c <= '9');
}

auto is_id_start(char c) -> bool {
  return c == '_' || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}

auto is_space(char c) -> bool {
  return c == ' ' || c == '\t' || is_vspace(c);
}

auto is_vspace(char c) -> bool {
  return c == '\n' || c == '\r';
}

void lex(const Options* options) {
  auto file = options->in;
  auto buf = read_file(file);
  const auto start = buf;
  while (true) {
    auto token = next_token(buf);
    // Fix offsets based on global buffer state.
    buf += token.begin;
    auto len = token.end;
    token.begin = buf - start;
    token.end += token.begin;
    buf += len;
    // Store file name.
    token.file = file;
    // Debug print.
    char old = start[token.end];
    start[token.end] = '\0';
    printf("hey: %s (%zu, %zu): %s\n", token_name(token), token.begin, token.end, &start[token.begin]);
    start[token.end] = old;
    // End at end. TODO Stream out tokens or pregenerate all?
    if (token.kind == Token::Kind::Eof) {
      break;
    }
  }
  free(start);
}

auto next_token(const char* buf) -> Token {
  auto start = buf;
  auto skip = buf;
  auto finish = [&](Token token) {
    token.begin += skip - start;
    return token;
  };
  // Skip whitespace between tokens.
  // printf("buf\n");
  for (; *buf && is_space(*buf); buf += 1) {}
  skip = buf;
  // printf("after space: %c\n", *buf);
  // Check starting condition.
  char c = *buf;
  if (is_id_start(c)) {
    return finish(next_token_id(buf));
  }
  switch (c) {
    case '/': {
      switch (*(buf + 1)) {
        case '/': return finish(next_token_comment(buf));
      }
      break;
    }
  }
  // Burn the rest for now.
  for (; *buf; buf += 1) {
    //
  }
  return {Token::Kind::Eof};
}

auto next_token_comment(const char* buf) -> Token {
  auto start = buf;
  for (; *buf && !is_vspace(*buf); buf += 1) {}
  return [&]() {
    Token token = {Token::Kind::Comment};
    token.begin = 0;
    token.end = buf - start;
    return token;
  }();
}

auto next_token_id(const char* buf) -> Token {
  auto start = buf;
  for (; *buf && is_id_part(*buf); buf += 1) {}
  auto len = buf - start;
  // if (!std::strncmp(start, "fun", len)) {
  //   //
  // }
  return [&]() {
    Token token = {Token::Kind::Key};
    token.key = Key::Fun;
    token.begin = 0;
    token.end = len;
    return token;
  }();
}

auto token_name(const Token& token) -> const char* {
  switch (token.kind) {
    case Token::Kind::Comment: return "Comment";
    case Token::Kind::Eof: return "Eof";
    case Token::Kind::Key: switch (token.key) {
      case Key::Fun: return "Fun";
      default: return "<key>";
    }
    default: return "<unknown>";
  }
}

}
