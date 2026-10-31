#!/usr/bin/env python3
"""Rio's size and speed, to track over time: prints them, and with --record adds them as a row of
bench/stats.csv (record one with each batch of changes, so the file's history shows what each did).

  python tools/stats.py [--record] [--note "what changed"] [--rio path/to/rio] [--cc "zig cc"]   (--record
                        redraws bench/stats.svg too)
                        [--root other/checkout --commit abc1234 --date 2026-10-09]   (an older tree: its sources)
  python tools/stats.py --backfill abc1234,def5678     (build each commit in a worktree, measure it, and add
                                                        its row to stats.csv in commit order; then --svg)
  python tools/stats.py --svg [--check]                (draw stats.csv as bench/stats.svg; --check: fail if the
                                                        checked-in one differs, as CI does)

  arm_vm, arm_compiler   bytes of code and data for the VM (common, image, runtime, vm) and the compiler
                         (the rest, jit.c too), built for the Fruit Jam's Cortex-M33 as Rinc builds them
                         (-O3, rio_SMALL); needs arm-none-eabi-gcc (the Pico SDK's, else on PATH)
  src_lines              lines of C in src
  rio_exe                bytes of the rio program given (this computer's build)
  test_img               bytes of tests/test.rio compiled to an image (how dense the code is)
  compile_ms             compiling tests/test.rio to an image, best of 5
  <bench>_vm, <bench>_c  each bench/*.rio's own timing (seconds, best of 3) in the VM, and as C (rio -c,
                         then the C compiler at -O2; left empty with no C compiler)

Times depend on the computer: the machine column says which, so compare rows from the same one. A
measure an older tree can't take (a flag it lacks, a file it doesn't have) is left empty."""
import csv, datetime, glob, io, math, os, platform, shutil, subprocess, sys, tempfile, time

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
STATS = os.path.join(HERE, "bench", "stats.csv")
SVG = os.path.join(HERE, "bench", "stats.svg")
VM = ["common", "image", "runtime", "vm"]
COMPILER = ["builtins", "codegen", "compile", "decls", "expr", "host", "lexer", "modules", "operators", "repl",
            "statements", "tables", "jit"]
ARM_FLAGS = ["-mcpu=cortex-m33", "-mthumb", "-march=armv8-m.main+fp+dsp", "-mfloat-abi=softfp", "-O3", "-DNDEBUG",
             "-Drio_SMALL", "-Drio_MAX_FFI=256"]

def arg(name, default=None):
  return sys.argv[sys.argv.index(name) + 1] if name in sys.argv else default

def git(*args, root=HERE):
  return subprocess.run(["git", "-C", root, *args], capture_output=True, text=True).stdout.strip()

def armTool(name):
  found = sorted(glob.glob(os.path.expanduser(f"~/.pico-sdk/toolchain/*/bin/arm-none-eabi-{name}*")))
  return found[-1] if found else shutil.which(f"arm-none-eabi-{name}")

def armSizes(root, d):
  gcc, size = armTool("gcc"), armTool("size")
  if not gcc or not size: return "", ""
  if not all(os.path.exists(os.path.join(root, "src", n + ".c")) for n in VM): return "", ""  # (an older tree, all one rio.c: no split)
  def group(names):
    total = 0
    for n in names:
      src = os.path.join(root, "src", n + ".c")
      if not os.path.exists(src): continue  # (an older tree, before it had this file)
      obj = os.path.join(d, n + ".o")
      subprocess.run([gcc, *ARM_FLAGS, "-I", os.path.join(root, "src"), "-c", src, "-o", obj], check=True)
      text, data = subprocess.run([size, obj], capture_output=True, text=True, check=True).stdout.splitlines()[1].split()[:2]
      total += int(text) + int(data)
    return total
  return group(VM), group(COMPILER)

def best(times, f):
  return min(f() for _ in range(times))

def timed(cmd):
  t = time.perf_counter()
  subprocess.run(cmd, check=True, capture_output=True)
  return time.perf_counter() - t

def benchTime(cmd):
  """the time a bench prints last (its own clock)"""
  out = subprocess.run(cmd, check=True, capture_output=True, text=True).stdout.split()
  return float(out[-1])

def tried(f):
  """f's measure, or empty when this tree can't take it"""
  try:
    return f()
  except (subprocess.CalledProcessError, OSError, ValueError, IndexError):
    return ""

def defaultRio(root):
  return os.path.join(root, "build-win", "Release", "rio.exe") if os.name == "nt" else os.path.join(root, "build", "rio")

def measure(root, rio, cc, commit, date, note):
  row = {"date": date, "commit": commit, "machine": platform.node()}
  with tempfile.TemporaryDirectory() as d:
    row["arm_vm"], row["arm_compiler"] = tried(lambda: armSizes(root, d)) or ("", "")
    row["src_lines"] = sum(len(open(p, encoding="utf-8", errors="replace").readlines()) for p in glob.glob(os.path.join(root, "src", "*.[ch]")))
    row["rio_exe"] = os.path.getsize(rio)
    img = os.path.join(d, "test.img")
    test = os.path.join(root, "tests", "test.rio")
    row["compile_ms"] = tried(lambda: round(best(5, lambda: timed([rio, "-i", img, test])) * 1000, 1))
    row["test_img"] = os.path.getsize(img) if os.path.exists(img) else ""
    for b in sorted(glob.glob(os.path.join(root, "bench", "*.rio"))):
      name = os.path.splitext(os.path.basename(b))[0]
      row[name + "_vm"] = tried(lambda: best(3, lambda: benchTime([rio, b])))
      row[name + "_c"] = ""
      if cc:
        c, exe = os.path.join(d, name + ".c"), os.path.join(d, name + ".exe")
        def native():
          subprocess.run([rio, "-c", c, b], check=True, capture_output=True)
          subprocess.run([*cc, "-O2", "-w", c, "-o", exe, *([] if os.name == "nt" else ["-lm"])], check=True, capture_output=True)
          return best(3, lambda: benchTime([exe]))
        row[name + "_c"] = tried(native)
  row["note"] = note
  return row

# ------------------------------------------------------------------ the csv

def readStats():
  if not os.path.exists(STATS): return [], []
  with open(STATS, encoding="utf-8", newline="") as f:
    r = csv.DictReader(f)
    return list(r.fieldnames), list(r)

def writeStats(head, rows):
  out = io.StringIO()
  out.write(",".join(head) + "\n")
  for row in rows:
    out.write(",".join('"%s"' % row.get(k, "") if k == "note" else str(row.get(k, "")) for k in head) + "\n")
  with open(STATS, "w", encoding="utf-8", newline="") as f:
    f.write(out.getvalue())

def commitTime(row):
  """when the row's commit was made (+: changes on top of it, so just after), for putting rows in order"""
  c = row["commit"]
  t = git("show", "-s", "--format=%ct", c.rstrip("+"))
  return (int(t) if t.isdigit() else 0) + (0.5 if c.endswith("+") else 0)

def record(row):
  head, rows = readStats()
  if not head: head = list(row)
  rows.append(row)
  writeStats(head, rows)
  print("recorded in bench/stats.csv")

def backfill(commits, cc):
  """each commit, built in a worktree of its own and measured, its row put in order among the rest"""
  head, rows = readStats()
  for c in commits:
    with tempfile.TemporaryDirectory() as d:
      tree = os.path.join(d, "tree")
      subprocess.run(["git", "-C", HERE, "worktree", "add", "--detach", tree, c], check=True, capture_output=True)
      try:
        build = os.path.join(tree, "build-win" if os.name == "nt" else "build")
        print(c, "building", flush=True)
        subprocess.run(["cmake", "-S", tree, "-B", build, "-DCMAKE_BUILD_TYPE=Release"], check=True, capture_output=True)
        subprocess.run(["cmake", "--build", build, "--config", "Release", "--target", "rio"], check=True, capture_output=True)
        short, date, subject = git("show", "-s", "--format=%h|%ad|%s", "--date=short", c).split("|", 2)
        row = measure(tree, defaultRio(tree), cc, short, date, "backfilled: " + subject)
        print(c, {k: v for k, v in row.items() if k != "note"}, flush=True)
        rows.append(row)
      except subprocess.CalledProcessError as e:
        print(c, "failed:", " ".join(e.cmd[:3]), (e.stdout or "")[-300:], flush=True)
      finally:
        subprocess.run(["git", "-C", HERE, "worktree", "remove", "--force", tree], capture_output=True)
    rows.sort(key=commitTime)  # (stable: rows of one commit keep their order)
    writeStats(head, rows)

# ------------------------------------------------------------------ the charts

# light and dark: the surface, inks, hairlines, and four series hues (validated as a set for each)
STYLE = """
svg { --surface: #fcfcfb; --ink: #0b0b0b; --ink2: #52514e; --muted: #898781; --grid: #e1e0d9; --axis: #c3c2b7;
  --s1: #2a78d6; --s2: #eb6834; --s3: #1baf7a; --s4: #eda100; }
@media (prefers-color-scheme: dark) {
  svg { --surface: #1a1a19; --ink: #f0efec; --ink2: #c3c2b7; --muted: #898781; --grid: #2c2c2a; --axis: #383835;
    --s1: #3987e5; --s2: #d95926; --s3: #199e70; --s4: #c98500; }
}
text { font-family: system-ui, -apple-system, "Segoe UI", sans-serif; fill: var(--ink2); }
.title { font-size: 13px; font-weight: 600; fill: var(--ink); }
.tick { font-size: 10px; fill: var(--muted); font-variant-numeric: tabular-nums; }
.label { font-size: 11px; }
.grid { stroke: var(--grid); stroke-width: 1; }
.axis { stroke: var(--axis); stroke-width: 1; }
.line { fill: none; stroke-width: 2; stroke-linejoin: round; stroke-linecap: round; }
.hit { fill: transparent; }
"""

# (title, unit, columns, scale): each panel one measure, one axis
PANELS = [
  ("The VM on the board", "KB", ["arm_vm"], 1 / 1024),
  ("The compiler on the board", "KB", ["arm_compiler"], 1 / 1024),
  ("Lines of C in src", "lines", ["src_lines"], 1),
  ("tests/test.rio as an image", "KB", ["test_img"], 1 / 1024),
  ("Compiling tests/test.rio", "ms", ["compile_ms"], 1),
  ("rio.exe on this machine", "KB", ["rio_exe"], 1 / 1024),
  ("Benches in the VM", "seconds", None, 1),       # (None: every <bench>_vm column)
  ("Benches as C", "milliseconds", None, 1000),    # (every <bench>_c)
]
W, H, COLS, GAP = 400, 200, 2, 20
LEFT, RIGHT, TOP, BOTTOM = 44, 64, 46, 24

def niceTicks(lo, hi, n=4):
  """about n round ticks covering lo..hi"""
  if hi <= lo: hi = lo + 1
  raw = (hi - lo) / n
  mag = 10 ** math.floor(math.log10(raw))
  step = next(s * mag for s in (1, 2, 2.5, 5, 10) if s * mag >= raw)
  start = math.floor(lo / step) * step
  ticks = []
  t = start
  while t <= hi + step * 1e-9:
    ticks.append(round(t, 10))
    t += step
  if ticks[-1] < hi: ticks.append(round(ticks[-1] + step, 10))
  return ticks

def fmt(v):
  return f"{v:,.0f}" if abs(v) >= 100 else f"{v:.3g}"

def esc(s):
  return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace('"', "&quot;")

def num(s):
  try:
    return float(s)
  except ValueError:
    return None

def panel(rows, title, unit, cols, scale, ox, oy):
  out = []
  pw, ph = W - LEFT - RIGHT, H - TOP - BOTTOM
  series = [(c, [None if num(r.get(c, "")) is None else num(r[c]) * scale for r in rows]) for c in cols]
  values = [v for _, vs in series for v in vs if v is not None]
  out.append(f'<g transform="translate({ox},{oy})">')
  out.append(f'<text class="title" x="0" y="16">{esc(title)}</text>')
  out.append(f'<text class="label" x="0" y="32">{esc(unit)}</text>')
  if not values:
    out.append("</g>")
    return out
  ticks = niceTicks(min(values), max(values))
  lo, hi = ticks[0], ticks[-1]
  n = len(rows)
  x = lambda i: LEFT + (pw * i / (n - 1) if n > 1 else pw / 2)
  y = lambda v: TOP + ph - (v - lo) / (hi - lo) * ph
  for t in ticks:
    cls = "axis" if t == lo else "grid"
    out.append(f'<line class="{cls}" x1="{LEFT}" x2="{LEFT + pw}" y1="{y(t):.1f}" y2="{y(t):.1f}"/>')
    out.append(f'<text class="tick" x="{LEFT - 6}" y="{y(t) + 3:.1f}" text-anchor="end">{fmt(t)}</text>')
  last = -99  # (dates where they change, kept apart)
  for i, r in enumerate(rows):
    if (i == 0 or r["date"] != rows[i - 1]["date"]) and x(i) - last >= 44:
      d = datetime.date.fromisoformat(r["date"])
      out.append(f'<text class="tick" x="{x(i):.1f}" y="{TOP + ph + 15}" text-anchor="middle">{d:%b} {d.day}</text>')
      last = x(i)
  ends = []
  for k, (c, vs) in enumerate(series):
    color = f"var(--s{k + 1})"
    path, pen = [], "M"
    for i, v in enumerate(vs):
      if v is None:
        pen = "M"
        continue
      path.append(f"{pen}{x(i):.1f},{y(v):.1f}")
      pen = "L"
    out.append(f'<path class="line" stroke="{color}" d="{" ".join(path)}"/>')
    other = rows[-1]["machine"]
    for i, v in enumerate(vs):
      if v is not None and rows[i]["machine"] != other:  # (another machine's: its times aren't comparable)
        out.append(f'<circle cx="{x(i):.1f}" cy="{y(v):.1f}" r="4" fill="var(--surface)" stroke="{color}" stroke-width="2"/>')
    i = max((i for i, v in enumerate(vs) if v is not None), default=None)
    if i is not None:
      out.append(f'<circle cx="{x(i):.1f}" cy="{y(vs[i]):.1f}" r="4" fill="{color}" stroke="var(--surface)" stroke-width="2"/>')
      name = c.rsplit("_", 1)[0] if len(series) > 1 else fmt(vs[i])
      ends.append([y(vs[i]), name])
    for i, v in enumerate(vs):  # (hover, where the svg is opened as a page: each row's value)
      if v is not None:
        r = rows[i]
        out.append(f'<circle class="hit" cx="{x(i):.1f}" cy="{y(v):.1f}" r="6"><title>{esc(r["date"])} {esc(r["commit"])}'
                   f'{" " + esc(c.rsplit("_", 1)[0]) if len(series) > 1 else ""}: {fmt(v)} {esc(unit)}'
                   f'{chr(10) + esc(r["note"][:120]) if r.get("note") else ""}</title></circle>')
  ends.sort()
  for j in range(1, len(ends)):  # (end labels kept from overlapping, and inside the plot)
    ends[j][0] = max(ends[j][0], ends[j - 1][0] + 12)
  if ends and ends[-1][0] > TOP + ph:
    ends[-1][0] = TOP + ph
    for j in range(len(ends) - 2, -1, -1):
      ends[j][0] = min(ends[j][0], ends[j + 1][0] - 12)
  if len(series) > 1:  # (a legend too, beside the unit)
    lx = 80
    for k, (c, _) in enumerate(series):
      name = c.rsplit("_", 1)[0]
      out.append(f'<line x1="{lx}" x2="{lx + 14}" y1="28" y2="28" stroke="var(--s{k + 1})" stroke-width="2" stroke-linecap="round"/>')
      out.append(f'<text class="label" x="{lx + 18}" y="32">{esc(name)}</text>')
      lx += 26 + 7 * len(name)
  for ly, name in ends:
    out.append(f'<text class="label" x="{LEFT + pw + 8}" y="{ly + 4:.1f}">{esc(name)}</text>')
  out.append("</g>")
  return out

def drawSvg():
  head, rows = readStats()
  rowsN = (len(PANELS) + COLS - 1) // COLS
  width, height = COLS * W + (COLS - 1) * GAP, rowsN * H + (rowsN - 1) * GAP
  machines = sorted({r["machine"] for r in rows})
  out = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height + 24}" width="{width}" height="{height + 24}" '
         f'role="img" aria-label="Rio size and speed over time, from bench/stats.csv">',
         f"<style>{STYLE}</style>", f'<rect width="100%" height="100%" rx="8" fill="var(--surface)"/>']
  for k, (title, unit, cols, scale) in enumerate(PANELS):
    if cols is None:  # (the benches: each name with both a _vm and a _c column)
      suffix = "_vm" if "VM" in title else "_c"
      cols = [h for h in head if h.endswith(suffix) and h[:-len(suffix)] + ("_c" if suffix == "_vm" else "_vm") in head]
    ox, oy = (k % COLS) * (W + GAP), (k // COLS) * (H + GAP)
    out += panel(rows, title, unit, cols, scale, ox + 8, oy + 4)
  note = (f"{len(rows)} rows of bench/stats.csv, {rows[0]['date']} to {rows[-1]['date']}, timed on "
          f"{', '.join(machines)}" + (" (a hollow point: another machine than the last row's)" if len(machines) > 1 else "")
          if rows else "no rows yet")
  out.append(f'<text class="tick" x="8" y="{height + 16}">{esc(note)}</text>')
  out.append("</svg>")
  return "\n".join(out) + "\n"

def main():
  cc = (arg("--cc") or os.environ.get("CC") or ("zig cc" if shutil.which("zig") else "cc" if shutil.which("cc") else "")).split()
  if "--backfill" in sys.argv:
    backfill(arg("--backfill").split(","), cc)
    sys.argv.append("--svg")
  if "--svg" in sys.argv:
    svg = drawSvg()
    old = open(SVG, encoding="utf-8").read() if os.path.exists(SVG) else ""
    if "--check" in sys.argv:
      if svg != old: sys.exit("bench/stats.svg is stale: run tools/stats.py --svg")
      print("bench/stats.svg is current")
    else:
      with open(SVG, "w", encoding="utf-8", newline="\n") as f: f.write(svg)
      print("drew bench/stats.svg")
    return
  root = arg("--root", HERE)
  rio = arg("--rio", defaultRio(root))
  dirty = git("status", "--porcelain", "--", "src", "tests", "bench", root=root)
  commit = arg("--commit") or git("rev-parse", "--short", "HEAD", root=root) + ("+" if dirty else "")  # (+: on top of it, with uncommitted changes)
  row = measure(root, rio, cc, commit, arg("--date", datetime.date.today().isoformat()), arg("--note", ""))
  for k, v in row.items(): print(f"{k:14} {v}")
  if "--record" in sys.argv:
    record(row)
    with open(SVG, "w", encoding="utf-8", newline="\n") as f: f.write(drawSvg())
    print("drew bench/stats.svg")

if __name__ == "__main__":
  main()
