#pragma once

#include "resolve.hpp"

namespace rio {

// TODO Define externally and include (preparsed?) during build.
const std::string_view std_source = R"std_source(

type String
type Void

def print(line: String): Void = extern

)std_source";

auto std_init_c() -> Script {
  Script script{std_source};
  // std::cout << "print: " << script.root.get_def("print") << std::endl;
  auto& print = static_cast<DefNode&>(*script.root.get_def("print")->info);
  print.generate = [](std::ostream& out, Node& call) {
    //
  };
  return script;
}

}
