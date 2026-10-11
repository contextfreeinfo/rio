#!/usr/bin/env python3
"""Differential test: random rio programs that always finish must print the same in the VM and as C.

  python3 tests/difftest.py RIO CC [count] [seed]

RIO is the rio binary, CC a C compiler for the generated C. Runtime errors are part of the output,
so an out-of-bounds index must fail the same way, on the same line, both ways. A program that differs
is saved as difftest-fail-<n>.rio. Generated C is built with -ffp-contract=off, so the C compiler
can't fuse a multiply and an add into one (differently rounded) step."""
import os, random, re, subprocess, sys, tempfile

class Gen:
  def __init__(self, rnd):
    self.r = rnd
    self.lines = []
    self.ints, self.floats = [], []
    self.loops = []           # loop counters: readable, never assigned
    self.procs = []           # (name, params) returning Int; params are Int/Float type names
    self.depth = 0
    self.n = 0

  def name(self, p):
    self.n += 1
    return f"{p}{self.n}"

  def emit(self, s):
    self.lines.append("  " * self.depth + s)

  def ival(self):
    return str(self.r.choice([0, 1, 2, 3, 7, -1, -5, 100, 65535, 2147483647, -2147483648 + 1]))

  def iexpr(self, d=0):
    r = self.r.random()
    if d > 3 or r < 0.25:
      return self.r.choice(self.ints) if self.ints and self.r.random() < 0.7 else self.ival()
    k = self.r.randrange(10)
    a, b = self.iexpr(d + 1), self.iexpr(d + 1)
    if k == 0: return f"({a} {self.r.choice(['+', '-', '*', '&', '|', '^'])} {b})"
    if k == 1: return f"({a} {self.r.choice(['/', '%'])} ({b} | 1))"
    if k == 2: return f"({a} {self.r.choice(['<<', '>>'])} {self.r.randrange(32)})"
    if k == 3: return f"{self.r.choice(['-', '~'])}{a}"
    if k == 4: return f"{self.r.choice(['min', 'max'])}({a}, {b})"
    if k == 5: return f"abs({a})"
    if k == 6: return f"({self.fexpr(d + 1)}).toInt()"
    if k == 7: return f"arr[abs({a}) % 8]" if self.r.random() < 0.85 else f"arr[({a}) % 8]"  # mostly in range
    if k == 8 and self.procs:
      pn, ps = self.r.choice(self.procs)
      return f"{pn}(" + ", ".join(self.iexpr(d + 1) if t == "Int" else self.fexpr(d + 1) for t in ps) + ")"
    return f"rec.x + {a}"

  def fexpr(self, d=0):
    r = self.r.random()
    if d > 3 or r < 0.25:
      if self.floats and self.r.random() < 0.7: return self.r.choice(self.floats)
      return self.r.choice(["0.0", "1.5", "-2.25", "1000.0", "0.1", "3.0"])
    k = self.r.randrange(6)
    a, b = self.fexpr(d + 1), self.fexpr(d + 1)
    if k == 0: return f"({a} {self.r.choice(['+', '-', '*'])} {b})"
    if k == 1: return f"({a} / ({b} + 0.5))"
    if k == 2: return f"sqrt(abs({a}))"
    if k == 3: return f"({self.iexpr(d + 1)}).toFloat()"
    if k == 4: return f"{self.r.choice(['floor', 'ceil', 'round'])}({a})"
    return f"rec.y * {a}"

  def bexpr(self, d=0):
    k = self.r.randrange(5)
    if d > 2 or k < 2: return f"({self.iexpr(d + 1)} {self.r.choice(['<', '<=', '==', '!=', '>', '>='])} {self.iexpr(d + 1)})"
    if k == 2: return f"({self.fexpr(d + 1)} < {self.fexpr(d + 1)})"
    if k == 3: return f"({self.bexpr(d + 1)} {self.r.choice(['&&', '||'])} {self.bexpr(d + 1)})"
    return f"!{self.bexpr(d + 1)}"

  def stmt(self, budget):
    k = self.r.randrange(12)
    if k <= 1:
      v = self.name("v"); self.emit(f"{v} := {self.iexpr()}"); self.ints.append(v)
    elif k == 2:
      v = self.name("f"); self.emit(f"{v} := {self.fexpr()}"); self.floats.append(v)
    elif k == 3 and [v for v in self.ints if v not in self.loops]:
      self.emit(f"{self.r.choice([v for v in self.ints if v not in self.loops])} {self.r.choice(['=', '+=', '-=', '*='])} {self.iexpr()}")
    elif k == 4:
      self.emit(f"arr[abs({self.iexpr()}) % 8] = {self.iexpr()}" if self.r.random() < 0.85 else f"arr[({self.iexpr()}) % 8] = {self.iexpr()}")
    elif k == 5:
      self.emit(f"rec.x = {self.iexpr()}" if self.r.random() < 0.5 else f"rec.y = {self.fexpr()}")
    elif k == 6 and budget > 1 and self.depth < 3:
      self.emit(f"if {self.bexpr()}"); self.block(budget // 2)
      if self.r.random() < 0.5: self.emit("else"); self.block(budget // 2)
      self.emit("end")
    elif k == 7 and budget > 1 and self.depth < 3:
      i = self.name("i")
      self.emit(f"for {i} in 0..<{self.r.randrange(4)}"); self.ints.append(i); self.loops.append(i)
      self.block(budget // 2); self.ints.remove(i); self.loops.remove(i); self.emit("end")
    elif k == 8:
      self.emit(f"switch Dir.fromInt((({self.iexpr()}) % 3 + 3) % 3)")
      for c in ["north", "east", "south"]:
        self.emit(f"case .{c}"); self.depth += 1; self.emit(f"log(\"{c}\", {self.iexpr()})"); self.depth -= 1
      self.emit("end")
    elif k == 9:
      self.emit(f"u = {self.iexpr()}" if self.r.random() < 0.5 else f"u = Pt{{x = {self.iexpr()}, y = {self.fexpr()}}}")
      self.emit("if u is Pt"); self.depth += 1; self.emit("log(\"pt\", u.x, u.y)"); self.depth -= 1
      self.emit("else if u is Int"); self.depth += 1; self.emit("log(\"int\", u)"); self.depth -= 1; self.emit("end")
    else:
      vals = [self.iexpr(), self.fexpr(), self.bexpr()]
      self.emit("log(" + ", ".join(self.r.sample(vals, self.r.randrange(1, 4))) + ")")

  def block(self, budget):
    self.depth += 1
    saved_i, saved_f = list(self.ints), list(self.floats)
    for _ in range(max(1, self.r.randrange(budget + 1))): self.stmt(budget)
    self.ints, self.floats = saved_i, saved_f
    self.depth -= 1

  def program(self):
    self.emit("Dir :: enum"); self.emit("  north, east, south"); self.emit("end")
    self.emit("Pt :: struct"); self.emit("  x: Int"); self.emit("  y: Float"); self.emit("end")
    self.emit("PU :: union"); self.emit("  nil"); self.emit("  Pt, Int"); self.emit("end")
    self.emit("arr: [8]Int"); self.emit("rec: Pt"); self.emit("u: PU")
    for _ in range(self.r.randrange(4)):
      pn = self.name("p"); ps = [self.r.choice(["Int", "Float"]) for _ in range(self.r.randrange(3))]
      args = [self.name("a") for _ in ps]
      self.emit(f"{pn} :: proc(" + ", ".join(f"{a}: {t}" for a, t in zip(args, ps)) + ") -> Int")
      outer_i, outer_f = self.ints, self.floats
      self.ints = [a for a, t in zip(args, ps) if t == "Int"]; self.floats = [a for a, t in zip(args, ps) if t == "Float"]
      self.depth += 1; self.emit(f"return {self.iexpr()}"); self.depth -= 1
      self.ints, self.floats = outer_i, outer_f
      self.emit("end"); self.procs.append((pn, ps))
    for _ in range(self.r.randrange(4, 14)): self.stmt(6)
    self.emit("log(arr[0], arr[7], rec.x, rec.y)")
    return "\n".join(self.lines) + "\n"

def run(cmd):
  try: p = subprocess.run(cmd, capture_output=True, text=True, timeout=20)
  except subprocess.TimeoutExpired: return "(hung)"
  out = p.stdout + p.stderr
  return re.sub(r"^.*?\.rio:(\d+)", r"\1", out, flags=re.M)  # the VM names the file, C doesn't

def main():
  rio, cc = sys.argv[1], sys.argv[2]
  count = int(sys.argv[3]) if len(sys.argv) > 3 else 200
  seed = int(sys.argv[4]) if len(sys.argv) > 4 else 1
  fails = skipped = 0
  with tempfile.TemporaryDirectory() as d:
    for n in range(count):
      src = Gen(random.Random(seed * 100003 + n)).program()
      rf, cf, ef = os.path.join(d, "p.rio"), os.path.join(d, "p.c"), os.path.join(d, "p.exe")
      open(rf, "w").write(src)
      vm = run([rio, rf])
      if re.search(r"^\d+:\d+: ", vm, re.M): skipped += 1; continue  # rejected when compiling (say, a constant index out of range)
      subprocess.run([rio, "-c", cf, rf], check=True)
      subprocess.run([cc, "-O1", "-ffp-contract=off", "-w", cf, "-o", ef, "-lm"], check=True)
      c = run([ef])
      if vm != c:
        fails += 1
        open(f"difftest-fail-{n}.rio", "w").write(src)
        print(f"program {n} differs:\n  vm: {vm[:300]!r}\n  c:  {c[:300]!r}")
  print(f"{count - fails - skipped} of {count - skipped} programs agree ({skipped} didn't compile, which is fine)")
  return 1 if fails else 0

if __name__ == "__main__":
  sys.exit(main())
