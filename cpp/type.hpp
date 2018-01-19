#pragma once

#include "util.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace rio {

struct Type;

struct TypeParam {

  std::string name;

  // Intersection. (TODO Generally elsewhere?)
  std::vector<Type*> constraints;

};

struct Type {

  // TODO Where do the types live?
  std::unordered_map<std::string, Type*> arg_map;

  std::vector<TypeParam> args;

  optref<Type> generic;

  std::unordered_map<std::string, Type*> param_map;

  std::vector<TypeParam> params;

  std::vector<std::unique_ptr<Type*>> specifics;

};

// TODO Arbitrary sum/union types?
struct EnumType: Type {

  std::vector<Type*> cases;

};

struct StructType: Type {

  std::map<std::string, Type*> fields;

};

}
