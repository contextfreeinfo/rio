#include "rio.h"

namespace rio {

auto is_id_part(char c) -> bool;
auto is_id_start(char c) -> bool;
auto is_space(char c) -> bool;
auto is_vspace(char c) -> bool;
auto next_token_comment(const char* buf) -> Token;
auto next_token_id(const char* buf) -> Token;
auto next_token_string(const char* buf) -> Token;
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
  usize line = 1;
  usize col = 1;
  while (true) {
    auto token = next_token(buf);
    // Fix offsets based on global buffer state.
    buf += token.begin.index;
    auto len = token.end.index;
    token.begin.index = buf - start;
    token.end.index += token.begin.index;
    buf += len;
    // Also fix line and col.
    if (token.begin.line) {
      // Also go from zero-index to one-index.
      line += token.begin.line;
      col = token.begin.col + 1;
      // printf("new line at col %zu\n", col);
    } else {
      col += token.begin.col;
      // printf("same line at col %zu\n", col);
    }
    token.begin.line = line;
    token.begin.col = col;
    col += len;
    // Store file name.
    token.file = file;
    // Debug print.
    char old = start[token.end.index];
    start[token.end.index] = '\0';
    printf("hey: %s (%zu, %zu): %s\n", token_name(token), token.begin.line, token.begin.col, &start[token.begin.index]);
    start[token.end.index] = old;
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
  usize line = 0;
  usize col = 0;
  auto finish = [&](Token token) {
    token.begin.index += skip - start;
    token.begin.line = line;
    token.begin.col = col;
    return token;
  };
  auto simple = [&](Token::Kind kind) {
    Token token = {kind};
    token.end.index = 1;
    return finish(token);
  };
  // Skip whitespace between tokens.
  // printf("buf\n");
  for (; *buf && is_space(*buf); buf += 1) {
    // Don't bother with old-time Mac cr-only convention.
    if (*buf == '\n') {
      line += 1;
      col = 0;
    } else {
      col += 1;
    }
  }
  skip = buf;
  // printf("after space: %c\n", *buf);
  // Check starting condition.
  char c = *buf;
  if (is_id_start(c)) {
    return finish(next_token_id(buf));
  }
  switch (c) {
    case '{': return simple(Token::Kind::CurlyL);
    case '}': return simple(Token::Kind::CurlyR);
    case '(': return simple(Token::Kind::RoundL);
    case ')': return simple(Token::Kind::RoundR);
    case '"': return finish(next_token_string(buf));
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
    token.end.index = buf - start;
    return token;
  }();
}

auto next_token_id(const char* buf) -> Token {
  auto start = buf;
  for (; *buf && is_id_part(*buf); buf += 1) {}
  auto len = buf - start;
  auto key_count = sizeof(key_ids) / sizeof(*key_ids);
  auto key = Key::None;
  for (usize k = 0; k < key_count; ++k) {
    if (!strncmp(key_ids[k].id, start, len)) {
      key = key_ids[k].key;
    }
  }
  // if (!std::strncmp(start, "fun", len)) {
  //   //
  // }
  return [&]() {
    Token token = {key == Key::None ? Token::Kind::Id : Token::Kind::Key};
    if (key != Key::None) {
      token.key = Key::Fun;
    }
    token.end.index = len;
    return token;
  }();
}

auto next_token_string(const char* buf) -> Token {
  auto start = buf;
  auto escape = false;
  // Skip the open quote.
  buf += 1;
  for (; *buf && !is_vspace(*buf); buf += 1) {
    auto c = *buf;
    if (c == '\\') {
      escape = true;
    } else if (escape) {
      escape = false;
    } else if (c == '"') {
      buf += 1;
      break;
    }
  }
  return [&]() {
    Token token = {Token::Kind::String};
    token.end.index = buf - start;
    return token;
  }();
}

auto token_name(const Token& token) -> const char* {
  switch (token.kind) {
    case Token::Kind::Comment: return "Comment";
    case Token::Kind::CurlyL: return "CurlyL";
    case Token::Kind::CurlyR: return "CurlyR";
    case Token::Kind::Eof: return "Eof";
    case Token::Kind::Id: return "Id";
    case Token::Kind::Key: switch (token.key) {
      case Key::Fun: return "Fun";
      default: return "<key>";
    }
    case Token::Kind::RoundL: return "RoundL";
    case Token::Kind::RoundR: return "RoundR";
    case Token::Kind::String: return "String";
    default: return "<unknown>";
  }
}

}
