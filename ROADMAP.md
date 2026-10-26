# Rio roadmap

The goal is a Rio 1.0 that's robust, nearly complete, and specified well enough that anyone could
implement it from the [spec](docs/spec/language.md). After 1.0, only bug fixes and implementation
improvements, with no 1.1, so each item below is decided as now or never. Releases may stay 0.x, or a
1.0 beta, until Rio has more users.

This is the list of everything still being considered. Each item says where it stands and which way
it leans. Rio means to stay small, so some of these will end up as no.

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
- **Named arguments:** for long argument lists. A proc's parameters would be positional first, then
  named-only, never mixed. Proc types, and so proc values, would have positional parameters only.
  - **Struct arguments** (`text("hi", 3, 4, {color = 8})`) need structs holding views allowed as
    parameters. Today such a struct must be a global, so options that include a `String` can't be
    passed this way.
  - **Named-only parameters with constant defaults:** `text :: proc(s: String, x, y: Int,
    color := 7, scale := 1)`, called `text("hi", 3, 4, scale = 2)`. These avoid that problem: each one
    is an ordinary parameter, and a view in one is borrowed like any other. A parameter with a default
    is named-only, and a default must be a constant, so the caller fills it in and the callee sees
    plain arguments. Host procs could take them with no change to how hosts receive arguments.
  - Leaning: named-only parameters, as above. Allowing struct parameters that hold views would only
    loosen a rule, so it could still come later. Settle this before Rinc's API freezes, since long
    `pc` calls would be reshaped to use it.
- **Formatters as values:** if `hex(n)` and `pad(s, 8)` cover what `b.format(settings, ...)` does,
  `format` could go, which changes every call to it. No Rinc or Studio app calls it yet, so the change
  is cheap now and costly later. Leaning: decide whether `format` stays before building the formatters.
- **`do` as a keyword:** this would break any program that uses `do` as a name. No current app does,
  so it costs nothing now.

## Next

- **Unicode escapes:** `\u{1F600}` in strings, written out as UTF-8. Numbers past 10FFFF and
  surrogates are errors.
- **`end` in index brackets:** the last index, as in `xs[end]` and `xs[end - 2..=end]`. It works only
  inside the brackets of the thing being indexed.
- **Proc type aliases:** `Less :: proc(a, b: Int) -> Bool`, just as constants already alias other types
  (`Vec3 :: [3]Float`).
- **Enums and Bools in bitfields:** `on: Bool(1)`, and `kind: Shape(2)` with a compile-time check that
  2 bits hold every Shape, so adding a value to an enum can't silently break a packed layout.
  Unpacking a value that isn't a Shape is a runtime error, as with `E.fromInt`.
- **Number parsing from text:** String to Int in any base from 2 to 36, and String to Float, with bad
  input reported. Without it, no program can read numbers safely.
- **`>>>`:** an unsigned right shift. Unsigned compare, divide and remainder would be builtins rather
  than operators.

## Leaning yes

- **One-line blocks with `do`:** `if done do return`, `for i in 0..<n do total += xs[i]`,
  `double :: proc(n: Int) -> Int do return n * 2`. They'd work on proc, for, switch and if headers, and
  on case and else lines, so a switch can put each case on one line:
  ```
  switch light
  case .red do stop()
  case .green do go()
  end
  ```
  The body is a single statement, never another `do` header, so one-line blocks don't nest. A one-line
  if has no `else`, so a branch with an else gets written out in full.
- **Scope blocks, `do ... end`:** a `do` on a line of its own starts a block whose names end with it:
  ```
  do
    total := 0
    for x in xs do total += x
    log(total)
  end
  ```
- **Generic procs, Odin style:** `swap :: proc(a, b: &$T)`, `find :: proc(xs: []$T, x: T) -> Int`.
  - **One compiled copy each,** never a copy per type. Each call passes T's size and layout as hidden
    arguments.
  - **What the body can do:** in the body, a T value can only be passed, stored, copied, compared
    with `==`, and indexed in `[]T` and `[..]T`.
  - **No constraints and no traits:** anything type-specific, like `<`, comes in as a proc argument:
    `sort(xs[], less)`.
  - **Builtins declared for real:** this would let core.rio declare its generic builtins as
    `([]$T).len` and `([..]$T).push`. Their rules, like what can be packed or printed, stay in comments.
  - **Sort and find:** these could then be written in Rio. Without generics, they'd be host builtins
    taking a less proc.
- **Constant tables:** `[N]T` constants built while compiling and kept in the read-only image, so a
  lookup table costs no RAM on small boards.
- **Formatters as values:** `hex(n)`, `pad(s, 8)` and the like, returning `Fmt` values that work
  anywhere a value can print. Whether they replace `format` is under Decide first.

## Maybe

- **Generic types,** also Odin style (`Pair :: struct($T: typeid)`), only if they stay simple. They're
  what generic maps would need. Unsure for now.
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
- **Compile-time `when`:** today, `if CONST` still type-checks a body it never runs. Only if
  differences between hosts need it.
- **Converting values to Int:** Bool's `toInt`, as enums already have. No truthiness.
- **A foreign-call table supplied by the host:** a fixed array of host procs that programs can call
  without a `host proc` declaration for each.

## Decided no

- **`**`:** `pow` covers Floats.
- **Coroutines:** each needs a saved frame, which costs memory and is hard to specify.
- **Traits and constraints on generics.**
- **Annotations, `@`:** programs don't define their own, since one the compiler checks would need a
  language for what to check, and one it doesn't check is a comment a typo can break. Tools that want
  markers can read comments.
- **JIT requests in the language:** native code is an implementation's choice. A host short on code
  space picks which loops get it, by how often they run and how big they are.
- **Host procs with no marker:** a proc with no body would read exactly like a proc type alias, so
  `host` says the body is elsewhere.
- **A builtin Map or Dict:** without generic types it would be a special case, and Rio never
  allocates, so a map's capacity and layout are the program's business. Programs write tables for
  their own keys: `[E]T` by enum, a sorted array, or a hash table over arrays.
- **Truthiness:** conditions stay Bool.

## Before 1.0

- **Memory spec:** what a program can see of memory, so that two implementations agree. Cached images
  needn't be portable between implementations. Source is the portable form.
- **The full spec:** every rule written as an example that the tests run.
- **Audits and cleanup:** every compile error reachable and tested.
- **Keywords:** a last pass makes as many keywords contextual as can be, without hurting error
  messages or highlighting. Declaration words, which only follow `::` (`struct`, `enum`, `union`), are
  the easy ones. Control words at the start of a statement (`if`, `for`, `return`) are riskier, since
  `return (x)` and a call `return(x)` would look alike.
- **Printing every value:** everything that is a value prints today, with these gaps:
  - a proc's name as an argument, `log(f)`, is "not a value", though `g: proc(x: Int) -> Int = f`
    works;
  - a proc value prints only `proc`, not which proc. Its name would mean keeping proc names in
    images, as enum names are kept.

## Done recently

- **Packing:** `pack`, `unpack`, `getInt` and `setInt`.
- **Hex escapes,** with logs escaping control bytes.
- **Bitfields:** `Int(n)` and `Int(-n)`.
- **Enums:** always 0-based and contiguous.
- **Proc values as elements.**
- **Ranges as values,** with range slicing instead of `:`.
- **Names:** `[..N]T` is a builder and `[..]T` a builder view.
- **No `@`:** `cls* :: host proc(c: Int)` and `Shape :: diverse union`. Both words are ordinary names
  anywhere else.
