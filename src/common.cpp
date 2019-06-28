#include "rio.h"

namespace rio {

void fail(const char* message) {
  perror(message);
  exit(1);
  // throw std::runtime_error(message);
}

void* xmalloc(size_t nbytes) {
  void* result = malloc(nbytes);
  if (!result) {
    fail("bad malloc");
  }
  return result;
}

}
