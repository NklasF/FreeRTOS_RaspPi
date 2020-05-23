#include <FreeRTOS.h>
#include <task.h>
#include "Drivers/gpio.h"
#include "Drivers/uart.h"



void vTask31()
{
uart_putc('1');
	for(;;)
	{
		SetGpio(17, 0);
		int i=0,x=0;
		for(i=0;i<300000;i++)
		{
			x++;
		}
		SetGpio(17, 1);
		i=0,x=0;
		for(i=0;i<300000;i++)
		{
			x++;
		}
	}
}
void vTask62()
{
uart_putc('2');
	for(;;)
	{
		SetGpio(27, 0);
		int i=0,x=0;
		for(i=0;i<300000;i++)
		{
			x++;
		}
		SetGpio(27, 1);
		i=0,x=0;
		for(i=0;i<300000;i++)
		{
			x++;
		}
	}
}
void vTask124()
{
uart_putc('3');
	for(;;)
	{
		SetGpio(4, 0);
		int i=0,x=0;
		for(i=0;i<300000;i++)
		{
			x++;
		}
		SetGpio(4, 1);
		i=0,x=0;
		for(i=0;i<300000;i++)
		{
			x++;
		}
	}
}

#define UTILIZATION1 1

int main(void) {
	SetGpioFunction(17, 1);	//yellow
	SetGpioFunction(18, 1); //Buzzer
	SetGpioFunction(4, 1);	//green
	SetGpioFunction(27, 1); //red
	SetGpioFunction(11, 1);	//button
	SetGpioDirection(11,GPIO_IN);
	uart_init();
	/* Create a task*/
	portBASE_TYPE xReturned;
	uart_puts("start up\n");

	//EDF
	//Utitlization 100 %
	#if UTILIZATION1
	uart_puts("utilization 100%\n");
	xReturned = xTaskCreate(vTask31, "t31", 128, NULL, 3, NULL,100,300,300);
	if(xReturned != pdTRUE)
		if(xReturned == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY)
		{
			uart_putc('1');
			uart_putc('!');
		}
	xReturned = xTaskCreate(vTask62, "t62", 128, NULL, 3, NULL,200,600,600);
	if(xReturned != pdTRUE)
		if(xReturned == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY)
		{
			uart_putc('2');
			uart_putc('!');
		}
	xReturned = xTaskCreate(vTask124, "t124", 128, NULL, 3, NULL,400,1200,1200);
	if(xReturned != pdTRUE)
		if(xReturned == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY)
		{
			uart_putc('3');
			uart_putc('!');
		}

	//Utitlization >100 %
	#else
	uart_puts("utilization >100%\n");
	xReturned = xTaskCreate(vTask31, "t31", 128, NULL, 3, NULL,400,800,800);
	if(xReturned != pdTRUE)
		if(xReturned == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY)
		{
			uart_putc('1');
			uart_putc('!');
		}
	xReturned = xTaskCreate(vTask62, "t62", 128, NULL, 3, NULL,350,600,600);
	if(xReturned != pdTRUE)
		if(xReturned == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY)
		{
			uart_putc('2');
			uart_putc('!');
		}
	xReturned = xTaskCreate(vTask124, "t124", 128, NULL, 3, NULL,100,200,200);
	if(xReturned != pdTRUE)
		if(xReturned == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY)
		{
			uart_putc('3');
			uart_putc('!');
		}
	#endif

	uart_puts("task_created\n");
	/* Start the scheduler to start the tasks executing. */
	vTaskStartScheduler();
	uart_puts("scheduler_started\n");

	/* The following line should never be reached because vTaskStartScheduler() 
	will only return if there was not enough FreeRTOS heap memory available to
	create the Idle and (if configured) Timer tasks.  Heap management, and
	techniques for trapping heap exhaustion, are described in the book text. */
	for( ;; );
	return 0;
}
