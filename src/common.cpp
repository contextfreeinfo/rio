#include "main.h"
// #include <stdexcept>

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
  const Str key_str = {const_cast<char*>(text), nbytes};
  auto value = engine->interns.get(key_str);
  if (!value) {
    // If missing, push on to arena for lifetime.
    Str str =
      {static_cast<char*>(engine->arena.alloc_bytes(nbytes + 1)), nbytes};
    strncpy(str.items, text, nbytes);
    str.items[nbytes] = '\0';
    engine->interns.put(str, str.items);
    value = str.items;
    // printf("hey: %s, %zu, %x\n", value, nbytes, value[nbytes]);
  }
  return value;
}

auto intern_str(Engine* engine, const Str& str) -> const char* {
  return intern(engine, str.items, str.len);
}

auto read_file(const char* name) -> char* {
  FILE* file = fopen(name, "rb");
  if (!file) {
    fprintf(stderr, "file name: %s\n", name);
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
