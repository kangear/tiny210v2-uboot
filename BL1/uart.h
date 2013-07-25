#ifdef _UART_H_
#define _UART_H_

extern void putc(char c);
extern int puthex(volatile unsigned char a);
extern int put32bits(volatile unsigned int a);

#endif
