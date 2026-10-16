"""Runs the examples in the docs as tests: python tests/docs.py path/to/rio docs README.md

Each ```rio block in a Markdown file is a whole program, run with the rio CLI:
  ```rio                  must run without errors; an ```output block right after it is its exact output
  # error: message        (ending a line) compiling must fail there, with a message containing this
  # runtime error: msg    (ending a line) running must fail at that line
  ```rio file=geo.rio     a file for the next example to import or include (paths may have dirs;
                          files under lib/ are packages: the example runs with -L lib)
  ```rio fragment         shown, not run (a piece of a program)
"""
import os, re, subprocess, sys, tempfile

FENCE = re.compile(r'^(\s*)```(.*)$')
MARK = re.compile(r'#\s*(runtime error|error):\s*(.*?)\s*$')


def blocks(path):
    """(info, first line number of the content, text) for each fenced block"""
    out, lines = [], open(path, encoding='utf-8').read().split('\n')
    i = 0
    while i < len(lines):
        m = FENCE.match(lines[i])
        if not m:
            i += 1
            continue
        indent, info, start, body = len(m.group(1)), m.group(2).strip(), i + 2, []
        i += 1
        while i < len(lines) and not re.match(r'^\s*```\s*$', lines[i]):
            body.append(lines[i][indent:])
            i += 1
        out.append((info, start, '\n'.join(body) + '\n', i))
        i += 1
    return out, lines


def run(rio, md):
    found, lines = blocks(md)
    files, fails, count = {}, [], 0
    for k, (info, start, text, end) in enumerate(found):
        words = info.split()
        if not words or words[0] != 'rio' or 'fragment' in words:
            continue
        named = [w[5:] for w in words if w.startswith('file=')]
        if named:
            files[named[0]] = text
            continue
        count += 1
        expected = None
        if k + 1 < len(found) and found[k + 1][0] == 'output' and all(not l.strip() for l in lines[end + 1:found[k + 1][1] - 2]):
            expected = found[k + 1][2]
        want = None  # (file, line, kind, message)
        for name, src in list(files.items()) + [('main.rio', text)]:
            for n, line in enumerate(src.split('\n'), 1):
                m = MARK.search(line)
                if m and not line.lstrip().startswith('#'):
                    want = (name, n, m.group(1), m.group(2))
        with tempfile.TemporaryDirectory() as tmp:
            for name, src in list(files.items()) + [('main.rio', text)]:
                p = os.path.join(tmp, name)
                os.makedirs(os.path.dirname(p), exist_ok=True)
                open(p, 'w', encoding='utf-8', newline='\n').write(src)
            args = [rio] + (['-L', os.path.join(tmp, 'lib')] if os.path.isdir(os.path.join(tmp, 'lib')) else []) + [os.path.join(tmp, 'main.rio')]
            r = subprocess.run(args, capture_output=True, text=True, timeout=30, cwd=tmp)
            got = (r.stdout + r.stderr).replace('\r\n', '\n').replace(tmp + os.sep, '').replace(tmp + '/', '')
        files = {}
        where = '%s:%d' % (md, start)
        if want:
            name, n, kind, msg = want
            ok = r.returncode != 0 and msg in got and (
                re.search(r'(^|\n)%s:%d: runtime error: ' % (re.escape(name), n), got) if kind == 'runtime error'
                else re.search(r'(^|\n)%s:%d:\d+: ' % (re.escape(name), n), got) and 'runtime error' not in got)
            if not ok:
                fails.append('%s: expected %s at %s:%d (%s), got: %s' % (where, kind, name, n, msg, got.strip() or '(nothing)'))
        elif r.returncode != 0:
            fails.append('%s: failed: %s' % (where, got.strip()))
        elif expected is not None and got != expected:
            fails.append('%s: output differs\n--- expected\n%s--- got\n%s' % (where, expected, got))
    return count, fails


def main():
    rio, total, fails, mds = os.path.abspath(sys.argv[1]), 0, [], []
    for root in sys.argv[2:]:  # Markdown files, and directories of them
        if os.path.isfile(root):
            mds.append(root)
        for d, _, names in sorted(os.walk(root)):
            mds += [os.path.join(d, name) for name in sorted(names) if name.endswith('.md')]
    for md in mds:
        n, f = run(rio, md)
        total += n
        fails += f
    for f in fails:
        print('FAIL ' + f)
    print('%d of %d doc examples failed' % (len(fails), total) if fails else 'all %d doc examples passed' % total)
    sys.exit(1 if fails else 0)


main()
