# Declarations

`::` declares something that doesn't change: a constant, a type, a proc, or another name for one.
`:=` and `:` declare variables.

## Before use

Every name must be declared before it's used, in the order the source is read. So a proc can't call
itself, or a proc declared after it, and Rio has no recursion:

```rio
f :: proc()
  g()  # error: undefined name
end
g :: proc()
end
```

## Constants

`NAME :: value` names a value known when compiling: a number, `Bool` or `String`, or operators and
conversions on those. A constant number takes the type its place needs, as a literal does:

```rio
N :: 16
HALF :: N / 2
SCALE :: 1.5
NAME :: "rio"
x: Float = N             # 16.0: N is a number, not just an Int
log(HALF, x, NAME)
```

```output
8 16.0 rio
```

Constants can be declared in procs too. Top-level constants can be set from outside: see
[defines](host.md#defines).

## Variables

```rio
count := 0              # the type from the value: Int
speed: Float = 2        # the type written, and a value
grid: [4][4]Int         # the type written: starts as zero
log(count, speed, grid[3][3])
```

```output
0 2.0 0
```

A variable at the top level is a global, living as long as the program. One in a proc is a local; it
starts again each time its declaration runs. A declaration is visible from there to the end of its
block. Declaring a name again, in the same block or an inner one, makes a new variable that hides the
old one from then on:

```rio
x := 1
x := "one"               # a new x
f :: proc()
  x := 2.5               # a local x, in f only
  log(x)
end
f()
log(x)
```

```output
2.5
one
```

## Copies

Assigning, passing and returning copy values. To keep copies cheap and visible, a copy that isn't
written out is limited to 16 words: a bigger `x := big` is an error. Write the type to copy on
purpose, or take a view (`big[]`) or a reference (`&big`). Parameters and results are limited to 16
words; pass bigger values as `&T` or `[]T`, and fill in a `&T` parameter instead of returning one.
Plain assignment, `b = a`, copies any size.

```rio
Big :: struct
  a: [20]Int
end
g: Big
f :: proc()
  copy: Big = g          # fine: the type says it's a copy
  x := g  # error: this copies a big struct
end
```

## Procs

```rio
add :: proc(a, b: Int) -> Int
  return a + b
end
greet :: proc(name: String)
  log("hi", name)
end
greet("ann")
log(add(2, 3))
```

```output
hi ann
5
```

A parameter marked with `?`, as in `scale?: Int`, is optional: a call may leave it out, and then it's
zero, as a variable is (`nil` for a maybe, empty for a `String`, a struct's fields all zero). Only the
last parameters can be optional, so arguments still go by position, and forgetting any other one is
still an error. A struct last makes a proc's options easy to leave out or give a few of:

```rio
Style :: struct
  scale: Int
  shadow: Bool
end
label :: proc(text: String, x: Int, style?: Style)
  log(text, x, style.scale, style.shadow)
end
label("hi", 4)
label("hi", 4, {scale = 2})
```

```output
hi 4 0 false
hi 4 2 false
```

```rio
f :: proc(a?: Int, b: Int)  # error: after an optional parameter (name?: T), the rest are optional too
end
```

A `&` parameter can't be optional, since it needs a place. A proc type says which parameters are
optional the same way, `proc(a: Int, b?: Int)`, and it's part of the type: a proc goes where a proc
value does only if the two agree.

Procs are declared at the top level. Parameters are values the proc can change without affecting the
caller, except `&T` parameters (see [references](expressions.md#references)). A proc with a result must
end with `return` on every path:

```rio
sign :: proc(n: Int) -> Int
  if n < 0
    return -1
  end
end  # error: missing return
```

A proc can't return a view of its own locals, or store one where it outlives the call (a global, or
through a `&` parameter): the next call reuses that memory.

```rio
f :: proc() -> []Int
  xs: [3]Int
  return xs[]  # error: this returns a view of the proc's own memory
end
```

The same goes for a struct, array or union holding views, such as a struct with a `String` field or a
`?String`: it can be returned if what it holds views memory other than the proc's own.

```rio
Entry :: struct
  key: String
  n: Int
end
entry :: proc(line: String) -> Entry
  for c, i in line
    if c == '=' do return Entry{key = line[0..<i], n = line[i + 1..].toInt() or 0}
  end
  return Entry{key = line}
end
firstWord :: proc(s: String) -> ?String
  for c, i in s
    if c == ' ' do return s[0..<i]
  end
  return nil
end
e := entry("hp=12")
log(e.key, e.n, firstWord("hello there") or "-", firstWord("one") or "-")
```

```output
hp 12 hello -
```

A view that comes in as a parameter may be one of the caller's own, so it's only borrowed: it can't
be stored where it outlives the call either. To keep what it views, copy the bytes:

```rio
name := ""
rename :: proc(s: String)
  name = s  # error: this view came in as a parameter
end
```

```rio
name: Blob[..32]
rename :: proc(s: String)
  name.clear()
  ok := name.pushAll(s)
end
f :: proc()
  typed: Blob[8]
  typed[0] = 'A'
  rename(typed[0..<1].asString())
end
f()
log(name[].asString())
```

```output
A
```

What's read through a `&` parameter or `self` is the caller's too, so a view read there is only
borrowed in the same way. A call's result, when it's a view or holds one, may view what went in, so it
counts as a view of whatever those were. That includes the places of `&` arguments and `self`, when
there's an array or builder in them for a view to view:

```rio
Box :: struct
  xs: [4]Int
end
front :: proc(b: &Box) -> []Int
  return b.xs[0..<2]
end
kept: []Int
f :: proc()
  b: Box
  kept = front(&b)  # error: this view of the proc's own memory would outlive the call
end
```

Which locals hold such views is worked out in the order the code is written. A store in a local's own
block (the one it's declared in) replaces what it held. A store in an `if` or a loop inside that block
only adds to it, since it may not run. And in a loop, the code before a store runs after it too, so a
local from before the loop can't start holding such a view there:

```rio
kept: []Int
g: [4]Int
f :: proc(c: Bool)
  xs: [4]Int
  v := xs[]
  v = g[]  # its own block: v views only g now
  kept = v
  w := xs[]
  if c
    w = g[]
  end
  kept = w  # error: this view of the proc's own memory would outlive the call
end
```

```rio
kept: []Int
g: [4]Int
f :: proc()
  xs: [4]Int
  v := g[]
  for i in 0..<2
    kept = v
    v = xs[]  # error: in a loop, into a variable from before it that held none
  end
end
```

And a call's result, when it's a view or holds one, may view what went in, so it's a view of the
caller's own memory if any argument was. Each rule is about one proc's own code, so changing a proc's body never
changes what its callers may do.

## Methods

`Type.name :: proc` declares a method on a type. Inside, `self` is the value it was called on: for
structs, the caller's own value; for `Int`, `Float`, `Bool`, `String` and `Blob`, a copy.

```rio
Ship :: struct
  x, vx: Float
  hp: Int
end
Ship.move :: proc(dt: Float)
  self.x += self.vx * dt
end
Ship.hurt :: proc(n: Int) -> Bool
  self.hp -= n
  return self.hp <= 0
end
Int.double :: proc() -> Int
  return self * 2
end
s := Ship{vx = 2, hp = 3}
s.move(0.5)
log(s.x, s.hurt(5), 21.double())
```

```output
1.0 true 42
```

Each type has its own method names, apart from plain procs. A method can't have the same name as a
field of its type. Fields are always `self.x`; a plain `x` is never a field.

`Type.name :: static proc` declares a static proc: one called on the type, `Type.name(...)`, with no
`self`, as a constructor is (`static` is a word only there). It's one of the type's method names, and
travels with the type as its methods do, but a value can't call it, and the type can't call a method:

```rio
Rect :: struct
  pos, size: [2]Float
end
Rect.of :: static proc(x, y, w, h: Float) -> Rect
  return Rect{pos = {x, y}, size = {w, h}}
end
Rect.area :: proc() -> Float
  return self.size.x * self.size.y
end
r := Rect.of(1, 2, 3, 4)
make := Rect.of  # a proc value, as any proc's name is
log(r.area(), make(0, 0, 2, 2).area())
```

```output
12.0 4.0
```

## Structs, enums and unions

```rio fragment
Vec :: struct
  x, y: Float            # several fields of one type
end
Dir :: enum
  north, east, south, west
end
Shape :: union
  Circle, Box
end
```

Field types, and the types in a union, must already be declared, so no type contains itself. See
[types](types.md) for what they mean. Types are declared at the top level.

## Other names for things

`::` with an existing type, proc or module makes another name for it:

```rio
add :: proc(a, b: Int) -> Int
  return a + b
end
plus :: add
Point :: [2]Float
p: Point
log(plus(1, 2), p.len())
```

```output
3 2
```

A proc type takes parentheses, since `Less :: proc(a, b: Int) -> Bool` alone would start a proc of
that name:

```rio
Less :: (proc(a, b: Int) -> Bool)
lt :: proc(a, b: Int) -> Bool
  return a < b
end
sorted :: proc(xs: []Int, less: Less) -> Bool
  for i in 1..<xs.len()
    if less(xs[i], xs[i - 1]) do return false
  end
  return true
end
xs := [3]Int{1, 2, 3}
log(sorted(xs[], lt))
```

```output
true
```

## Exports

A `*` after a top-level name exports it: other modules can use it, and the host can find it (see
[modules](modules.md) and [programs](host.md)):

```rio fragment
Vec* :: struct
  x, y: Float
end
score* := 0
update* :: proc()
end
```

## Generic procs

`proc[T, U](...)` names types a proc works on whatever they are. Each is found at every call, from
the arguments, so each must be in a parameter's type. The proc is compiled once, for every T: a call
passes, unseen, what T is, and so its size. In the body a T's size is known only while running, so a T
is only ever reached through a reference or a slice, never held:

- **Where a T goes:** `x: &T` and `xs: []T`, as parameters, in a proc type's parameters
  (`less: proc(a, b: &T) -> Bool`), and in a variable as `[]T`.
- **What a T does:** it's copied from place to place (`xs[i] = x`, `a = b`), swapped
  (`xs.swap(i, j)`), and passed on, to a generic proc or a proc value as `&xs[i]`. Anything else is an
  error: comparing (pass in a proc that compares), arithmetic, printing, keeping one in a variable, a
  `for` over a `[]T` (index it), slicing a `[]T`. A generic proc can't return a T, or be a proc value.
- **What a call checks:** each T found agrees everywhere it's found. And a proc that copies its T into
  memory it was given can't take a T holding views (a `String`, a struct with one): it could keep a
  view past what it views, as any proc could by storing a borrowed view (`xs.swap` is fine, since both
  places are in `xs`).

```rio
sortBy :: proc[T](xs: []T, less: proc(a, b: &T) -> Bool)
  for i in 1..<xs.len()
    j := i
    for j > 0 and less(&xs[j], &xs[j - 1])
      xs.swap(j, j - 1)
      j -= 1
    end
  end
end
find :: proc[T](xs: []T, x: &T, eq: proc(a, b: &T) -> Bool) -> Int
  for i in 0..<xs.len()
    if eq(&xs[i], &x) do return i
  end
  return -1
end
Player :: struct
  name: String
  score: Int
end
byScore :: proc(a, b: &Player) -> Bool do return a.score < b.score
intEq :: proc(a, b: &Int) -> Bool do return a == b
players := [_]{Player{name = "ann", score = 30}, Player{name = "bo", score = 10}}
sortBy(players[], byScore)
nums := [_]{4, 8, 15}
eight := 8
nine := 9
log(players[0].name, find(nums[], &eight, intEq), find(nums[], &nine, intEq))
```

```output
bo 1 -1
```

A proc type can take types too, named and then filled in where it's used: `Less :: (proc[T](a, b:
&T) -> Bool)`, then `Less[Point]`, or `Less[T]` in a generic proc. It's the same type as one written
out, so `sortBy :: proc[T](xs: []T, less: Less[T])` reads as the one above.

A struct can take types the same way, `Table :: struct[K, V]`, filled in where it's named:
`Table[Int, Point]`, or `Table[K, V]` in a generic proc, which finds K and V through it. A T goes in a
field only in a `[]T` or a proc type, so every field's size is the same whatever T is: each filled-in
struct is its own type, laid out as the others are, and no code is made per type. Each type parameter
must be in some field's type. A generic struct can't have methods yet; generic procs take it instead.

```rio
Table :: struct[K, V]
  keys: []K
  values: []V
  eq: proc(a, b: &K) -> Bool
end
find :: proc[K, V](t: &Table[K, V], key: &K) -> Int
  for i in 0..<t.keys.len()
    if t.eq(&t.keys[i], &key) do return i
  end
  return -1
end
intEq :: proc(a, b: &Int) -> Bool do return a == b
ks := [_]{7, 3}
names := [_]{"seven", "three"}
t := Table[Int, String]{keys = ks[], values = names[], eq = intEq}
three := 3
log(t.values[find(&t, &three)], t)
```

```output
three {keys = [7, 3], values = ["seven", "three"], eq = proc}
```
## Host procs

`host proc` and `host struct` declare something the host provides: see
[host procs](host.md#declaring-host-procs). Elsewhere, `host` is an ordinary name.
