#include "main.h"

#include "common.cpp"
#include "lex.cpp"
#include "parse.cpp"
#include "genc.cpp"

namespace rio {

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
    char* arg = (char*)argv[i];
    if (!strcmp(arg, "-i")) {
      i += 1;
      if (i < argc) {
        options.in = (char*)argv[i];
      } else {
        fail("missing arg");
      }
    }
  }
  return options;
}

void run(Engine* engine) {
  auto tokens = [&]() {
    auto file = engine->options.in;
    if (engine->verbose) printf("in: %s\n", file);
    auto buf = read_file(file);
    auto tokens = lex(engine, file, buf);
    free(buf);
    return tokens;
  }();
  if (engine->verbose) printf("tokens: %zu\n", tokens.len);
  auto& tree = parse(engine, tokens.items);
  c::gen(engine, tree);
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
