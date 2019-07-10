- Top level source files works. Auto include of `src` dirs, too.
- Prohibit libstdc++ for faster compile.

`
./test -g > tests/test1.c-mem.txt 2>&1
./test > tests/test1.c.txt 2>&1
./test > tests/test1.c
`
