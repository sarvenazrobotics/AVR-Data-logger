#include <io.h>
#include <interrupt.h>
#include <delay.h>
#include "lcd.h"

/* Software chip select / latch */
#define SS(x) do { \
    if (x) PORTC |= (1 << 3); \
    else   PORTC &= ~(1 << 3); \
} while (0)

/* Keypad columns
   C1 = PB0
   C2 = PB1
   C3 = PC2  (moved from PB2 because PB2 is SPI SS)
   C4 = PB4
*/
#define C1 ((PINB & (1 << 0)) ? 1 : 0)
#define C2 ((PINB & (1 << 1)) ? 1 : 0)
#define C3 ((PINC & (1 << 2)) ? 1 : 0)
#define C4 ((PINB & (1 << 4)) ? 1 : 0)

/* Function prototypes */
void send_data_7seg_keypad(unsigned char data_7seg,
                           unsigned char sel_byte);

char decode_keypad(void);

/* Seven-segment codes for digits 0-9 */
flash unsigned char ss_code[10] =
{
    0x3F,   /* 0 */
    0x06,   /* 1 */
    0x5B,   /* 2 */
    0x4F,   /* 3 */
    0x66,   /* 4 */
    0x6D,   /* 5 */
    0x7D,   /* 6 */
    0x07,   /* 7 */
    0x7F,   /* 8 */
    0x6F    /* 9 */
};

/* Keypad map */
flash char keymap[4][4] =
{
    {'7', '8', '9', '/'},
    {'4', '5', '6', '*'},
    {'1', '2', '3', '-'},
    {'C', '0', '=', '+'}
};

/* Variables shared between main program and interrupt */
volatile unsigned char d1 = 0;
volatile unsigned char d2 = 0;
volatile unsigned char d3 = 0;
volatile unsigned char d4 = 0;

volatile unsigned char sel = 0;
volatile unsigned char keypad = 0;

volatile unsigned char key_rows[4] =
{
    0, 0, 0, 0
};

/* =========================================================
   MAIN
   ========================================================= */

void main(void)
{
    unsigned char last_key;

    last_key = 0;

    /* PB3 = MOSI output
       PB5 = SCK output
       PB2 = hardware SS output, required for SPI master mode
       PB0, PB1, PB4 = keypad column inputs
    */
    DDRB = (1 << 3) | (1 << 5) | (1 << 2);

    /* Enable pull-ups on keypad columns PB0, PB1, PB4 */
    PORTB = (1 << 0) | (1 << 1) | (1 << 4);

    /* PC3 = software chip select / latch output
       PC2 = keypad column C3 input
    */
    DDRC |= (1 << 3);
    DDRC &= ~(1 << 2);

    /* Enable pull-up on PC2 */
    PORTC |= (1 << 2);

    /* Keep software chip select inactive initially */
    PORTC |= (1 << 3);

    /* Configure SPI as master
       SPI enabled, clock prescaler = 16
    */
    SPCR = (1 << SPE) |
           (1 << MSTR) |
           (1 << SPR0);

    SPSR = 0;

    /* Initialize LCD */
    lcd_init();
    lcd_puts("welcome");

    /* Timer0 normal mode, prescaler = 64 */
    TCCR0A = 0x00;
    TCCR0B = 0x03;

    /* Enable Timer0 overflow interrupt */
    TIMSK0 = (1 << TOIE0);

    /* Initial timer count */
    TCNT0 = 5;

    /* Enable global interrupts */
    #asm("sei")

    while (1)
    {
        /* Process a newly pressed key only once */
        if ((keypad != 0) && (last_key == 0))
        {
            if ((keypad >= '0') && (keypad <= '9'))
            {
                /* Shift displayed digits to the left */
                d1 = d2;
                d2 = d3;
                d3 = d4;

                /* Convert ASCII character to numeric digit */
                d4 = keypad - '0';
            }

            /* Remember the key until it is released */
            last_key = keypad;
        }

        /* Allow the same key to be pressed again */
        if (keypad == 0)
        {
            last_key = 0;
        }
    }
}

/* =========================================================
   SEND DATA TO TWO CASCADED 74HC595 REGISTERS
   ========================================================= */

void send_data_7seg_keypad(unsigned char data_7seg,
                           unsigned char sel_byte)
{
    /* Start transfer */
    SS(0);

    /* Allow chip select to settle */
    delay_us(2);

    /* First byte goes to the farther shift register */
    SPDR = sel_byte;

    while (!(SPSR & (1 << SPIF)))
    {
    }

    /* Second byte goes to the nearer shift register */
    SPDR = data_7seg;

    while (!(SPSR & (1 << SPIF)))
    {
    }

    /* Latch the transferred data */
    SS(1);
}

/* =========================================================
   TIMER0 OVERFLOW INTERRUPT
   Refreshes display and scans keypad rows
   ========================================================= */

interrupt [TIM0_OVF] void timer0_ovf_isr(void)
{
    unsigned char cols;

    /* Reload timer for approximately 1 ms at 16 MHz / 64 */
    TCNT0 = 6;

    /* Blank display while changing selection */
    send_data_7seg_keypad(0x00, 0xF0);

    /* Select digit and corresponding keypad row */
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

    /* Allow outputs to settle before reading keypad */
    delay_us(5);

    /* Read active-low keypad columns */
    cols = 0;

    if (C1 == 0)
    {
        cols |= (1 << 0);
    }

    if (C2 == 0)
    {
        cols |= (1 << 1);
    }

    if (C3 == 0)
    {
        cols |= (1 << 2);
    }

    if (C4 == 0)
    {
        cols |= (1 << 3);
    }

    /* Store column state for the currently selected row */
    key_rows[sel] = cols;

    /* Move to next digit / keypad row */
    sel++;

    if (sel >= 4)
    {
        sel = 0;

        /* Decode key after scanning all four rows */
        keypad = decode_keypad();
    }
}

/* =========================================================
   DECODE MATRIX KEYPAD
   Returns the pressed key, or 0 if no key is pressed
   ========================================================= */

char decode_keypad(void)
{
    unsigned char r;
    unsigned char c;

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

    return 0;
}