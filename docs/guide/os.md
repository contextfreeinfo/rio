# The CLI's module os

The `rio` command has a module of its own, `os`, for programs it runs: the program's arguments and
environment, text in and out, files and folders, running other programs, and time. A program imports it
(it isn't there unless it does), and it's in the REPL, and in a program's C (`rio -c`), too. It's the
command's, not Rio's: Rio as a library, riocore, has no `os`, and a host that embeds Rio has modules of
its own (Rinc's `pc`). `src/os/os.rio` declares it, and each proc says what it does there; `src/os/os.h` has
them in C. All of it is in `src/os`, apart from Rio itself.

```rio
import os

buf: Blob[256]
check := os.fileWrite("os-example.txt", "one\ntwo\n") == 0
n := os.fileRead("os-example.txt", buf[])
os.write(buf[0..<n].asString())
log(check, n, os.fileDelete("os-example.txt"), os.fileKind("os-example.txt"))
```

```output
one
two
true 8 0 0
```

As in Rinc's `pc`, what a proc reads goes into a `Blob` it's given (as much as fits), and it says the
whole length, or -1 when there's none. What a proc does says 0, or -1 when it couldn't. Paths are as
the system takes them; on Windows, `/` works as `\` does.

| procs | |
|---|---|
| `argCount`, `arg`, `env`, `envSet`, `exit` | the program's arguments (`rio tool.rio a b`: `a` and `b`), its environment, and its end, with an exit code |
| `write`, `writeErr`, `readLine`, `readAll` | text out (as it is: `log` adds spaces and a line break) and in |
| `fileRead`, `fileWrite`, `fileAppend`, `fileSize`, `fileKind`, `fileDelete`, `fileRename` | files (and empty folders, deleted) |
| `dir`, `dirMake`, `cwd`, `cwdSet` | folders: a folder's entries (`d name` and `f name` lines, folders first, by name), a new one, the current one |
| `run` | another program, with its arguments as they are (no shell, unless asked for), waited for: its exit code, and what it writes (`RunOptions`) |
| `clock`, `time`, `date`, `sleep` | the processor's time used, seconds since the start, the date and time here, and a pause |

`os.run` runs a program with a list of arguments, each passed just as it is: there's no shell
between, so nothing needs quoting, and nothing in an argument can run another command. `RunOptions`,
left out by default, keeps what it writes (`out`), standard error too (`errorsToo`), or hands
`args[0]` to the system's shell as a command line (`shell`: then its pipes and redirects work, and
its quoting is the command line's to get right):

```rio fragment
import os

log(os.run({"git", "status"})[0])           # its exit code (-1: it couldn't run)
buf: Blob[4096]
r := os.run({"git", "log", "-1"}, {out = buf[]})
log(buf[0..<min(r[1], buf.len())].asString())
```

`os.readLine` reads a line typed after a prompt. At a terminal it's edited as it's typed, as the
REPL's lines are: the arrows, Home and End, Ctrl+Left and Right (or Alt+B and F) by words, Backspace,
Delete, Ctrl+K and Ctrl+U (cutting to the end or the start), Ctrl+W (a word back), Ctrl+Y (what was
cut, back), and Up and Down (or Ctrl+P and N) for the lines typed before. Ctrl+D on an empty line is the
end of the input (-1), and Ctrl+C stops the program, as it does anywhere. Reading from a pipe or a
file, it reads a line as it comes.

A view into a builder is the builder's bytes as they are now: building two paths in the same builder,
the first view shows the second path. Give each its own builder:

```rio
import os

a: Blob[..64]
b: Blob[..64]
_ = a.pushAll("os-old.txt")
_ = b.pushAll("os-new.txt")
from := a[0..<a.len()].asString()
to := b[0..<b.len()].asString()
log(os.fileWrite(from, "x"), os.fileRename(from, to), os.fileKind(from), os.fileKind(to), os.fileDelete(to))
```

```output
0 0 0 1 0
```
