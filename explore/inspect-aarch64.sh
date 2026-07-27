as -o test-aarch64.o test-aarch64.s
objdump -d test-aarch64.o

# CFLAGS=-O2

gcc $CFLAGS -c main.c -o main.o
gcc $CFLAGS -S main.c -o main.s
objdump -d main.o

rm *.o

# >>> import re
# >>> re.sub(r"(.{4})(?=.)", r"\1_", f"{0x0b010000:032b}")
# '0000_1011_0000_0001_0000_0000_0000_0000'
