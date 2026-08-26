set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR armv7-a)
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_C_FLAGS_INIT
    "-mthumb -mcpu=cortex-a72 -mfloat-abi=hard -mfpu=neon-fp-armv8"
)
