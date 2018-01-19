#pragma once

#include <unordered_map>

namespace rio {

template<typename Item>
using Opt = Item*;

template<typename Key, typename Value>
using Map = std::unordered_map<Key, Value>;

using USize = size_t;

}
