
#ifndef TWI_H
#define TWI_H

void twi_init(void);
unsigned char twi_start(void);
void twi_stop(void);
unsigned char twi_write(unsigned char data);
unsigned char twi_read(unsigned char ack);

#endif