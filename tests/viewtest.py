#!/usr/bin/env python3
"""Views test: random rio programs that pass views of local and global memory around, to check the rules
that keep a view of a proc's own memory (or its caller's) from outliving the call.

  python3 tests/viewtest.py RIO [count] [seed]

Every array holds values that say whose memory it is: globals 1000 and up, a proc's own 2000 and up, and
nothing writes to them through a view. After each top-level statement, and each store into a global view,
the program logs the global views' elements. Views go into locals that are views, structs holding one
(V), arrays of them and unions of one, passed by value and by &, and through methods. Two kinds of
programs:

  unsafe  as safe, but one view, somewhere, goes anywhere: rio may reject them, but if it doesn't, no
          global view may read a value under 1000 or of 2000 and up (it would be viewing a proc's memory)
  safe    views only go where rio's rules let them, so rio must accept them (and they must log only
          global values too): a local holds the kinds of view it was last given in its own block (the
          one it's declared in), and in blocks inside that, only those kinds (a struct's field and an
          array's element named by a constant each their own); a global holds only views of global memory, and a
          return those or a parameter's; and a call's result views what its arguments do (the memory of
          & arguments and method receivers too, when there's an array or builder in it)

A program that fails is saved as viewtest-fail-<n>.rio."""
import os, random, re, subprocess, sys, tempfile

G, P, L = 1, 2, 4  # a view's kinds of memory: global, a parameter's (the caller's), the proc's own
ALL = G | P | L

def held(var):
  """the kinds of view a local holds, read as a whole"""
  return var["kinds"][0] | var["kinds"][1] if var["type"] == "A" else var["kinds"]

class Gen:
  def __init__(self, rnd, safe):
    self.r = rnd
    self.safe = safe
    self.lines = []
    self.procs = []  # (name, params as [(name, 'view' | 'ref' | 'V' | 'bool')], returns a view)
    self.depth = 0
    self.sinks = 0  # (places a view was stored or returned so far: an unsafe program's goes anywhere at one)
    self.leakAt = rnd.randrange(16)

  def emit(self, s):
    self.lines.append("  " * self.depth + s)

  def chance(self, p):
    return self.r.random() < p

  def sink(self, want):
    """the kinds of view a store or return may take: in an unsafe program, at one of them, any"""
    self.sinks += 1
    return ALL if not self.safe and self.sinks == self.leakAt else want

  def view(self, scope, want):
    """a view expression and its kinds, within want"""
    options = [(f"ga{k}[]", G) for k in range(3)] + [("ga0[1..]", G), ("gB.buf[]", G), ("gV.v", G), ("gV.get()", G), ("gB.view()", G)]
    options += [(f"gv{k}", G) for k in range(3)]
    if scope:
      options += [("la[]", L), ("la[1..]", L), ("lb.buf[]", L), ("lb.view()", L)]
      for p, t in scope["params"]:
        if t == "view": options.append((p, P))
        elif t == "ref": options += [(f"{p}.buf[]", P), (f"{p}.view()", P)]
        elif t == "V": options += [(f"{p}.v", P), (f"{p}.get()", P)]  # (a V has no memory of its own to view: what it holds)
      for var in scope["vars"] * 4:  # (locals most: views flow through them)
        v, t = var["name"], var["type"]
        if t == "view": options.append((v, held(var)))
        elif t == "V": options += [(f"{v}.v", held(var)), (f"{v}.get()", held(var))]
        elif t == "A": options += [(f"{v}[0]", var["kinds"][0]), (f"{v}[1]", var["kinds"][1])]
    options = [o for o in options if o[1] & ~want == 0]
    calls = [p for p in self.procs if p[2] in ("view", "V")]
    if calls and self.chance(0.25):
      proc = self.r.choice(calls)
      got = self.call(proc, scope, want)
      if got: return got[0] + (".v" if proc[2] == "V" else ""), got[1]
    return self.r.choice(options)

  def struct(self, scope, want):
    """a V expression and its kinds, within want; None if there's no way"""
    options = [("gV", G)]
    if scope:
      options += [(p, P) for p, t in scope["params"] if t == "V"]
      options += [(var["name"], held(var)) for var in scope["vars"] if var["type"] == "V"]
    options = [o for o in options if o[1] & ~want == 0]
    calls = [p for p in self.procs if p[2] == "V"]
    if calls and self.chance(0.2):
      got = self.call(self.r.choice(calls), scope, want)
      if got: return got
    if not options or self.chance(0.4):
      e, k = self.view(scope, want)
      return f"V{{v = {e}, n = 1}}", k
    return self.r.choice(options)

  def call(self, proc, scope, want):
    """a call of proc, its arguments within want; None if there's no way"""
    name, params, _ = proc
    args, kinds = [], 0
    for p, t in params:
      if t == "view":
        e, k = self.view(scope, want)
      elif t == "V":
        e, k = self.struct(scope, want)
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

  def store(self, scope):
    """a store into a local: in its own block, it now holds what it's given; inside that, only what it held"""
    var = self.r.choice(scope["vars"])
    v, t, own = var["name"], var["type"], self.depth == 1
    if t == "A":
      i = self.r.randrange(2)
      e, got = self.view(scope, self.sink(ALL if own else var["kinds"][i] | G))
      self.emit(f"{v}[{i}] = {e}")
      if own: var["kinds"][i] = got or G
      return
    want = self.sink(ALL if own else var["kinds"] | G)
    if t == "V" and self.chance(0.5):
      e, got = self.struct(scope, want)
      self.emit(f"{v} = {e}")
    elif t == "U" and self.chance(0.3):
      e, got = "7", G
      self.emit(f"{v} = 7")
    else:
      e, got = self.view(scope, want)
      self.emit(f"{v}{'.v' if t == 'V' else ''} = {e}")
    if own: var["kinds"] = got or G

  def logGlobal(self, g):
    self.emit(f"for x in {g} do log(x)")  # (a global may never view a proc's memory, even while it runs)

  def stmt(self, scope, budget):
    roll = self.r.random()
    if scope["vars"] and roll < 0.35:
      self.store(scope)
    elif roll < 0.5:
      e, _ = self.view(scope, self.sink(G))
      g = self.r.choice(["gv0", "gv1", "gv2", "gV.v"])
      self.emit(f"{g} = {e}")
      self.logGlobal(g)
    elif roll < 0.55:
      e, _ = self.struct(scope, self.sink(G))
      self.emit(f"gV = {e}")
      self.logGlobal("gV.v")
    elif roll < 0.6 and any(var["type"] == "U" for var in scope["vars"]):
      u = self.r.choice([var for var in scope["vars"] if var["type"] == "U"])
      if held(u) & ~self.sink(G) == 0:
        self.emit(f"if {u['name']} is []Int")
        self.emit(f"  gv0 = {u['name']}")
        self.depth += 1
        self.logGlobal("gv0")
        self.depth -= 1
        self.emit("end")
    elif roll < 0.72 and budget > 1:
      conds = ["gc"] + [p for p, t in scope["params"] if t == "bool"]
      self.emit(f"if {self.r.choice(conds)}")
      self.block(scope, budget - 1)
    elif roll < 0.84 and budget > 1:
      self.emit("for i in 0..<2")
      self.block(scope, budget - 1)
    elif self.procs:
      got = self.call(self.r.choice(self.procs), scope, ALL)
      if got: self.emit(got[0])
    else:
      self.emit("gc = not gc")

  def block(self, scope, budget):
    self.depth += 1
    for _ in range(self.r.randrange(1, 4)): self.stmt(scope, budget)
    self.depth -= 1
    self.emit("end")

  def declare(self, scope, i):
    """a local, holding the kinds of view it starts with"""
    t = self.r.choice(["view", "view", "V", "A", "U"])
    v = {"view": "v", "V": "s", "A": "arr", "U": "u"}[t] + str(i)
    if t == "A":
      self.emit(f"{v}: [2][]Int")
      kinds = []
      for k in range(2):
        e, got = self.view(scope, ALL)
        self.emit(f"{v}[{k}] = {e}")
        kinds.append(got or G)
    elif t == "V":
      e, got = self.struct(scope, ALL)
      self.emit(f"{v}: V = {e}")
      kinds = got or G
    else:
      calls = [p for p in self.procs if p[2] == "U"]
      got = self.call(self.r.choice(calls), scope, ALL) if t == "U" and calls and self.chance(0.4) else None
      e, got = got or self.view(scope, ALL)
      self.emit(f"{v} := {e}" if t == "view" else f"{v}: U = {e}")
      kinds = got or G
    scope["vars"].append({"name": v, "type": t, "kinds": kinds})

  def proc(self):
    name = f"p{len(self.procs)}"
    params = [(f"a{i}", self.r.choice(["view", "view", "ref", "V", "bool"])) for i in range(self.r.randrange(0, 4))]
    returns = self.r.choice(["view", "view", "V", "U"]) if self.chance(0.7) else None  # (what it returns: a view, or what holds one)
    types = {"view": "[]Int", "ref": "&B", "V": "V", "bool": "Bool", "U": "U"}
    self.emit(f"{name} :: proc(" + ", ".join(f"{p}: {types[t]}" for p, t in params) + ")" + (f" -> {types[returns]}" if returns else ""))
    self.depth += 1
    self.emit("la: [4]Int")
    self.emit(f"for i in 0..<4 do la[i] = {2000 + 10 * len(self.procs)} + i")
    self.emit("lb: B")
    self.emit(f"for i in 0..<4 do lb.buf[i] = {2500 + 10 * len(self.procs)} + i")
    scope = {"params": params, "vars": []}
    for i in range(self.r.randrange(1, 5)): self.declare(scope, i)
    for _ in range(self.r.randrange(1, 6)): self.stmt(scope, 3)
    if returns == "V": self.emit(f"return {self.struct(scope, self.sink(G | P))[0]}")
    elif returns == "U" and self.chance(0.2): self.emit("return 7")
    elif returns: self.emit(f"return {self.view(scope, self.sink(G | P))[0]}")
    self.depth -= 1
    self.emit("end")
    self.procs.append((name, params, returns))

  def program(self):
    for line in ["B :: struct", "  buf: [4]Int", "end", "V :: struct", "  v: []Int", "  n: Int", "end",
                 "V.get :: proc() -> []Int do return self.v", "B.view :: proc() -> []Int do return self.buf[]", "U :: union", "  []Int", "  Int", "end", "gc := false"]:
      self.emit(line)
    for k in range(3):
      self.emit(f"ga{k}: [4]Int")
      self.emit(f"for i in 0..<4 do ga{k}[i] = {1000 + 10 * k} + i")
    self.emit("gB: B")
    self.emit("for i in 0..<4 do gB.buf[i] = 1900 + i")
    for k in range(3): self.emit(f"gv{k} := ga{k}[]")
    self.emit("gV := V{v = ga1[], n = 0}")
    for _ in range(self.r.randrange(1, 6)): self.proc()
    for _ in range(self.r.randrange(2, 7)):
      roll = self.r.random()
      calls = [p for p in self.procs if p[2] in ("view", "V")]
      if calls and roll < 0.6:
        proc = self.r.choice(calls)
        got = self.call(proc, None, self.sink(G))
        if got and proc[2] == "V" and self.chance(0.5): self.emit(f"gV = {got[0]}")
        elif got: self.emit(f"gv{self.r.randrange(3)} = {got[0]}" + (".v" if proc[2] == "V" else ""))
      elif roll < 0.8:
        self.emit("gc = not gc")
      got = self.call(self.r.choice(self.procs), None, G)
      if got: self.emit(got[0])
      for g in ["gv0", "gv1", "gv2", "gV.v"]: self.logGlobal(g)
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
