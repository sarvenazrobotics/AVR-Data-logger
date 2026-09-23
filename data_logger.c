#include <io.h>
#include <delay.h>
#include "lcd.h"

void main(void)
{
    // Initialize LCD
    lcd_init();
    
    // Display "welcome"
    lcd_puts("welcome");
    
    while (1)
    {
        // Main loop
    }
}