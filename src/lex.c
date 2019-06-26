#include "rio.h"

char* read_file(const char* name) {
  FILE* file = fopen(name, "rb");
  if (!file) {
    fail("bad file");
  }
  fseek(file, 0, SEEK_END);
  long len = ftell(file);
  fseek(file, 0, SEEK_SET);
  char* buf = xmalloc(len + 1);
  if (len && fread(buf, len, 1, file) != 1) {
    fclose(file);
    free(buf);
    fail("bad read");
  }
  fclose(file);
  buf[len] = 0;
  return buf;
}

void lex(const Options* options) {
  char* buf = read_file(options->in);
  printf("lex: %s\n", options->in);
  // printf("%s", buf);
  free(buf);
}
