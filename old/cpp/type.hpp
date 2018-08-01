#pragma once

#include "util.hpp"
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>

namespace rio {

struct Node;

struct EnumType;
struct FunctionType;
struct NumberType;
struct StructType;
struct TypeParam;

struct VoidType {};

struct Type;

struct TypeInfo {

  std::string name;

  virtual ~TypeInfo() {}

};

enum TypeKind {
  Enum,
  Function,
  Number,
  Param,
  Struct,
  Void,
};

// In most cases, types should live on the typedefs.
// They can then be references from the existing symbol scopes.
struct Type {

  TypeKind kind = TypeKind::Void;

  Box<TypeInfo> info;

  Opt<Node> node = nullptr;

  template<typename Info>
  auto make(Opt<Node> node = nullptr) -> Info& {
    this->node = node;
    kind = Info::type_kind;
    info.reset(new Info);
    return static_cast<Info&>(*info);
  }

};

enum struct NumberKind {
  Float,
  Int,
  UInt,
};

struct NumberType: TypeInfo {

  static constexpr auto type_kind = TypeKind::Number;

  NumberKind kind;

  USize nbits;

};

struct TypeParam: TypeInfo {

  static constexpr auto type_kind = TypeKind::Param;

  // Intersection. (TODO Generally elsewhere?)
  std::vector<Type*> constraints;

};

struct GenericType: TypeInfo {

  // TODO This takes up lots of ram.
  // TODO Some way to pay the price only where needed?

  Map<std::string, Type*> arg_map;

  std::vector<TypeParam> args;

  Opt<Type> generic;

  Map<std::string, Type*> param_map;

  std::vector<TypeParam> params;

  std::vector<std::unique_ptr<Type*>> specifics;

};

// TODO Arbitrary sum/union types?
// TODO C has unions but no type indicator. Have "union" and "union_raw"?
struct EnumType: GenericType {

  static constexpr auto type_kind = TypeKind::Enum;

  std::vector<Type*> cases;

};

struct StructType: GenericType {

  static constexpr auto type_kind = TypeKind::Struct;

  Map<std::string, Type*> fields;

};

struct FunctionType: GenericType {

  static constexpr auto type_kind = TypeKind::Function;

  // Opt<Type> capture;
  // Opt<Type> error;
  Type input;
  Type output;

};

}
