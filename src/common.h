#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
// #include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace rio {

template<typename Value>
using Array = std::vector<Value>;

template<typename Key, typename Value>
using Map = std::unordered_map<Key, Value>;

template<typename Value>
struct Slice {

  Value* items;
  usize len;

  auto operator[](usize index) -> Value& {
    return items[index];
  }

  auto operator[](usize index) const -> const Value& {
    return items[index];
  }

};

}
