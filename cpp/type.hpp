#pragma once

#include <vector>

namespace rio {

struct TypeParam {

  std::string name;

  // TODO Constraints

};

struct GenericType {

  std::vector<TypeParam> params;

  // std::vector<...> positionals;

};

struct StructType {

  // std::map<std::string, ...> fields;

  // std::vector<...> positionals;

};

}
