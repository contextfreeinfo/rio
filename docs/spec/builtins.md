# Builtins

The builtins are there in every module without an import. [`src/core.rio`](https://github.com/contextfreeinfo/rio/blob/main/src/core.rio)
declares each one with its types, and the tests check those declarations against the compiler. The
ones that would need generics are written there in comments for now.

## log

`log(a, b, ...)` writes its arguments as text on one line, with spaces between, to the host's log (the
CLI prints it). It takes any value: numbers, `Bool`s, `String`s, `Blob`s, enums, and structs, arrays,
slices, lists and unions:

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

Structs, arrays, slices, lists and unions are written as Rio writes them, with text inside them quoted:

```rio
Point :: struct
  x, y: Int
end
Shape :: union
  nil
  Point
end
s: Shape
log(Point{x = 1, y = 2}, s)
s = Point{x = 3, y = 4}
names := [2]String{"Ann", "Bo"}
log(s, names, names[1:2])
big: [40]Int
log(big)
```

```output
Point{x = 1, y = 2} nil
Point{x = 3, y = 4} ["Ann", "Bo"] ["Bo"]
[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ... 30 more]
```

A log line keeps it short: a long array, slice or list shows its first 10 elements and how many more,
text inside a value its first 40 bytes, and a value nested more than 8 deep `...`. A line is cut at the
host's log size too (256 bytes on most hosts). `print` and `format` write all of a value, or none of
it when it doesn't fit.

How values become text, in `log`, `print` and `format` (unless format's settings say otherwise):

- `Int`: decimal, with `-` when negative.
- `Bool`: `true` or `false`. Enums: the value's name.
- `String` and `Blob`: their bytes as they are. Inside another value, quoted, with `"`, `\`, line
  breaks, tabs and zeros escaped as in Rio's strings.
- Structs: `Point{x = 1, y = 2}`. Arrays, slices and lists: `[1, 2, 3]` (of bytes: as text). Unions: the
  value they hold, or `nil`. Proc values: `proc`.
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
Spaces and line breaks can go between pairs of digits (a string over more lines gives it line breaks);
anything else, or an odd number of digits, is a compile error. It takes only a literal. The bytes are stored once, like a global, so each time the same
call runs it gives the same bytes:

```rio
b := hexBlob(
  "48 69
  "21"
)
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
| `b.print(...) -> Bool` | for byte lists: appends its arguments as text, as `log` would but without spaces; all or none |
| `b.format(settings, ...) -> Bool` | as `print`, as its settings say (see [format](#format)) |

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
title.print("hp ", 7, " ", 2.5)
log(xs.len(), xs[0], last, title[:])
```

```output
1 30 20 hp 7 2.5
```

```rio
xs: [..2]Int
log(xs.pop())  # runtime error: pop from empty list
```

## format

`b.format(settings, ...)` appends its values as `print` does, but as its first argument, a `Format`,
says. A struct literal there needs no type name:

```rio
Format :: struct    # (built in)
  width: Int        # numbers: at least this many bytes, padded with spaces (negative: on the right)
  digits: Int       # numbers: at least this many digits before the point, padded with zeros
  places: Int       # Floats: exactly this many digits after the point, rounded
  base: Int         # Ints: 2 to 16, of the bits
  textWidth: Int    # text, Bools and enums' names: as width does for numbers
end
```

Each field's 0 is as `print` writes it. The settings apply to every value after them, and to every
number and name inside a struct, array, slice, list or union (text inside a value stays quoted, and
isn't padded). A `Format` after the first argument is just a value. For different settings, use
another call:

```rio
b: Blob[..120]
b.format({width = 4}, 7, ":", 42, " ")
b.format({textWidth = -6}, "hp", true)
b.format({digits = 2, places = 1}, 3.14159, " ", -0.25, " ")
b.format({base = 16, digits = 4}, 48879, " ", 10, " ", -1)
log(b[:])
b.clear()
b.format({digits = 3}, [3]Int{1, 20, 300})
hex := Format{base = 16}
b.format(hex, hex)
log(b[:])
```

```output
   7:  42 hp    true  03.1 -00.3 beef 000a ffffffff
[001, 020, 300]Format{width = 0, digits = 0, places = 0, base = 10, textWidth = 0}
```

`base` writes an `Int`'s 32 bits, so a negative one has no `-` (`-1` is `ffffffff`). It's ignored for
`Float`s, as `places` is for `Int`s.

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
