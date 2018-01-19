#pragma once

#include <unordered_map>

namespace rio {

template<typename Item>
using Opt = Item*;

using Index = size_t;

template<typename Key, typename Value>
using Map = std::unordered_map<Key, Value>;

}
