#pragma once

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
// #include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace rio {

// Int types.

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using isize = intptr_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using usize = uintptr_t;

// private

template<typename Key, typename Value>
using Map = std::unordered_map<Key, Value>;

enum struct Key {
  None,
  Fun,
};

struct Options {
  char* in;
};

struct Pos {
  usize index;
  usize line;
  usize col;
};

struct Token {
  enum struct Kind {
    // TODO Use cache files to remember ids of enums, so source order doesn't
    // TODO matter.
    Comment,
    CurlyL,
    CurlyR,
    FileEnd,
    Id,
    Junk,
    Key,
    LineEnd,
    RoundL,
    RoundR,
    String,
  };
  Kind kind;
  union {
    const char* text;
    Key key;
  };
  Pos begin;
  usize len;
  const char* file;
};

struct Engine {
  Map<std::string, std::string> ids;
  Options options = {0};
};

void fail(const char* message);
auto has_text(const Token& token) -> bool;
auto intern(Engine* engine, const char* text, usize nbytes) -> const char*;
auto read_file(const char* name) -> char*;
auto token_name(const Token& token) -> const char*;
auto token_text(const Token& token) -> const char*;
auto xmalloc(usize nbytes) -> void*;

}
