#include <io.h>
#include "uart.h"

/* Initialize UART: 9600 baud, 8N1, F_CPU = 16 MHz */
void uart_init(void)
{
    /* Baud rate = 9600 */
    UBRR0H = 0;
    UBRR0L = 103;

    /* Normal speed mode */
    UCSR0A = 0x00;

    /* Enable transmitter and receiver */
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);

    /* 8 data bits, 1 stop bit, no parity */
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

/* Transmit one character */
void uart_putchar(char data)
{
    while (!(UCSR0A & (1 << UDRE0)))
    {
    }

    UDR0 = data;
}

/* Transmit a null-terminated string */
void uart_puts(char *str)
{
    while (*str)
    {
        uart_putchar(*str);
        str++;
    }
}