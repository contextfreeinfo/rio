# Modules

Each file is a module. Names ending in `*` are exported; the rest stay private. Import a file next to
yours with a leading `.`:

```rio file=shapes.rio
Rect* :: struct
  w, h: Int
end
Rect.area* :: proc() -> Int
  return self.w * self.h
end
```

```rio
import .shapes
r := shapes.Rect{w = 3, h = 4}
log(r.area())
```

```output
12
```

Without the `.`, an import names a package from your library directories: `rio -L libs game.rio`
finds `import tween` as `libs/tween.rio` or `libs/tween/tween.rio`. Bring in single names with
`import tween.{lerp}`, and rename with `as`.

`include "part.rio"` pulls another file into this module, private names and all, to split a big
module up.

Next: [embedding Rio in C](embedding.md).
