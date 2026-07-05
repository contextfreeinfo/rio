as -o test-aarch64.o test-aarch64.s
objdump -d test-aarch64.o

gcc -O2 -c main.c -o main.o
gcc -O2 -S main.c -o main.s
objdump -d main.o

rm *.o
