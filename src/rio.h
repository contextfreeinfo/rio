#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
// #include <stdexcept>
#include <unordered_map>
#include <vector>

namespace rio {

struct Options {
  char* in;
};

// private

void fail(const char* message);
void* xmalloc(size_t nbytes);

}
