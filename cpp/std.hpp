#pragma once

#include "tree.hpp"

namespace rio {

// TODO Define externally and include (preparsed?) during build.
const std::string_view std_source = R"std_source(

type String
type Void

def print(line: String): Void = extern

)std_source";

const std::string_view std_print = R"std_print(

#include <stdio.h>

typedef struct rio_String {
  size_t size;
  char* data;
} rio_String;

rio_String rio_string(size_t size, char* data) {
  rio_String string = {size, data};
  return string;
}

void print(rio_String string) {
  fwrite(string.data, 1, string.size, stdout);
  fwrite("\n", 1, 1, stdout);
  fflush(stdout);
}

)std_print";

auto std_init_c() -> Script {
  Script script{std_source};
  // std::cout << "print: " << script.root.get_def("print") << std::endl;
  // auto& print = static_cast<DefNode&>(*script.root.get_def("print")->info);
  // print.generate = [](GenState& state, Node& call) {
  //   //
  // };
  return script;
}

}
