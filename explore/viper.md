## Summaries

Any arm32 or riscv without further context means on rp2350 fruit jam. Some
numbers are scaled, due to having been tested on smaller or larger loops.

All times are in seconds.

- rv32 double - Micropython interpreter: 60.78288
- arm32 double - Micropython interpreter: 54.03616
- rv32 double - Micropython native: 50.98424
- arm32 double - Micropython native: 44.37842
- arm32 int - Micropython interpreter: 17.466754
- arm32 double - Lua 5.4 interpreter: 7.0276
- arm32 double - Unoptimized C: 1.7881
- rpi4 double - Python interpreter: 1.6414802074432373
- arm32 int - Lua 5.4 interpreter: 1.5387
- arm32 int - Viper compiled: 0.733376
- arm32 float - Unoptimized C: 0.5407
- rpi4 int - Python interpreter: 0.4927072525024414
- rv32 int - Viper compiled: 0.466719
- arm32 int - Unoptimized C: 0.4575
- rv32 int - Viper assembly: 0.333497
- arm32 int - Viper assembly: 0.200125
- rpi4 double - Lua 5.4 interpreter: 0.1003533
- rpi4 int - Lua 5.4 interpreter: 0.0770083

Some observations:

- On rp2350, lua interpreter is about 3-4x slower than unoptimized C here.
- On rp2350, python interpreter is about 30x slower than unoptimized C here.
- On rp2350, viper compiled ints are close to unoptimized C here.
- Viper doesn't do much useful with floats.
- RPi4 python interpreter is about the same as unoptimized rp2350 C here.

## Connecting to fruit jam console

```sh
screen /dev/ttyACM0 115200
```

## rpi4 python 3.14

```py
>>> def spin(n: int):
...     for _ in range(n):
...         pass
...
>>> t = time.time(); spin(10_000_000); time.time() - t
0.4927072525024414
# or about 0.13597 on my x64 laptop
# or about 0.23200 on my x64 laptop in firefox at https://pyodide.org/en/stable/console.html
>>> def spinf(n: float):
...     i = float(0)
...     while i < n:
...         i += 1.0
...
>>> t = time.time(); spinf(10_000_000); time.time() - t
1.6414802074432373
```

## rp2350 arm32

```py
>>> def spin(n: int):
...     for _ in range(n):
...         pass
...
...
...
>>> @micropython.viper
... def vspin(n: int):
...     for _ in range(n):
...         pass
...
...
...
>>> import time
>>> t = time.ticks_us(); spin(10_000_000); time.ticks_diff(time.ticks_us(), t) / 1e6
17.466754
>>> t = time.ticks_us(); vspin(10_000_000); time.ticks_diff(time.ticks_us(), t) / 1e6
1.40004
>>> 17.466754 / 1.40004
12.475897
>>> @micropython.viper
... def vspin2(n: int):
...     i = int(0)
...     while i < n:
...         i += 1
...
...
...
>>> t = time.ticks_us(); vspin2(10_000_000); time.ticks_diff(time.ticks_us(), t) / 1e6
0.733376
>>> t = time.ticks_us(); vspin2(100_000_000); time.ticks_diff(time.ticks_us(), t) / 1e6
7.333371
>>> t = time.ticks_us(); vspin2(10_000_000); time.ticks_diff(time.ticks_us(), t) / 1e6
0.733369
>>> 17.466754 / 0.733376
23.816916
>>> @micropython.asm_thumb
... def aspin(r0):
...     mov(r1, 0)
...     label(LOOP_START)
...     cmp(r1, r0)
...     bge(END)
...     add(r1, r1, 1)
...     b(LOOP_START)
...     label(END)
...     mov(r0, r1)
...
...
...
>>> t = time.ticks_us(); aspin(10_000_000); time.ticks_diff(time.ticks_us(), t) / 1e6
10000000
0.200125
>>> t = time.ticks_us(); aspin(100_000_000); time.ticks_diff(time.ticks_us(), t) / 1e6
100000000
2.000111
>>> 17.466754 / 0.200125
87.27923
>>> 17.466754 / 1.40004
12.475897
>>> 1.40004 / 0.733376
1.9090344
>>> 0.733376 / 0.200125
3.66459
>>> def spinf(n: float):
...     i = float(0)
...     while i < n:
...         i += 1.0
...
...
...
>>> t = time.ticks_us(); spinf(1_000_000); time.ticks_diff(time.ticks_us(), t) / 1e6
5.403616
>>> @micropython.native
... def nspinf(n: float):
...     i = float(0)
...     while i < n:
...         i += 1.0
...
...
...
>>> t = time.ticks_us(); nspinf(1_000_000); time.ticks_diff(time.ticks_us(), t) / 1e6
4.437842
>>> @micropython.asm_thumb
... def aspinf(r0):
...     vmov(s2, r0)
...     vcvt_f32_s32(s0, s2)
...     vsub(s1, s0, s0)
...     movwt(r1, 0x3f800000)
...     vmov(s3, r1)
...     label(LOOP_START)
...     vcmp(s1, s0)
...     vmrs(APSR_nzcv, FPSCR)
...     bge(END)
...     vadd(s1, s1, s3)
...     b(LOOP_START)
...     label(END)
...     vcvt_s32_f32(s2, s1)
...     vmov(r0, s2)
...
...
...
>>> t = time.ticks_us(); aspinf(1_000_000); time.ticks_diff(time.ticks_us(), t) / 1e6
1000000
0.053447
>>> t = time.ticks_us(); aspinf(10_000_000); time.ticks_diff(time.ticks_us(), t) / 1e6
10000000
0.533435
```

## rp2350 risc-v

```py
>>> def spin(n: int):
...     for _ in range(n):
...         pass
...
...
...
>>> import time
>>> t = time.ticks_us(); spin(10_000_000); time.ticks_diff(time.ticks_us(), t) / 1e6
17.933448
>>> @micropython.viper
... def vspin(n: int):
...     for _ in range(n):
...         pass
...
...
...
>>> t = time.ticks_us(); vspin(10_000_000); time.ticks_diff(time.ticks_us(), t) / 1e6
1.066721
>>> @micropython.viper
... def vspin2(n: int):
...     i = int(0)
...     while i < n:
...         i += 1
...
...
...
>>> t = time.ticks_us(); vspin2(10_000_000); time.ticks_diff(time.ticks_us(), t) / 1e6
0.466719
>>> @micropython.asm_rv32
... def aspin(a0):
...     li(a1, 0)
...     label(LOOP_START)
...     bge(a1, a0, END)
...     addi(a1, a1, 1)
...     jal(x0, LOOP_START)
...     label(END)
...     mv(a0, a1)
...
...
...
>>> t = time.ticks_us(); aspin(10_000_000); time.ticks_diff(time.ticks_us(), t) / 1e6
10000000
0.333497
>>> def spinf(n: float):
...     i = float(0)
...     while i < n:
...         i += 1.0
...
...
...
>>> t = time.ticks_us(); spinf(1_000_000); time.ticks_diff(time.ticks_us(), t) / 1e6
6.078288
>>> @micropython.viper
... def vspinf(n: float):
...     i = float(0)
...     while i < n:
...         i += 1.0
...
...
...
Traceback (most recent call last):
  File "<stdin>", line 3, in vspinf
ViperTypeError: unknown type 'float'
>>> @micropython.native
... def nspinf(n: float):
...     i = float(0)
...     while i < n:
...         i += 1.0
...
...
...
>>> t = time.ticks_us(); nspinf(1_000_000); time.ticks_diff(time.ticks_us(), t) / 1e6
5.098424
>>> @micropython.asm_rv32
... def aspinf(a0):
...     fcvt_s_w(fa0, a0)
...     fsub_s(fa1, fa0, fa0)
...     li(t0, 0x3f800000)
...     fmv_w_x(fa2, t0)
...     label(LOOP_START)
...     fle_s(t1, fa0, fa1)
...     bnez(t1, END)
...     fadd_s(fa1, fa1, fa2)
...     jal(x0, LOOP_START)
...     label(END)
...     fcvt_w_s(a0, fa1)
...
...
...
Traceback (most recent call last):
  File "<stdin>", line 3, in aspinf
SyntaxError: invalid RV32 instruction 'fcvt_s_w'
```

## rp2350 wili8jam

```lua
> t = time(); for _ = 1, 10000000, 1 do; end; print(time() - t)
1.5387
> t = time(); for _ = 1, 10000000, 1.0 do; end; print(time() - t)
7.0276
> t = time(); for _ = 1, 10000000, 1.5 do; end; print(time() - t)
4.6573
> t = time(); spini(10000000); print(time() - t)
0.4575
> t = time(); spinf(10000000); print(time() - t)
0.5407
> t = time(); spinf(5000000, 0.5); print(time() - t)
0.5407
> t = time(); spind(10000000); print(time() - t)
1.7881
> t = time(); spind(10000000, 0.5); print(time() - t)
3.5761
```

```c
void __attribute__((optimize("O0"))) spin(void) {
    clock_t t = clock();
    for (int i = 0; i < 100'000'000; i += 1) {
        // pass
    }
    float elapsed = (float)(clock() - t) / (float)CLOCKS_PER_SEC;
    p8_console_printf("spin: %f\n", elapsed);
}
// spin: 3.990000
void __attribute__((optimize("O0"))) spin(void) {
    clock_t t = clock();
    for (int i = 0; i < 10'000'000; i += 1) {
        // pass
    }
    float elapsed = (float)(clock() - t) / (float)CLOCKS_PER_SEC;
    p8_console_printf("spin: %f\n", elapsed);
}
// spin: 0.400000
void __attribute__((optimize("O0"))) spin(void) {
    clock_t t = clock();
    // for (int i = 0; i < 10'000'000; i += 1) {
    //     // pass
    // }
    for (float i = 0.0; i < 10'000'000.0; i += 1.0) {
        // pass
    }
    float elapsed = (float)(clock() - t) / (float)CLOCKS_PER_SEC;
    p8_console_printf("spin: %f\n", elapsed);
}
// spin: 0.480000
void __attribute__((optimize("O0"))) spin(void) {
    clock_t t = clock();
    // for (int i = 0; i < 10'000'000; i += 1) {
    //     // pass
    // }
    for (float i = 0.0; i < 10'000'000.0; i += 0.5) {
        // pass
    }
    float elapsed = (float)(clock() - t) / (float)CLOCKS_PER_SEC;
    p8_console_printf("spin: %f\n", elapsed);
}
// hang
void __attribute__((optimize("O0"))) spin(void) {
    clock_t t = clock();
    // for (int i = 0; i < 10'000'000; i += 1) {
    //     // pass
    // }
    for (float i = 0.0; i < 10'000'000.0; i += 1.5) {
        // pass
    }
    float elapsed = (float)(clock() - t) / (float)CLOCKS_PER_SEC;
    p8_console_printf("spin: %f\n", elapsed);
}
// spin: 0.310000
void __attribute__((optimize("O0"))) spin(void) {
    clock_t t = clock();
    // for (int i = 0; i < 10'000'000; i += 1) {
    //     // pass
    // }
    for (double i = 0.0; i < 10'000'000.0; i += 1.5) {
        // pass
    }
    float elapsed = (float)(clock() - t) / (float)CLOCKS_PER_SEC;
    p8_console_printf("spin: %f\n", elapsed);
}
// spin 1.200000
```

## rpi4 lua 5.4.7

```lua
> t = os.clock(); for _ = 1, 10000000, 1 do; end; print(os.clock() - t)
0.142049
> t = os.clock(); for _ = 1, 10000000, 1 do; end; print(os.clock() - t)
0.077202
> t = os.clock(); for _ = 1, 100000000, 1 do; end; print(os.clock() - t)
0.816999
> t = os.clock(); for _ = 1, 100000000, 1 do; end; print(os.clock() - t)
0.770083
> t = os.clock(); for _ = 1, 100000000, 1.0 do; end; print(os.clock() - t)
1.067942
> t = os.clock(); for _ = 1, 100000000, 1.0 do; end; print(os.clock() - t)
1.003533
> t = os.clock(); for _ = 1, 100000000, 1.5 do; end; print(os.clock() - t)
0.668098
> t = os.clock(); for _ = 1, 100000000, 1.5 do; end; print(os.clock() - t)
0.708609
```
