#include <io.h>
#include <delay.h>
#include "lcd.h"
#define SS(x)(x==1 ? (PORTC|= 1<<3):(PORTC&=~(1<<3)))

void send_data_7seg_keypad(char data_7seg,char sel);
flash char ss_code[]=
{
    0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};


void main(void)
{   DDRB=0x28;
    PORTB=0x17;
    PORTC|=0x30;
    PORTD|=0x01;
    DDRC=0x08;
    
    SPCR=(1<<MSTR)|(1<<SPE)|(1<<SPR0)|(0<<SPR1); 
    SPSR=(0<<SPI2X);
   
    lcd_init();
    lcd_puts("welcome");
    //send_data_7seg_keypad(0x3F,0xF1);  
    
    TCCR0B=0x3;
    TIMSK0=0x1;
    TCNT0=5;
    
    while (1)
    {
        
        send_data_7seg_keypad(0x3F,0xF1);
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
