#include <FreeRTOS.h>
#include <task.h>

#include "Drivers/interrupts.h"
#include "Drivers/gpio.h"
#include "Drivers/uart.h"
#include "Drivers/print_473.h"

// This program will print "Hello World_473" to the UART.
// uart_init() configures the UART with the following parameters:

// GPIO14  TXD0 and TXD1
// GPIO15  RXD0 and RXD1
// alt function 5 for uart1
// alt function 0 for uart0


// This is the baud conversion function w/baud = 115200:
// (3000000 / (16 * 115200) = 1.627
// (0.627*64)+0.5 = 40
// int 1 frac 40 


void main(void)
{

    uart_init();
    printf_473("Hello World_%d\n", 473);

}
