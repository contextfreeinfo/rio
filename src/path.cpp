#include "main.h"

namespace rio {

auto push_parent(StrBuf* buf, string path) -> void {
  auto last = strrchr(path, '/');
  if (last) {
    // Include the slash.
    auto len = static_cast<usize>(last - path + 1);
    buf->push_str({const_cast<char*>(path), len});
  } else {
    buf->push_string("./");
  }
}

}
