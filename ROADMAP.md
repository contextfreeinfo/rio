# Rio roadmap

The goal is a Rio 1.0 that's robust, nearly complete, and specified well enough that anyone could
implement it from the [spec](docs/spec/language.md). After 1.0, only bug fixes and implementation
improvements, with no 1.1, so each item below is decided as now or never. Releases may stay 0.x, or a
1.0 beta, until Rio has more users.

This is the list of everything still being considered. Each item says where it stands and which way
it leans. Rio means to stay small, so some of these will end up as no. Ideas already turned down,
with the reasons, are in [REJECTED.md](REJECTED.md).

## Decide first: changes that would break existing source

Everything else here only adds, so existing programs keep working. These would change programs
already written, such as Rinc's and Studio's apps, so they come first.

- **Full-slice syntax:** `a[]` matches the type `[]T`, but it can look like an empty slice.
  Alternatives include `a[..=]` and `a[..]`. `a[..]` looks too much like a builder view type
  (`Blob[..]`), so it's out. Leaning: keep `a[]` unless it misleads in practice. A change would be a mechanical migration, like the one
  that removed `:`.
- **Open-ended slices:** `a[i..]` is the only spelling today. `a[i..<]` and `a[i..=]` are errors.
  Both would mean the same thing, since nothing past the end is ever taken. Leaning: keep one
  spelling, so there's nothing for a formatter to standardize. If a second one ever comes in, fmt
  rewrites it to `a[i..]`.
- **Long argument lists:** two routes, still being weighed. Either way, what a call leaves out starts
  as zero or nil, as variables do, and a `?T` tells "not given" apart from zero.
  - **A. Struct arguments:** `text("hi", 3, 4, {color = 8, scale = 2})`. This works for any small struct
    now, through proc values too: structs holding views (a `String` field) can be locals and by-value
    parameters, with no new syntax for "limited scope". A struct parameter's views are borrowed like a
    `String` parameter's, and a local counts as a view of whatever its views view (a field named on
    its own, of what was stored there). tests/viewtest.py checks these rules with random programs.
    - For: options are values, so a style can be kept in a global, reused and forwarded. Proc
      values work unchanged, and there's no new parameter syntax. Locals holding strings are worth
      having anyway.
    - Against: a type to declare for each set of options, which readers look up away from the proc.
      Host procs can't take structs yet, so the host interface would need them too, perhaps as the
      struct's words in field order. Braces at every call, unless calls may drop them when the last
      parameter is a struct (`text("hi", 3, 4, scale = 2)`). A proc can't keep a struct parameter's
      views without copying their bytes.
    - **References:** done. A view read through a `&T` parameter, `self`, a `for x in xs` element or
      `r := &place` counts as borrowed (or as the proc's own memory, when the place is), like a
      `String` parameter's, so `g = p.name` takes a copy. That also closed a hole (a call returning a
      view of a local passed by `&`), and broke no Rinc or Studio app.
    - **Also:** done for arrays and unions holding views. Builders holding views, and big values
      holding them, must still be globals.
  - **B. Named-only parameters,** after the positional ones, with a marker between them:
    ```
    text :: proc(s: String, x, y: Int; color: Int, scale: ?Int)
    text("hi", 3, 4, scale = 2)
    ```
    - For: the options are in the signature. Each one is an ordinary parameter, so views need
      nothing new. Hosts receive plain arguments.
    - Against: new syntax in parameters and calls. Proc types would have positional parameters only,
      so a proc with named ones can't fully be a proc value. Options can't be bundled or forwarded.
    - **The marker:** `;` is short but easy to miss. Plainer: a `?` in a parameter's place (as
      Python uses `*`), or `|`.
  - Leaning: A. Its missing piece (structs holding strings as locals) is a gap either way, and the
    call shorthand without braces would give A the look of B. If struct arguments work out, B is
    dropped. Settle this before Rinc's API freezes, since long `pc` calls would be reshaped either way.
- **Formatters as values:** if `hex(n)` and `pad(s, 8)` cover what `b.format(settings, ...)` does,
  `format` could go, which changes every call to it. No Rinc or Studio app calls it yet, so the change
  is cheap now and costly later. Leaning: decide whether `format` stays before building the formatters.

## Next

- **Unicode escapes:** `\u{1F600}` in strings, written out as UTF-8. Numbers past 10FFFF and
  surrogates are errors.
- **`end` in index brackets:** the last index, as in `xs[end]` and `xs[end - 2..=end]`. It works only
  inside the brackets of the thing being indexed.
- **Proc type aliases:** `Less :: proc(a, b: Int) -> Bool`, just as constants already alias other types
  (`Vec3 :: [3]Float`).
- **Number parsing from text:** String to Int in any base from 2 to 36, and String to Float, with bad
  input reported. Without it, no program can read numbers safely.
- **`>>>`:** an unsigned right shift. Unsigned compare, divide and remainder would be builtins rather
  than operators.

## Leaning yes

- **Static procs on types:** `P.make :: static proc(x: Int) -> P`, called as `P.make(3)`, as the
  builtin `E.fromInt(n)` already is. Today that call is "not a value". Methods stay as they are, with an
  implicit `self`, since they're the common case: the marker goes on the rare kind. Deciding from
  whether a body uses `self` would let a body change what callers may do.
- **Generic procs:** `swap :: proc[T](a, b: &T)`, `find :: proc[T](xs: []T, x: T) -> Int`. The type
  parameters are listed in brackets after `proc`, where nothing else can go.
  - **Inferred at every call:** `find(xs[], 3)`. There's no `find[Int](...)`, which would read like
    indexing.
  - **One compiled copy each,** never a copy per type (no monomorphization). Each call passes T's
    descriptor as a hidden argument: the same descriptors that print structs now, kept in the image.
  - **What the body can do:** in the body, a T value can only be passed, stored, copied, compared
    with `==`, and indexed in `[]T` and `[..]T`. Builtins that read the descriptor work on it too:
    `log` and `format` print it, `==` compares Strings by their text, `pack` and `unpack` take it (a
    T that can't be packed is a compile error at the call, where T is known), and a hash builtin would
    serve a generic map.
  - **No constraints and no traits:** anything type-specific, like `<`, comes in as a proc argument:
    `sort(xs[], less)`.
  - **Builtins declared for real:** this would let core.rio declare its generic builtins as
    `([]T).len` and `([..]T).push`. Their rules, like what can be packed or printed, stay in comments.
  - **Sort and find:** these could then be written in Rio. Without generics, they'd be host builtins
    taking a less proc.
- **Constant tables:** `[N]T` constants built while compiling and kept in the read-only image, so a
  lookup table costs no RAM on small boards.
- **Formatters as values:** `hex(n)`, `pad(s, 8)` and the like, returning `Fmt` values that work
  anywhere a value can print. Whether they replace `format` is under Decide first.

## Maybe

- **Generic types,** in brackets as generic procs are: `Pair :: struct[T]`, used as `Pair[Int]`, only if
  they stay simple. They're what generic maps would need. Some of a type's parameters may be constant
  Ints rather than types, as the builtins' already are (`Blob[16]`, `[..N]T`). Unsure for now.
- **Reflection, through generics:** not now, but designed so generic procs leave room for it. A generic
  proc's hidden descriptor is all reflection needs, so it works on any value at any call, locals
  included, with no stored `Any`: the caller knows the type, as with any generic call.
  - **No recursion:** Rio has none, so a nested value isn't walked by a proc calling itself. A builtin
    `for` walks it instead, a leaf at a time, depth first: `for leaf in leaves(x)`, each leaf a
    builtin union of what can be one (`Int`, `Float`, `Bool`, `String`, an enum as its number and
    name, `nil` for an empty union), with its field's name and depth.
  - **What it would give:** printing and text serialization written in Rio (a writer of JSON, say),
    equality and hashing of whole values, and reading values back from text, field by field.
  - **Writing back** through `&x`, only leaf by leaf, from a leaf of the same kind: a view only from a
    view, an enum checked as `E.fromInt` is, so bytes never become an address.
  - **Costs:** descriptors kept for every type passed as a T, field names too (bytes in the image); a
    slower walk than code written for one type; and a reflection API frozen in the 1.0 spec, which
    every implementation must build. So it waits for a real use, such as a serializer, and builtins
    on descriptors (printing, `==`, pack, hashing) may cover most of what it would.
- **Iterator structs** instead of generators: `for x in it` would take any struct with a
  `next(&self) -> ?T` method, so a lazy sequence costs one small struct and no saved frames. This would
  only come after generic procs.
- **if and switch as values, only to set a variable:** this form works only right after `:=`, `=` or
  `return`. The value of each branch is its last line, and an `else` is required:
  ```
  size := switch kind
  case .small
    1
  else
    4
  end
  ```
  It never puts a block inside an expression.
- **A very limited lifetime marker on `&` parameters:** one marker, nothing more, saying a `&T`
  parameter may point at the caller's locals, so views read through it count as borrowed. Unmarked
  `&T` keeps today's meaning (views read through it are global), so nothing breaks. It would let `&`
  reach locals holding views (Strings, slices, builder views, structs) without changing every `&T`.
  A method's `self` would need the same choice.
- **Comparing a maybe with `==`:** `x == 4` for a `?Int`, false when x is nil, and `x == nil`. Today
  both are errors, and `x is nil` is the way to test for nil.
- **Compile-time `when`:** today, `if CONST` still type-checks a body it never runs. Only if
  differences between hosts need it.
- **Converting values to Int:** Bool's `toInt`, as enums already have. No truthiness.
- **A foreign-call table supplied by the host:** a fixed array of host procs that programs can call
  without a `host proc` declaration for each.

## Before 1.0

- **Memory spec:** what a program can see of memory, so that two implementations agree. Cached images
  needn't be portable between implementations. Source is the portable form.
- **The full spec:** every rule written as an example that the tests run.
- **Audits and cleanup:** every compile error reachable and tested.
- **Keywords:** a last pass makes as many keywords contextual as can be, without hurting error
  messages or highlighting. Declaration words, which only follow `::` (`struct`, `enum`, `union`), are
  the easy ones. Control words at the start of a statement (`if`, `for`, `return`) are riskier, since
  `return (x)` and a call `return(x)` would look alike.
- **Better printing of everything:** every value should print, and print usefully, in `log`,
  `print` and `format` alike. Known gaps:
  - **Procs:** a proc's name as an argument, `log(f)`, is "not a value", though
    `g: proc(x: Int) -> Int = f` works. A proc value prints only `proc`, not which proc; its name would
    mean keeping proc names in images, as enum names are kept.
  - **Floats:** printed to about 7 significant digits, so `1.0 / 3.0` prints `0.333333` and doesn't read
    back as the same Float. The shortest digits that read back exactly would print `0.33333334`.
  - **Types:** `log(Int)` is "not a value". A type's name could print, which would help in generic
    procs if they come.

## Done recently

- **`sizeOf(T)`, `offsetOf(S, .field)` and `bitOffsetOf(S, .field)`:** constants, for packed records
  and binary formats.

- **Enums and Bools in bitfields:** `on: Bool(1)`, and `kind: Shape(2)`, with a compile-time check
  that 2 bits hold every Shape. They print by name. Unpacking a value its type doesn't have (an enum,
  a Bool or a union's tag, bitfield or not) is a runtime error, as with `E.fromInt`.

- **Narrowing through `and`:** in `if x is Int and x > 2`, x is an Int after the `and`, not just in
  the if's body.
- **Packing:** `pack`, `unpack`, `getInt` and `setInt`.
- **Hex escapes,** with logs escaping control bytes.
- **Bitfields:** `Int(n)` and `Int(-n)`.
- **Enums:** always 0-based and contiguous.
- **Proc values as elements.**
- **Ranges as values,** with range slicing instead of `:`.
- **Names:** `[..N]T` is a builder and `[..]T` a builder view.
- **A map in Rio:** tests/map.rio, a hash table in user code, String to Int and Int to a struct. Keys
  are copied into an arena of the map's own, since views passed in are borrowed. Each key and value
  type needs its own copy of the code until generic procs, which it's to be tried again with.
- **`do`:** one-line blocks (`if done do return`, `case .red do stop()`), and `do ... end` blocks
  for names.
- **No `@`:** `cls* :: host proc(c: Int)` and `Shape :: diverse union`. Both words are ordinary names
  anywhere else.
