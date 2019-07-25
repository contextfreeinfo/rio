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

struct ModManager;

struct ModInfo {

  Engine* engine = nullptr;

  string file = "";

  string name = "";

  // The root mod file of a multifile mod.
  ModManager* root = nullptr;

};

struct ModManager: ModInfo {

  // Arena per module, so we can reload just changed modules in server mode.
  // Well, the downstream clients of changed modules count as changed, too.
  Arena arena;

  Node* tree = nullptr;

  // All the mod files in a multifile mod.
  // Only tracked for the root mod file.
  // And the root doesn't track itself, because we can be more efficient without
  // allocating here on single file mods.
  List<ModManager*> parts;

  // 'Use' imports.
  List<ModManager*> uses;

};

struct Engine {

  ~Engine() {
    // TODO Create my own unique_ptr and place these in the engine's arena?
    for (auto mod: mods) {
      mod->~ModManager();
    }
  }

  // TODO Allocate error list in arena.
  Arena arena;
  // FILE* info;
  Options options = {0};
  Map<const char*> interns;
  List<ModManager*> mods;
  // bool verbose{false};

};

constexpr bool verbose = false;

auto has_text(const Token& token) -> bool;
auto intern(Engine* engine, const char* text, usize nbytes) -> const char*;
auto intern_str(Engine* engine, const Str& str) -> const char*;
auto load_mod(const ModInfo& info) -> ModManager*;
auto read_file(const char* name) -> char*;
auto token_kind_name(Token::Kind kind) -> const char*;
auto token_name(const Token& token) -> const char*;
auto token_text(const Token& token) -> const char*;

}
