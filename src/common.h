#pragma once

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rio.h"

auto operator new(size_t, void* item) -> void* {
  return item;
}

namespace rio {

void fail(const char* message);

u64 hash_bytes(const void *bytes, usize nbytes) {
  // From Vognsen's ion.
  u64 x = 0xcbf29ce484222325;
  const char* buf = static_cast<const char*>(bytes);
  for (usize i = 0; i < nbytes; ++i) {
    x ^= buf[i];
    x *= 0x100000001b3;
    x ^= x >> 32;
  }
  return x;
}

template<typename Value>
auto max(Value a, Value b) -> Value {
  return a >= b ? a : b;
}

template<typename Value>
auto min(Value a, Value b) -> Value {
  return a <= b ? a : b;
}

auto xmalloc(usize nbytes) -> void*;

template<typename Item>
struct Slice {

  Item* items;
  // TODO With Item* end, iteration becomes easier ...
  // TODO Some iterator/range type for that? ...
  usize len;
  // TODO Alternative with stride also, called Slide?

  auto operator[](usize index) -> Item& {
    assert(index < len);
    return items[index];
  }

  auto operator[](usize index) const -> const Item& {
    return const_cast<Slice&>(*this)[index];
  }

  auto begin() -> Item* {
    return this->items;
  }

  auto begin() const -> const Item* {
    return this->items;
  }

  auto end() -> Item* {
    return this->items + this->len;
  }

  auto end() const -> const Item* {
    return const_cast<Slice*>(this)->end();
  }

};

using Str = Slice<char>;

auto str_cmp(Str a, Str b) -> int {
  auto len = min(a.len, b.len);
  auto result = strncmp(a.items, b.items, len);
  if (result) {
    return result;
  }
  return static_cast<isize>(a.len) - static_cast<isize>(b.len);
}

auto str_eq(Str a, Str b) -> bool {
  return !str_cmp(a, b);
}

template<typename Item>
struct List: Slice<Item> {

  usize capacity;

  List(): Slice<Item>{0}, capacity{0} {}

  ~List() {
    free(this->items);
  }

  auto back() -> Item& {
    return this->items[this->len - 1];
  }

  auto clear() -> void {
    // TODO Also memset all to 0?
    this->len = 0;
  }

  virtual void distribute(Slice<Item> new_items, Slice<Item> old_items) {
    // For now, presume this is good enough.
    // We're not trying to replicate all of libstdc++.
    memcpy(new_items.items, old_items.items, old_items.len * sizeof(Item));
  }

  auto push(Item& item) -> Item& {
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
    return back();
  }

  auto push_val(Item item) -> Item& {
    return push(item);
  }

  auto reserve(usize capacity) -> void {
    if (capacity <= this->capacity) {
      return;
    }
    auto items = this->items;
    Item* new_items = static_cast<Item*>(xmalloc(capacity * sizeof(Item)));
    memset(new_items, 0, capacity * sizeof(Item));
    if (items) {
      distribute({new_items, capacity}, {items, this->len});
      free(items);
    }
    items = this->items = new_items;
    this->capacity = capacity;
  }

};

struct StrBuf: List<char> {

  StrBuf() {
    // Keep a null char.
    push_val('\0');
    // But don't include it in the length.
    len = 0;
  }

  auto push_str(const Str& text) -> void {
    reserve(len + text.len + 1);
    strncpy(items + len, text.items, text.len);
    len += text.len;
    items[len] = '\0';
  }

  // Not really safe.
  auto push_strn(string text, usize len) -> void {
    push_str({const_cast<char*>(text), len});
  }

  auto push_string(string text) -> void {
    push_strn(text, strlen(text));
  }

  auto shrink_string(usize len) -> void {
    assert(len <= this->len);
    this->len = len;
    items[len] = '\0';
  }

};

struct Arena {

  ~Arena() {
    for (auto& box: boxes) {
      box.~List();
    }
  }

  void* alloc_bytes(usize nbytes) {
    List<u8>* box = nullptr;
    usize index = 0;
    if (boxes.len) {
      // TODO Better to loop through all current boxes for space?
      box = &boxes.back();
      // TODO Parameter to override alignment for an entry.
      index = box->len;
      if (index % align) {
        index += align - (index % align);
      }
      if (box->capacity - index < nbytes) {
        // No space here.
        box = nullptr;
        index = 0;
      }
    }
    if (!box) {
      // Make a new box.
      boxes.push_val({});
      box = &boxes.back();
      // TODO Double box size each time???
      box->reserve(max(nbytes, default_box_size));
    }
    // Allocate the needed bytes, for which we already know we have space.
    u8* end = box->items + index;
    box->len = index + nbytes;
    // printf("alloc %zu at %zu, now %zu\n", nbytes, index, box->len);
    memset(end, 0, nbytes);
    return end;
  }

  template<typename Item>
  auto alloc() -> Item& {
    auto& item = *static_cast<Item*>(alloc_bytes(sizeof(Item)));
    new(&item) Item;
    return item;
  }

 private:
  usize align = 8;
  List<List<u8>> boxes;
  usize default_box_size = 8 << 10;
};

template<typename Value>
struct Pair {
  Str key;
  Value value;
};

template<typename Value>
struct Map {

  using Pair = rio::Pair<Value>;

  struct PairList: List<Pair> {
    PairList(Map* map_): map{*map_} {}
    virtual void distribute(Slice<Pair> new_items, Slice<Pair> old_items) {
      // Remember old then reset for new capacity.
      usize old_capacity = map.pairs.capacity;
      map.pairs.items = new_items.items;
      map.pairs.len = 0;
      map.pairs.capacity = new_items.len;
      // Rehash and refill.
      for (usize i = 0; i < old_capacity; i += 1) {
        // Cheat into lower level to avoid bounds check.
        // We abuse the meaning of len here.
        auto& pair = map.pairs.items[i];
        if (pair.key.items) {
          map.fit(pair.key, pair.value);
        }
      }
    }
    Map& map;
  };

  PairList pairs{this};

  void fit(Str key, Value value) {
    // Put it in.
    isize index = get_index(key);
    if (index < 0) {
      fail("no space in map");
    }
    auto& slot = pairs.items[index];
    if (!slot.key.items) {
      slot.key = key;
      pairs.len += 1;
    }
    slot.value = value;
  }

  auto get(const Str& key) -> Value {
    isize index = get_index(key);
    if (index < 0) {
      return nullptr;
    } else {
      auto& pair = pairs.items[index];
      if (!pair.key.items) {
        return nullptr;
      }
      return pair.value;
    }
  }

  auto get_index(const Str& key) -> isize {
    if (!pairs.capacity) {
      return -1;
    }
    usize index = hash_bytes(key.items, key.len) % pairs.capacity;
    usize original = index;
    do {
      auto& slot = pairs.items[index];
      if (slot.key.items) {
        if (str_eq(slot.key, key)) {
          // printf("get_index %s: %zu, %zu\n", std::string(key.items, key.len).c_str(), original, index);
          return index;
        }
      } else {
        // printf("get_index %s: %zu, %zu\n", std::string(key.items, key.len).c_str(), original, index);
        return index;
      }
      index = (index + 1) % pairs.capacity;
    } while (index != original);
    return -1;
  }

  void put(Str key, Value value) {
    // Also, key must outlive the map.
    assert(key.items);
    auto capacity = pairs.capacity;
    auto len = pairs.len;
    if (capacity <= 2 * len) {
      if (capacity) {
        capacity *= 2;
      } else {
        capacity = 2;
      }
      pairs.reserve(capacity);
    }
    fit(key, value);
  }

};

}