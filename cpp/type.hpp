#pragma once

#include "util.hpp"
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>

namespace rio {

struct EnumType;
struct NumberType;
struct StructType;
struct TypeParam;

// In most cases, types should live on the typedefs.
// They can then be references from the existing symbol scopes.
using Type = std::variant<EnumType, StructType, NumberType, TypeParam>;

struct TypeBase {
  std::string name;
};

enum struct NumberKind {
  Float,
  Signed,
  Unsigned,
};

struct NumberType: TypeBase {

  NumberKind kind;

  USize nbits;

};

struct TypeParam: TypeBase {

  // Intersection. (TODO Generally elsewhere?)
  std::vector<Type*> constraints;

};

struct GenericType: TypeBase {

  Map<std::string, Type*> arg_map;

  std::vector<TypeParam> args;

  Opt<Type> generic;

  Map<std::string, Type*> param_map;

  std::vector<TypeParam> params;

  std::vector<std::unique_ptr<Type*>> specifics;

};

// TODO Arbitrary sum/union types?
struct EnumType: GenericType {

  std::vector<Type*> cases;

};

struct StructType: GenericType {

  Map<std::string, Type*> fields;

};

}
