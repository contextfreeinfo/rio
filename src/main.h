#pragma once

// The idea is that internal code just includes this and is good to go.

// Some of these are redundant, but eh.
#include "rio.h"
#include "common.h"
#include "node.h"

namespace rio {

struct Engine;

struct Options {
  const char* in;
};

struct ModManager {
  Arena arena;
  Engine* engine;
};

struct Engine {
  // TODO Arena per file, so we can reload just changed files in server mode.
  // TODO Allocate error list in arena.
  Arena arena;
  // FILE* info;
  Options options = {0};
  Map<const char*> interns;
  // bool verbose{false};
};

constexpr bool verbose = false;

auto has_text(const Token& token) -> bool;
auto intern(Engine* engine, const char* text, usize nbytes) -> const char*;
auto read_file(const char* name) -> char*;
auto token_name(const Token& token) -> const char*;
auto token_text(const Token& token) -> const char*;

}
