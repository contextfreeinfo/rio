# Lists and text

Rio never allocates memory while running, so lists have a fixed capacity, and a length that changes.
`[..N]T` holds up to N:

```rio
Enemy :: struct
  x, hp: Int
end
enemies: [..64]Enemy
for i in 0..<3
  enemies.push({x = i * 10, hp = 3})
end
for e in enemies
  e.hp -= 1
end
enemies.swapRemove(0)       # quick: the last one takes its place
log(enemies.len(), enemies[0].x, enemies.cap())
```

```output
2 20 64
```

`push` gives `false` when the list is full, and changes nothing. A proc that adds to a list takes a
`[..]T`, a view that pushes into the caller's list:

```rio
spawn :: proc(list: [..]Int, n: Int)
  for i in 0..<n
    list.push(i)
  end
end
xs: [..8]Int
spawn(xs, 3)
spawn(xs, 2)
log(xs.len(), xs[4])
```

```output
5 1
```

## Building text

A list of bytes, `Blob[..N]`, builds text. `print` appends values as `log` would show them, but
without spaces:

```rio
line: Blob[..32]
hp := 7
line.print("hp: ", hp, "/", 10)
line.push('!')
log(line[])
```

```output
hp: 7/10!
```

If it doesn't all fit, `print` changes nothing and gives `false`. `format` does the same, with
settings for widths, digits and bases first (see [format](../spec/builtins.md#format)):

```rio
row: Blob[..32]
row.format({width = 3}, 7, 42, 100)
log(row[])
```

```output
  7 42100
```

Next: [methods, enums, unions and maybes](types.md).
