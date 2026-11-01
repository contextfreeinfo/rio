#!/usr/bin/env python3
"""rio fmt, tried hard on real code: that it never changes what a program means, and settles.

  python tools/fmtcheck.py path/to/rio [--rounds 3] [--seed 1] [file.rio or dir]...   (default: bench, src
                                                                                       and tests, here)

For each file: rio fmt must leave it as it is if it's laid out already (else it's listed, as rio fmt -l
would), and formatting it must settle (twice is the same as once). Then each is scrambled, rounds
times: every line's indent changed, and the spaces between its tokens (only where Rio's tokens stay the
same), and also flattened (every indent gone). Each must format without rio fmt refusing it, settle,
and come back to how the original formats, except where rio fmt keeps what it's given (a comment's
column, a line going on, a run of spaces lining up); those lines are counted, not failed. A file rio
compiles on its own must compile, scrambled and formatted, to the same image, byte for byte. The
scrambled files are written beside the originals (so their imports and includes still work), and
removed after."""
import argparse, os, random, re, subprocess, sys

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOKEN = re.compile(r"""
   (?P<comment>\#.*)
 | (?P<string>"(?:\\.|[^"\\])*("|$))
 | (?P<char>'(?:\\.|[^'\\])*')
 | (?P<number>\d\w*(\.\d\w*)?)
 | (?P<name>[A-Za-z_]\w*)
 | (?P<op>>>>=|\.\.<|\.\.=|>>>|<<=|>>=|::|:=|==|!=|<=|>=|&&|\|\||<<|>>|->|\+=|-=|\*=|/=|%=|&=|\|=|\^=|.)
""", re.X)
LONE = set("()[]{},")  # (punctuation that never joins its neighbor: a gap beside it can go)

def scramble(text, rnd):
  out = []
  for line in text.split("\n"):
    cr = "\r" if line.endswith("\r") else ""
    body = line[:-1] if cr else line
    stripped = body.strip(" \t")
    pieces, i, last = [], 0, None
    while i < len(stripped):
      if stripped[i] in " \t":  # (a gap: wider, or gone beside punctuation that can't join)
        j = i
        while j < len(stripped) and stripped[j] in " \t":
          j += 1
        after = TOKEN.match(stripped, j)
        if after and after.lastgroup == "comment":
          pieces.append(stripped[i:j])
        elif last in LONE or (after and after.group() in LONE):
          pieces.append(rnd.choice(["", " ", "  ", stripped[i:j]]))
        else:
          pieces.append(rnd.choice([" ", "  ", stripped[i:j]]))
        i = j
        continue
      m = TOKEN.match(stripped, i)
      pieces.append(m.group())
      if m.lastgroup == "comment":
        break
      ahead = stripped[m.end():m.end() + 1]
      if ahead and ahead not in " \t" and (m.group() in LONE or ahead in LONE) and m.lastgroup != "string" and rnd.random() < 0.3:
        pieces.append(" ")
      last, i = m.group(), m.end()
    out.append((" " * rnd.randint(0, 9) if stripped else rnd.choice(["", " "])) + "".join(pieces).rstrip(" \t") + cr)
  return "\n".join(out)

def run(*args, cwd=None):
  return subprocess.run(list(args), capture_output=True, cwd=cwd)

def main():
  ap = argparse.ArgumentParser()
  ap.add_argument("rio")
  ap.add_argument("paths", nargs="*")
  ap.add_argument("--rounds", type=int, default=3)
  ap.add_argument("--seed", type=int, default=1)
  a = ap.parse_args()
  rio = os.path.abspath(a.rio)
  rnd = random.Random(a.seed)
  files = []
  for p in a.paths or [os.path.join(HERE, d) for d in ("bench", "src", "tests")]:
    if os.path.isdir(p):
      files += sorted(os.path.join(d, f) for d, _, fs in os.walk(p) for f in fs if f.endswith(".rio") and not f.startswith("_fmt"))
    else:
      files.append(p)
  problems = unlaid = linesOff = linesAll = images = 0
  for n, path in enumerate(files):
    original = open(path, "rb").read()
    tidy = run(rio, "fmt", path)
    if tidy.returncode:
      print("FAIL", path, tidy.stderr.decode(errors="replace").strip())
      problems += 1
      continue
    want = tidy.stdout
    if want != original:
      print("not laid out:", path)
      unlaid += 1
    tries = [scramble(original.decode("utf-8"), rnd) for _ in range(a.rounds)]
    tries.append("\n".join(l.lstrip(" ") for l in original.decode("utf-8").split("\n")))
    s = os.path.join(os.path.dirname(path), f"_fmtcheck{n}.rio")
    try:
      open(s, "wb").write(original)  # (the image it should make, under the name the others get)
      made = run(rio, "-i", s + ".img", s, cwd=os.path.dirname(path))
      base = open(s + ".img", "rb").read() if made.returncode == 0 and os.path.exists(s + ".img") else None
      for r, text in enumerate(tries):
        how = "flattened" if r == a.rounds else f"scrambled {r}"
        open(s, "wb").write(text.encode("utf-8"))
        got = run(rio, "fmt", s)
        if got.returncode:
          print(f"FAIL {path} ({how}): {got.stderr.decode(errors='replace').strip()}")
          problems += 1
          continue
        open(s, "wb").write(got.stdout)
        if run(rio, "fmt", s).stdout != got.stdout:
          print(f"FAIL {path} ({how}): formatting it again changed it")
          problems += 1
        if base is not None:
          images += 1
          made = run(rio, "-i", s + ".img", s, cwd=os.path.dirname(path))
          if made.returncode or open(s + ".img", "rb").read() != base:
            print(f"FAIL {path} ({how}): a different image")
            problems += 1
        pairs = list(zip(want.decode().split("\n"), got.stdout.decode().split("\n")))
        linesAll += len(pairs)
        linesOff += sum(x != y for x, y in pairs)
    finally:
      for f in (s, s + ".img"):
        if os.path.exists(f):
          os.remove(f)
  print(f"{len(files)} files, {problems} problems, {unlaid} not laid out yet; {images} images the same; "
        f"{linesOff} of {linesAll} lines kept as given (not back to rio fmt's)")
  return 1 if problems else 0

if __name__ == "__main__":
  sys.exit(main())
