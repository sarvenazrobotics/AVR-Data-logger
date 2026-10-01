#include <io.h>
#include <stdio.h>
#include "lcd.h"
#include "adc.h"

/* Initialize ADC: AVcc reference, prescaler 128 */
void adc_init(void)
{
    ADMUX = (1 << REFS0);

    ADCSRA = (1 << ADEN)  |
             (1 << ADPS2) |
             (1 << ADPS1) |
             (1 << ADPS0);
}

/* Read ADC channel 0 to 7 */
unsigned int adc_read(unsigned char channel)
{
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

    ADCSRA |= (1 << ADSC);

    while (ADCSRA & (1 << ADSC))
    {
    }

    return ADCW;
}

/* Return LM35 temperature in tenths of a degree Celsius */
unsigned int adc_get_temperature(void)
{
    unsigned int adc_value;
    unsigned long temp10;

    adc_value = adc_read(2);

    /* AVcc = 5000 mV, LM35 = 10 mV per degree C */
    temp10 = (unsigned long)adc_value * 5000UL / 1024UL;

    return (unsigned int)temp10;
}

/* Display temperature on LCD second line */
void adc_display_temperature(void)
{
    unsigned int temp10;
    char buffer[17];

    temp10 = adc_get_temperature();

    sprintf(buffer, "Temp: %u.%u C",
            (unsigned int)(temp10 / 10),
            (unsigned int)(temp10 % 10));

    lcd_gotoxy(0, 1);
    lcd_puts(buffer);
}