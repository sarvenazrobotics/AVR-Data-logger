#include "menu.h"
#include "lcd.h"

static unsigned char menu_state;


/* Initialize menu */
void menu_init(void)
{
    menu_state = MENU_MAIN;
}


/* Return current menu state */
unsigned char menu_get_state(void)
{
    return menu_state;
}


/* Display main menu */
void menu_show(void)
{
    lcd_clear();

    lcd_gotoxy(0, 0);
    lcd_puts("1.Clock 2.Temp");

    lcd_gotoxy(0, 1);
    lcd_puts("3.UART   C=Back");
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
            }

            else if (key == '2')
            {
                menu_state = MENU_TEMP;
            }

            else if (key == '3')
            {
                menu_state = MENU_UART;
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