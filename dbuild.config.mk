
CFLAGS += -march=armv6z -Wall -Wno-main -Wno-pointer-sign -Wno-unused-but-set-variable #-Wextra 
CFLAGS += -I $(BASE)FreeRTOS/Source/portable/GCC/RaspberryPi/
CFLAGS += -I $(BASE)FreeRTOS/Source/include/
CFLAGS += -I $(BASE)Demo/Drivers/

TOOLCHAIN=arm-none-eabi-

