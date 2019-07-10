#include "main.h"

namespace rio {

auto is_id_part(char c) -> bool;
auto is_id_start(char c) -> bool;
auto is_hspace(char c) -> bool;
auto is_space(char c, bool was_line_end) -> bool;
auto is_vspace(char c) -> bool;
auto next_token_comment(const char* buf) -> Token;
auto next_token_id(const char* buf) -> Token;
auto next_token_string(const char* buf) -> Token;
auto next_token(const char* buf, bool was_line_end) -> Token;

struct KeyId {
  const char* id;
  Token::Kind key;
};

// TODO Make into a map?
const KeyId key_ids[] = {
  {"do", Token::Kind::Do},
  {"end", Token::Kind::End},
  {"fun", Token::Kind::Fun},
};

auto has_text(const Token& token) -> bool {
  switch (token.kind) {
    case Token::Kind::Comment:
    case Token::Kind::Id:
    case Token::Kind::String: {
      return true;
    }
    default: {
      return false;
    }
  }
}

auto is_hspace(char c) -> bool {
  return c == ' ' || c == '\t';
}

auto is_id_part(char c) -> bool {
  return is_id_start(c) || ('0' <= c && c <= '9');
}

auto is_id_start(char c) -> bool {
  return c == '_' || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}

auto is_space(char c, bool was_line_end) -> bool {
  return is_hspace(c) || (was_line_end && is_vspace(c));
}

auto is_vspace(char c) -> bool {
  return c == '\n' || c == '\r';
}

auto lex(Engine* engine, const char* file, const char* buf) -> List<Token> {
  List<Token> tokens;
  const auto start = buf;
  usize line = 1;
  usize col = 1;
  // Effectively starts at a new line.
  auto was_line_end = true;
  while (true) {
    auto token = next_token(buf, was_line_end);
    // Fix offsets based on global buffer state.
    buf += token.begin.index;
    auto len = token.len;
    token.begin.index = buf - start;
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
    if (token.kind == Token::Kind::LineEnd) {
      was_line_end = true;
      line += 1;
      col = 1;
    } else {
      if (token.kind != Token::Kind::Comment) {
        was_line_end = false;
      }
      col += len;
    }
    // Intern strings.
    if (has_text(token)) {
      token.text = intern(engine, &start[token.begin.index], len);
    }
    // Store file name.
    token.file = file;
    // Done with this one.
    tokens.push(token);
    // End at end.
    if (token.kind == Token::Kind::FileEnd) {
      break;
    }
  }
  return tokens;
}

auto next_token(const char* buf, bool was_line_end) -> Token {
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
  auto simple_len = [&](Token::Kind kind, usize len) {
    Token token = {kind};
    token.len = len;
    return finish(token);
  };
  auto simple = [&](Token::Kind kind) {
    return simple_len(kind, 1);
  };
  // Skip whitespace between tokens.
  // printf("buf\n");
  for (; *buf && is_space(*buf, was_line_end); buf += 1) {
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
    case '\0': return simple(Token::Kind::FileEnd);
    case '\n': return simple(Token::Kind::LineEnd);
    case '\r': {
      usize len = 1;
      if (*(buf + 1) == '\n') {
        len += 1;
      }
      return simple_len(Token::Kind::LineEnd, len);
    }
    case '=': return simple(Token::Kind::Assign);
    case ',': return simple(Token::Kind::Comma);
    case '{': return simple(Token::Kind::CurlyL);
    case '}': return simple(Token::Kind::CurlyR);
    case '(': return simple(Token::Kind::RoundL);
    case ')': return simple(Token::Kind::RoundR);
    case '"': return finish(next_token_string(buf));
    case '#': return finish(next_token_comment(buf));
    case ':': {
      switch (*(buf + 1)) {
        case '=': return simple_len(Token::Kind::Update, 2);
      }
      break;
    }
    case '/': {
      switch (*(buf + 1)) {
        case '/': return finish(next_token_comment(buf));
      }
      break;
    }
    default: break;
  }
  // TODO Coalesce multiple junk somewhere?
  return simple(Token::Kind::Junk);
}

auto next_token_comment(const char* buf) -> Token {
  auto start = buf;
  for (; *buf && !is_vspace(*buf); buf += 1) {}
  return [&]() {
    Token token = {Token::Kind::Comment};
    token.len = buf - start;
    return token;
  }();
}

auto next_token_id(const char* buf) -> Token {
  auto start = buf;
  for (; *buf && is_id_part(*buf); buf += 1) {}
  auto len = buf - start;
  auto key_count = sizeof(key_ids) / sizeof(*key_ids);
  auto kind = Token::Kind::Id;
  for (usize k = 0; k < key_count; ++k) {
    if (!strncmp(key_ids[k].id, start, len)) {
      kind = key_ids[k].key;
    }
  }
  return [&]() {
    Token token = {kind};
    token.len = len;
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
    token.len = buf - start;
    return token;
  }();
}

auto token_name(const Token& token) -> const char* {
  switch (token.kind) {
    case Token::Kind::Comment: return "Comment";
    case Token::Kind::CurlyL: return "CurlyL";
    case Token::Kind::CurlyR: return "CurlyR";
    case Token::Kind::FileEnd: return "FileEnd";
    case Token::Kind::Fun: return "Fun";
    case Token::Kind::Id: return "Id";
    case Token::Kind::Junk: return "Junk";
    case Token::Kind::LineEnd: return "LineEnd";
    case Token::Kind::RoundL: return "RoundL";
    case Token::Kind::RoundR: return "RoundR";
    case Token::Kind::String: return "String";
    default: return "<unknown>";
  }
}

auto token_text(const Token& token) -> const char* {
  return has_text(token) ? token.text : "";
}

}
