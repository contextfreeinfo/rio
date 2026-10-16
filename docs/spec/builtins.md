# Builtins

The builtins are there in every module without an import. [`src/core.rio`](../../src/core.rio)
declares each one with its types, and the tests check those declarations against the compiler. The
ones that would need generics are written there in comments for now.

## log

`log(a, b, ...)` writes its arguments as text on one line, with spaces between, to the host's log (the
CLI prints it). It takes `Int`, `Float`, `Bool`, `String`, `Blob` and enum values:

```rio
Dir :: enum
  north, east
end
log("n", 42, 1.5, true, Dir.east)
log()
log(-0.25, 100.0, 1e9, 1.0 / 3.0, 0.00001, 123456789.0)
```

```output
n 42 1.5 true east

-0.25 100.0 1.0e9 0.333333 1.0e-5 123456792.0
```

How values become text, in `log` and `format`:

- `Int`: decimal, with `-` when negative.
- `Bool`: `true` or `false`. Enums: the value's name.
- `String` and `Blob`: their bytes as they are.
- `Float`: `nan`, `inf` or `-inf`, or else digits with one decimal point. Values from `1e-4` up to
  (not including) `1e9` are written plainly; others as a number from 1 to 10 and `e` with a power of
  ten, like `1.5e12`. Either way: about 7 significant digits, at most 6 after the point, rounded, with
  trailing zeros dropped down to one.

## Math

| | |
|---|---|
| `sqrt sin cos tan asin acos atan exp ln floor ceil round` | `(x: Float) -> Float` |
| `atan2(y, x)`, `pow(x, y)`, `fmod(x, y)` | `(Float, Float) -> Float` |
| `abs(x)` | `Int` or `Float` |
| `min(a, b)`, `max(a, b)` | two `Int`s or two `Float`s |

`ln` is the natural log; `round` rounds halves away from zero; `fmod` is the `Float` remainder, with
the sign of `x`. A number literal argument becomes a `Float` where one is needed, so `min(2, 3.5)` is
`2.0`. Results are as from the C library's single-precision functions, so the last digit may differ
between hosts.

```rio
log(sqrt(16), floor(-2.5), round(2.5), round(-2.5), abs(-3), min(2, 3.5), pow(2, 10))
```

```output
4.0 -3.0 3.0 -3.0 3 2.0 1024.0
```

## assert

`assert(condition)` or `assert(condition, why)` is a runtime error, at its line, when the condition is
false:

```rio
assert(1 + 1 == 2)
assert(2 + 2 == 5, "math")  # runtime error: assertion failed: math
```

## hexBlob

`hexBlob("...")` is the bytes the hex digits spell, as a writable `Blob`, decoded while compiling.
Spaces and line breaks can go between pairs of digits; anything else, or an odd number of digits, is a
compile error. It takes only a literal. The bytes are stored once, like a global, so each time the same
call runs it gives the same bytes:

```rio
b := hexBlob("48 69
  21")
log(b.len(), b[0], b)
```

```output
3 72 Hi!
```

## Sequences

Every array, slice, list, builder, `String` and `Blob` has:

- `xs.len()`: how many elements (for a list or builder, how many it has now).
- `xs.cap()`: how many it can hold (the same as `len` for the fixed-length kinds).

## Lists and builders

These change a list `[..N]T` or a builder `[..]T` (`Blob[..N]` and `Blob[..]` too):

| | |
|---|---|
| `xs.push(x) -> Bool` | adds one element at the end; `false`, and no change, when full |
| `xs.pushAll(ys) -> Bool` | adds every element of an array, slice or list of the same type (a `String` for byte lists): all or none |
| `xs.pop() -> T` | removes the last element and gives it; a runtime error when empty |
| `xs.clear()` | removes them all |
| `xs.remove(i)` | removes element `i`, keeping the order of the rest |
| `xs.swapRemove(i)` | removes element `i`, moving the last one into its place |
| `b.format(...) -> Bool` | for byte lists: appends its arguments as text, as `log` would but without spaces; all or none |

`remove` and `swapRemove` with `i` outside the list are runtime errors.

```rio
xs: [..4]Int
for i in 0..<5
  xs.push(i * 10)            # the fifth doesn't fit
end
xs.remove(0)                 # 10 20 30
xs.swapRemove(0)             # 30 20
last := xs.pop()             # 20; leaves 30
title: Blob[..16]
title.format("hp ", 7, " ", 2.5)
log(xs.len(), xs[0], last, title[:])
```

```output
1 30 20 hp 7 2.5
```

```rio
xs: [..2]Int
log(xs.pop())  # runtime error: pop from empty list
```

A builder `b: [..]T = array` (or a slice of one) builds into that memory, starting empty. Its length
lives in a hidden word next to `b`, which every copy of `b` shares:

```rio
mem: [4]Int
b: [..]Int = mem
b.push(7)
fill :: proc(out: [..]Int)
  out.push(8)
end
fill(b)
log(b.len(), mem[0], mem[1])
```

```output
2 7 8
```

A builder needs a declaration to hold its length, so passing an array straight to a `[..]T`
parameter is an error. A builder can't be made over read-only bytes, such as a `String`.

## Enums

For an enum `E`, `E.fromInt(n)` is the value `n` (a runtime error if `E` has no such value, or a
compile error if `n` is a constant), `E.has(n)` says whether it has one, and `e.toInt()` is `e`'s
number:

```rio
Tile :: enum
  nil
  wall = 3
end
n := 3
log(Tile.fromInt(n), Tile.has(2), Tile.wall.toInt())
```

```output
wall false 3
```

## Conversions

`toInt`, `toFloat`, `toBool` and `asString`: see [conversions](expressions.md#conversions).
