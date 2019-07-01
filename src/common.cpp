#include "rio.h"

namespace rio {

void fail(const char* message) {
  perror(message);
  exit(1);
  // throw std::runtime_error(message);
}

char* read_file(const char* name) {
  FILE* file = fopen(name, "rb");
  if (!file) {
    fail("bad file");
  }
  fseek(file, 0, SEEK_END);
  long len = ftell(file);
  fseek(file, 0, SEEK_SET);
  char* buf = static_cast<char*>(xmalloc(len + 1));
  if (len && fread(buf, len, 1, file) != 1) {
    fclose(file);
    free(buf);
    fail("bad read");
  }
  fclose(file);
  buf[len] = 0;
  return buf;
}

void* xmalloc(size_t nbytes) {
  void* result = malloc(nbytes);
  if (!result) {
    fail("bad malloc");
  }
  return result;
}

}
