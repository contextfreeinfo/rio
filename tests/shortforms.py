"""python tests/shortforms.py rio out.img: compiles tests/shortforms.rio to an image, and checks its code
uses each short form that reads constants and globals (src/internal.h: rio_SHORT_MORE), so the packer
chose them; then runs it (its asserts check what they do)."""
import os, re, struct, subprocess, sys

here = os.path.dirname(os.path.abspath(__file__))
rio, img = sys.argv[1], sys.argv[2]
src = open(os.path.join(here, '..', 'src', 'internal.h')).read()
first = re.findall(r'X\((\w+),\s*"', src.split('#define rio_SHORT_OPS(X)')[1].split('#define rio_SHORT_MORE')[0])
more = re.findall(r'X\((\w+),\s*\w+,\s*"', src.split('#define rio_SHORT_MORE(X)')[1].split('#define rio_SHORT_ENUM')[0])
number = {name: 128 + len(first) + i for i, name in enumerate(more)} # (after the first forms)
subprocess.run([rio, '-i', img, os.path.join(here, 'shortforms.rio')], check=True)
b = open(img, 'rb').read()
h = struct.unpack('<21I', b[:84])
codeLen, constCap, top, ffiCount, codeAt, sources = h[3], h[5], h[7], h[17], h[18], h[20]
at = 84
for _ in range(ffiCount):
  for _ in range(2): at = b.index(b'\0', at) + 1
for _ in range(sources): at = b.index(b'\0', at) + 1 + 9
at = (at + 3) & ~3
if codeAt: at += constCap * 4 + (codeAt - top)
units = struct.unpack('<%dI' % codeLen, b[at:at + codeLen * 4])
seen, pc = set(), 0
while pc < len(units):
  op = units[pc] & 255
  if op >= 128: seen.add(op); pc += 1
  else: pc += 4 if pc + 2 < len(units) and (units[pc + 2] & 255) == 255 else 2
missing = [name for name, n in number.items() if n not in seen]
if missing: sys.exit('not packed short: ' + ' '.join(missing))
print('all %d forms used' % len(number))
sys.exit(subprocess.run([rio, os.path.join(here, 'shortforms.rio')]).returncode)
