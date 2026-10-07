arm-none-eabi-as -o test-pico.o test-pico.s
arm-none-eabi-objdump -d test-pico.o

arm-none-eabi-gcc -O0 -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -c main.c -o main.o
# arm-none-eabi-gcc -O0 -mcpu=cortex-a72 -mfpu=neon-fp-armv8 -mfloat-abi=hard -mthumb -c main.c -o main.o
arm-none-eabi-gcc -O0 -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -S main.c -o main.s
arm-none-eabi-objdump -d main.o

# arm-linux-gnueabihf-gcc -O0 -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb main.c -o thumb.bin
arm-linux-gnueabihf-gcc -O0 -mcpu=cortex-a72 -mfpu=neon-fp-armv8 -mfloat-abi=hard -mthumb main.c -o thumb.bin
rm *.o
