#pragma once

#include <stddef.h>
#include <stdint.h>

// TODO Make this central for core and c, with main separate?

namespace rio {

// Int types.
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
// I'd call this int if `int` weren't a keyword. In rio itself, it's int.
using rint = ptrdiff_t;
// Unsigned.
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using usize = size_t;

auto int_to_usize(rint i) -> rint {
  // TODO Fail on too small?
  return i < 0 ? 0 : static_cast<usize>(i);
}

auto usize_to_int(usize u) -> rint {
  // TODO Fail on too big?
  return u > (SIZE_MAX >> 1) ? INT_FAST32_MAX : static_cast<rint>(u);
}

// Float types.
using f32 = float;
using f64 = double;

// Aggregates.

// In rio: string = *u8 = *u8..0
using string = const char*;
using opt_string = const char*;

template<typename Item>
using Opt = Item*;

}
