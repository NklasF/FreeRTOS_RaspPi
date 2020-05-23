#include <FreeRTOS.h>
#include <task.h>

#include "Drivers/interrupts.h"
#include "Drivers/gpio.h"

#define LED_PIN 16
#define ON 0
#define OFF 1
#define INT_PIN 25

/* -----------------------------------------------------------------------------
// This template can be used as a starting point for building interrupt driven
// systems. Currently, this template is configured to toggle the LED when a
// rising edge is detected on INT_PIN (p01-22).   
*/


// Fill in the two macros to complete 

#define IRQn <FILL IN> 		// Interrupt number associated GPIO (from datasheet)
#define IRQ_TYPE <FILL IN>	// Detection type

void rupt(int num, void*parm){
	DisableInterrupts();
	ClearGpioInterrupt( INT_PIN ); 	// This clears interrupts based on pin number

	if( ReadGpio( LED_PIN ) == OFF )
		SetGpio( LED_PIN, ON );
	else
		SetGpio( LED_PIN, OFF );

	EnableInterrupts();

}

void main(void) {
	DisableInterrupts();

	InitInterruptController();

	RegisterInterrupt( IRQn, rupt, NULL );

	SetGpioFunction(LED_PIN, 1);

	SetGpioFunction(INT_PIN, 0);

	// Enable detection (see gpio.h for type)
	EnableGpioDetect( INT_PIN, IRQ_TYPE );
	
	EnableInterrupt( IRQn );

	EnableInterrupts();

	volatile int i = 0;
	while(1) {
		i++;
	}
}
