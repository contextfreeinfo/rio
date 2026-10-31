# Expressions

## Precedence

From tightest to loosest. Operators on one line group left to right:

| operators | |
|---|---|
| `f(x)` `a[i]` `a[i..<j]` `a.b` `T{...}` | calls, indexing, slicing, members, literals |
| `-` `~` | negation, bitwise not |
| `*` `/` `%` `&` `<<` `>>` `>>>` | |
| `+` `-` `\|` `^` | |
| `==` `!=` `<` `<=` `>` `>=` `is` | comparisons don't chain |
| `not` | |
| `and` | |
| `or` `..<` `..=` | ranges: see [ranges](#ranges) |

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
use their low 5 bits, `>>` keeps the sign, and `>>>` shifts in zeros, as for the bits of an unsigned
number:

```rio
big := 2147483647
log(big + 1, 7 / 2, -7 / 2, -7 % 3, 7 % -3)
log(1 << 33, -16 >> 2, -16 >>> 28, 6 & 3, 6 | 3, 6 ^ 3, ~0)
```

```output
-2147483648 3 -3 -1 1
2 -4 15 2 7 5 -1
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
| `s.toInt()`, `s.toInt(base)` | text as an `Int`, a `?Int`: nil if it isn't one |
| `s.toFloat()` | text as a `Float`, a `?Float`: nil if it isn't one |

```rio
log(3.9.toInt(), (-3.9).toInt(), (1e20).toInt(), 5.toBool(), true.toInt())
```

```output
3 -3 2147483647 true 1
```

### Text as numbers

A `String`, a `Blob` or a byte builder's text becomes a number with `toInt` or `toFloat`. The result
is a maybe, nil when the text isn't a number, so bad input is never a wrong number:

```rio
n := "42".toInt()
if n is Int
  log(n + 1)
end
log("-17".toInt(), "4x".toInt(), "".toInt(), " 1".toInt())
```

```output
43
-17 nil nil nil
```

The text is a sign or none, then digits, and nothing else: no spaces, no `_`, and no `0x`. Letters
are digits past 9 in either case. `toInt` reads base 10 unless given a base from 2 to 36 (a base of
0 is 10, as leaving it out is; any other is an error). Base 10 must be in the `Int` range; any other
base is the bits, up to 32 of them, so `ffffffff` in base 16 is -1, as `format` writes it:

```rio
log("ff".toInt(16), "FF".toInt(16), "ffffffff".toInt(16), "1010".toInt(2), "z".toInt(36))
log("2147483647".toInt(), "-2147483648".toInt(), "2147483648".toInt(), "12".toInt(2))
```

```output
255 255 -1 10 35
2147483647 -2147483648 nil nil
```

`toFloat` reads what a `Float` literal is, in base 10, with a sign or none: digits, then a point and
more digits if any, then an exponent if any. It gives the same `Float` as that literal would:

```rio
log("1.5".toFloat(), "-2e-3".toFloat(), "7".toFloat(), "3.25E2".toFloat())
log("1.".toFloat(), ".5".toFloat(), "1e".toFloat(), "inf".toFloat())
assert("0.1".toFloat() is Float)
```

```output
1.5 -0.002 7.0 325.0
nil nil nil nil
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

`a[i]` is element `i` of an array, slice, builder, `String` or `Blob`, counting from 0. An index outside
`0 <= i < a.len()` is a runtime error (or a compile error, when it's a constant).

`a[i..<j]` is a view of elements `i` up to but not including `j`, and `a[i..=j]` up to and including `j`.
`a[i..]` runs to the end, `a[..<j]` and `a[..=j]` start at 0, and `a[]` is all of it. A [range](#ranges)
value works too: `a[r]` is `r.first` to `r.last`. It's a runtime error unless the slice is inside `a`
(`0 <= i <= j <= a.len()`, with `j` one past the last). Slicing an array or builder gives a `[]T`, a
`String` a `String`, and a `Blob` a `Blob`:

```rio
xs: [5]Int
for x, i in xs
  x = i * 10
end
mid := xs[1..<4]
s := "hello"
r := 1..=2
log(mid.len(), mid[0], s[1..<3], s[3..], s[..=1], s[r], xs[].len())
n := 2
log(xs[n - 1..<n + 1], xs[n..])
```

```output
3 10 el lo he el 5
[10, 20] [20, 30, 40]
```

```rio
xs: [3]Int
log(xs[2..<1].len())  # runtime error: slice out of bounds
```

Inside an index's brackets, `end` is the last index of what's indexed, `a.len() - 1`: `a[end]` is the
last element, and `a[end - 2..=end]` the last three. The brackets must be the innermost around it, so
in `ys[xs[end]]` it's `xs`'s, and it means nothing elsewhere:

```rio
xs := [5]Int{10, 20, 30, 40, 50}
s := "hello"
log(xs[end], xs[end - 2..=end], s[end], s[..<end])
```

```output
50 [30, 40, 50] 111 hell
```

An array indexed by an enum takes the enum's values as indexes, and as slice bounds:

```rio
Dir :: enum
  north, east, south, west
end
counts: [Dir]Int
counts[.east] += 2
log(counts[.east], counts[.east..].len(), counts.len())
```

```output
2 3 4
```

Its indexes are the enum's values, by name: not a number, and not `end` (which would move to a new
value added at the enum's end).

## Ranges

`a..<b` is the `Int`s from `a` up to but not including `b`, and `a..=b` up to and including `b`. Either
is a `Range`, a built-in struct of its first and last values (inclusive, so it reaches every `Int`), and
it binds looser than anything but `or`, so `0..<n + 1` is `0..<(n + 1)`:

```rio
Range :: struct    # (built in)
  first: Int
  last: Int        # empty when last < first
end
```

`r.len()` is how many values it has (0 when empty) and `r.has(i)` whether `i` is one of them. It's
written `first..=last` (an empty one made by `a..<a` is `a..=a - 1`):

```rio
r := 2..<5
log(r, r.first, r.last, r.len(), r.has(4), r.has(5), (3..<3).len())
```

```output
2..=4 2 4 3 true false 0
```

A range is a slice's bounds (see [slicing](#indexing-and-slicing)), and a loop's: `for i in r` counts
from `r.first` to `r.last`. `by` steps it: a positive step counts up from `first`, a negative one down
from `last`, and neither goes past the other end (or wraps, at the ends of `Int`). The step can be any
`Int` expression; a step of 0 is a compile error when it's a constant, else a runtime error when the
loop starts. `by` means this only after a `for`'s range: elsewhere it's a name like any other.

```rio
out: Blob[..80]
for i in 0..<10 by 3
  out.print(i, " ")
end
for i in 0..<5 by -1           # 4 down to 0
  out.print(i, " ")
end
r := 2..=8
step := -3
for i in r by step             # from r.last down
  out.print(i, " ")
end
log(out[])
```

```output
0 3 6 9 4 3 2 1 0 8 5 2 
```

```rio
step := 0
for i in 0..<3 by step  # runtime error: a loop's step is 0
end
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
values than the array holds is an error. `[..N]T{a, b, ...}` makes a builder holding those values. As
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

`[_]T{a, b, ...}` is sized by its values, and `[_]{a, b, ...}` takes its element type from the first
value too (a whole number then a fraction is an error: write `[_]Float`). `[]T{...}` and `[]{...}`
are slice literals: the array is kept where a variable would be, and the literal is a slice of it, so
a `{...}` can go where a slice is wanted. In a proc that's the proc's own memory, built again each
time the literal runs, so it can't be returned or kept past the call:

```rio
sum :: proc(xs: []Int) -> Int
  total := 0
  for x in xs do total += x
  return total
end
primes := [_]Int{2, 3, 5, 7}
names := [_]{"ann", "bo"}
odds := []{1, 3, 5}
odds[0] = 9
log(primes.len(), names[1], sum(odds), sum({1, 2}), sum([]{10, 20}))
```

```output
4 bo 17 3 30
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

## Arithmetic on arrays

Arrays of numbers do arithmetic element by element: `+ - * / %` and unary `-`. Both sides have the
same type, or one is a plain number, used for every element. Nesting works too. Structs don't do
arithmetic as wholes: for a vector, use an array, whose [swizzles](#members) (`.x`, `.y`) name its
elements:

```rio
Point :: [2]Float
p := Point{1, 2}
v := Point{10, 20}
p += v * 0.5
grid: [2][2]Int
grid += 3
log(p.x, p.y, grid[1][1], (p - 1.0).x)
```

```output
6.0 12.0 3 5.0
```

A result that isn't stored in place is limited to 16 numbers; update bigger arrays in place.

An array literal with no type written takes its partner's type, on the right of an operator whose left
side is an array, or after `+=` and the like: `p + {1, 0}`, `p *= {2, 1}`.

The math builtins (`abs`, `min`, `max`, `sqrt`, `floor`, `round`, `atan2`, `pow` and the rest), and
`toInt` and `toFloat`, work on arrays of numbers element by element too, with a plain number used for
every element, so clamping a point is one line. `toInt` and `toFloat` give an array of the same shape:

```rio
Point :: [2]Float
p: Point = {12.6, -3.2}
p = max(Point{0, 0}, min(p, 10.0))
log(p, abs(Point{-1.5, 2}), round(p + {0.5, 0}), p.toInt(), [3]Int{1, -2, 3}.toFloat())
```

```output
[10.0, 0.0] [1.5, 2.0] [11.0, 0.0] [10, 0] [1.0, -2.0, 3.0]
```

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
`&` parameter) means its `T` value (a read-only copy). The right side of an `and` is narrowed the
same way by its left side, since it runs only when that's true, so `if n is Int and n > 2` works.
Testing a narrowed `u` with `is` again is known while compiling. Assigning to `u` there replaces the
whole union, and from then on `u` is the union again:

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
n: ?Int = 4
log(n is Int and n > 2)
```

```output
6.0
true
true
```

A test that's false narrows too, when it leaves one type: for a union of two types (a maybe is one, of
`nil` and `T`), `u is A` being false means u holds the other. So `not u` (for a maybe), `u is nil` or
`not (u is A)` narrows u where it's known to be false: on the right side of an `or` it's the left side
of; in the branches after it (`case`, `else`), when it's one of an `if` condition's `or`-joined parts;
and after the `if`, to the end of the block holding it, when its branch and any before it leave (with
`return`, `break` or `continue`), as Swift's `guard let`:

```rio
first :: proc(xs: []String) -> Int
  for x in xs
    n := x.toInt()
    if not n do continue
    return n  # n is the Int: the if left when it was nil
  end
  return -1
end
twice :: proc(s: String) -> Int
  n := s.toInt()
  if n is nil
    return 0
  else
    return n * 2
  end
end
words := [3]String{"a", "12", "7"}
m := "5".toInt()
log(first(words[]), twice("21"), twice("x"), not m or m > 3)
```

```output
12 42 0 true
```
