# Data

## Structs

A struct groups named fields. A literal names the fields it sets; the rest are zero:

```rio
Vec :: struct
  x, y: Float
end
Ball :: struct
  pos, vel: Vec
  bounces: Int
end
b := Ball{pos = {x = 10}, vel = {x = 1, y = -2}}
b.pos.y += b.vel.y
log(b.pos.x, b.pos.y, b.bounces)
```

```output
10.0 -2.0 0
```

The literal's type can be left off where it's known, as in `pos = {x = 10}` above. Structs are values:
`c := b` is a copy, and changing `c` leaves `b` alone.

Structs made of numbers do math a field at a time, which suits vectors:

```rio
Vec :: struct
  x, y: Float
end
pos := Vec{x = 1, y = 1}
vel := Vec{x = 3, y = 4}
pos += vel * 0.5
log(pos.x, pos.y)
```

```output
2.5 3.0
```

## Arrays and slices

`[N]T` is N values in a row. A slice `[]T` is a view of some of them, and it's what procs usually take:

```rio
total :: proc(xs: []Int) -> Int
  sum := 0
  for x in xs
    sum += x
  end
  return sum
end
scores: [5]Int
for s, i in scores
  s = i * 10                # s is the element itself
end
log(total(scores), total(scores[1..<3]), scores.len())
```

```output
100 30 5
```

Every index is checked, so a bad one stops the program instead of reading the wrong memory.

An array literal gives the values in order, and the rest are zero. As with structs, the type can be
left off where it's known:

```rio
primes := [6]Int{2, 3, 5, 7}
corners: [4][2]Int = {{0, 0}, {1, 0}, {1, 1}}
log(primes[3], primes[5], corners[2][1], corners[3][0])
```

```output
7 0 1 0
```

## Changing the caller's things

Parameters are copies. To let a proc change something of yours, give it a `&` parameter, and pass
`&` the thing:

```rio
Ship :: struct
  hp: Int
end
hit :: proc(s: &Ship, damage: Int)
  s.hp -= damage
end
fleet: [3]Ship
for s in fleet
  s.hp = 10
end
hit(&fleet[1], 4)
log(fleet[0].hp, fleet[1].hp)
```

```output
10 6
```

## Strings and bytes

A `String` is read-only text, and a `Blob` is bytes you can change. Indexing either gives a byte as an
`Int`. `b[]` views a `Blob[N]` as a `Blob`:

```rio
s := "rio"
b: Blob[3]
for c, i in s
  b[i] = c - 32             # to uppercase
end
log(s[0], b[].asString(), s == "rio")
```

```output
114 RIO true
```

Next: [lists and text](lists.md).
