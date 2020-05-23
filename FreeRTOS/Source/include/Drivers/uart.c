#include "uart.h"

//------------------------------------------------------------------------
inline void PUT32 ( unsigned int addr, unsigned int val ){
    *(volatile unsigned int *)addr = val;
}
//------------------------------------------------------------------------
inline unsigned int GET32 ( unsigned int addr ){
    return *(volatile unsigned int *)addr;
}
//------------------------------------------------------------------------
void uart_putc ( unsigned int c )
{
    while(1)
    {
        if((GET32(UART0_FR)&0x20)==0) break;
    }
    PUT32(UART0_DR,c);
}

void uart_puts(char* str)
{
	while(*str)
		uart_putc(*str++);
//	{
//		uart_putc(*str);
//		str++;
//	}
}

void uart_putl(long num)
{	
	num = num%100000;
	char x5=num/10000;
	num = (num-x5*10000);
	char x4 = num/1000;
	num = (num-x4*1000);
	char x3 = num/100;
	num = (num-x3*100);
	char x2 = num/10;
	num = (num-x2*10);
	uart_putc('0'+x5);
	uart_putc('0'+x4);
	uart_putc('0'+x3);
	uart_putc('0'+x2);
	uart_putc('0'+num);
}
//------------------------------------------------------------------------
unsigned int uart_getc ( void )
{
    while(1)
    {
        if((GET32(UART0_FR)&0x10)==0) break;
    }
    return(GET32(UART0_DR));
}
//------------------------------------------------------------------------
void uart_init ( void )
{
    volatile unsigned int ra;

    PUT32(UART0_CR,0);

    ra=GET32(GPFSEL1);
    ra&=~(7<<12); //gpio14
    ra|=4<<12;    //alt0
    ra&=~(7<<15); //gpio15
    ra|=4<<15;    //alt0
    PUT32(GPFSEL1,ra);

    PUT32(GPPUD,0);
    for(ra=0;ra<150;ra++);
    PUT32(GPPUDCLK0,(1<<14)|(1<<15));
    for(ra=0;ra<150;ra++);
    PUT32(GPPUDCLK0,0);

    PUT32(UART0_ICR,0x7FF);
    PUT32(UART0_IBRD,1);
    PUT32(UART0_FBRD,40); 		// Set baud rate: 115200
    PUT32(UART0_LCRH,0x70);
    PUT32(UART0_CR,0x301);
}
// //------------------------------------------------------------------------
// void hexstrings ( unsigned int d )
// {
//     //unsigned int ra;
//     unsigned int rb;
//     unsigned int rc;

//     rb=32;
//     while(1)
//     {
//         rb-=4;
//         rc=(d>>rb)&0xF;
//         if(rc>9) rc+=0x37; else rc+=0x30;
//         uart_putc(rc);
//         if(rb==0) break;
//     }
//     uart_putc(0x20);
// }
// //------------------------------------------------------------------------
// void hexstring ( unsigned int d )
// {
//     hexstrings(d);
//     uart_putc(0x0D);
//     uart_putc(0x0A);
// }
// //------------------------------------------------------------------------
// int notmain ( unsigned int earlypc )
// {
//     unsigned int ra;

//     uart_init();
//     hexstring(0x12345678);
//     hexstring(earlypc);

//     for(ra=0;ra<30000;ra++)
//     {
//         uart_putc(0x30|(ra&7));
//     }

//     for(ra=0;ra<100;ra++) uart_putc(0x55);

//     //probably a better way to flush the rx fifo.  depending on if and
//     //which bootloader you used you might have some stuff show up in the
//     //rx fifo.
//     while(1)
//     {
//         if(GET32(UART0_FR)&0x10) break;
//         GET32(UART0_DR);
//     }

//     while(1)
//     {
//         ra=uart_getc();
//         if(ra==0x0D) uart_putc(0x0A);
//         uart_putc(ra);
//     }


//     return(0);
// }
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
//
// Copyright (c) 2012 David Welch dwelch@dwelch.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------
