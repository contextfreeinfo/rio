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

  // To be used only in the root of a multimod.
  // The globals defined in this multimod.
  // In the case of multiple definitions in the same mod, they'll both be here.
  // Errors can be flagged elsewhere.
  List<Def*> global_defs;

  // To be used only in the root of a multimod.
  // Includes definitions from inside this multimod as well as all use imports.
  // If conflicts, set to null, and the multiple contenders are in errors
  // elsewhere if someone tries to use them.
  // Each mod should allocate their own globals, so in the simple case, this is
  // just a single pointer to that.
  // We only need to allocate new arrays for slices in case of conflicts.
  Map<string, Opt<Def>> global_refs;

  // To be used only in the root of a multimod.
  // All the mod files in a multifile mod.
  // And the root doesn't track itself, because we can be more efficient without
  // allocating here on single file mods.
  List<ModManager*> parts;

  bool resolve_started = false;

  // To be used only in the root of a multimod.
  // 'Use' imports, pointing only to roots.
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
  Map<Str, string> interns;
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
