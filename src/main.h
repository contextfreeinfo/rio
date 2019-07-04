#pragma once

// The idea is that internal code just includes this and is good to go.

// Some of these are redundant, but eh.
#include "rio.h"
#include "common.h"
#include "node.h"

namespace rio {

struct Options {
  char* in;
};

struct Engine {
  Array<Node> node_buf;
  Options options = {0};
  Map<std::string, std::string> texts;
};

void fail(const char* message);
auto has_text(const Token& token) -> bool;
auto intern(Engine* engine, const char* text, usize nbytes) -> const char*;
auto read_file(const char* name) -> char*;
auto token_name(const Token& token) -> const char*;
auto token_text(const Token& token) -> const char*;
auto xmalloc(usize nbytes) -> void*;

}
