# Rio roadmap

The goal is a Rio 1.0 that's robust, nearly complete, and specified well enough that anyone could
implement it from the [spec](docs/spec/language.md). After 1.0, only bug fixes and implementation
improvements, with no 1.1, so each item below is decided as now or never. Releases may stay 0.x, or a
1.0 beta, until Rio has more users.

This is the list of everything still being considered. Each item says where it stands and which way
it leans. Rio means to stay small, so some of these will end up as no.

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
  if has no `else`, so a branch with an else gets written out in full. `do` would be a reserved
  keyword.
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
  anywhere a value can print.
- **Named arguments as struct arguments:** `text("hi", 3, 4, {color = 8})`, options being plain values.

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
- **`@` syntax:** if programs never get decorations of their own, `@host` and `@diverse` could become
  contextual keywords. If `@` stays, perhaps so programs can ask for native code explicitly (`@jit`
  on a proc or loop, to fit a small board), one general rule would cover every `@name`. That rule would
  say where an `@name` can go and what it applies to, from a fixed set of names.
- **Converting values to Int:** Bool's `toInt`, as enums already have. No truthiness.
- **A foreign-call table supplied by the host:** a fixed array of host procs that programs can call
  without new syntax, depending on how `@host` settles.

## Decided no

- **`**`:** `pow` covers Floats.
- **Coroutines:** each needs a saved frame, which costs memory and is hard to specify.
- **Traits and constraints on generics.**
- **Truthiness:** conditions stay Bool.

## Before 1.0

- **Memory spec:** what a program can see of memory, so that two implementations agree. Cached images
  needn't be portable between implementations. Source is the portable form.
- **The full spec:** every rule written as an example that the tests run.
- **Audits and cleanup:** every compile error reachable and tested.

## Done recently

- **Packing:** `pack`, `unpack`, `getInt` and `setInt`.
- **Hex escapes,** with logs escaping control bytes.
- **Bitfields:** `Int(n)` and `Int(-n)`.
- **Enums:** always 0-based and contiguous.
- **Proc values as elements.**
- **Ranges as values,** with range slicing instead of `:`.
- **Names:** `[..N]T` is a builder and `[..]T` a builder view.
