#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unordered_map>

#include "rio.h"

namespace rio {

template<typename Value>
auto max(Value a, Value b) -> Value {
  return a >= b ? a : b;
}

auto xmalloc(usize nbytes) -> void*;

template<typename Key, typename Value>
using Map = std::unordered_map<Key, Value>;

template<typename Item>
struct Slice {

  Item* items;
  usize len;
  // TODO Alternative with stride also, called Slide?

  auto operator[](usize index) -> Item& {
    return items[index];
  }

  auto operator[](usize index) const -> const Item& {
    return items[index];
  }

};

template<typename Item>
struct List: Slice<Item> {

  usize capacity;

  List(): capacity{0} {
    this->items = nullptr;
    this->len = 0;
  }

  ~List() {
    free(this->items);
  }

  auto back() -> Item& {
    return this->items[this->len - 1];
  }

  void push(Item& item) {
    auto capacity = this->capacity;
    auto len = this->len;
    if (capacity <= len) {
      if (capacity) {
        capacity *= 2;
      } else {
        capacity = 1;
      }
      reserve(capacity);
    }
    // To keep things consistent, memcpy here, too.
    // items[len] = item;
    memcpy(this->items + len, &item, sizeof(Item));
    this->len += 1;
  }

  void push_val(Item item) {
    push(item);
  }

  void reserve(usize capacity) {
    if (capacity <= this->capacity) {
      return;
    }
    auto items = this->items;
    Item* new_items = static_cast<Item*>(xmalloc(capacity * sizeof(Item)));
    memset(new_items, 0, capacity * sizeof(Item));
    if (items) {
      // For now, presume this is good enough.
      // We're not trying to replicate all of libstdc++.
      memcpy(new_items, items, this->len * sizeof(Item));
      free(items);
    }
    items = this->items = new_items;
    this->capacity = capacity;
  }

};

struct Arena {

  ~Arena() {
    for (usize i = 0; i < boxes.len; ++i) {
      boxes[i].~List();
    }
  }

  void* alloc(usize nbytes) {
    List<u8>* box = nullptr;
    if (boxes.len) {
      // TODO Better to loop through all current boxes for space?
      box = &boxes.back();
      if (box->capacity - box->len < nbytes) {
        // No space here.
        box = nullptr;
      }
    }
    if (!box) {
      // Make a new box.
      boxes.push_val({});
      box = &boxes.back();
      box->reserve(max(nbytes, default_box_size));
    }
    // Allocate the needed bytes, for which we already know we have space.
    u8* end = box->items + box->len;
    box->len += nbytes;
    return end;
  }

  template<typename Item>
  auto alloc() -> Item& {
    return *static_cast<Item*>(alloc(sizeof(Item)));
  }

 private:

  List<List<u8>> boxes;

  usize default_box_size = 16 << 10;

};

}
