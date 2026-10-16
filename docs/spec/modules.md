# Modules

Every file is a module. Its top-level names are private unless they end in `*`. A module's exported
types bring their exported methods along.

```rio file=geo.rio
Vec* :: struct
  x, y: Float
end
sq :: proc(v: Float) -> Float     # private
  return v * v
end
Vec.len2* :: proc() -> Float
  return sq(self.x) + sq(self.y)
end
count* := 0
```

```rio
import .geo
v := geo.Vec{x = 3, y = 4}
geo.count += 1
log(v.len2(), geo.count)
```

```output
25.0 1
```

```rio file=geo.rio
sq :: proc(v: Float) -> Float
  return v * v
end
```

```rio
import .geo
log(geo.sq(2.0))  # error: not exported by that module
```

## import

```rio fragment
import .geo                       # geo.rio next to this file, as geo
import .ui.button as btn          # ui/button.rio, as btn
import tween                      # a package
import tween.{ease, lerp as mix}  # just some names, used without tween.
```

- **Local or package.** A leading `.` is local: a path from the importing file's directory, going
  only downward. Without it, the first name is a package, which the host finds (the CLI looks in its
  `-L` directories).
- **Files and directories.** `.a.b` is `a/b.rio`, or, if `a/b` is a directory, its entry file
  `a/b/b.rio`. A directory module's own local imports start in its directory.
- **Names.** `import .geo` declares `geo`; `as` picks another name. `.{a, b as c}` declares those
  exported names directly. Imported names are used in expressions, types and literals alike:
  `geo.Vec{x = 1}`, `v: geo.Vec`.
- **Imports go at the top level**, and are visible to the rest of the file.

## Packages

A package's root module decides what other code sees. Its submodules are private unless the root
publishes them with `import .name*`:

```rio file=lib/tween/tween.rio
import .easing*
import .curves
lerp* :: proc(a, b, t: Float) -> Float
  return a + (b - a) * t
end
```

```rio file=lib/tween/easing.rio
quad* :: proc(t: Float) -> Float
  return t * t
end
```

```rio file=lib/tween/curves.rio
x* := 1
```

```rio
import tween
import tween.easing
log(tween.lerp(0, 10, 0.5), easing.quad(0.5), tween.easing.quad(1))
```

```output
5.0 0.25 1.0
```

Asking for a submodule the root doesn't publish is an error ("that package doesn't publish curves").
Two packages of the same name where the host looks is an error too, not a silent choice.

## Exporting other names

There's no bulk re-export, so each exported name has one home. To export something imported, give
it an exported name of your own: `Sprite* :: gfx.Sprite`. Writing `import .a.{x*}` is an error that
says so.

## include

`include "path.rio"` makes another file part of this module: the same names, private ones included.
The path is relative to the including file, goes only downward, and names a `.rio` file. Each file can
be included once. Parts are read in place, so the declare-before-use order runs through them:

```rio file=parts/helpers.rio
double :: proc(x: Int) -> Int
  return x * factor
end
```

```rio
factor := 2
include "parts/helpers.rio"
log(double(21))
```

```output
42
```

## Order

A module is compiled and its top-level code runs once, at its first `import`. Every importer shares
its globals. So modules run in the order they're first imported, before the code after that import.
Importing a module that's still being compiled (a cycle) is an error.

```rio file=a.rio
log("a runs")
x* := 1
```

```rio
log("main starts")
import .a
import .a
log("main has", a.x)
```

```output
main starts
a runs
main has 1
```

## Host modules

The host can provide modules of its own procs, such as `pc` with `pc.cls`. See
[host procs](host.md#host-procs).
