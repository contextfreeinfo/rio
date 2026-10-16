# Rio

Disclaimer: Claude Code produced almost every line of code so far, under my direction.

Rio is a small, statically typed scripting language for embedding, from desktop apps down to
microcontrollers. It's one C99 library, a compiler and a VM, that runs in a single block of memory you
hand it, and never allocates any more.

```rio
Ball :: struct
  x, y, vy: Float
end
balls: [..64]Ball

drop :: proc(x: Float)
  balls.push({x, y = 0})
end

update* :: proc(dt: Float)
  for b in balls              # b is the element itself
    b.vy += 10 * dt
    b.y += b.vy * dt
  end
end

drop(1)
drop(2)
for i in 0..<4
  update(0.5)
end
log("fell", balls[0].y, "with", balls.len(), "balls")
```

```output
fell 25.0 with 2 balls
```

- **Fast and small.** A register VM with typed instructions, about 2–4× faster than Lua. The VM alone
  is about 11 KB of ARM code; with the compiler, 68 KB.
- **Predictable.** No garbage collector, no allocation, and no recursion, so every proc has one fixed
  frame and a program's memory is known before it runs. Indexes are checked and integers wrap, so
  errors stop with a line number, never with undefined behavior.
- **Made for hosts.** The host provides drawing, sound or files as host procs, declared in Rio with
  `@host` and checked by the compiler. Compiled programs can be saved as images, or turned into C.
- **Friendly to read.** Types are inferred, blocks end with `end`, and structs, lists and text builders
  are plain values with no pointers to manage.

## Try it

```sh
cmake -S . -B build && cmake --build build --config Release
build/rio game.rio       # run a file (build/Release/rio.exe with MSVC)
build/rio                # or a REPL
```

Or get a build from the [nightly release](https://github.com/contextfreeinfo/rio/releases/tag/nightly).

## Learn more

- [Tutorial](docs/tutorial/README.md): Rio by example, a page at a time.
- [Spec](docs/spec/README.md): what Rio programs mean, mostly as examples, which the tests run.
- [Guide](docs/guide/README.md): the C library and CLI: embedding, memory, images, C output, ARM, and
  performance.

The docs build into a web site with `mdbook build docs`.

## License

CC0 1.0: dedicated to the public domain (see LICENSE). Use it for anything, with no need to credit; it
comes with no warranty.
