#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
// #include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "rio.h"

namespace rio {

template<typename Value>
auto max(Value a, Value b) -> Value {
  return a >= b ? a : b;
}

template<typename Value>
using Array = std::vector<Value>;

template<typename Key, typename Value>
using Map = std::unordered_map<Key, Value>;

template<typename Value>
struct Slice {

  Value* items;
  usize len;
  // TODO Alternative with stride also, called Slide?

  auto operator[](usize index) -> Value& {
    return items[index];
  }

  auto operator[](usize index) const -> const Value& {
    return items[index];
  }

};

struct Arena {

  void* alloc(usize nbytes) {
    Array<u8>* box = nullptr;
    if (!boxes.empty()) {
      // TODO Better to loop through all current boxes for space?
      box = &boxes.back();
      if (box->capacity() - box->size() < nbytes) {
        // No space here.
        box = nullptr;
      }
    }
    if (!box) {
      // Make a new box.
      boxes.resize(boxes.size() + 1);
      box = &boxes.back();
      box->reserve(max(nbytes, default_box_size));
    }
    // Allocate the needed bytes.
    u8* end = box->data() + box->size();
    box->resize(box->size() + nbytes);
    return end;
  }

  template<typename Item>
  auto alloc() -> Item& {
    return *static_cast<Item*>(alloc(sizeof(Item)));
  }

 private:

  Array<Array<u8>> boxes;

  usize default_box_size = 16 << 10;

};

}
