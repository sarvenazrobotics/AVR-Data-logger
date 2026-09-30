
#ifndef I2C_H
#define I2C_H

#include <io.h>

/* CPU clock */
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

/* I2C clock frequency */
#define TWI_SCL_HZ       100000UL

/* External 24C02 EEPROM 7-bit address */
#define EEPROM_I2C_ADDR  0x50

/* TWI status codes */
#define TWI_START        0x08
#define TWI_REP_START    0x10
#define TWI_MT_SLA_ACK   0x18
#define TWI_MT_DATA_ACK  0x28
#define TWI_MR_SLA_ACK   0x40

/* I2C function declarations */
void twi_init(void);

unsigned char twi_start(void);
void twi_stop(void);

unsigned char twi_write(unsigned char data);
unsigned char twi_read(unsigned char ack);

/* External EEPROM functions */
unsigned char i2c_eeprom_write(unsigned char address,
                               unsigned char data);

unsigned char i2c_eeprom_read(unsigned char address);

#endif