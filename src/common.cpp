#include "main.h"

namespace rio {

void fail(const char* message) {
  perror(message);
  exit(1);
  // throw std::runtime_error(message);
}

auto intern(Engine* engine, const char* text, usize nbytes) -> const char* {
  // This implementation depends on pointer data staying constant over map
  // resizing, which would be the efficient way to do things.
  // But is it guaranteed?
  // TODO Should probably use a set instead of a map, if this works.
  // TODO Also, replace all this with non-libc++ implementation.
  std::string key{text, nbytes};
  const char* value;
  auto current = engine->texts.find(key);
  if (current == engine->texts.end()) {
    value = (engine->texts[key] = key).c_str();
  } else {
    value = current->second.c_str();
  }
  return value;
}

auto read_file(const char* name) -> char* {
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
