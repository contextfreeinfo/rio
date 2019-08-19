#include "main.h"

namespace rio {

auto is_digit(char c) -> bool;
auto is_hspace(char c) -> bool;
auto is_num_start(char c) -> bool;
auto is_space(char c, bool was_line_end) -> bool;
auto is_vspace(char c) -> bool;
auto next_token_comment(const char* buf) -> Token;
auto next_token_id(const char* buf) -> Token;
auto next_token_num(const char* buf) -> Token;
auto next_token_string(const char* buf) -> Token;
auto next_token(const char* buf, bool was_line_end) -> Token;

struct KeyId {
  Str id;
  Token::Kind key;
};

// TODO Make into a map.
const KeyId key_ids[] = {
  {str_from("do"), Token::Kind::Do},
  {str_from("end"), Token::Kind::End},
  // If functions are pure, calls can be reordered, so extracts for statements
  // can be kept simpler ...
  {str_from("for"), Token::Kind::For},
  {str_from("fun"), Token::Kind::Fun},
  {str_from("include"), Token::Kind::Include},
  {str_from("proc"), Token::Kind::Proc},
  {str_from("pub"), Token::Kind::Pub},
  {str_from("struct"), Token::Kind::Struct},
  {str_from("use"), Token::Kind::Use},
};

auto has_text(const Token& token) -> bool {
  switch (token.kind) {
    case Token::Kind::Comment:
    case Token::Kind::Float:
    case Token::Kind::Id:
    case Token::Kind::Int:
    case Token::Kind::String: {
      return true;
    }
    default: {
      return false;
    }
  }
}

auto is_digit(char c) -> bool {
  return '0' <= c && c <= '9';
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

auto is_num_start(char c) -> bool {
  return c == '.' || c == '+' || c == '-' || is_digit(c);
}

auto is_space(char c, bool was_line_end) -> bool {
  return is_hspace(c) || (was_line_end && is_vspace(c));
}

auto is_vspace(char c) -> bool {
  return c == '\n' || c == '\r';
}

auto lex(
  ModManager* mod, const char* file, const char* buf, List<Token>* tokens
) -> void {
  const auto start = buf;
  rint line = 1;
  rint col = 1;
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
      // if (verbose) printf("new line at col %zu\n", col);
    } else {
      col += token.begin.col;
      // if (verbose) printf("same line at col %zu\n", col);
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
      // TODO Intern only symbols and short strings.
      // TODO Put longer string data into the mod's own arena.
      token.text = intern(mod->engine, &start[token.begin.index], len);
    }
    // Store file name.
    token.file = file;
    // Done with this one.
    tokens->push(token);
    // End at end.
    if (token.kind == Token::Kind::FileEnd) {
      break;
    }
  }
}

auto next_token(const char* buf, bool was_line_end) -> Token {
  auto start = buf;
  auto skip = buf;
  rint line = 0;
  rint col = 0;
  auto finish = [&](Token token) {
    token.begin.index += skip - start;
    token.begin.line = line;
    token.begin.col = col;
    return token;
  };
  auto simple_len = [&](Token::Kind kind, rint len) {
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
  if (is_num_start(c)) {
    return finish(next_token_num(buf));
  }
  switch (c) {
    case '\0': return simple(Token::Kind::FileEnd);
    case '\n': return simple(Token::Kind::LineEnd);
    case '\r': {
      rint len = 1;
      if (*(buf + 1) == '\n') {
        len += 1;
      }
      return simple_len(Token::Kind::LineEnd, len);
    }
    case '=': return simple(Token::Kind::Assign);
    case ',': return simple(Token::Kind::Comma);
    case '{': return simple(Token::Kind::CurlyL);
    case '}': return simple(Token::Kind::CurlyR);
    case '.': return simple(Token::Kind::Dot);
    case '(': return simple(Token::Kind::RoundL);
    case ')': return simple(Token::Kind::RoundR);
    case '[': return simple(Token::Kind::SquareL);
    case ']': return simple(Token::Kind::SquareR);
    case '"': return finish(next_token_string(buf));
    case '#': return finish(next_token_comment(buf));
    case ':': {
      switch (*(buf + 1)) {
        case '=': return simple_len(Token::Kind::Update, 2);
        default: return simple(Token::Kind::Colon);
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
  rint len = buf - start;
  auto key_count = usize_to_int(sizeof(key_ids) / sizeof(*key_ids));
  auto kind = Token::Kind::Id;
  for (rint k = 0; k < key_count; ++k) {
    if (str_eq(key_ids[k].id, str_sized(start, len))) {
      kind = key_ids[k].key;
    }
  }
  return [&]() {
    Token token = {kind};
    token.len = len;
    return token;
  }();
}

auto next_token_num(const char* buf) -> Token {
  const char* start = buf;
  char c = *buf;
  if (c == '-' || c == '+') {
    buf += 1;
    char c2 = *buf;
    // Checking ahead is safe because of null termination.
    if (!(is_digit(c2) || (c2 == '.' && is_digit(buf[1])))) {
      return [&]() {
        Token token = {c == '-' ? Token::Kind::Minus : Token::Kind::Plus};
        token.len = 1;
        return token;
      }();
    }
  }
  if (*buf == '.' && !is_digit(buf[1])) {
    // If we had a + or - before this, we'd have already cut out earlier.
    return [&]() {
      Token token = {Token::Kind::Dot};
      token.len = 1;
      return token;
    }();
  }
  for (; is_digit(*buf); buf += 1) {}
  auto kind = Token::Kind::Int;
  if (*buf == '.') {
    kind = Token::Kind::Float;
    buf += 1;
    for (; is_digit(*buf); buf += 1) {}
  }
  // TODO Exp, Hex, Bin, and Octal, too. Though later.
  return [&]() {
    Token token = {kind};
    token.len = buf - start;
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

auto token_kind_name(Token::Kind kind) -> const char* {
  switch (kind) {
    case Token::Kind::None: return "<none>";
    case Token::Kind::Colon: return "Colon";
    case Token::Kind::Comma: return "Comma";
    case Token::Kind::Comment: return "Comment";
    case Token::Kind::CurlyL: return "CurlyL";
    case Token::Kind::CurlyR: return "CurlyR";
    case Token::Kind::Do: return "Do";
    case Token::Kind::Dot: return "Dot";
    case Token::Kind::End: return "End";
    case Token::Kind::FileEnd: return "FileEnd";
    case Token::Kind::Float: return "Float";
    case Token::Kind::Fun: return "Fun";
    case Token::Kind::Id: return "Id";
    case Token::Kind::Include: return "Include";
    case Token::Kind::Int: return "Int";
    case Token::Kind::Junk: return "Junk";
    case Token::Kind::LineEnd: return "LineEnd";
    case Token::Kind::Minus: return "Minus";
    case Token::Kind::Plus: return "Plus";
    case Token::Kind::RoundL: return "RoundL";
    case Token::Kind::RoundR: return "RoundR";
    case Token::Kind::SquareL: return "SquareL";
    case Token::Kind::SquareR: return "SquareR";
    case Token::Kind::String: return "String";
    case Token::Kind::Struct: return "Struct";
    case Token::Kind::Update: return "Update";
    case Token::Kind::Use: return "Use";
    default: return "<unknown>";
  }
}

auto token_name(const Token& token) -> const char* {
  return token_kind_name(token.kind);
}

auto token_text(const Token& token) -> const char* {
  return has_text(token) ? token.text : "";
}

}
