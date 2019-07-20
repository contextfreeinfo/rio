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
  auto nodes = mod->tree->Block.items;
  for (usize i = 0; i < nodes.len; i += 1) {
    auto node = nodes[i];
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
    // Now actually load things.
    load_mod(mod->engine, path);
  }
}

auto load_mod(Engine* engine, const char* file) -> ModManager* {
  // TODO Alloc mods individually so we don't gradually bleed on reloads.
  // TODO Or can we placement new over old discarded mods?
  auto& mod = engine->arena.alloc<ModManager>();
  engine->mods.push_val(&mod);
  mod.engine = engine;
  mod.file = file;
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

auto parse_options(int argc, const char** argv) -> const Options {
  Options options = {0};
  auto past_flags = false;
  for (int i = 1; i < argc; i += 1) {
    const char* arg = argv[i];
    if (!past_flags && *arg == '-') {
      // TODO Default to actually running things.
      // TODO Use "-c" to mean just generate c output.
      if (!strcmp(arg, "--")) {
        past_flags = true;
      }  // TODO else if ...
    } else {
      if (!options.in) {
        options.in = argv[i];
      }
    }
  }
  return options;
}

void run(Engine* engine) {
  auto mod = load_mod(engine, engine->options.in);
  load_imports(mod);
  // TODO Load imported mods.
  resolve(engine, mod->tree);
  c::gen(engine, *mod->tree);
}

}  // namespace rio

int main(int argc, const char** argv) {
  using namespace rio;
  Engine engine;
  engine.options = parse_options(argc, argv);
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
