#include <io.h>
#include <stdio.h>
#include "lcd.h"
#include "menu.h"
#include "adc.h"
#include "uart.h"

/* Menu states */
#define MENU_MAIN   0
#define MENU_CLOCK  1
#define MENU_TEMP   2
#define MENU_UART   3

unsigned char menu_state;
unsigned char menu_item;

/* Initialize menu */
void menu_init(void)
{
    menu_state = MENU_MAIN;
    menu_item = 1;
}

/* Show main menu */
void menu_show(void)
{
    lcd_clear();

    lcd_gotoxy(0,0);
    lcd_puts("1.Clock 2.Temp");

    lcd_gotoxy(0,1);
    lcd_puts("3.UART   Select");
}

/* Process keypad key */
void menu_process_key(char key)
{
    switch (menu_state)
    {
        /* ================= MAIN MENU ================= */
        case MENU_MAIN:

            if (key == '1')
            {
                menu_state = MENU_CLOCK;

                lcd_clear();
                lcd_gotoxy(0,0);
                lcd_puts("CLOCK");
            }

            else if (key == '2')
            {
                menu_state = MENU_TEMP;

                lcd_clear();
                lcd_gotoxy(0,0);
                lcd_puts("TEMPERATURE");
            }

            else if (key == '3')
            {
                menu_state = MENU_UART;

                lcd_clear();
                lcd_gotoxy(0,0);
                lcd_puts("UART DATA");
            }

            break;


        /* ================= CLOCK ================= */

        case MENU_CLOCK:

            if (key == 'C')
            {
                menu_state = MENU_MAIN;
                menu_show();
            }

            break;


        /* ================= TEMPERATURE ================= */

        case MENU_TEMP:

            if (key == 'C')
            {
                menu_state = MENU_MAIN;
                menu_show();
            }

            break;


        /* ================= UART ================= */

        case MENU_UART:

            if (key == 'C')
            {
                menu_state = MENU_MAIN;
                menu_show();
            }

            break;
    }
}