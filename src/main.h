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
  // Arena per module, so we can reload just changed modules in server mode.
  // Well, the downstream clients of changed modules count as changed, too.
  Arena arena;
  Engine* engine{nullptr};
  Node* tree{nullptr};
};

struct Engine {

  ~Engine() {
    // TODO Create my own unique_ptr and place these in the engine's arena?
    for (usize i = 0; i < mods.len; i += 1) {
      mods[i].~ModManager();
    }
  }

  // TODO Allocate error list in arena.
  Arena arena;
  // FILE* info;
  Options options = {0};
  Map<const char*> interns;
  List<ModManager> mods;
  // bool verbose{false};

};

constexpr bool verbose = false;

auto has_text(const Token& token) -> bool;
auto intern(Engine* engine, const char* text, usize nbytes) -> const char*;
auto load_mod(Engine* engine, const char* file) -> ModManager*;
auto read_file(const char* name) -> char*;
auto token_name(const Token& token) -> const char*;
auto token_text(const Token& token) -> const char*;

}
