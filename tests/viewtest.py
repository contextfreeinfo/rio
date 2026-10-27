#!/usr/bin/env python3
"""Views test: random rio programs that pass views of local and global memory around, to check the rules
that keep a view of a proc's own memory (or its caller's) from outliving the call.

  python3 tests/viewtest.py RIO [count] [seed]

Every array holds values that say whose memory it is: globals 1000 and up, a proc's own 2000 and up, and
nothing writes to them through a view. After each top-level statement, and each store into a global view,
the program logs the global views' elements. Two kinds of programs:

  unsafe  as safe, but one view, somewhere, goes anywhere: rio may reject them, but if it doesn't, no
          global view may read a value under 1000 or of 2000 and up (it would be viewing a proc's memory)
  safe    views only go where rio's rules let them, so rio must accept them (and they must log only
          global values too): a local holds the kinds of view it was last given in its own block (the
          one it's declared in), and in blocks inside that, only those kinds; a global holds only views
          of global memory, and a return those or a parameter's; and a call's result views what its
          arguments do (the memory of & arguments too)

A program that fails is saved as viewtest-fail-<n>.rio."""
import os, random, re, subprocess, sys, tempfile

G, P, L = 1, 2, 4  # a view's kinds of memory: global, a parameter's (the caller's), the proc's own

class Gen:
  def __init__(self, rnd, safe):
    self.r = rnd
    self.safe = safe
    self.lines = []
    self.procs = []  # (name, params as [(name, 'view' | 'ref' | 'bool')], returns a view)
    self.depth = 0
    self.sinks = 0  # (places a view was stored or returned so far: an unsafe program's goes anywhere at one)
    self.leakAt = rnd.randrange(12)

  def emit(self, s):
    self.lines.append("  " * self.depth + s)

  def chance(self, p):
    return self.r.random() < p

  def sink(self, want):
    """the kinds of view a store or return may take: in an unsafe program, at one of them, any"""
    self.sinks += 1
    return G | P | L if not self.safe and self.sinks == self.leakAt else want

  def view(self, scope, want):
    """a view expression and its kinds, within want (all kinds in unsafe programs)"""
    options = [(f"ga{k}[]", G) for k in range(3)] + [("ga0[1..]", G), ("gB.buf[]", G)] + [(f"gv{k}", G) for k in range(3)]
    if scope:
      options += [("la[]", L), ("la[1..]", L), ("lb.buf[]", L)]
      options += [(p, P) for p, t in scope["params"] if t == "view"]
      options += [(f"{p}.buf[]", P) for p, t in scope["params"] if t == "ref"]
      options += [(v, k) for v, k in scope["vars"]] * 4  # (locals most: views flow through them)
    options = [o for o in options if o[1] & ~want == 0]
    calls = [p for p in self.procs if p[2]]
    if calls and self.chance(0.25):
      got = self.call(self.r.choice(calls), scope, want)
      if got: return got
    return self.r.choice(options)

  def call(self, proc, scope, want):
    """a call of proc, its arguments within want; None if there's no way"""
    name, params, _ = proc
    args, kinds = [], 0
    for p, t in params:
      if t == "view":
        e, k = self.view(scope, want)
      elif t == "ref":
        refs = [("&gB", G)] + ([("&lb", L)] if scope else [])
        refs = [x for x in refs if x[1] & ~want == 0]
        if not refs: return None
        e, k = self.r.choice(refs)
      else:
        e, k = self.r.choice(["true", "false", "gc"] + ([f"not {p}" for p, t in scope["params"] if t == "bool"] if scope else [])), 0
      args.append(e)
      kinds |= k
    if kinds & ~want: return None
    return f"{name}({', '.join(args)})", kinds

  def stmt(self, scope, budget):
    roll = self.r.random()
    if scope["vars"] and roll < 0.35:
      i = self.r.randrange(len(scope["vars"]))
      v, k = scope["vars"][i]
      own = self.depth == 1  # (in its own block, where it was declared: it now holds what it's given)
      e, got = self.view(scope, self.sink(G | P | L if own else k))
      self.emit(f"{v} = {e}")
      if own: scope["vars"][i] = (v, got or G)
    elif roll < 0.55:
      e, _ = self.view(scope, self.sink(G))
      k = self.r.randrange(3)
      self.emit(f"gv{k} = {e}")
      self.emit(f"for x in gv{k} do log(x)")  # (a global may never view a proc's memory, even while it runs)
    elif roll < 0.7 and budget > 1:
      conds = ["gc"] + [p for p, t in scope["params"] if t == "bool"]
      self.emit(f"if {self.r.choice(conds)}")
      self.block(scope, budget - 1)
    elif roll < 0.82 and budget > 1:
      self.emit("for i in 0..<2")
      self.block(scope, budget - 1)
    elif self.procs:
      got = self.call(self.r.choice(self.procs), scope, G | P | L)
      if got: self.emit(got[0])
    else:
      self.emit("gc = not gc")

  def block(self, scope, budget):
    self.depth += 1
    for _ in range(self.r.randrange(1, 4)): self.stmt(scope, budget)
    self.depth -= 1
    self.emit("end")

  def proc(self):
    name = f"p{len(self.procs)}"
    params = [(f"a{i}", self.r.choice(["view", "view", "ref", "bool"])) for i in range(self.r.randrange(0, 4))]
    returns = self.chance(0.7)
    types = {"view": "[]Int", "ref": "&B", "bool": "Bool"}
    self.emit(f"{name} :: proc(" + ", ".join(f"{p}: {types[t]}" for p, t in params) + ")" + (" -> []Int" if returns else ""))
    self.depth += 1
    self.emit("la: [4]Int")
    self.emit(f"for i in 0..<4 do la[i] = {2000 + 10 * len(self.procs)} + i")
    self.emit("lb: B")
    self.emit(f"for i in 0..<4 do lb.buf[i] = {2500 + 10 * len(self.procs)} + i")
    scope = {"params": params, "vars": []}
    for i in range(self.r.randrange(1, 4)):  # locals, each holding the kinds of view it starts with
      e, k = self.view(scope, G | P | L)
      if not k: k = G
      v = f"v{i}"
      self.emit(f"{v} := {e}")
      scope["vars"].append((v, k))
    for _ in range(self.r.randrange(1, 6)): self.stmt(scope, 3)
    if returns: self.emit(f"return {self.view(scope, self.sink(G | P))[0]}")
    self.depth -= 1
    self.emit("end")
    self.procs.append((name, params, returns))

  def program(self):
    self.emit("B :: struct")
    self.emit("  buf: [4]Int")
    self.emit("end")
    self.emit("gc := false")
    for k in range(3):
      self.emit(f"ga{k}: [4]Int")
      self.emit(f"for i in 0..<4 do ga{k}[i] = {1000 + 10 * k} + i")
    self.emit("gB: B")
    self.emit("for i in 0..<4 do gB.buf[i] = 1900 + i")
    for k in range(3): self.emit(f"gv{k} := ga{k}[]")
    for _ in range(self.r.randrange(1, 6)): self.proc()
    for _ in range(self.r.randrange(2, 7)):
      roll = self.r.random()
      calls = [p for p in self.procs if p[2]]
      if calls and roll < 0.6:
        got = self.call(self.r.choice(calls), None, self.sink(G))
        if got: self.emit(f"gv{self.r.randrange(3)} = {got[0]}")
      elif roll < 0.8:
        self.emit("gc = not gc")
      got = self.call(self.r.choice(self.procs), None, G)
      if got: self.emit(got[0])
      for k in range(3): self.emit(f"for x in gv{k} do log(x)")
    return "\n".join(self.lines) + "\n"

def run(cmd):
  try: p = subprocess.run(cmd, capture_output=True, text=True, timeout=20)
  except subprocess.TimeoutExpired: return "(hung)"
  return p.stdout + p.stderr

def main():
  rio = sys.argv[1]
  count = int(sys.argv[2]) if len(sys.argv) > 2 else 400
  seed = int(sys.argv[3]) if len(sys.argv) > 3 else 1
  fails = rejected = unsafe = 0
  with tempfile.TemporaryDirectory() as d:
    for n in range(count):
      safe = n % 2 == 1
      unsafe += not safe
      src = Gen(random.Random(seed * 100003 + n), safe).program()
      rf = os.path.join(d, "p.rio")
      open(rf, "w").write(src)
      out = run([rio, rf])
      error = re.search(r"^.*?\.rio:\d+:\d+: .*", out, re.M)
      why = None
      if error and safe: why = "rejected, but every view in it is safe: " + error.group(0)
      elif error: rejected += 1; continue
      elif re.search(r"runtime error|\(hung\)", out): why = "failed when run: " + out.strip()[-300:]
      else:
        bad = [v for v in re.findall(r"^-?\d+$", out, re.M) if not 1000 <= int(v) < 2000]
        if bad: why = f"a global view read {bad[0]}: a proc's own memory"
      if why:
        fails += 1
        open(f"viewtest-fail-{n}.rio", "w").write(src)
        print(f"program {n} ({'safe' if safe else 'unsafe'}): {why}")
  print(f"{count - fails} of {count} programs pass ({rejected} of the {unsafe} unsafe ones rejected)")
  return 1 if fails else 0

if __name__ == "__main__":
  sys.exit(main())
