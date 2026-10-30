# Rio

Rio is a small, statically typed scripting language for embedding, from desktop apps down to
microcontrollers: one C99 library that runs in a single block of memory, with no allocation, no
garbage collector and no recursion.

```rio
Point :: [2]Float
Ball :: struct
  pos, vel: Point
end
b := Ball{pos = {1, 2}, vel = {3, 4}}
b.pos += b.vel * 0.5
log("at", b.pos.x, b.pos.y)
```

```output
at 2.5 4.0
```

- The [tutorial](tutorial/start.md) shows Rio by example.
- The [spec](spec/language.md) says what Rio programs mean, mostly with examples, which the tests run.
- The [guide](guide/building.md) covers the C library and the `rio` command: embedding, memory, images,
  C output and native code on ARM.

Source: [github.com/contextfreeinfo/rio](https://github.com/contextfreeinfo/rio).
