#pragma once

#include "tree.hpp"

namespace rio {

// TODO Define externally and include (preparsed?) during build.
const std::string_view std_source = R"std_source(

# Always 1D for now.
# Sometime to support ND later or in separate library.
type Array[Item]
# type Array[Item, let size: Int]

type Bool
# type Bool: Opt[Void] # Could be? Array of 0 or 1 (Opt) vs Void as array of 0?
# let false = Bool::None
# let true = Bool::Some()
# enum Bool # Maybe?
#    False
#    True
# end

type Box[Item]
# type Box[Item]: Array[Item, 1] ???

# TODO Formally, should be Octets.
# TODO No force null-terminated here? Just on literals?
# TODO Slices won't have 
type Bytes: Array[U8]

type Error

type F32

type F64

# Rust default float type.
type Float: F64

# type Function[Input, Output]
# struct Function[Input, Output, throws: Bool]
#   # Need specialization for throws or not ...
#   call: CFunction[Input: (Void&, ...Input), Output]
#   data: Void&
# end
# Input -> Output throws

type I8

type I16

type I32

type I64

# Rust default integer type, even for 64-bit systems.
# TODO(tjp): But it runs higher risk of overflow in math ...
type Int: I32

type ISize

type Opt[Item]
# enum Opt[Item]
#   None
#   Some(Item)
# end

# TODO Can enum be a library-level thing with vararg or tuple generics?
# TODO Like variant in c++?
# TODO Also, allow arbitary sum types of other types?
# enum Result[Item, E: Error]
#   Ok(Item)
#   Err(E)
# end

# TODO Slices don't own, and Arrays do? Is that all?
# TODO Dimensionality on slices?
type Slice[Item]

type U8

type U16

type U32

type U64

type USize

# TODO Or is Unit better? Or call it Void but use as Unit?
# TODO Just that Unit and Uint are so close. Maybe that's why () as a type?
type Void

def print(line: Bytes) -> Void = extern

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
