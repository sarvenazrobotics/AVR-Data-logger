#include <io.h>
#include <interrupt.h>
#include <delay.h>
#include "lcd.h"
#define SS(x)(x==1 ? (PORTC|= 1<<3):(PORTC&=~(1<<3)))
#define C1 ((PINB & (1 << 0)) >> 0)
#define C2 ((PINB & (1 << 1)) >> 1)
#define C3 ((PINB & (1 << 2)) >> 2)
#define C4 ((PINB & (1 << 4)) >> 4)


void send_data_7seg_keypad(char data_7seg,char sel);
flash char ss_code[]=
{
    0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};
    
char d1,d2,d3,d4,sel;
char keypad;
volatile char keypad = 0;  // Last detected key; 0 means no key
volatile unsigned char key_rows[4] = {0, 0, 0, 0};

flash char keymap[4][4] =
{
    {'7', '8', '9', '/'},
    {'4', '5', '6', '*'},
    {'1', '2', '3', '-'},
    {'C', '0', '=', '+'}
};


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
    
    #asm("sei")
    
    while (1)
    {
        
        
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

interrupt [TIM0_OVF] void timer0_ovf_isr(void)
{

    keypad=(C4<<4)|(C3<<3)|(C2<<2)|(C1<<1);
        send_data_7seg_keypad(ss_code[d1], 0xE1);
delay_us(5);

if (C1 == 0)
{
    keypad = 7;
}
else if (C2 == 0)
{
    keypad = 8;
}
else if (C3 == 0)
{
    keypad = 9;
}
else if (C4 == 0)
{
    keypad = 10;  // Example: division key
}
    // Blank all digits first
    send_data_7seg_keypad(0x00, 0xF0);

    switch (sel)
    {
        case 0:
            send_data_7seg_keypad(ss_code[d1], 0xE1);
            break;

        case 1:
            send_data_7seg_keypad(ss_code[d2], 0xD2);
            break;

        case 2:
            send_data_7seg_keypad(ss_code[d3], 0xB4);
            break;

        case 3:
            send_data_7seg_keypad(ss_code[d4], 0x78);
            break;
    }

    // Cycle through digits 0, 1, 2, 3
    sel++;
    if (sel >= 4)
        sel = 0;
}


char decode_keypad(void)
{
    unsigned char r, c;

    for (r = 0; r < 4; r++)
    {
        for (c = 0; c < 4; c++)
        {
            if (key_rows[r] & (1 << c))
            {
                return keymap[r][c];
            }
        }
    }

    return 0;  // No key pressed
}