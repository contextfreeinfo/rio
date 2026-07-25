## ARM

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

## RISC-V

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
