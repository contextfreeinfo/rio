"""The REPL at a terminal (a pseudo-terminal: Linux and macOS): python3 tests/tty.py path/to/rio

Keys as a terminal sends them, through the line editor in raw mode (os.h's): editing, history (kept in
~/.rio_history, a folder of its own for HOME here), Ctrl+C dropping a line, Ctrl+D quitting. The tests
of the editing itself are tests/line.c's; this is the terminal around it."""
import os, pty, re, select, sys, tempfile, time

rio = os.path.abspath(sys.argv[1])
home = tempfile.mkdtemp()
pid, fd = pty.fork()
if pid == 0:
    os.environ["HOME"] = home
    os.execv(rio, [rio])
out = b""

def drain(t):
    global out
    end = time.time() + t
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            try:
                out += os.read(fd, 4096)
            except OSError:
                return

def send(keys, t=0.5):
    os.write(fd, keys.encode())
    drain(t)

drain(1.5)
send("x := 40\r")
send("log(x + 1)\r")                                   # 41
send("lg(x)\x1b[D\x1b[D\x1b[D\x1b[Do\r")               # left four times, o: log(x), 40
send("one two\x17\x15log(9)\x1b[H\x1b[3~l\r")          # Ctrl+W, Ctrl+U, Home, Delete: log(9), 9
send("\x1b[A\x1b[A\x1b[A\r")                           # Up three times: log(x + 1) again, 41
send("abc\x03")                                        # Ctrl+C: dropped
send("log(1 + 1)\r")                                   # 2
send("\x04", 1.5)                                      # Ctrl+D: the end
_, status = os.waitpid(pid, 0)
shown = re.sub(rb"\x1b\[[0-9;?]*[A-Za-z]", b"", out).replace(b"\r", b"").decode("utf-8", "replace")
results = [l for l in shown.split("\n") if l and not l.startswith(">") and not l.startswith("rio REPL")]
history = open(os.path.join(home, ".rio_history")).read().split("\n")
fails = []
if results != ["41", "40", "9", "41", "2"]:
    fails.append("results: %r" % results)
if history[:6] != ["x := 40", "log(x + 1)", "log(x)", "log(9)", "log(x + 1)", "log(1 + 1)"]:
    fails.append("history: %r" % history)
if not os.WIFEXITED(status) or os.WEXITSTATUS(status) != 0:
    fails.append("exit: %r" % status)
for f in fails:
    print("FAIL " + f)
print("the REPL came through" if not fails else "%d failed" % len(fails))
sys.exit(1 if fails else 0)
