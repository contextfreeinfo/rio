#include "main.h"

#include "common.cpp"
#include "extract.cpp"
#include "genc.cpp"
#include "lex.cpp"
#include "parse.cpp"
#include "resolve.cpp"

namespace rio {

auto load_mod(Engine* engine, const char* file) -> ModManager* {
  auto& mod = engine->mods.push_val({});
  mod.engine = engine;
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
  for (int i = 0; i < argc; i += 1) {
    const char* arg = argv[i];
    if (!strcmp(arg, "-i")) {
      i += 1;
      if (i < argc) {
        options.in = argv[i];
      } else {
        fail("missing arg");
      }
    }
  }
  return options;
}

void run(Engine* engine) {
  auto mod = load_mod(engine, engine->options.in);
  // TODO Load imported mods.
  resolve(engine, mod->tree);
  c::gen(engine, *mod->tree);
}

}

int main(int argc, const char** argv) {
  using namespace rio;
  Engine engine;
  engine.options = parse_options(argc, argv);
  // TODO Use stderr or stdout if verbosity level high enough.
  // TODO Close by destructor.
  // engine.info = open_nul();
  run(&engine);
  // Clean up.
  // fclose(engine.info);
  return 0;
}
