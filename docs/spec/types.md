# Types

Every expression has a type, known when compiling. There are no implicit conversions, except that
a number literal (or a constant made of them) takes the type its place needs: an `Int` literal can
be a `Float`.

## Basic types

| type | values | size |
|---|---|---|
| `Int` | 32-bit signed integers; arithmetic wraps | 1 word |
| `Float` | 32-bit IEEE floats | 1 word |
| `Bool` | `true` and `false` | 1 word |
| `String` | a read-only view of bytes | 2 words |
| `Blob` | a writable view of bytes | 2 words |

A byte read from a `String` or `Blob` is an `Int` from 0 to 255. Storing an `Int` as a byte keeps its
low 8 bits:

```rio
b: Blob[2]
b[0] = 300
b[1] = -1
log(b[0], b[1], "hi"[1])
```

```output
44 255 105
```

## Arrays, slices, lists

| | owns its memory | a view of it (what procs take) |
|---|---|---|
| fixed length | `[N]T` | `[]T` |
| grows up to a capacity | `[..N]T` (a list) | `[..]T` (a builder) |
| bytes | `Blob[N]`, `Blob[..N]` | `Blob`, `Blob[..]` |

`N` is a positive `Int` constant: a number, a constant's name, or arithmetic on those (`[W * H]T`,
`[geo.SIZE + 1]T`), worked out while compiling. An array indexed by an enum, `[E]T`, has one element per value of
`E` (see [enums](#enums)). Arrays nest: `[4][4]Float` is 4 rows of 4.

- An **array** `[N]T` is N elements, one after another. It's a value: assigning it copies it.
- A **slice** `[]T` views some elements of an array (2 words: where they start, how many).
- A **list** `[..N]T` holds up to N elements, and knows how many it has (a length word, then N
  elements). New lists are empty, unless made from a literal.
- A **builder** `[..]T` views a list, or any array, as a list (3 words: where the length lives,
  where the elements start, the capacity). Pushing through a builder changes the length it views.
- `Blob[N]` is N bytes, padded to whole words; `Blob[..N]` is a list of bytes, a text builder.

```rio
xs: [4]Int               # 4 Ints, all 0
part := xs[1:3]          # a []Int of 2, viewing xs
part[0] = 7
names: [..8]String       # up to 8; empty for now
names.push("ann")
log(xs[1], part.len(), names.len(), names.cap())
```

```output
7 2 1 8
```

## Structs

A struct is fields, in the order declared, one after another:

```rio
Vec :: struct
  x, y: Float
end
Ship :: struct
  pos, vel: Vec
  hp: Int
  tag: Blob[4]
end
s: Ship
log(s.pos.x, s.hp, s.tag.len())
```

```output
0.0 0 4
```

## Enums

An enum is a named set of `Int` values, its own type. Values count up from 0 unless given, each one
more than the last. One value must be 0, since that's what new variables start as; listing `nil`
first makes the zero mean "nothing":

```rio
Dir :: enum
  north, east, south, west
end
Tile :: enum
  nil
  wall = 3
  water          # 4
end
d: Dir
t: Tile
log(d, t, Tile.water.toInt())
```

```output
north nil 4
```

Values increase down the list. `[E]T` needs `E`'s values to be exactly 0, 1, 2 and so on.

## Unions

A union holds a value of one of its types, and remembers which (a tag word, then room for the biggest
type). Its zero is the first type's zero, so listing `nil` first makes an empty union:

```rio
Circle :: struct
  r: Float
end
Box :: struct
  w, h: Float
end
Shape :: union
  nil
  Circle, Box
end
s: Shape
log(s is nil)
s = Box{w = 2, h = 3}
log(s is Box)
```

```output
true
true
```

Types in a union that differ in size by more than 16 words are an error, since every value takes the
biggest size. `union @diverse` allows it on purpose.

## Maybes

`?T` is a maybe: nil, or a `T`. It's a union of `nil` and `T` (a tag word, then the `T`), so it starts
as nil and works as unions do: a `T` or `nil` can be stored in it, `is` tests it and narrows it, and a
`switch` takes `case nil` and `case T`. It can be written anywhere a type can, and every `?T` is the
same type, wherever it's written:

```rio
Point :: struct
  x, y: Int
end
Enemy :: struct
  hp: Int
  target: ?Point   # (nothing to chase, to start)
end
find :: proc(xs: []Int, want: Int) -> ?Int
  for x, i in xs
    if x == want
      return i
    end
  end
  return nil
end
nums := [3]Int{5, 7, 9}
at := find(nums[:], 9)
if at is Int
  log("at", at)  # here, at is the Int
end
log(find(nums[:], 4))
e: Enemy
log(e)
e.target = Point{x = 3, y = 4}
log(e.target)
grid: [3]?Int
grid[1] = 5
log(grid)
```

```output
at 2
nil
Enemy{hp = 0, target = nil}
Point{x = 3, y = 4}
[nil, 5, nil]
```

Prefixes nest, innermost last: `?[]?Int` is a maybe slice of maybe `Int`s. A `?T` can't be used as a
`T` until it's checked:

```rio
x: ?Int = 2
log(x + 1)  # error: this may be nil
```

A union that lists `nil` already, or a maybe, takes no `?`:

```rio
y: ??Int  # error: this can already be nil
```

A `?T` that holds views (`?String`, `?[]Int`) must be a global, as other unions holding them must
(see [where views can live](#where-views-can-live)).

## Procs

A proc type is written `proc(T, U) -> R`, or without `-> R` when there's no result. Parameter names
are allowed, for reading: `proc(x, y: Int) -> Int`. See [proc values](expressions.md#proc-values).

## References

`&T` is a reference: a name for a place that belongs to someone else. References exist only as
parameters and local names, and can't be stored, returned or put in a struct. See
[references](expressions.md#references).

## Zero values

Every variable starts as zero: `0`, `0.0`, `false`, empty strings, slices and lists, an enum's 0
value, a union's first type, and structs and arrays of zeros.

## Small and big values

Values up to 16 words (64 bytes) are small: vectors, colors, 4x4 matrices, most structs. Bigger ones
can't be copied without saying so, passed by value or returned. See
[copies](declarations.md#copies).

## Where views can live

A struct, array, list or union that holds views (slices, `String`, `Blob`, builders) must be a global,
so the view can't outlive what it views:

```rio
Named :: struct
  name: String
end
people: [3]Named           # fine: global
f :: proc()
  n: Named  # error: holding slices or strings must be global
end
```

Views themselves can be locals and parameters.

## Type identity

Structs, enums and unions are the same type only if they're the same declaration. Other types are
the same when they're written the same: `[4]Int` is `[4]Int` everywhere.
