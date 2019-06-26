#include "rio.h"

#include "common.c"
#include "lex.c"

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

void run(const Options* options) {
  lex(options);
}

int main(int argc, const char** argv) {
  const Options options = parse_options(argc, argv);
  run(&options);
  return 0;
}
