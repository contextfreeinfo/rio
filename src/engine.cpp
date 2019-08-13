#include <stdlib.h>

#include "main.h"

namespace rio {

auto compare_mods(const void* a, const void* b) -> int;
auto set_mod_indices(ModManager* mod) -> void;

auto order_mods(Engine* engine) -> void {
  // Extract mod roots, and set partial indices.
  for (auto mod: engine->mods) {
    if (mod == mod->root) {
      engine->roots.push(mod);
      set_mod_indices(mod);
    }
  }
  // Sort them, and set total indices.
  qsort(
    engine->roots.items, engine->roots.len, sizeof(ModManager*), compare_mods
  );
  for (usize index = 0; index < engine->roots.len; index += 1) {
    engine->roots[index]->index = index + 1;
  }
}

auto compare_mods(const void* a, const  void* b) -> int {
  const ModManager& mod_a = **static_cast<ModManager* const*>(a);
  const ModManager& mod_b = **static_cast<ModManager* const*>(b);
  return static_cast<isize>(mod_a.index) - static_cast<isize>(mod_b.index);
}

auto set_mod_indices(ModManager* mod) -> void {
  if (mod->index) {
    return;
  }
  usize index = 0;
  for (auto import: mod->uses) {
    set_mod_indices(import);
    index = max(index, import->index);
  }
  // One more than the most so far.
  mod->index = index + 1;
}

}
