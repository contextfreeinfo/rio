#include <stdlib.h>

#include "main.h"

namespace rio {

auto compare_defs(const void* a, const void* b) -> int;
auto compare_mods(const void* a, const void* b) -> int;
auto set_def_indices(Def* def) -> void;
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

auto order_types(ModManager* mod) -> void {
  for (auto def: mod->global_defs) {
    set_def_indices(def);
  }
  qsort(
    mod->global_defs.items, mod->global_defs.len, sizeof(Def*), compare_defs
  );
}

auto compare_defs(const void* a, const void* b) -> int {
  const Def& def_a = **static_cast<Def* const*>(a);
  const Def& def_b = **static_cast<Def* const*>(b);
  return def_a.depth - def_b.depth;
}

auto compare_mods(const void* a, const void* b) -> int {
  const ModManager& mod_a = **static_cast<ModManager* const*>(a);
  const ModManager& mod_b = **static_cast<ModManager* const*>(b);
  return mod_a.index - mod_b.index;
}

auto set_def_indices(Def* def) -> void {
  // TODO Change depth back to index?
  if (def->depth) {
    return;
  }
  rint depth = 0;
  // This is just a complicated way of looping through ref'd types.
  // TODO Lacking generators, it still might be nice to simplify this.
  if (def->type && def->type->kind == Type::Kind::Array) {
    if (def->type->arg) {
      auto arg_def = def->type->arg->def;
      // No need to cross mods.
      if (arg_def && arg_def->mod == def->mod) {
        set_def_indices(arg_def);
        depth = max(depth, arg_def->depth);
      }
    }
  } else if (def->top) {
    auto node = def->top;
    switch (node->kind) {
      case Node::Kind::Struct: {
        if (node->Fun.expr->kind == Node::Kind::Block) {
          for (auto item: node->Fun.expr->Block.items) {
            // No need to cross mods.
            if (item->type.def && item->type.def->mod == def->mod) {
              set_def_indices(item->type.def);
              depth = max(depth, item->type.def->depth);
            }
          }
        }
        break;
      }
      default: break;
    }
  }
  def->depth = depth + 1;
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
