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

   PB2 = hardware SPI SS pin
   PB2 is manually used as 74HC595 latch
   ===================================================== */

#define SS(x) do {                         \
    if (x)                                 \
        PORTB |= (1 << 2);                 \
    else                                   \
        PORTB &= ~(1 << 2);               \
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
   7-SEGMENT CODES

   Common-cathode / active-high segment codes.

        -- a --
       |       |
       f       b
       |       |
        -- g --
       |       |
       e       c
       |       |
        -- d --    DP

   0 = 0x3F
   1 = 0x06
   ...
   9 = 0x6F

   Bit 7 is used for decimal point.
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

/* Digits currently displayed */
volatile unsigned char d1 = 0;
volatile unsigned char d2 = 0;
volatile unsigned char d3 = 0;
volatile unsigned char d4 = 0;

/* Current multiplex position */
volatile unsigned char sel = 0;

/* Last decoded keypad key */
volatile unsigned char keypad = 0;

/* Keypad row results */
volatile unsigned char key_rows[4] =
{
    0, 0, 0, 0
};


/* =====================================================
   DISPLAY MODE

   0 = main menu
   1 = clock
   2 = temperature
   3 = UART
   ===================================================== */

#define DISPLAY_MAIN   0
#define DISPLAY_CLOCK  1
#define DISPLAY_TEMP   2
#define DISPLAY_UART   3

volatile unsigned char display_mode = DISPLAY_MAIN;


/* =====================================================
   BCD TO DECIMAL
   ===================================================== */

unsigned char bcd_to_dec(unsigned char bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}


/* =====================================================
   READ TIME FROM DS1307 USING HARDWARE TWI

   Returns:
       1 = successful
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


    /* -------------------------------------------------
       START
       ------------------------------------------------- */

    status = twi_start();

    if (status != 0x08 && status != 0x10)
    {
        twi_stop();
        return 0;
    }


    /* -------------------------------------------------
       DS1307 ADDRESS + WRITE
       ------------------------------------------------- */

    status = twi_write(DS1307_ADDR << 1);

    if (status != 0x18)
    {
        twi_stop();
        return 0;
    }


    /* -------------------------------------------------
       SELECT REGISTER 0x00 = SECONDS
       ------------------------------------------------- */

    status = twi_write(0x00);

    if (status != 0x28)
    {
        twi_stop();
        return 0;
    }


    /* -------------------------------------------------
       REPEATED START
       ------------------------------------------------- */

    status = twi_start();

    if (status != 0x10)
    {
        twi_stop();
        return 0;
    }


    /* -------------------------------------------------
       DS1307 ADDRESS + READ
       ------------------------------------------------- */

    status = twi_write((DS1307_ADDR << 1) | 1);

    if (status != 0x40)
    {
        twi_stop();
        return 0;
    }


    /* -------------------------------------------------
       READ SECONDS
       ACK
       ------------------------------------------------- */

    sec_bcd = twi_read(1);


    /* -------------------------------------------------
       READ MINUTES
       ACK
       ------------------------------------------------- */

    min_bcd = twi_read(1);


    /* -------------------------------------------------
       READ HOURS
       NACK
       ------------------------------------------------- */

    hour_bcd = twi_read(0);


    /* -------------------------------------------------
       STOP
       ------------------------------------------------- */

    twi_stop();


    /* -------------------------------------------------
       CONVERT SECONDS
       ------------------------------------------------- */

    *sec = bcd_to_dec(sec_bcd & 0x7F);


    /* -------------------------------------------------
       CONVERT MINUTES
       ------------------------------------------------- */

    *min = bcd_to_dec(min_bcd & 0x7F);


    /* -------------------------------------------------
       CONVERT HOURS

       DS1307 can be:
       - 24-hour mode
       - 12-hour mode
       ------------------------------------------------- */

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
   SET 7-SEGMENT TO MAIN MENU

   Display:

       0000

   ===================================================== */

void display_main(void)
{
    d1 = 0;
    d2 = 0;
    d3 = 0;
    d4 = 0;
}


/* =====================================================
   SET 7-SEGMENT TO CLOCK

   Example:

       14:30

   Since the current segment configuration does not
   explicitly define a colon, the numeric display is:

       1430
   ===================================================== */

void display_clock(unsigned char hour,
                   unsigned char min)
{
    d1 = hour / 10;
    d2 = hour % 10;

    d3 = min / 10;
    d4 = min % 10;
}


/* =====================================================
   SET 7-SEGMENT TO TEMPERATURE

   Example:

       25.3

   We use:

       d1 = 2
       d2 = 5 + decimal point
       d3 = 3
       d4 = blank/0

   Because the existing multiplex code always uses
   ss_code[d4], d4 is set to 0.

   The decimal point is added to d2.

   ===================================================== */

void display_temperature(unsigned int temp10)
{
    unsigned char whole;
    unsigned char decimal;

    whole = temp10 / 10;
    decimal = temp10 % 10;


    /* -----------------------------------------------
       Example: 25.3
       ----------------------------------------------- */

    if (whole >= 100)
    {
        /* For temperatures >= 100 C:
           display 100+ without decimal */

        d1 = (whole / 100) % 10;
        d2 = (whole / 10) % 10;
        d3 = whole % 10;
        d4 = 0;
    }
    else
    {
        d1 = whole / 10;

        /* Decimal point on second digit */
        d2 = whole % 10;

        d3 = decimal;

        d4 = 0;
    }
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

    char key;

    unsigned int temp10;

    char lcd_buffer[17];
    char uart_buffer[48];

    unsigned char uart_counter;


    /* =================================================
       INITIAL VALUES
       ================================================= */

    h = 0;
    m = 0;
    s = 0;

    rtc_ok = 0;

    last_key = 0;

    uart_counter = 0;


    /* =================================================
       PORT CONFIGURATION
       ================================================= */

    /*
       PB3 = MOSI
       PB5 = SCK
       PB2 = 74HC595 latch

       PB0 = keypad C1
       PB1 = keypad C2
       PB4 = keypad C4
    */

    DDRB = (1 << 3) |
           (1 << 5) |
           (1 << 2);


    /* -------------------------------------------------
       Keypad column pull-ups

       PB0
       PB1
       PB4

       PB2 latch HIGH
       ------------------------------------------------- */

    PORTB = (1 << 0) |
            (1 << 1) |
            (1 << 4) |
            (1 << 2);


    /* -------------------------------------------------
       PC3 = keypad C3
       ------------------------------------------------- */

    DDRC &= ~(1 << 3);

    PORTC |= (1 << 3);


    /* -------------------------------------------------
       PC4 = SDA
       PC5 = SCL

       Hardware TWI controls these pins.
       ------------------------------------------------- */

    DDRC &= ~((1 << 4) | (1 << 5));


    /* =================================================
       SPI CONFIGURATION
       ================================================= */

    SPCR = (1 << SPE) |
           (1 << MSTR) |
           (1 << SPR0);

    SPSR = 0;


    /* =================================================
       TWI INITIALIZATION
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
       lcd_init() MUST happen before menu_show().
       ================================================= */

    lcd_init();

    delay_ms(50);

    lcd_clear();


    /* =================================================
       MENU INITIALIZATION
       ================================================= */

    menu_init();


    /* =================================================
       DISPLAY STARTUP MESSAGE
       ================================================= */

    lcd_gotoxy(0, 0);
    lcd_puts("RTC Monitor");

    lcd_gotoxy(0, 1);
    lcd_puts("Starting...");


    /* 7-segment startup */
    display_main();


    delay_ms(500);


    /* =================================================
       SHOW MAIN MENU
       ================================================= */

    menu_show();

    display_mode = DISPLAY_MAIN;


    /* =================================================
       TIMER0 CONFIGURATION

       Normal mode
       Prescaler = 64
       Overflow interrupt enabled
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
        unsigned char current_menu;


        /* ---------------------------------------------
           Read current menu state
           --------------------------------------------- */

        current_menu = menu_get_state();


        /* ---------------------------------------------
           PROCESS NEW KEYPAD KEY

           Only process once per key press.
           --------------------------------------------- */

        if ((keypad != 0) && (last_key == 0))
        {
            key = keypad;


            /* -----------------------------------------
               Send key to menu state machine
               ----------------------------------------- */

            menu_process_key(key);


            /* -----------------------------------------
               Update menu state after processing
               ----------------------------------------- */

            current_menu = menu_get_state();


            /* -----------------------------------------
               MAIN MENU
               ----------------------------------------- */

            if (current_menu == MENU_MAIN)
            {
                display_mode = DISPLAY_MAIN;

                display_main();

                menu_show();
            }


            /* -----------------------------------------
               CLOCK MENU
               ----------------------------------------- */

            else if (current_menu == MENU_CLOCK)
            {
                display_mode = DISPLAY_CLOCK;

                lcd_clear();

                lcd_gotoxy(0, 0);
                lcd_puts("Clock");

                lcd_gotoxy(0, 1);
                lcd_puts("C = Back");
            }


            /* -----------------------------------------
               TEMPERATURE MENU
               ----------------------------------------- */

            else if (current_menu == MENU_TEMP)
            {
                display_mode = DISPLAY_TEMP;

                lcd_clear();

                lcd_gotoxy(0, 0);
                lcd_puts("Temperature");

                lcd_gotoxy(0, 1);
                lcd_puts("C = Back");
            }


            /* -----------------------------------------
               UART MENU
               ----------------------------------------- */

            else if (current_menu == MENU_UART)
            {
                display_mode = DISPLAY_UART;

                lcd_clear();

                lcd_gotoxy(0, 0);
                lcd_puts("UART DATA");

                lcd_gotoxy(0, 1);
                lcd_puts("Sending...");
            }


            last_key = keypad;
        }


        /* ---------------------------------------------
           KEY RELEASE

           Allows the next press to be detected.
           --------------------------------------------- */

        if (keypad == 0)
        {
            last_key = 0;
        }


        /* =================================================
           MENU MAIN
           ================================================= */

        if (current_menu == MENU_MAIN)
        {
            /*
               Main menu remains on LCD.

               7-segment:
                   0000
            */

            display_main();
        }


        /* =================================================
           CLOCK MENU
           ================================================= */

        else if (current_menu == MENU_CLOCK)
        {
            /* -----------------------------------------
               Read RTC
               ----------------------------------------- */

            rtc_ok = rtc_get_time_twi(&h, &m, &s);


            if (rtc_ok)
            {
                /* -------------------------------------
                   7-segment:

                       HHMM

                   Example:

                       1430
                   ------------------------------------- */

                display_clock(h, m);


                /* -------------------------------------
                   LCD

                       Time: 14:30:25
                       C = Back
                   ------------------------------------- */

                sprintf(lcd_buffer,
                        "Time: %02u:%02u:%02u",
                        (unsigned int)h,
                        (unsigned int)m,
                        (unsigned int)s);

                lcd_gotoxy(0, 0);
                lcd_puts("                ");

                lcd_gotoxy(0, 0);
                lcd_puts(lcd_buffer);

                lcd_gotoxy(0, 1);
                lcd_puts("C = Back        ");
            }
            else
            {
                /* RTC communication error */

                lcd_gotoxy(0, 0);
                lcd_puts("RTC I2C ERROR   ");

                lcd_gotoxy(0, 1);
                lcd_puts("C = Back        ");

                /* Blank-like value */
                display_main();
            }
        }


        /* =================================================
           TEMPERATURE MENU
           ================================================= */

        else if (current_menu == MENU_TEMP)
        {
            /* -----------------------------------------
               Read LM35
               ----------------------------------------- */

            temp10 = adc_get_temperature();


            /* -----------------------------------------
               7-segment:

                   25.3
               ----------------------------------------- */

            display_temperature(temp10);


            /* -----------------------------------------
               LCD
               ----------------------------------------- */

            sprintf(lcd_buffer,
                    "Temp: %u.%u C",
                    (unsigned int)(temp10 / 10),
                    (unsigned int)(temp10 % 10));

            lcd_gotoxy(0, 0);
            lcd_puts("                ");

            lcd_gotoxy(0, 0);
            lcd_puts("Temperature");

            lcd_gotoxy(0, 1);
            lcd_puts("                ");

            lcd_gotoxy(0, 1);
            lcd_puts(lcd_buffer);
        }


        /* =================================================
           UART MENU
           ================================================= */

        else if (current_menu == MENU_UART)
        {
            /* -----------------------------------------
               Read RTC
               ----------------------------------------- */

            rtc_ok = rtc_get_time_twi(&h, &m, &s);


            /* -----------------------------------------
               Read temperature
               ----------------------------------------- */

            temp10 = adc_get_temperature();


            /* -----------------------------------------
               Show time on 7-segment

               HHMM
               ----------------------------------------- */

            if (rtc_ok)
            {
                display_clock(h, m);
            }
            else
            {
                display_main();
            }


            /* -----------------------------------------
               UART transmission

               Approximately every 500 ms.

               Main loop delay = 100 ms
               Counter = 5
               ----------------------------------------- */

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
                    uart_puts("RTC I2C ERROR | Temperature available\r\n");
                }
            }


            /* -----------------------------------------
               LCD
               ----------------------------------------- */

            lcd_gotoxy(0, 0);
            lcd_puts("UART DATA       ");

            lcd_gotoxy(0, 1);
            lcd_puts("Sending... C=Back");
        }


        /* =================================================
           MAIN LOOP DELAY
           ================================================= */

        delay_ms(100);
    }
}


/* =====================================================
   SEND DATA TO TWO CASCADED 74HC595 REGISTERS

   First byte:
       farther 74HC595

   Second byte:
       nearer 74HC595

   PB2:
       latch
   ===================================================== */

void send_data_7seg_keypad(unsigned char data_7seg,
                           unsigned char sel_byte)
{
    /* -----------------------------------------------
       Latch LOW
       ----------------------------------------------- */

    SS(0);

    delay_us(2);


    /* -----------------------------------------------
       First byte -> farther 74HC595
       ----------------------------------------------- */

    SPDR = sel_byte;

    while (!(SPSR & (1 << SPIF)))
    {
    }


    /* -----------------------------------------------
       Second byte -> nearer 74HC595
       ----------------------------------------------- */

    SPDR = data_7seg;

    while (!(SPSR & (1 << SPIF)))
    {
    }


    /* -----------------------------------------------
       Latch HIGH
       ----------------------------------------------- */

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


    /* -----------------------------------------------
       Reload Timer0
       ----------------------------------------------- */

    TCNT0 = 6;


    /* -----------------------------------------------
       BLANK DISPLAY

       Prevent ghosting during digit switching.
       ----------------------------------------------- */

    send_data_7seg_keypad(0x00, 0xF0);


    /* -----------------------------------------------
       SELECT CURRENT DIGIT
       ----------------------------------------------- */

    switch (sel)
    {
        case 0:

            send_data_7seg_keypad(ss_code[d1],
                                  0xE1);

            break;


        case 1:

            /*
               Decimal point for temperature.

               When displaying temperature:

                   25.3

               d2 contains the '5'.

               Add DP to d2.
            */

            if (display_mode == DISPLAY_TEMP)
            {
                send_data_7seg_keypad(ss_code[d2] | 0x80,
                                      0xD2);
            }
            else
            {
                send_data_7seg_keypad(ss_code[d2],
                                      0xD2);
            }

            break;


        case 2:

            send_data_7seg_keypad(ss_code[d3],
                                  0xB4);

            break;


        case 3:

            send_data_7seg_keypad(ss_code[d4],
                                  0x78);

            break;
    }


    /* Small settling time */

    delay_us(5);


    /* -----------------------------------------------
       READ ACTIVE-LOW KEYPAD COLUMNS
       ----------------------------------------------- */

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


    /* -----------------------------------------------
       Store keypad row
       ----------------------------------------------- */

    key_rows[sel] = cols;


    /* -----------------------------------------------
       Advance display/keypad position
       ----------------------------------------------- */

    sel++;


    if (sel >= 4)
    {
        sel = 0;

        keypad = decode_keypad();
    }
}


/* =====================================================
   DECODE MATRIX KEYPAD

   Returns:
       pressed key

   Returns:
       0 if no key is pressed
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