#pragma once

#include <stdint.h>

namespace rio {

// Int types.
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using isize = intptr_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using usize = uintptr_t;

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
