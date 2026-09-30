
#include <io.h>
#include <delay.h>
#include "i2c.h"

/* =====================================================
   TWI INITIALIZATION
   ATmega328P:
   SDA = PC4
   SCL = PC5
   ===================================================== */

void twi_init(void)
{
    /* Prescaler = 1 */
    TWSR = 0x00;

    /* Set I2C clock to 100 kHz */
    TWBR = (unsigned char)
           ((F_CPU / TWI_SCL_HZ - 16UL) / 2UL);

    /* Enable TWI peripheral */
    TWCR = (1 << TWEN);
}

/* =====================================================
   GENERATE START CONDITION
   Returns TWI status code
   ===================================================== */

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

/* =====================================================
   GENERATE STOP CONDITION
   ===================================================== */

void twi_stop(void)
{
    TWCR = (1 << TWINT) |
           (1 << TWSTO) |
           (1 << TWEN);
}

/* =====================================================
   TRANSMIT ONE BYTE
   Returns TWI status code
   ===================================================== */

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

/* =====================================================
   RECEIVE ONE BYTE
   ack = 1: send ACK
   ack = 0: send NACK
   ===================================================== */

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

/* =====================================================
   WRITE ONE BYTE TO EXTERNAL 24C02 EEPROM
   Returns 1 on success, 0 on error
   ===================================================== */

unsigned char i2c_eeprom_write(unsigned char address,
                               unsigned char data)
{
    unsigned char status;

    /* START */
    status = twi_start();

    if ((status != TWI_START) &&
        (status != TWI_REP_START))
    {
        twi_stop();
        return 0;
    }

    /* EEPROM address + WRITE */
    status = twi_write(EEPROM_I2C_ADDR << 1);

    if (status != TWI_MT_SLA_ACK)
    {
        twi_stop();
        return 0;
    }

    /* EEPROM memory address */
    status = twi_write(address);

    if (status != TWI_MT_DATA_ACK)
    {
        twi_stop();
        return 0;
    }

    /* Data byte */
    status = twi_write(data);

    if (status != TWI_MT_DATA_ACK)
    {
        twi_stop();
        return 0;
    }

    /* STOP */
    twi_stop();

    /* Wait for EEPROM internal write cycle */
    delay_ms(5);

    return 1;
}

/* =====================================================
   READ ONE BYTE FROM EXTERNAL 24C02 EEPROM
   Returns data byte, or 0 on error
   ===================================================== */

unsigned char i2c_eeprom_read(unsigned char address)
{
    unsigned char status;
    unsigned char data;

    /* START */
    status = twi_start();

    if ((status != TWI_START) &&
        (status != TWI_REP_START))
    {
        twi_stop();
        return 0;
    }

    /* EEPROM address + WRITE */
    status = twi_write(EEPROM_I2C_ADDR << 1);

    if (status != TWI_MT_SLA_ACK)
    {
        twi_stop();
        return 0;
    }

    /* Set EEPROM memory address pointer */
    status = twi_write(address);

    if (status != TWI_MT_DATA_ACK)
    {
        twi_stop();
        return 0;
    }

    /* Repeated START */
    status = twi_start();

    if ((status != TWI_START) &&
        (status != TWI_REP_START))
    {
        twi_stop();
        return 0;
    }

    /* EEPROM address + READ */
    status = twi_write((EEPROM_I2C_ADDR << 1) | 1);

    if (status != TWI_MR_SLA_ACK)
    {
        twi_stop();
        return 0;
    }

    /* Read one byte, finish with NACK */
    data = twi_read(0);

    /* STOP */
    twi_stop();

    return data;
}