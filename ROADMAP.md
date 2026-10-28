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
- **Formatters as values:** if `hex(n)` and `pad(s, 8)` cover what `b.format(settings, ...)` does,
  `format` could go, which changes every call to it. No Rinc or Studio app calls it yet, so the change
  is cheap now and costly later. Leaning: decide whether `format` stays before building the formatters.

## Next

- **No more `diverse union`** (decided): any union is allowed, whatever its types' sizes, with no
  keyword. Each value takes its biggest type's room, a cost in memory like a big array's, which Rio
  doesn't gate with a keyword either; `rio -m` shows where memory goes. Nothing uses `diverse` yet,
  and `diverse` goes back to being an ordinary name.
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
- **Iterator structs,** instead of generators: `for x in it` takes any struct with a `next` method
  returning `?T`, and runs until it returns nil. A lazy sequence costs one small struct and no saved
  frames. It needs no generics: each loop knows its struct's type, so `next` and T are known there, as
  for a call written out. Generics would only add procs that take any iterator, such as `map` and
  `filter`.
  ```
  Words :: struct
    text: String
    at: Int
  end
  Words.next :: proc() -> ?String   # the next word, or nil
  for w in Words{text = line}
    log(w)
  end
  ```
  - **The struct:** a variable is stepped in place, so after a `break` it carries on where it stopped
    (for a parser, say); any other value, such as a literal or a call's result, is kept for the loop.
  - **Views:** a `next` that returns a view of the struct's text follows the usual rules for what a
    method returns, so nothing new.
  - **An index too:** whether `for i, x in it` counts from 0, as it does for slices.

## Leaning yes

- **Static procs on types:** `P.make :: static proc(x: Int) -> P`, called as `P.make(3)`, as the
  builtin `E.fromInt(n)` already is. Today that call is "not a value". Methods stay as they are, with an
  implicit `self`, since they're the common case: the marker goes on the rare kind. Deciding from
  whether a body uses `self` would let a body change what callers may do. (Builtins about a type are
  procs taking it, `sizeOf(T)`, which leaves `T.name` for these.)
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
- **Returning any value:** today a proc returns a value of up to 16 words with no views in it, host
  procs too. Each of these would lift a limit, for both, and they'd come when something needs them:
  - **Values holding views** (a struct with a `String` field, a union of strings): returned as a
    `String` is now. A view of the proc's own memory is still an error, and the caller's copy counts as
    viewing what the arguments viewed, as a by-value struct parameter's does.
  - **Big values** (over 16 words): the caller passes an address for the proc to fill in, and `return`
    writes through it. `x := f()` and `x = f()` fill x itself, so nothing is copied (and no type need
    be written to copy it); elsewhere, the result goes in a temporary kept in the caller's frame, one
    for each such call.
- **`core`, a module, and hosts giving their modules' sources:** the builtins become module `core`,
  whose contents are imported everywhere unqualified (`core.len` still reaches one a program has
  shadowed), as Rinc's `pc` is imported everywhere qualified. `src/core.rio` becomes that module, not
  a document the tests check. A host registers each of its modules' source with its host procs, so
  declarations ship with the code they declare and can't drift (Rinc builds `pc.rio` in already). The
  sources can then be shown on demand, by help or an editor, or as read-only files: copies written out
  to edit would change nothing, which would mislead. `rio -H` would scaffold the rest of a host: with
  the C structs and sigs it writes now, the module's source as a C string, and a table of its procs to
  fill in, registered (source too) with one call. A hash of each module's source, kept in images made
  with it, would make a quick check and a name for what an image was made with ("pc 3f2a..."), but
  not the gate: any change to the module, even one that adds a proc, would change it, and images are
  meant to survive what only adds. When the hash differs, each host proc it calls is checked, as now.
- **Enums by name in layouts:** an enum parameter or field is `Int` in a sig's layout now, so an image
  made before a host module's enum had its values reordered would load and pass the wrong ones.
  Written as its values' names (`Pointer{game,ui,none}`), a reorder doesn't fit, and values added at
  the end still do, as fields added at a struct's end do.
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
- **Layout checks for packed data:** `layoutOf(T)`, a constant hash of a struct's layout (each field's
  name and what it is, as host procs' sigs give it), to keep in a file's header, so reading records
  saved by another version of a program is an error, not misread fields. `sizeOf(T)` in the header
  covers only a change of size.
- **Comparing a maybe with `==`:** `x == 4` for a `?Int`, false when x is nil, and `x == nil`. Today
  both are errors, and `x is nil` is the way to test for nil.
- **Compile-time `when`:** today, `if CONST` still type-checks a body it never runs. Only if
  differences between hosts need it.
- **A `sys` module for scripting,** in the Rio repo but not standard Rio: a separate `rio` build whose
  host module has files, directories, running programs, the environment and arguments, so scripts
  (tools such as `tools/stats.py`) can be Rio. Built on hosts registering their modules' sources.
- **A foreign-call table supplied by the host:** a fixed array of host procs that programs can call
  without a `host proc` declaration for each.
- **Host procs calling back into Rio while they run:** a sort with a comparison, a parser handing back
  each element, a physics step calling back on each collision. Today a host proc is a leaf: it can't
  call into the program while it runs. What allowing it would take:
  - **No recursion still:** a host proc marked as calling back counts as calling through a proc value
    for its callers, so they can't be proc values, and no callback can come back around to them (each
    proc's one frame stays safe), by the rule proc values already follow.
  - **The VM entered again** from inside a host proc, nested on the C stack (a cap on how deep, for
    boards with a small stack), with a runtime error in a callback coming back out through the host,
    and the JIT's loops and the C output handling it too.
  - **What it's worth:** mock up the parser and physics cases first, against what already works for
    them: results as data (an array of events), iterator structs, sort in Rio once generics come, and
    proc values the host keeps and calls between runs (`rio_callValue`).

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

- **Host procs as proc values:** `f := pc.cls`, and as an argument or element. Each one used as a value
  gets a small proc of its own that calls it, made at the end of compiling, so calls through values
  stay as they are and the host call does the converting (structs by address, results placed). A host
  proc can't call back into the program, so it can always be a value. With this, what one kind of proc
  can do, the other can.

- **Host procs returning structs and maybes** (`>{...}`, `>Union{nil|Int}` in a sig): the call passes
  where the result goes first, and the host writes it with `rio_putStruct`, so a struct result may
  gain fields at its end and older images still get what they hold.

- **Host procs taking `&T`:** an `Int`, a `Float` or a struct the host may change (`&i`, `&f`,
  `&{...}` in its sig), written back with `rio_putStruct`, no more than the place holds.

- **Optional trailing parameters:** `style?: Style`, left out of a call, is zero, as a variable is: in
  procs, methods, proc values and host procs alike. Only zero, the rule images already follow for host
  procs that gained parameters at their end, so a host proc can gain options once its API is frozen
  (Rinc's `pc`) without breaking programs, in source or compiled.

- **Long argument lists, as struct arguments:** `text("hi", 3, 4, {color = 8, scale = 2})`, what's
  left out zero, as for variables. Structs holding views (a `String` field) can be locals and by-value
  parameters, borrowed like a `String` parameter's (tests/viewtest.py checks the rules with random
  programs), and host procs take structs, by address, with their layouts in the sig; `rio -H` writes
  the matching C from the module's source. Images bind only the host procs they call, and a host proc
  may gain parameters at its end, or a struct fields at its end, which older images give as zeros.

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
