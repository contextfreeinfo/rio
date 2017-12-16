#pragma once

#include "parse.hpp"

namespace rio {

// TODO Define externally and include (preparsed?) during build.
const std::string_view std_source = R"std_source(

type String
type Void

def print(line: String): Void = extern

)std_source";

struct Context {
  //
};

struct Resolver {
  //
};

// const auto std_root = parse(std_source);

}
