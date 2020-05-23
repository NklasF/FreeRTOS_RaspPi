#ifndef _GPIO_H_
#define _GPIO_H_

/* GPIO event detect types */
enum DETECT_TYPE {
	DETECT_NONE,
	DETECT_RISING,
	DETECT_FALLING,
	DETECT_HIGH,
	DETECT_LOW,
	DETECT_RISING_ASYNC,
	DETECT_FALLING_ASYNC
};

/* GPIO pull up or down states.
 * Beware: The values/order of the enum constants directly reflect the
 * register values expected by the hardware. So the enum values are used directly as
 * an input value for the GPPUD control register. Modifying the enum by
 * assigning other values or reordering breaks the PudGpio functionality!
 */
enum PULL_STATE {
	PULL_DISABLE = 0,
	PULL_DOWN,
	PULL_UP,
	PULL_RESERVED
};

/* Pin data direction */
enum GPIO_DIR {
	GPIO_IN,
	GPIO_OUT
};

/* GPIO pin setup */
void SetGpioFunction	(unsigned int pinNum, unsigned int funcNum);
/* A simple wrapper around SetGpioFunction */
void SetGpioDirection	(unsigned int pinNum, enum GPIO_DIR dir);

/* Set GPIO output level */
void SetGpio			(unsigned int pinNum, unsigned int pinVal);

/* Read GPIO pin level */
int ReadGpio			(unsigned int pinNum);

/* GPIO pull up/down resistor control function.
 * This method sets the pull up/down for a single GPIO pin.
 */
int PudGpio				(unsigned int pinNum, enum PULL_STATE state);

/* GPIO pull up/down resistor control function.
 * This methode sets the pull up/down for multiple GPIO pins simultaneously.
 * It is provided because modifying the pull up/down requires busy waiting and the hardware supports
 * batch editing of multiple pins(which is used by this function). This function is significantly
 * faster when modifying multiple pins.
 * The first two parameters (gppudclk0 & gppudclk1) are used as a bitmask. Each 1 in the mask
 * sets the pull up/down behaviour of the associated pin to the given state (third parameter).
 * All Values below are counted from 0!
 * Bitmask layout: gppudclk0: GPIO pin 0 (LSB) to GPIO pin 31 (MSB)
 *                 gppudclk1: GPIO pin 32 (LSB) to GPIO pin 53 (Bit 21), Bit 22 to 31 are reserved, write 0.
 * See BCM2835 ARM peripherals documentation data sheet, Page 101: »GPIO Pull-up/down Clock Registers (GPPUDCLKn)«
 */
int PudMultiGpio		(unsigned int gppudclk0, unsigned int gppudclk1, enum PULL_STATE state);

/* Interrupt related functions */
void EnableGpioDetect	(unsigned int pinNum, enum DETECT_TYPE type);
void DisableGpioDetect	(unsigned int pinNum, enum DETECT_TYPE type);
void ClearGpioInterrupt	(unsigned int pinNum);

#endif
