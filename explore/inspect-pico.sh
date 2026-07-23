arm-none-eabi-as -o test-pico.o test-pico.s
arm-none-eabi-objdump -d test-pico.o

arm-none-eabi-gcc -O2 -mcpu=cortex-m33 -mthumb -c main.c -o main.o
arm-none-eabi-gcc -O2 -mcpu=cortex-m33 -mthumb -S main.c -o main.s
arm-none-eabi-objdump -d main.o

rm *.o
