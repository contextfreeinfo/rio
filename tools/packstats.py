"""python tools/packstats.py app.rioi ...: how an image's code packed (rio/plans/smaller-instructions.md):
short and wide instructions, overall and for each op that has a short form."""
import collections, os, re, struct, sys

here = os.path.dirname(os.path.abspath(__file__))
src = open(os.path.join(here, '..', 'src', 'internal.h')).read()
ops = re.findall(r'X\((\w+)\)', src.split('#define rio_OPS(X)')[1].split('\n\n')[0])
short = re.findall(r'X\((\w+), "', src.split('#define rio_SHORT_OPS(X)')[1].split('#define rio_SHORT_MORE')[0])
short += re.findall(r'X\((\w+), \w+, "', src.split('#define rio_SHORT_MORE(X)')[1].split('#define rio_SHORT_ENUM')[0]) # (the more forms, after them)

def units(path):
  b = open(path, 'rb').read()
  h = struct.unpack('<21I', b[:84])
  codeLen, constCap, top, ffiCount, codeAt, sources = h[3], h[5], h[7], h[17], h[18], h[20]
  at = 84
  for _ in range(ffiCount):
    for _ in range(2): at = b.index(b'\0', at) + 1
  for _ in range(sources): at = b.index(b'\0', at) + 1 + 9
  at = (at + 3) & ~3
  if codeAt: at += constCap * 4 + (codeAt - top)
  return struct.unpack('<%dI' % codeLen, b[at:at + codeLen * 4])

total = collections.Counter(); perOp = collections.Counter()
for path in sys.argv[1:]:
  u = units(path); pc = 0; c = collections.Counter()
  while pc < len(u):
    op = u[pc] & 255
    if op >= 128: c['short'] += 1; perOp[short[op - 128] + ' short'] += 1; pc += 1; continue
    data = pc + 2 < len(u) and (u[pc + 2] & 255) == 255
    c['wide'] += 1; perOp[ops[op] + ' wide'] += 1
    pc += 4 if data else 2
  c['units'] = len(u); total.update(c)
  n = c['short'] + c['wide']
  print('%-16s %6d instructions, %d%% short, %d bytes (%d as all wide)' % (os.path.basename(path), n, 100 * c['short'] // n, len(u) * 4, (len(u) + c['short']) * 4))
n = total['short'] + total['wide']
print('%-16s %6d instructions, %d%% short, code %d%% smaller' % ('all', n, 100 * total['short'] // n, 100 * total['short'] // (total['units'] + total['short'])))
if '-v' in os.environ.get('PACKSTATS', ''):
  for o in short: print('%-6s short %6d wide %6d' % (o, perOp[o + ' short'], perOp[o + ' wide']))
