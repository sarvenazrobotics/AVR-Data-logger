#ifndef UART_H
#define UART_H

void uart_init(void);
void uart_putchar(char data);
void uart_puts(char *str);

#endif