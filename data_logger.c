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

   PB2 = SPI SS / 74HC595 latch
   ===================================================== */

#define SS(x) do {                  \
    if (x)                          \
        PORTB |= (1 << 2);         \
    else                            \
        PORTB &= ~(1 << 2);        \
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
   DS1307
   ===================================================== */

#define DS1307_ADDR 0x68


/* =====================================================
   FUNCTION PROTOTYPES
   ===================================================== */

void send_data_7seg_keypad(unsigned char data_7seg,
                           unsigned char sel_byte);

char decode_keypad(void);

unsigned char bcd_to_dec(unsigned char bcd);

unsigned char rtc_get_time_twi(unsigned char *hour,
                               unsigned char *min,
                               unsigned char *sec);


/* =====================================================
   7-SEGMENT DIGIT CODES
   ===================================================== */

flash unsigned char ss_code[10] =
{
    0x3F,       /* 0 */
    0x06,       /* 1 */
    0x5B,       /* 2 */
    0x4F,       /* 3 */
    0x66,       /* 4 */
    0x6D,       /* 5 */
    0x7D,       /* 6 */
    0x07,       /* 7 */
    0x7F,       /* 8 */
    0x6F        /* 9 */
};


/* =====================================================
   KEYPAD MAP

        C1  C2  C3  C4

   R1   7   8   9   /
   R2   4   5   6   *
   R3   1   2   3   -
   R4   C   0   =   +
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
   READ TIME FROM DS1307
   Hardware TWI

   Return:
       1 = success
       0 = error
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


    /* Read seconds */

    sec_bcd = twi_read(1);

    /* Read minutes */

    min_bcd = twi_read(1);

    /* Read hours - last byte, NACK */

    hour_bcd = twi_read(0);

    twi_stop();


    /* Convert seconds */

    *sec = bcd_to_dec(sec_bcd & 0x7F);


    /* Convert minutes */

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

    unsigned int temp10;

    unsigned char uart_counter;

    char key;

    char lcd_buffer[17];

    char uart_buffer[48];


    /* =================================================
       INITIAL VALUES
       ================================================= */

    h = 0;
    m = 0;
    s = 0;

    rtc_ok = 0;

    last_key = 0;

    temp10 = 0;

    uart_counter = 0;


    /* =================================================
       PORT CONFIGURATION
       ================================================= */

    /*
       PB3 = MOSI
       PB5 = SCK
       PB2 = SPI SS / 74HC595 latch

       PB0 = keypad C1
       PB1 = keypad C2
       PB4 = keypad C4
    */

    DDRB = (1 << 3) |
           (1 << 5) |
           (1 << 2);


    /* Enable keypad pull-ups */

    PORTB = (1 << 0) |
            (1 << 1) |
            (1 << 4) |
            (1 << 2);


    /*
       PC3 = keypad C3
    */

    DDRC &= ~(1 << 3);

    PORTC |= (1 << 3);


    /*
       PC4 = SDA
       PC5 = SCL

       Hardware TWI
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


    /* =================================================
       ADC INITIALIZATION
       ================================================= */

    adc_init();


    /* =================================================
       UART INITIALIZATION
       ================================================= */

    uart_init();


    /* =================================================
       LCD INITIALIZATION

       IMPORTANT:
       LCD must be initialized BEFORE menu_show().
       ================================================= */

    lcd_init();

    delay_ms(50);

    lcd_clear();


    /* =================================================
       MENU INITIALIZATION
       ================================================= */

    menu_init();

    menu_show();


    /* =================================================
       TIMER0 CONFIGURATION

       Timer0:
       - 7-segment refresh
       - keypad scanning
       ================================================= */

    TCCR0A = 0x00;

    TCCR0B = 0x03;

    TIMSK0 = (1 << TOIE0);

    TCNT0 = 6;


    /* =================================================
       ENABLE GLOBAL INTERRUPTS
       ================================================= */

    #asm("sei")


    /* =================================================
       MAIN LOOP
       ================================================= */

    while (1)
    {

        /* =================================================
           KEYPAD PROCESSING

           Timer0 ISR detects the key.
           Main loop processes it.
           ================================================= */

        if ((keypad != 0) && (last_key == 0))
        {
            key = keypad;

            /*
               Send key to menu state machine
            */

            menu_process_key(key);

            /*
               Remember pressed key.
               This prevents repeated execution while
               the key is being held down.
            */

            last_key = keypad;
        }


        /* =================================================
           DETECT KEY RELEASE
           ================================================= */

        if (keypad == 0)
        {
            last_key = 0;
        }


        /* =================================================
           GET CURRENT MENU STATE
           ================================================= */

        switch (menu_get_state())
        {

            /* =============================================
               MAIN MENU
               ============================================= */

            case MENU_MAIN:

                /*
                   menu_show() already displays the menu.

                   Nothing else is continuously written here.
                */

                break;


            /* =============================================
               CLOCK MENU
               ============================================= */

            case MENU_CLOCK:

                /*
                   Read DS1307
                */

                rtc_ok = rtc_get_time_twi(&h, &m, &s);


                if (rtc_ok)
                {
                    /*
                       Line 1:
                       Time: 12:34:56
                    */

                    sprintf(lcd_buffer,
                            "Time: %02u:%02u:%02u",
                            (unsigned int)h,
                            (unsigned int)m,
                            (unsigned int)s);

                    lcd_gotoxy(0, 0);

                    lcd_puts(lcd_buffer);
                }
                else
                {
                    lcd_gotoxy(0, 0);

                    lcd_puts("RTC I2C ERROR   ");
                }


                /*
                   Line 2
                */

                lcd_gotoxy(0, 1);

                lcd_puts("C = Back        ");

                break;


            /* =============================================
               TEMPERATURE MENU
               ============================================= */

            case MENU_TEMP:

                /*
                   Display title
                */

                lcd_gotoxy(0, 0);

                lcd_puts("TEMPERATURE     ");


                /*
                   adc_display_temperature()
                   displays the LM35 result on LCD.
                */

                adc_display_temperature();


                break;


            /* =============================================
               UART MENU
               ============================================= */

            case MENU_UART:

                /*
                   Read RTC
                */

                rtc_ok = rtc_get_time_twi(&h, &m, &s);


                /*
                   Read LM35
                */

                temp10 = adc_get_temperature();


                /*
                   LCD
                */

                lcd_gotoxy(0, 0);

                lcd_puts("UART DATA       ");


                lcd_gotoxy(0, 1);

                lcd_puts("Sending...      ");


                /*
                   UART transmission

                   uart_counter counts main-loop cycles.

                   This prevents the UART from being
                   flooded continuously.
                */

                uart_counter++;

                if (uart_counter >= 5)
                {
                    uart_counter = 0;


                    if (rtc_ok)
                    {
                        sprintf(uart_buffer,
                                "Time: %02u:%02u:%02u | Temp: %u.%u C\r\n",
                                (unsigned int)h,
                                (unsigned int)m,
                                (unsigned int)s,
                                (unsigned int)(temp10 / 10),
                                (unsigned int)(temp10 % 10));

                        uart_puts(uart_buffer);
                    }
                    else
                    {
                        sprintf(uart_buffer,
                                "RTC I2C ERROR | Temp: %u.%u C\r\n",
                                (unsigned int)(temp10 / 10),
                                (unsigned int)(temp10 % 10));

                        uart_puts(uart_buffer);
                    }
                }

                break;


            /* =============================================
               SAFETY
               ============================================= */

            default:

                menu_init();

                menu_show();

                break;
        }


        /*
           Small delay.

           Timer0 interrupt continues running during
           this delay, so keypad scanning and 7-segment
           refresh continue.
        */

        delay_ms(100);
    }
}


/* =====================================================
   SEND DATA TO TWO CASCADED 74HC595
   ===================================================== */

void send_data_7seg_keypad(unsigned char data_7seg,
                           unsigned char sel_byte)
{
    /*
       Enable shift register
    */

    SS(0);

    delay_us(2);


    /*
       First byte:
       farther 74HC595
    */

    SPDR = sel_byte;

    while (!(SPSR & (1 << SPIF)))
    {
    }


    /*
       Second byte:
       nearer 74HC595
    */

    SPDR = data_7seg;

    while (!(SPSR & (1 << SPIF)))
    {
    }


    /*
       Latch data
    */

    SS(1);
}


/* =====================================================
   TIMER0 OVERFLOW INTERRUPT

   Responsibilities:
   1. Refresh 7-segment display
   2. Scan keypad
   ===================================================== */

interrupt [TIM0_OVF] void timer0_ovf_isr(void)
{
    unsigned char cols;


    /* Reload timer */

    TCNT0 = 6;


    /* =================================================
       BLANK DISPLAY
       ================================================= */

    send_data_7seg_keypad(0x00, 0xF0);


    /* =================================================
       SELECT DIGIT / KEYPAD ROW
       ================================================= */

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


    /* =================================================
       READ KEYPAD COLUMNS

       Columns are active LOW.
       ================================================= */

    cols = 0;


    if (C1 == 0)
        cols |= (1 << 0);


    if (C2 == 0)
        cols |= (1 << 1);


    if (C3 == 0)
        cols |= (1 << 2);


    if (C4 == 0)
        cols |= (1 << 3);


    /*
       Store columns for current row
    */

    key_rows[sel] = cols;


    /* =================================================
       NEXT ROW
       ================================================= */

    sel++;


    if (sel >= 4)
    {
        sel = 0;

        /*
           After all four rows have been scanned,
           decode the keypad.
        */

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