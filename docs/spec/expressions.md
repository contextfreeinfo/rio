# Expressions

## Precedence

From tightest to loosest. Operators on one line group left to right:

| operators | |
|---|---|
| `f(x)` `a[i]` `a[i:j]` `a.b` `T{...}` | calls, indexing, slicing, members, literals |
| `-` `~` | negation, bitwise not |
| `*` `/` `%` `&` `<<` `>>` | |
| `+` `-` `\|` `^` | |
| `==` `!=` `<` `<=` `>` `>=` `is` | comparisons don't chain |
| `not` | |
| `and` | |
| `or` | |

```rio
log(2 + 3 * 4, 1 << 2 + 1, 7 & 3 + 1, -2 * 3)
log(not 1 == 2, true or false and false)
```

```output
14 5 4 -6
true true
```

`not a == b` is `not (a == b)`, as in Python.

## Int arithmetic

`Int` math wraps around at 32 bits. `/` rounds toward zero, and `%` takes the sign of the left side.
Dividing by zero is a runtime error; `-2147483648 / -1` wraps to itself, and `% -1` is 0. Shift counts
use their low 5 bits, and `>>` keeps the sign:

```rio
big := 2147483647
log(big + 1, 7 / 2, -7 / 2, -7 % 3, 7 % -3)
log(1 << 33, -16 >> 2, 6 & 3, 6 | 3, 6 ^ 3, ~0)
```

```output
-2147483648 3 -3 -1 1
2 -4 2 7 5 -1
```

```rio
n := 0
log(10 / n)  # runtime error: division by zero
```

## Float arithmetic

`Float` math is IEEE single precision: dividing by zero gives an infinity, and `0.0 / 0.0` is NaN,
which isn't equal to anything, itself included. `%`, `&` and the other bit operators are for `Int`s
only (`fmod` is the `Float` remainder).

```rio
zero := 0.0
log(1.0 / zero, 0.0 / zero, 0.1 + 0.2, 7.0 / 2)
nan := zero / zero
log(nan == nan)
```

```output
inf nan 0.3 3.5
false
```

The two sides of an operator have the same type. A literal or constant number becomes a `Float`
where one is needed; otherwise convert with `.toFloat()` and `.toInt()`:

```rio
x: Float = 1.5
log(x * 2, x + 1, 2.toFloat() / 4)
n := 3
log(x * n)  # error: type mismatch
```

## Comparisons and logic

`==` and `!=` compare numbers, `Bool`s, enums and `String`s (the same bytes). `<`, `<=`, `>` and `>=`
compare numbers. Structs and arrays don't compare. `and`, `or` and `not` take `Bool`s, and `and` and
`or` skip their right side when the left decides:

```rio
calls := 0
yes :: proc() -> Bool
  calls += 1
  return true
end
a := false and yes()
b := true or yes()
log(calls, "ab" == "ab", "a" != "b")
```

```output
0 true true
```

```rio
Dir :: enum
  north, south
end
log(Dir.north < Dir.south)  # error: enums compare only with == and !=
```

## Conversions

Conversions are methods. `to` makes a new value; `as` views the same bytes:

| | |
|---|---|
| `n.toFloat()` | `Int` to `Float` |
| `f.toInt()` | toward zero; past the `Int` range it gives the nearest end, and NaN gives 0 |
| `n.toBool()` | `n != 0` |
| `b.toInt()` | 1 or 0 |
| `e.toInt()`, `E.fromInt(n)` | an enum's value; `fromInt` is a runtime error if `n` isn't one |
| `blob.asString()` | the same bytes, read-only |

```rio
log(3.9.toInt(), (-3.9).toInt(), (1e20).toInt(), 5.toBool(), true.toInt())
```

```output
3 -3 2147483647 true 1
```

## Calls

Arguments are evaluated left to right, then the call runs. They must have the parameters' types,
except that number literals adapt. A `&T` parameter takes `&place`. See [procs](declarations.md#procs)
and [builtins](builtins.md).

A comma can end the arguments, as it can any list in brackets, so a call over more lines can put one
argument on each line:

```rio
area :: proc(w: Int, h: Int) -> Int
  return w * h
end
log(
  area(3, 4),
  area(
    5,
    6,
  ),
)
```

```output
12 30
```

## Indexing and slicing

`a[i]` is element `i` of an array, slice, list, `String` or `Blob`, counting from 0. An index outside
`0 <= i < a.len()` is a runtime error (or a compile error, when it's a constant).

`a[i:j]` is a view of elements `i` up to but not including `j`. Leaving out `i` means 0, and `j` means
the length. It's a runtime error unless `0 <= i <= j <= a.len()`. Slicing an array or list gives a
`[]T`, a `String` a `String`, and a `Blob` a `Blob`:

```rio
xs: [5]Int
for x, i in xs
  x = i * 10
end
mid := xs[1:4]
s := "hello"
log(mid.len(), mid[0], s[1:3], s[3:], xs[:].len())
```

```output
3 10 el lo 5
```

```rio
xs: [3]Int
log(xs[2:1].len())  # runtime error: slice out of bounds
```

An array indexed by an enum takes the enum's values as indexes, and as slice bounds:

```rio
Dir :: enum
  north, east, south, west
end
counts: [Dir]Int
counts[.east] += 2
log(counts[.east], counts[.east:].len(), counts.len())
```

```output
2 3 4
```

## Members

`a.f` is field `f` of a struct (or of a struct reached through a reference or a slice element), and
`a.m(...)` calls method `m`. The parentheses make it a call; a method isn't a value.

Arrays of 1 to 4 numbers or `Bool`s have **swizzles**, the letters `x y z w` or `r g b a`. One letter
is that element, and can be assigned. Two to four letters make a new array:

```rio
v: [3]Float
v.x = 1
v.y = 2
w := v.yx
c: [4]Int
c.a = 255
log(w.x, v.zyx.z, c.ar.x)
```

```output
2.0 1.0 255
```

## Struct literals

`T{field = value, ...}` makes a struct. Fields can come in any order, and the ones left out are zero.
Where the type is already decided (a typed declaration, assignment, argument, return, field or
`push`), `T` can be left off. A name or a dotted path alone fills the field of the same name as its
last part:

```rio
Vec :: struct
  x, y: Float
end
Ship :: struct
  pos: Vec
  hp: Int
end
x := 3.0
a := Vec{y = 2, x = 1}
b: Vec = {x}                  # {x = x}
s := Ship{pos = {a.y}, hp = 5}  # {y = a.y}
s.pos = {y = 9}
log(a.x, b.x, b.y, s.pos.y, s.hp)
```

```output
1.0 3.0 0.0 9.0 5
```

`v := {x = 1}` is an error: nothing says which struct it is.

## Array literals

`[N]T{a, b, ...}` makes an array: its values in order, and zero for the elements after them. More
values than the array holds is an error. `[..N]T{a, b, ...}` makes a list holding those values. As
with structs, where the type is already decided the type can be left off, so `{...}` is a struct or
an array by where it's going: a struct's fields are always named, and an array's values never are.
Literals nest:

```rio
Vec :: struct
  x, y: Float
end
Dir :: enum
  north, east, south, west
end
xs := [5]Int{1, 2, 3}
grid := [2][2]Int{{1, 2}, {3, 4}}
pts: [3]Vec = {{x = 1}, {y = 2}}
steps := [Dir]Int{-16, 1, 16, -1}  # in the enum's order
todo := [..8]Int{4, 5}
todo.push(6)
log(xs[2], xs[4], grid[1][0], pts[1].y, steps[.south], todo.len())
```

```output
3 0 3 2.0 16 3
```

A literal is built each time it runs, so a local one starts over on every call, and changing it
changes only that copy:

```rio
next :: proc() -> Int
  counts := [2]Int{10}
  counts[0] += 1
  return counts[0]
end
log(next(), next())
```

```output
11 11
```

```rio
a: [2]Int = {1, 2, 3}  # error: more values than the array holds
```

An array indexed by an enum can name its elements instead, as `.value = x`, in any order; the rest are
zero. Its values are all named or none are, each once, and a name can't stand for its value as a
struct's can (`{.north}` is an array holding `.north`):

```rio
Dir :: enum
  north, east, south, west
end
Vec :: struct
  x, y: Int
end
steps := [Dir]Int{.south = 16, .west = -1, .north = -16}
moves: [Dir]Vec = {.east = {x = 1}, .north = {y = -1}}
log(steps[.north], steps[.east], moves[.east].x, moves[.west].x)
```

```output
-16 0 1 0
```

```rio
Dir :: enum
  north, east
end
steps := [Dir]Int{.north = 1, 2}  # error: name each value
```

## Arithmetic on arrays and structs

Arrays of numbers, and structs whose fields are all `Int`s or all `Float`s, do arithmetic element by
element: `+ - * / %` and unary `-`. Both sides have the same type, or one is a plain number, used for
every element. Nesting works too:

```rio
Vec :: struct
  x, y: Float
end
p := Vec{x = 1, y = 2}
v := Vec{x = 10, y = 20}
p += v * 0.5
grid: [2][2]Int
grid += 3
log(p.x, p.y, grid[1][1], (p - 1.0).x)
```

```output
6.0 12.0 3 5.0
```

A result that isn't stored in place is limited to 16 numbers; update bigger arrays in place.

## References

`&place` is a reference to a place: a variable, field or element that can be changed. It's passed to
a `&T` parameter, or bound to a local name with `r := &place`. Using the reference uses the place
itself, and the place is checked once, when the reference is made:

```rio
Ship :: struct
  hp: Int
end
hurt :: proc(s: &Ship, n: Int)
  s.hp -= n
end
fleet: [3]Ship
hurt(&fleet[1], 2)
r := &fleet[2]
r.hp = 7
r = {hp = r.hp + 1}       # writes fleet[2]
copy := r                 # a plain := copies the value
copy.hp = 0
log(fleet[1].hp, fleet[2].hp)
```

```output
-2 8
```

References can't be stored, returned, put in structs or arrays, or pointed elsewhere, so they never
outlive what they refer to. A `String`'s bytes, constants and call results aren't places.

## Proc values

A proc's name is a value where a proc type is expected, and a proc value is called like a proc. Their
parameter and result types must match exactly:

```rio
twice :: proc(n: Int) -> Int
  return n * 2
end
apply :: proc(f: proc(Int) -> Int, n: Int) -> Int
  return f(f(n))
end
g := twice
log(apply(twice, 3), g(5))
```

```output
12 10
```

A proc value can't capture variables. To keep recursion impossible, a proc that calls through a proc
value (or calls one that does) can't itself be used as a value, and procs with `&` parameters can't
either. Calling a proc value that was never set is a runtime error:

```rio
f: proc(Int) -> Int
log(f(1))  # runtime error: this proc value isn't set
```

## Enum values

`E.name` is a value of enum `E`. Where the enum type is already known (a typed declaration,
assignment, argument, return, field, comparison or `case`), `.name` is enough:

```rio
Dir :: enum
  north, east, south, west
end
d := Dir.north
d = .west
log(d, d == .west)
```

```output
west true
```

## Unions and `is`

A value of one of a union's types can be used as the union. `u is T` tests which type it holds (any
type can be written there, `u is [3]Int` too, as in a `case`). To
reach the value inside, narrow it: in the body of an `if` whose condition has `u is T` as one of
its `and`-joined parts, or in a `case` naming one type, a union variable `u` (or a loop's element, or a
`&` parameter) means its `T` value (a read-only copy). Assigning to `u` there replaces the whole union, and from then on `u` is the union again:

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
s: Shape = Box{w = 2, h = 3}
if s is Box
  log(s.w * s.h)
  s = Circle{r = s.w}
end
log(s is Circle)
```

```output
6.0
true
```
