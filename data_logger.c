
#include <io.h>
#include <interrupt.h>
#include <stdio.h>
#include <delay.h>
#include "lcd.h"
#include "twi.h"
#include "adc.h"
#include "uart.h"
#include "menu.h"

/* =====================================================
   SPI CHIP SELECT / 74HC595 LATCH
   PB2 = hardware SPI SS pin, configured as OUTPUT
   ===================================================== */

#define SS(x) do { \
    if (x) PORTB |= (1 << 2); \
    else   PORTB &= ~(1 << 2); \
} while (0)

/* =====================================================
   KEYPAD COLUMNS
   C1 = PB0
   C2 = PB1
   C3 = PC3
   C4 = PB4
   ===================================================== */

#define C1 ((PINB & (1 << 0)) ? 1 : 0)
#define C2 ((PINB & (1 << 1)) ? 1 : 0)
#define C3 ((PINC & (1 << 3)) ? 1 : 0)
#define C4 ((PINB & (1 << 4)) ? 1 : 0)

/* =====================================================
   DS1307 ADDRESS
   7-bit address = 0x68
   ===================================================== */

#define DS1307_ADDR 0x68

/* Function prototypes */
void send_data_7seg_keypad(unsigned char data_7seg,
                           unsigned char sel_byte);

char decode_keypad(void);

unsigned char bcd_to_dec(unsigned char bcd);


unsigned char rtc_get_time_twi(unsigned char *hour,
                               unsigned char *min,
                               unsigned char *sec);

/* =====================================================
   SEVEN-SEGMENT DIGIT CODES
   ===================================================== */

flash unsigned char ss_code[10] =
{
    0x3F,  /* 0 */
    0x06,  /* 1 */
    0x5B,  /* 2 */
    0x4F,  /* 3 */
    0x66,  /* 4 */
    0x6D,  /* 5 */
    0x7D,  /* 6 */
    0x07,  /* 7 */
    0x7F,  /* 8 */
    0x6F   /* 9 */
};

/* =====================================================
   KEYPAD MAP
   ===================================================== */

flash char keymap[4][4] =
{
    {'7', '8', '9', '/'},
    {'4', '5', '6', '*'},
    {'1', '2', '3', '-'},
    {'C', '0', '=', '+'}
};

/* =====================================================
   VARIABLES SHARED WITH TIMER0 INTERRUPT
   ===================================================== */

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

/* =====================================================
   BCD TO DECIMAL
   ===================================================== */

unsigned char bcd_to_dec(unsigned char bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

/* =====================================================
   READ TIME FROM DS1307 USING HARDWARE TWI
   Returns 1 on success, 0 on error.
   ===================================================== */

unsigned char rtc_get_time_twi(unsigned char *hour,
                               unsigned char *min,
                               unsigned char *sec)
{
    unsigned char status;
    unsigned char sec_bcd;
    unsigned char min_bcd;
    unsigned char hour_bcd;
    unsigned char hour_value;

    /* START */
    status = twi_start();

    if (status != 0x08 && status != 0x10)
    {
        twi_stop();
        return 0;
    }

    /* DS1307 address + WRITE */
    status = twi_write(DS1307_ADDR << 1);

    if (status != 0x18)
    {
        twi_stop();
        return 0;
    }

    /* Select seconds register */
    status = twi_write(0x00);

    if (status != 0x28)
    {
        twi_stop();
        return 0;
    }

    /* Repeated START */
    status = twi_start();

    if (status != 0x10)
    {
        twi_stop();
        return 0;
    }

    /* DS1307 address + READ */
    status = twi_write((DS1307_ADDR << 1) | 1);

    if (status != 0x40)
    {
        twi_stop();
        return 0;
    }

    /* Read seconds and minutes with ACK */
    sec_bcd = twi_read(1);
    min_bcd = twi_read(1);

    /* Read hours with NACK */
    hour_bcd = twi_read(0);

    twi_stop();

    /* Convert seconds and minutes */
    *sec = bcd_to_dec(sec_bcd & 0x7F);
    *min = bcd_to_dec(min_bcd & 0x7F);

    /* Convert hours */
    if (hour_bcd & 0x40)
    {
        /* 12-hour mode */
        hour_value = bcd_to_dec(hour_bcd & 0x1F);

        if (hour_bcd & 0x20)
        {
            /* PM */
            if (hour_value < 12)
                hour_value += 12;
        }
        else
        {
            /* AM */
            if (hour_value == 12)
                hour_value = 0;
        }

        *hour = hour_value;
    }
    else
    {
        /* 24-hour mode */
        *hour = bcd_to_dec(hour_bcd & 0x3F);
    }

    return 1;
}

/* =====================================================
   MAIN
   ===================================================== */

void main(void)
{
    unsigned char h;
    unsigned char m;
    unsigned char s;
    unsigned char rtc_ok;

    unsigned char last_key;
    unsigned char col;
    char key;
    
    unsigned int adc_value;
    unsigned long temp10;
    char temp_buffer[17];
    char uart_buffer[48];

    char time_str[17];

    h = 0;
    m = 0;
    s = 0;
    rtc_ok = 0;

    last_key = 0;
    col = 0;

    /* =================================================
       PORT CONFIGURATION
       ================================================= */

    /* PB3 = MOSI output
       PB5 = SCK output
       PB2 = SPI SS / latch output
       PB0, PB1, PB4 = keypad column inputs
    */
    DDRB = (1 << 3) |
           (1 << 5) |
           (1 << 2);

    /* Enable pull-ups on keypad columns */
    PORTB = (1 << 0) |
            (1 << 1) |
            (1 << 4) |
            (1 << 2);

    /* PC3 = keypad column input */
    DDRC &= ~(1 << 3);
    PORTC |= (1 << 3);

    /* Hardware TWI uses PC4 = SDA and PC5 = SCL.
       Keep PC4 and PC5 configured as inputs.
    */
    DDRC &= ~((1 << 4) | (1 << 5));

    /* =================================================
       SPI CONFIGURATION
       ================================================= */

    SPCR = (1 << SPE) |
           (1 << MSTR) |
           (1 << SPR0);

    SPSR = 0;

    /* =================================================
       HARDWARE TWI INITIALIZATION
       ================================================= */

    twi_init();

    delay_ms(100);
    
    adc_init();
    uart_init(); 
    
    menu_init();
    menu_show();

    /* =================================================
       LCD INITIALIZATION
       ================================================= */

    lcd_init();
    delay_ms(50);
    lcd_clear();

    lcd_gotoxy(0, 0);
    lcd_puts("RTC starting...");

    lcd_gotoxy(0, 1);
    lcd_puts("                ");

    delay_ms(500);

    /* =================================================
       TIMER0 CONFIGURATION
       ================================================= */

    TCCR0A = 0x00;
    TCCR0B = 0x03;

    TIMSK0 = (1 << TOIE0);
    TCNT0 = 6;

    /* Enable global interrupts */
    #asm("sei")

    /* =================================================
       MAIN LOOP
       ================================================= */

    while (1)
    {     
    
        
        
        /* Read DS1307 */
        rtc_ok = rtc_get_time_twi(&h, &m, &s);

        /* Display time on first LCD line */
        lcd_gotoxy(0, 0);

        if (rtc_ok)
        {
            sprintf(uart_buffer, "Time: %02u:%02u:%02u",
                    (unsigned int)h,
                    (unsigned int)m,
                    (unsigned int)s);

            lcd_puts(uart_buffer);
            uart_puts(uart_buffer);
        }
        else
        {
            lcd_puts("RTC I2C ERROR   ");
            uart_puts(uart_buffer);
        }  
        
        adc_display_temperature();

        /* Process newly pressed keypad key once */
        if ((keypad != 0) && (last_key == 0))
        {
            key = keypad;

            if (key == 'C')
            {
                /* Clear second line */
                lcd_gotoxy(0, 1);
                lcd_puts("                ");
                col = 0;
            }
            else
            {
                /* Display keypad input on second line */
                lcd_gotoxy(col, 1);
                lcd_data(key);

                col++;

                if (col >= 16)
                {
                    col = 0;
                    lcd_gotoxy(0, 1);
                    lcd_puts("                ");
                }
            }

            last_key = keypad;
        }

        /* Detect key release */
        if (keypad == 0)
        {
            last_key = 0;
        }

        delay_ms(100);
    }
}

/* =====================================================
   SEND DATA TO TWO CASCADED 74HC595 REGISTERS
   ===================================================== */

void send_data_7seg_keypad(unsigned char data_7seg,
                           unsigned char sel_byte)
{
    SS(0);
    delay_us(2);

    /* First byte goes to farther register */
    SPDR = sel_byte;

    while (!(SPSR & (1 << SPIF)))
    {
    }

    /* Second byte goes to nearer register */
    SPDR = data_7seg;

    while (!(SPSR & (1 << SPIF)))
    {
    }

    /* Latch output */
    SS(1);
}

/* =====================================================
   TIMER0 OVERFLOW INTERRUPT
   Refreshes display and scans keypad
   ===================================================== */

interrupt [TIM0_OVF] void timer0_ovf_isr(void)
{
    unsigned char cols;

    /* Reload timer */
    TCNT0 = 6;

    /* Blank display during switching */
    send_data_7seg_keypad(0x00, 0xF0);

    /* Refresh selected digit and keypad row */
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

    delay_us(5);

    /* Read active-low keypad columns */
    cols = 0;

    if (C1 == 0)
        cols |= (1 << 0);

    if (C2 == 0)
        cols |= (1 << 1);

    if (C3 == 0)
        cols |= (1 << 2);

    if (C4 == 0)
        cols |= (1 << 3);

    key_rows[sel] = cols;

    /* Advance to next digit / row */
    sel++;

    if (sel >= 4)
    {
        sel = 0;
        keypad = decode_keypad();
    }
}

/* =====================================================
   DECODE MATRIX KEYPAD
   ===================================================== */

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