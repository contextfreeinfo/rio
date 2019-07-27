#include "main.h"

#include "common.cpp"
#include "extract.cpp"
#include "genc.cpp"
#include "lex.cpp"
#include "parse.cpp"
#include "path.cpp"
#include "resolve.cpp"

namespace rio {

auto load_imports(ModManager* mod) -> void {
  StrBuf buf;
  // TODO Extract path concat logic.
  // TODO Extract path concat logic.
  push_parent(&buf, mod->file);
  auto parent_len = buf.len;
  for (auto node: mod->tree->Block.items) {
    if (node->kind != Node::Kind::Use) {
      // Require all imports up top.
      break;
    }
    auto name = node->Use.name;
    if (!*name) {
      // Nothing actually given to use.
      continue;
    }
    // Good to go. See what we have.
    buf.shrink_string(parent_len);
    usize offset = 0;
    if (!strncmp(name + 1, "./", 2)) {
      // Relative.
      // TODO Parse the string to handle escapes, etc.
      buf.push_strn(name + 3, strlen(name) - 4);
    } else if (!strncmp(name + 1, "../", 3)) {
      // Punt on cleaning parent for now.
      // TODO We'll need to clean up to avoid duplicates in cross import.
      // TODO Parse the string to handle escapes, etc.
      buf.push_strn(name + 1, strlen(name) - 2);
    } else {
      // Absolute.
      // TODO Parse the string to handle escapes, etc.
      buf.push_strn(name + 1, strlen(name) - 2);
      offset = parent_len;
    }
    buf.push_string(".rio");
    string path = intern(mod->engine, buf.items + offset, buf.len - offset);
    // Skip non-relative imports for now.
    if (!offset) {
      // Now actually load things.
      // Skip if we already loaded it!
      Opt<ModManager> import = nullptr;
      for (auto mod: mod->engine->mods) {
        if (!strcmp(mod->file, path)) {
          import = mod;
          break;
        }
      }
      if (!import) {
        // It's new.
        // TODO Track imports for each file.
        // TODO Includes have priority.
        // TODO Conflicts between others require renaming or qualifying.
        import = load_mod([&]() {
          ModInfo dep;
          dep.engine = mod->engine;
          dep.file = path;
          if (node->Use.kind == Token::Kind::Include) {
            dep.name = mod->name;
            dep.root = mod->root;
          }
          return dep;
        }());
        if (node->Use.kind == Token::Kind::Include && mod->root) {
          mod->root->parts.push(import);
        }
      }
      // Import should be set by now.
      // Track only uses here, since includes are transitive, so we need other
      // other handling for those.
      if (node->Use.kind == Token::Kind::Use) {
        mod->root->uses.push(import->root);
      }
    }
  }
}

auto load_mod(const ModInfo& info) -> ModManager* {
  StrBuf buf;
  // TODO Alloc mods individually so we don't gradually bleed on reloads.
  // TODO Or can we placement new over old discarded mods?
  auto engine = info.engine;
  auto file = info.file;
  auto& mod = engine->arena.alloc<ModManager>();
  if (*info.name) {
    mod.name = info.name;
  } else {
    // TODO Random (remembered) name for scripts.
    mod.name = intern_str(engine, path_to_name(&buf, engine->options.in));
  }
  engine->mods.push_val(&mod);
  mod.engine = engine;
  mod.file = file;
  // TODO What if you `use` different roots of a multimod?
  // TODO Need to determine a best root? Or forbid such doings?
  mod.root = info.root ? info.root : &mod;
  auto tokens = [&]() {
    if (verbose) printf("in: %s\n", file);
    auto buf = read_file(file);
    auto tokens = lex(&mod, file, buf);
    free(buf);
    return tokens;
  }();
  if (verbose) printf("tokens: %zu\n", tokens.len);
  // TODO First parse just imports at top, then kick those off.
  // TODO Need some kind of "make"-style dependency driven async work engine.
  // TODO Then move forward here.
  mod.tree = &parse(&mod, tokens.items);
  extract(&mod);
  // TODO Option for non-recursive.
  load_imports(&mod);
  return &mod;
}

// auto open_nul() -> FILE* {
//   // TODO Just choose correct option by preproc.
//   FILE* nul = fopen("/dev/null", "w");
//   if (nul) {
//     return nul;
//   }
//   nul = fopen("nul", "w");
//   assert(nul);
//   return nullptr;
// }

auto parse_options(Engine* engine, Slice<string> args) -> Options {
  StrBuf buf;
  Options options = {0};
  auto past_flags = false;
  for (usize i = 1; i < args.len; i += 1) {
    auto arg = args[i];
    if (!past_flags && *arg == '-') {
      // TODO Default to actually running things.
      // TODO Use "-c" to mean just generate c output.
      if (!strcmp(arg, "--")) {
        past_flags = true;
      }  // TODO else if ...
    } else {
      if (!options.in) {
        options.in = intern_str(engine, normalized_path(&buf, args[i]));
      }
    }
  }
  return options;
}

void run(Engine* engine) {
  // Load everything.
  load_mod([&]() {
    ModInfo mod;
    mod.engine = engine;
    mod.file = engine->options.in;
    return mod;
  }());
  // Resolve.
  // TODO Smart inter-resolution.
  // TODO Resolve as we load, up the chain of dependencies.
  // TODO Implement parallel dependency engine a la make.
  for (auto mod: engine->mods) {
    resolve(engine, mod);
  }
  // Generate.
  c::gen(engine);
}

}  // namespace rio

int main(int argc, const char** argv) {
  using namespace rio;
  Engine engine;
  engine.options = parse_options(&engine, {argv, static_cast<usize>(argc)});
  if (!engine.options.in) {
    fail("no input file");
  }
  // TODO Use stderr or stdout if verbosity level high enough.
  // TODO Close by destructor.
  // engine.info = open_nul();
  run(&engine);
  // Clean up.
  // fclose(engine.info);
  return 0;
}
