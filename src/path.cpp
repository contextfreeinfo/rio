#include "main.h"

namespace rio {

auto normalize_path(char* path) -> void {
  while (true) {
    path = strchr(path, '\\');
    if (!path) {
      break;
    }
    *path = '/';
  }
}

auto normalized_path(StrBuf* buf, string path) -> Str {
  buf->clear();
  buf->push_string(path);
  normalize_path(buf->items);
  // Currently no length changes, but maybe later.
  buf->len = strlen(buf->items);
  return {buf->items, buf->len};
}

auto path_to_name(StrBuf* buf, string path) -> Str {
  // Allocate possibly a bit too much, just to make life easier.
  // This shouldn't be called 1000s of times.
  usize length_max = strlen(path);
  // TODO Make sure nested names like "name/name.rio" see only "name".
  // Find end, excluding any suffix.
  // TODO Extract this.
  auto end = strrchr(path, '.');
  if (end) {
    length_max = end - path;
  } else {
    end = path + length_max;
  }
  // Handle this common case since it's easy.
  if (!strncmp(path, "./", 2)) {
    length_max -= 2;
  }
  length_max = max(length_max, static_cast<usize>(1));
  buf->reserve(length_max + 1);
  auto res = buf->items;
  // Convert things.
  bool started = false;
  bool last_other = false;
  for (string str = path; str < end; str += 1) {
    char c = *str;
    if (!started) {
      // Wait until we get to a name char.
      if (is_id_start(c)) {
        *res = c;
        res += 1;
        started = true;
      }
    } else {
      if (is_id_part(c)) {
        *res = c;
        res += 1;
        last_other = false;
      } else {
        // Put in up to a single underscore between name parts.
        if (!last_other) {
          *res = '_';
          res += 1;
        }
        last_other = true;
      }
    }
  }
  if (started && last_other) {
    // Don't end on an underscore.
    res -= 1;
  }
  // Null-terminate.
  *res = '\0';
  buf->len = res - buf->items;
  return *buf;
}

auto push_parent(StrBuf* buf, string path) -> void {
  auto last = strrchr(path, '/');
  if (!last) {
    last = strrchr(path, '\\');
  }
  if (last) {
    // Include the slash.
    auto len = static_cast<usize>(last - path + 1);
    buf->push_str({const_cast<char*>(path), len});
    normalize_path(buf->items);
  } else {
    buf->push_string("./");
  }
}

}
