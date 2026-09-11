## Building

Run:

```sh
./run.sh examples/hi/hi.rio
```

Or for thumb2, if on a system that can run thumb2:

```sh
./run.sh --thumb examples/hi/hi.rio
```

## Thoughts

- No dynamic memory allocation
- Maybe no recursion???
- Refs only in parameters and locals
- Maybe a shadow stack for platforms with limited stack space
- &Reference, ?Maybe, [_]Array, and []Span types
- &mut Ref, []mut Span, else deeply imu?
- Refs can't be nil
- Named args good because can't store refs/spans in structs?
- No numeric address access? Still can compare for equality only?
- Numeric zero, empty slices, and empty arrays are falsy
- Vague inspiration from Lua and Odin
- Override constants from command line args including for global array sizes
- Clean module hierarchy on dots
- Direct native codegen in some cases, such as thumb-2 / rp2350 / Fruit Jam
- Fallback codegen/exec on wasm (and wamr or wasmi???)
- Range type for slicing
- Redefine any constant as command line arg definition
- Nil value of enum or union for false
- Operator `not` always gives Bool, but `and` and `or` generic
- Maybe `or panic()` for awkward failing?
- Maybe awkwardly explicit monomorphizing generics???
- Maybe 1-based indexing for bad zero default ints
- Maybe hash helpers into spans

## Code gen

Here's an example of code generated for thumb2. First, here's the rio source for
this example:

```rb
# Well, technically does more than greet, but meh.
greet :: proc()
  log("Hi there!")
  log("Bye now!")
end

# Just to prove nested and repeated calls work.
greetTwice :: proc()
  greet()
  greet()
end

# Main naturally goes last.
main :: proc()
  greetTwice()
end
```

For some reason, a few address bytes get saved wrong, but this is still
approximately the correct structure:

```
   0:	0000      	movs	r0, r0
   2:	0000      	movs	r0, r0
   4:	b580      	push	{r7, lr}
   6:	f24f 0014 	movw	r0, #61460	@ 0xf014
   a:	f2cf 7091 	movt	r0, #63377	@ 0xf791
   e:	f643 7c29 	movw	ip, #16169	@ 0x3f29
  12:	f2c0 0c8d 	movt	ip, #141	@ 0x8d
  16:	47e0      	blx	ip
  18:	f24f 0028 	movw	r0, #61480	@ 0xf028
  1c:	f2cf 7091 	movt	r0, #63377	@ 0xf791
  20:	f643 7c29 	movw	ip, #16169	@ 0x3f29
  24:	f2c0 0c8d 	movt	ip, #141	@ 0x8d
  28:	47e0      	blx	ip
  2a:	bd80      	pop	{r7, pc}
  2c:	b580      	push	{r7, lr}
  2e:	f7ff ffe9 	bl	0x4
  32:	f7ff ffe7 	bl	0x4
  36:	bd80      	pop	{r7, pc}
  38:	b580      	push	{r7, lr}
  3a:	f7ff fff7 	bl	0x2c
  3e:	bd80      	pop	{r7, pc}
```

And here's code and data in uglier form:

```
Code:
0xf7b20000: 00000000_80b54ff2_1400cff2_917043f6 ______O______pC_
0xf7b20010: 297cc0f2_8d0ce047_4ff22800_cff29170 )|_____GO_(____p
0xf7b20020: 43f6297c_c0f28d0c_e04780bd_80b5fff7 C_)|_____G______
0xf7b20030: e9fffff7_e7ff80bd_80b5fff7_f7ff80bd ________________
Data:
0xf791f008: 00000000_14f091f7_09000000_48692074 ____________Hi t
0xf791f018: 68657265_21000000_28f091f7_08000000 here!___(_______
0xf791f028: 42796520_6e6f7721                   Bye now!
```
