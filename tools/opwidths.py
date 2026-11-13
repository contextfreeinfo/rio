"""python tools/opwidths.py app.rioi ...: how many of each image's instructions would fit a short form of
8-bit operands counted from the running proc's frame (plans/smaller-instructions.md, option A). Each
operand is: in its proc's frame and under 256 from its start (frame); under 256 anyway (an immediate, an
offset, a low constant: small); a jump's target within 127 instructions (near); or none of those (far: a
global, a big constant, a far jump). An instruction is short with no far operand. Images from
`rinc app.rio -i app.rioi` (or rio's own)."""
import collections, os, re, struct, sys

here = os.path.dirname(os.path.abspath(__file__))
ops = re.findall(r'X\((\w+)\)', open(os.path.join(here, '..', 'src', 'internal.h')).read().split('#define rio_OPS(X)')[1].split('\n\n')[0])
JUMPS = set(range(ops.index('jmp'), ops.index('forby') + 1))
CALL = ops.index('call')

def image(path):
  b = open(path, 'rb').read()
  h = struct.unpack('<21I', b[:84])
  codeLen, constCap, top, procCount, procsAt, ffiCount, codeAt, sources = h[3], h[5], h[7], h[15], h[16], h[17], h[18], h[20]
  at = 84
  for _ in range(ffiCount):
    for _ in range(2): at = b.index(b'\0', at) + 1
  for _ in range(sources): at = b.index(b'\0', at) + 1 + 9
  at = (at + 3) & ~3
  memAt = at + constCap * 4 - top # (a mem address's place in the file, in the finalized block)
  codeFile = memAt + codeAt if codeAt else at
  procs = []
  for k in range(procCount):
    entry, end, fs, fe, pe, res = struct.unpack('<6H', b[memAt + procsAt + k * 14:memAt + procsAt + k * 14 + 12])
    procs.append((entry, end, fs, fe))
  code = [struct.unpack('<BBHHH', b[codeFile + i * 8:codeFile + i * 8 + 8]) for i in range(codeLen)]
  return code, procs, constCap

total = collections.Counter()
for path in sys.argv[1:]:
  code, procs, constCap = image(path)
  frameOf = [None] * len(code)
  for entry, end, fs, fe in procs:
    for pc in range(entry, min(end, len(code))): frameOf[pc] = (fs, fe)
  c = collections.Counter()
  for pc, (op, x, a, b, cc) in enumerate(code):
    c['all'] += 1
    if op == 255: c['data words'] += 1; continue
    kinds = []
    for k, v in (('a', a), ('b', b), ('c', cc)):
      if (op in JUMPS or op == CALL) and k == 'c':
        kinds.append('near' if abs(v - pc) < 128 and op != CALL else 'far')
      elif frameOf[pc] and frameOf[pc][0] <= v < frameOf[pc][1] and v - frameOf[pc][0] < 256: kinds.append('frame')
      elif v < 256: kinds.append('small')
      else: kinds.append('far')
    for k in kinds: c['operand ' + k] += 1
    if 'far' not in kinds and x == 0: c['short'] += 1
    elif 'far' not in kinds: c['short but x'] += 1
    else: c['long'] += 1
    if frameOf[pc] is None: c['in no proc (top level)'] += 1
  total.update(c)
  n = c['all']
  print('%-16s %6d' % (os.path.basename(path), n), ' '.join('%s %d%%' % (k, 100 * c[k] // n) for k in ('short', 'long', 'data words', 'in no proc (top level)')))
n = total['all']
print('%-16s %6d' % ('all', n), ' '.join('%s %d%%' % (k, 100 * total[k] // n) for k in ('short', 'long', 'data words', 'in no proc (top level)')))
ops3 = sum(v for k, v in total.items() if k.startswith('operand '))
print('operands:', ' '.join('%s %d%%' % (k[8:], 100 * v // ops3) for k, v in sorted(total.items()) if k.startswith('operand ')))
