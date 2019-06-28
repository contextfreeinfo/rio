#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

struct Options {
  char* in;
};

// private

void fail(const char* message);
void* xmalloc(size_t nbytes);
