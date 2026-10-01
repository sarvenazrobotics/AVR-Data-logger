#ifndef ADC_H
#define ADC_H

void adc_init(void);
unsigned int adc_read(unsigned char channel);
unsigned int adc_get_temperature(void);
void adc_display_temperature(void);

#endif