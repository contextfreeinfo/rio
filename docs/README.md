# Rio

Rio is a small, statically typed scripting language for embedding, from desktop apps down to
microcontrollers: one C99 library that runs in a single block of memory, with no allocation, no
garbage collector and no recursion.

```rio
Vec :: struct
  x, y: Float
end
pos := Vec{x = 1, y = 2}
vel := Vec{x = 3, y = 4}
pos += vel * 0.5
log("at", pos.x, pos.y)
```

```output
at 2.5 4.0
```

- The [tutorial](tutorial/README.md) shows Rio by example.
- The [spec](spec/README.md) says what Rio programs mean, mostly with examples, which the tests run.
- The [guide](guide/README.md) covers the C library and the `rio` command: embedding, memory, images,
  C output and native code on ARM.

Source: [github.com/contextfreeinfo/rio](https://github.com/contextfreeinfo/rio).
