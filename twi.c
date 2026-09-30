
#include <io.h>
#include "twi.h"

/* ATmega328P hardware TWI
   CPU clock = 16 MHz
   I2C clock = 100 kHz
*/

void twi_init(void)
{
    TWSR = 0x00;  /* Prescaler = 1 */
    TWBR = 72;    /* SCL = 100 kHz at 16 MHz */
    TWCR = (1 << TWEN);
}

unsigned char twi_start(void)
{
    TWCR = (1 << TWINT) |
           (1 << TWSTA) |
           (1 << TWEN);

    while (!(TWCR & (1 << TWINT)))
    {
    }

    return (TWSR & 0xF8);
}

void twi_stop(void)
{
    TWCR = (1 << TWINT) |
           (1 << TWSTO) |
           (1 << TWEN);
}

unsigned char twi_write(unsigned char data)
{
    TWDR = data;

    TWCR = (1 << TWINT) |
           (1 << TWEN);

    while (!(TWCR & (1 << TWINT)))
    {
    }

    return (TWSR & 0xF8);
}

unsigned char twi_read(unsigned char ack)
{
    if (ack)
    {
        TWCR = (1 << TWINT) |
               (1 << TWEN) |
               (1 << TWEA);
    }
    else
    {
        TWCR = (1 << TWINT) |
               (1 << TWEN);
    }

    while (!(TWCR & (1 << TWINT)))
    {
    }

    return TWDR;
}