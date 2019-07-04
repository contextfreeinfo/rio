#include "main.h"

#include "common.cpp"
#include "lex.cpp"
#include "parse.cpp"

namespace rio {

const Options parse_options(int argc, const char** argv) {
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
    printf("in: %s\n", file);
    auto buf = read_file(file);
    auto tokens = lex(engine, file, buf);
    free(buf);
    return tokens;
  }();
  parse(engine, tokens.data());
  printf("tokens: %zu\n", tokens.size());
}

}

int main(int argc, const char** argv) {
  using namespace rio;
  Engine engine;
  engine.options = parse_options(argc, argv);
  run(&engine);
  return 0;
}
