#include <io.h>
#include <delay.h>
#include "lcd.h"
#define SS(x)(x==1 ? (PORTC|= 1<<3):(PORTC&=~(1<<3)))

void send_data_7seg_keypad(char data_7seg,char sel);

void main(void)
{   DDRB=0x28;
    SPCR=(1<<MSTR)|(1<<SPE); 
    SPSR=(1<<SPI2X);
   
    lcd_init();
    lcd_puts("welcome");
    send_data_7seg_keypad(0x3F,0x2F);
    
    while (1)
    {
        // Main loop
    }
}


void send_data_7seg_keypad(char data_7seg,char sel)
{
	SS(0);
	delay_us(10);
	//SPDR=outputs;
	//while(!(SPSR & (1<<SPIF)));//wait until transfer is finished
	SPDR=sel;
	while(!(SPSR & (1<<SPIF)));//wait until transfer is finished
	SPDR=data_7seg;
	while(!(SPSR & (1<<SPIF)));//wait until transfer is finished
	SS(1);
}
