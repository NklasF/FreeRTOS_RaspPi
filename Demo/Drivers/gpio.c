/**
 *	Quick and very Dirty GPIO API.
 *
 **/

#include "gpio.h"

typedef struct {
	unsigned long	GPFSEL[6];	///< Function selection registers.
	unsigned long	Reserved_1;
	unsigned long	GPSET[2];
	unsigned long	Reserved_2;
	unsigned long	GPCLR[2];
	unsigned long	Reserved_3;
	unsigned long	GPLEV[2];
	unsigned long	Reserved_4;
	unsigned long	GPEDS[2];
	unsigned long	Reserved_5;
	unsigned long	GPREN[2];
	unsigned long	Reserved_6;
	unsigned long	GPFEN[2];
	unsigned long	Reserved_7;
	unsigned long	GPHEN[2];
	unsigned long	Reserved_8;
	unsigned long	GPLEN[2];
	unsigned long	Reserved_9;
	unsigned long	GPAREN[2];
	unsigned long	Reserved_A;
	unsigned long	GPAFEN[2];
	unsigned long	Reserved_B;
	unsigned long	GPPUD[1];
	unsigned long	GPPUDCLK[2];
	//Ignoring the reserved and test bytes
} BCM2835_GPIO_REGS;

volatile BCM2835_GPIO_REGS * const pRegs = (BCM2835_GPIO_REGS *) (0x20200000);


void SetGpioFunction(unsigned int pinNum, unsigned int funcNum) {

	int offset = pinNum / 10;

	unsigned long val = pRegs->GPFSEL[offset];	// Read in the original register value.

	int item = pinNum % 10;
	val &= ~(0x7 << (item * 3));
	val |= ((funcNum & 0x7) << (item * 3));
	pRegs->GPFSEL[offset] = val;
}

void SetGpioDirection(unsigned int pinNum, enum GPIO_DIR dir) {
	SetGpioFunction(pinNum,dir);
}

void SetGpio(unsigned int pinNum, unsigned int pinVal) {
	unsigned long offset=pinNum/32;
	unsigned long mask=(1<<(pinNum%32));

	if(pinVal) {
		pRegs->GPSET[offset] = mask;
	} else {
		pRegs->GPCLR[offset] = mask;
	}
}

int ReadGpio(unsigned int pinNum) {
	return ((pRegs->GPLEV[pinNum/32])>>(pinNum%32))&1;
}

int PudGpio(unsigned int pinNum, enum PULL_STATE state)
{
	/* Abort on »reserved« state.
	 *(And possibly other illegal integer values that might get passed into this function)*/
	if( state >= PULL_RESERVED )
		return -1;
	/* Follow the procedure described in the BCM2835 data sheet to alter the Pullup/Pulldown
	 * behaviour of a GPIO pin.*/
	
	/* Step 1: Write the control signal (the state) into the GPPUD register.
	 *         Beware that this uses the order (and therefore the value) of the
	 *         PULL_STATE enum directly to avoid unneccessary branching.*/
	pRegs->GPPUD[0] = state;
	/* Step 2: Wait 150 cycles to provide the required set up time for control signals.*/
	int wait=0;
	for(wait = 0; wait < 150; ++wait);
	unsigned long offset = pinNum/32;
	unsigned long mask = (1<<(pinNum%32));
	/* Step 3: Write into GPPUDCLK for the requested pin. The requested pin changes its state
	 *         to the state written into GPPUD.*/
	pRegs->GPPUDCLK[offset] = mask;
	/* Step 4: Wait 150 cycles to provide the required hold time for control signals.*/
	for(wait = 0; wait < 150; ++wait);
	/* Step 5: Reset GPPUD to 0 to remove the control signal.*/
	pRegs->GPPUD[0] = 0;
	/* Step 6: Reset the GPPUDCLK clock register to remove the clock.*/
	pRegs->GPPUDCLK[offset] = 0;
	return state;
}

int PudMultiGpio(unsigned int gppudclk0, unsigned int gppudclk1, enum PULL_STATE state)
{
	/* Abort on »reserved« state.
	 *(And possibly other illegal integer values that might get passed into this function)*/
	if( state >= PULL_RESERVED )
		return -1;
	/* Follow the procedure described in the BCM2835 data sheet to alter the Pullup/Pulldown
	 * behaviour of a GPIO pin.*/
	
	/* Step 1: Write the control signal (the state) into the GPPUD register.
	 *         Beware that this uses the order (and therefore the value) of the
	 *         PULL_STATE enum directly to avoid unneccessary branching.*/
	pRegs->GPPUD[0] = state;
	/* Step 2: Wait 150 cycles to provide the required set up time for control signals.*/
	/* Step 3: Write into GPPUDCLK for the requested pins. The requested pins change their state
	 *         to the state written into GPPUD.*/
	pRegs->GPPUDCLK[0] = gppudclk0;
	pRegs->GPPUDCLK[1] = gppudclk1;
	/* Step 4: Wait 150 cycles to provide the required hold time for control signals.*/
	int wait=0;
	for(wait = 0; wait < 150; ++wait);
	/* Step 5: Reset GPPUD to 0 to remove the control signal.*/
	pRegs->GPPUD[0] = 0;
	/* Step 6: Reset the GPPUDCLK clock registers to remove the clock.*/
	pRegs->GPPUDCLK[0] = 0;
	pRegs->GPPUDCLK[1] = 0;
	return state;
}


void EnableGpioDetect(unsigned int pinNum, enum DETECT_TYPE type)
{
	unsigned long mask=(1<<pinNum);
	unsigned long offset=pinNum/32;
	
	switch(type) {
	case DETECT_RISING:
		pRegs->GPREN[offset]|=mask;
		break;
	case DETECT_FALLING:
		pRegs->GPFEN[offset]|=mask;
		break;
	case DETECT_HIGH:
		pRegs->GPHEN[offset]|=mask;
		break;
	case DETECT_LOW:
		pRegs->GPLEN[offset]|=mask;
		break;
	case DETECT_RISING_ASYNC:
		pRegs->GPAREN[offset]|=mask;
		break;
	case DETECT_FALLING_ASYNC:
		pRegs->GPAFEN[offset]|=mask;
		break;
	case DETECT_NONE:
		break;
	}
}

void DisableGpioDetect(unsigned int pinNum, enum DETECT_TYPE type)
{
	unsigned long mask=~(1<<(pinNum%32));
	unsigned long offset=pinNum/32;
	
	switch(type) {
	case DETECT_RISING:
		pRegs->GPREN[offset]&=mask;
		break;
	case DETECT_FALLING:
		pRegs->GPFEN[offset]&=mask;
		break;
	case DETECT_HIGH:
		pRegs->GPHEN[offset]&=mask;
		break;
	case DETECT_LOW:
		pRegs->GPLEN[offset]&=mask;
		break;
	case DETECT_RISING_ASYNC:
		pRegs->GPAREN[offset]&=mask;
		break;
	case DETECT_FALLING_ASYNC:
		pRegs->GPAFEN[offset]&=mask;
		break;
	case DETECT_NONE:
		break;
	}
}

void ClearGpioInterrupt(unsigned int pinNum)
{
	unsigned long mask=(1<<(pinNum%32));
	unsigned long offset=pinNum/32;

	pRegs->GPEDS[offset]=mask;
}
