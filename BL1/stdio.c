#include "uart.h"


int putchar(int c)
{
	if (c == '\n')
		putc('\r');
	
	if (c == '\b')
	{
		putc('\b');
		putc(' ');	
	}
			
	putc((char)c);
	
	return c;	
}

int puts(const char * s)
{
	while (*s)
		putchar(*s++);
		
	return 0;
}

