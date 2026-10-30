# Rio roadmap

The goal is a Rio 1.0 that's robust, nearly complete, and specified well enough that anyone could
implement it from the [spec](docs/spec/language.md). After 1.0, only bug fixes and implementation
improvements, with no 1.1, so each item below is decided as now or never. Releases may stay 0.x, or a
1.0 beta, until Rio has more users.

This is the list of everything still being considered. Each item says where it stands and which way
it leans. Rio means to stay small, so some of these will end up as no. Ideas already turned down,
with the reasons, are in [REJECTED.md](REJECTED.md).

## Decide first: changes that would break existing source

Nothing open. Each change here was decided against, keeping Rio as it is: `a[]` for a whole slice,
`a[i..]` the one spelling of an open-ended slice, `end` for the last index (not C#'s `^1`), and
`format` staying, beside the formatters to come. [REJECTED.md](REJECTED.md) has the reasons. A
change that would break existing source goes here first, since programs already written, such as
Rinc's and Studio's apps, would change with it.

## Next

Nothing open: see Leaning yes.

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
- **Stats as charts:** `tools/stats.py --svg` drawing `bench/stats.csv` as SVGs (the VM's and the
  compiler's size, compile time, each bench's speed, over the rows), checked in beside it and shown in
  the README, so how each batch moved things is a glance, not a table. CI could make them again and
  fail when the checked-in ones are stale, as Rinc's `pc_declared` does for `pc_host.h`. (Stats from
  different machines aren't comparable: the charts would mark which machine each row came from.)
- **Constant tables:** `[N]T` constants built while compiling and kept in the read-only image, so a
  lookup table costs no RAM on small boards.
- **Formatters as values:** `hex(n)`, `pad(s, 8)` and the like, returning `Fmt` values that work
  anywhere a value can print, beside `b.format(settings, ...)`, which stays.
- **Bool as a maybe of nothing:** `true` a value of a type with nothing in it, `false` nil. Then the
  boolean operators are what maybes need, as Rust's `and_then` and `unwrap_or` are, with no new
  operators (as Kotlin's `?:` or C#'s `??` would be):

  | | for Bools | for maybes | as in Rust |
  |---|---|---|---|
  | `x or y` | true if either | x's value if it's there, else y | `unwrap_or`, `or_else` |
  | `x and e` | e if x, else false | e if x is there (x its value inside e), else nil | `and_then`, `map` |
  | `not x` | the other | true when nil | `is_none` |
  | `if x` | when true | when x is there, x its value in the body | `if let Some(x)` |

  - **Types:** a `?T` `or` a `T` is a `T` (so `n := x or 0` needs no `is` after it); `or` another `?T`,
    a `?T`. A `?T` `and` a `U` or a `?U` is a `?U`. A Bool mixes in as a maybe: `flag and x` is a
    `?Int`, false becoming nil.
  - **Narrowing does the work:** `x and x.size` sees x's value on the right, as `x is T and ...` does
    now.
  - **Only Bools and maybes:** an Int, a String or anything else is still no condition, so `if count`
    stays an error (no zero as false).
  - **`??T` too, for uniformity:** a maybe of a maybe, as `?Bool` (a maybe of a maybe of nothing)
    already would be. `if x` for a `?Bool` tests whether it's there, as for any maybe, and `x or false`
    its truth. Open: how a program writes the inner nil (the outer is plain `nil`), and how `is` and
    printing tell the two apart.
  - **The cost:** `and` and `or` giving something other than a Bool surprises people from C, Java or
    Rust (not those from Python, Lua or JavaScript), though typed, a surprise is a compile error,
    not a wrong value.
- **Comparing a maybe with `==`:** `x == 4` for a `?Int`, false when x is nil, and `x == nil`. Today
  both are errors, and `x is nil` is the way to test for nil.

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
- **A very limited lifetime marker on `&` parameters:** one marker, nothing more, saying a `&T`
  parameter may point at the caller's locals, so views read through it count as borrowed. Unmarked
  `&T` keeps today's meaning (views read through it are global), so nothing breaks. It would let `&`
  reach locals holding views (Strings, slices, builder views, structs) without changing every `&T`.
  A method's `self` would need the same choice.
- **Layout checks for packed data:** `layoutOf(T)`, a constant hash of a struct's layout (each field's
  name and what it is, as host procs' sigs give it), to keep in a file's header, so reading records
  saved by another version of a program is an error, not misread fields. `sizeOf(T)` in the header
  covers only a change of size.
- **Compile-time `when`:** today, `if CONST` still type-checks a body it never runs. Only if
  differences between hosts need it.
- **A `sys` module for scripting,** in the Rio repo but not standard Rio: a separate `rio` build whose
  host module has files, directories, running programs, the environment and arguments, so scripts
  (tools such as `tools/stats.py`) can be Rio. Built on hosts registering their modules' sources.
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

- **if and switch as values:** right after `:=`, `=` or `return`, each branch's last line its value;
  an `else` required, except in a switch with a case for every value of an enum or type of a union.
  Never part of an expression.

- **No more arithmetic on structs:** only arrays of numbers do arithmetic as wholes (a struct's is a
  compile error pointing to `[2]Float` and its swizzles), so a vector has one way to be written.

- **Math on arrays:** the math builtins, `min` and `max`, and `toInt` and `toFloat`, element by element
  on arrays of numbers, a number spread over every element, and `clamp(x, lo, hi)` for numbers and
  arrays alike (`clamp(p, 0, hi)`). An array literal
  with no type takes its partner's in arithmetic (`p + {1, 0}`, `p *= {2, 1}`). Arithmetic on arrays
  of up to 4 numbers compiles to an instruction an element, so native loops have
  it (on the Fruit Jam, 256 balls' physics with `[2]Float` points: 0.52 ms a frame before, 0.25 after,
  against 0.19 with separate x and y).

- **Iterator structs:** `for x in it` (and `for x, i in it`, the turn from 0) over any struct with a
  method `next() -> ?T`, until it gives nil: a variable stepped in place, so a `break` leaves it where it
  stopped; anything else kept for the loop. A `next` giving text (`-> ?String`, the next word of a line)
  waits on values holding views, under Returning any value.

- **Text as numbers:** `s.toInt()`, `s.toInt(base)` (2 to 36) and `s.toFloat()`, on Strings, Blobs and
  byte builders, each a maybe: nil when the text isn't a number (images are version 13). A Float reads
  as its literal would.

- **`>>>`:** an unsigned right shift, and `>>>=` (images are version 12). Unsigned compare, divide and
  remainder, if they come, would be builtins rather than operators.

- **Proc type aliases:** `Less :: (proc(a, b: Int) -> Bool)`, in parentheses, since a proc type alone
  after `::` starts a proc.

- **`end` in index brackets:** the last index of what's indexed, `xs[end]`, `xs[end - 2..=end]`: a
  constant for an array, the length less 1 for the rest; not for an `[E]T`, whose indexes are named.

- **Unicode escapes:** `\u{1F600}`, its code point's bytes in UTF-8 in a string, and the code point
  itself in a character literal; past 10FFFF and surrogates are errors.

- **No more `diverse union`:** any union is allowed, whatever its types' sizes; each value takes its
  biggest type's room, a cost in memory like a big array's (`rio -m` shows it). `diverse` is an
  ordinary name again.

- **Enums by name in layouts:** `Enum{game,ui,none}` (a bitfield's `Enum(2){...}`), as a struct's field
  and as a host proc's parameter in its sig, so an image made before an enum's values were reordered
  doesn't fit, while a parameter's values added at its end do. A plain `i` for an enum still works.

- **`core`, a module:** the builtins are module `core`, there everywhere unqualified as before, and as
  `core.name`, which reaches a builtin a program's own name hides. (`src/core.rio` stays its
  declarations, checked by the tests, not a source compiled into programs: that would cost every
  compile.)

- **Hosts giving their modules' sources:** `rio_addSource(vm, "pc", src, len)`, taken before the
  loader, so declarations go with the code they declare, and given back by `rio_hostSource` for help
  or an editor. `rio -H` writes the source (`pc_SOURCE`, `pc_HASH`) and the host procs as an X macro,
  `pc_HOST_PROCS(X)`, beside the structs and sigs. Images keep each source's name and hash, named in
  the message when a host proc doesn't fit: a name for what an image was made with, not a gate, since
  images are meant to survive what only adds. Images are version 11.

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
- **No `@`:** `cls* :: host proc(c: Int)`. `host` is an ordinary name anywhere else.
