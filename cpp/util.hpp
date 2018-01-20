#pragma once

#include <memory>
#include <string_view>
#include <unordered_map>

namespace rio {

// Or `My`?
template<typename Item>
using Box = std::unique_ptr<Item>;

template<typename Item>
using Opt = Item*;

template<typename Key, typename Value>
using Map = std::unordered_map<Key, Value>;

using Str = std::string_view;

using USize = size_t;

}
