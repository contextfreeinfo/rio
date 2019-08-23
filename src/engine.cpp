#include <stdlib.h>

#include "main.h"

namespace rio {

auto compare_mods(const void* a, const void* b) -> int;
auto set_mod_indices(ModManager* mod) -> void;

auto order_mods(Engine* engine) -> void {
  // Extract mod roots, and set partial indices.
  for (auto mod: engine->mod_parts) {
    if (mod == mod->root) {
      engine->mods.push(mod);
      set_mod_indices(mod);
    }
  }
  // Sort them, and set total indices.
  qsort(
    engine->mods.items, engine->mods.len, sizeof(ModManager*), compare_mods
  );
  for (rint index = 0; index < engine->mods.len; index += 1) {
    engine->mods[index]->index = index + 1;
  }
}

auto compare_mods(const void* a, const  void* b) -> int {
  const ModManager& mod_a = **static_cast<ModManager* const*>(a);
  const ModManager& mod_b = **static_cast<ModManager* const*>(b);
  return mod_a.index - mod_b.index;
}

auto set_mod_indices(ModManager* mod) -> void {
  if (mod->index) {
    return;
  }
  rint index = 0;
  for (auto import: mod->uses) {
    set_mod_indices(import);
    index = max(index, import->index);
  }
  // One more than the most so far.
  mod->index = index + 1;
}

}
