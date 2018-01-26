#pragma once

#include <memory>
#include <string_view>
#include <unordered_map>

namespace rio {

// Or `My`?
template<typename Item>
using Box = std::unique_ptr<Item>;

using F32 = float;
using F64 = double;

using I64 = int64_t;

// Indicate a non-null pointer to a single item.
// This is for cases where reassignment is needed so refs won't do.
template<typename Item>
using One = Item*;

template<typename Item>
using Opt = Item*;

template<typename Key, typename Value>
using Map = std::unordered_map<Key, Value>;

using Str = std::string_view;

using USize = size_t;

}
