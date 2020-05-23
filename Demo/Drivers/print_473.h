#ifndef __PRINT_473_H__
#define __PRINT_473_H__

#include <stdarg.h>
#include "uart.h"

#define PAD_RIGHT 1
#define PAD_ZERO 2
/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 12

void outbyte( int );

static void printchar(char **, int );

static int prints(char **, const char *, int , int );

static int printi(char **, int , int , int , int , int , int );

static int print( char **, const char *, va_list );

int printf_473(const char *, ...);

int sprintf_473(char *, const char *, ...);


int snprintf_473( char *buf, unsigned int count, const char *format, ... );

#endif // __PRINT_473_H__