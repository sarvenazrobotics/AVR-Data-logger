#include <io.h>
#include "adc.h"

/* Initialize ADC: AVcc reference, prescaler 128 */
void adc_init(void)
{
    ADMUX = (1 << REFS0);  /* AVcc reference, ADC0 selected */

    ADCSRA = (1 << ADEN)  |  /* Enable ADC */
             (1 << ADPS2) |
             (1 << ADPS1) |
             (1 << ADPS0);   /* Prescaler = 128 */
}

/* Read ADC channel (0 to 7) */
unsigned int adc_read(unsigned char channel)
{
    unsigned int result;

    /* Select ADC channel while preserving reference bits */
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

    /* Start conversion */
    ADCSRA |= (1 << ADSC);

    /* Wait for conversion to complete */
    while (ADCSRA & (1 << ADSC))
    {
    }

    result = ADCW;

    return result;
}