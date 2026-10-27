#!/usr/bin/env python3
"""Rio's size and speed, to track over time: prints them, and with --record adds them as a row of
bench/stats.csv (record one with each batch of changes, so the file's history shows what each did).

  python tools/stats.py [--record] [--note "what changed"] [--rio path/to/rio] [--cc "zig cc"]
                        [--root other/checkout --commit abc1234]   (an older tree, exported: its sources)

  arm_vm, arm_compiler   bytes of code and data for the VM (common, image, runtime, vm) and the compiler
                         (the rest, jit.c too), built for the Fruit Jam's Cortex-M33 as Rinc builds them
                         (-O3, rio_SMALL); needs arm-none-eabi-gcc (the Pico SDK's, else on PATH)
  src_lines              lines of C in src
  rio_exe                bytes of the rio program given (this computer's build)
  test_img               bytes of tests/test.rio compiled to an image (how dense the code is)
  compile_ms             compiling tests/test.rio to an image, best of 5
  <bench>_vm, <bench>_c  each bench/*.rio's own timing (seconds, best of 3) in the VM, and as C (rio -c,
                         then the C compiler at -O2; left empty with no C compiler)

Times depend on the computer: the machine column says which, so compare rows from the same one."""
import datetime, glob, os, platform, re, shutil, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VM = ["common", "image", "runtime", "vm"]
COMPILER = ["builtins", "codegen", "compile", "decls", "expr", "host", "lexer", "modules", "operators", "repl",
            "statements", "tables", "jit"]
ARM_FLAGS = ["-mcpu=cortex-m33", "-mthumb", "-march=armv8-m.main+fp+dsp", "-mfloat-abi=softfp", "-O3", "-DNDEBUG",
             "-Drio_SMALL", "-Drio_MAX_FFI=256"]

def arg(name, default=None):
  return sys.argv[sys.argv.index(name) + 1] if name in sys.argv else default

def armTool(name):
  found = sorted(glob.glob(os.path.expanduser(f"~/.pico-sdk/toolchain/*/bin/arm-none-eabi-{name}*")))
  return found[-1] if found else shutil.which(f"arm-none-eabi-{name}")

def armSizes(d):
  gcc, size = armTool("gcc"), armTool("size")
  if not gcc or not size: return "", ""
  def group(names):
    total = 0
    for n in names:
      obj = os.path.join(d, n + ".o")
      subprocess.run([gcc, *ARM_FLAGS, "-I", os.path.join(ROOT, "src"), "-c", os.path.join(ROOT, "src", n + ".c"), "-o", obj],
                     check=True)
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

def main():
  global ROOT
  ROOT = arg("--root", ROOT)
  rio = arg("--rio", os.path.join(ROOT, "build-win", "Release", "rio.exe") if os.name == "nt" else os.path.join(ROOT, "build", "rio"))
  cc = (arg("--cc") or os.environ.get("CC") or ("zig cc" if shutil.which("zig") else "cc" if shutil.which("cc") else "")).split()
  row = {}
  head = subprocess.run(["git", "-C", ROOT, "rev-parse", "--short", "HEAD"], capture_output=True, text=True).stdout.strip()
  dirty = subprocess.run(["git", "-C", ROOT, "status", "--porcelain", "--", "src", "tests", "bench"], capture_output=True, text=True).stdout.strip()
  row["date"] = datetime.date.today().isoformat()
  row["commit"] = arg("--commit") or head + ("+" if dirty else "")  # (+: on top of it, with uncommitted changes)
  row["machine"] = platform.node()
  with tempfile.TemporaryDirectory() as d:
    row["arm_vm"], row["arm_compiler"] = armSizes(d)
    row["src_lines"] = sum(len(open(p, encoding="utf-8").readlines()) for p in glob.glob(os.path.join(ROOT, "src", "*.[ch]")))
    row["rio_exe"] = os.path.getsize(rio)
    img = os.path.join(d, "test.img")
    test = os.path.join(ROOT, "tests", "test.rio")
    row["compile_ms"] = round(best(5, lambda: timed([rio, "-i", img, test])) * 1000, 1)
    row["test_img"] = os.path.getsize(img)
    for b in sorted(glob.glob(os.path.join(ROOT, "bench", "*.rio"))):
      name = os.path.splitext(os.path.basename(b))[0]
      row[name + "_vm"] = best(3, lambda: benchTime([rio, b]))
      row[name + "_c"] = ""
      if cc:
        c, exe = os.path.join(d, name + ".c"), os.path.join(d, name + ".exe")
        subprocess.run([rio, "-c", c, b], check=True)
        subprocess.run([*cc, "-O2", "-w", c, "-o", exe, *([] if os.name == "nt" else ["-lm"])], check=True)
        row[name + "_c"] = best(3, lambda: benchTime([exe]))
  row["note"] = arg("--note", "")
  for k, v in row.items(): print(f"{k:14} {v}")
  if "--record" in sys.argv:
    path = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "bench", "stats.csv")
    fresh = not os.path.exists(path)
    with open(path, "a", encoding="utf-8", newline="") as f:
      if fresh: f.write(",".join(row) + "\n")
      f.write(",".join('"%s"' % v if k == "note" else str(v) for k, v in row.items()) + "\n")
    print("recorded in bench/stats.csv")

if __name__ == "__main__":
  main()
