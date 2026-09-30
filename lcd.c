#include "lcd.h"

void lcd_enable(void)
{
    LCD_EN_PORT |= (1 << LCD_EN_PIN);
    delay_us(2);
    LCD_EN_PORT &= ~(1 << LCD_EN_PIN);
    delay_us(50);
}

void lcd_write_nibble(unsigned char nibble)
{
    // Clear all data pins first
    LCD_D4_PORT &= ~(1 << LCD_D4_PIN);
    LCD_D5_PORT &= ~(1 << LCD_D5_PIN);
    LCD_D6_PORT &= ~(1 << LCD_D6_PIN);
    LCD_D7_PORT &= ~(1 << LCD_D7_PIN);
    
    // Set data pins
    if (nibble & 0x01) LCD_D4_PORT |= (1 << LCD_D4_PIN);
    if (nibble & 0x02) LCD_D5_PORT |= (1 << LCD_D5_PIN);
    if (nibble & 0x04) LCD_D6_PORT |= (1 << LCD_D6_PIN);
    if (nibble & 0x08) LCD_D7_PORT |= (1 << LCD_D7_PIN);
    
    lcd_enable();
}

void lcd_cmd(unsigned char cmd)
{
    LCD_RS_PORT &= ~(1 << LCD_RS_PIN);
    delay_us(100);
    lcd_write_nibble(cmd >> 4);
    lcd_write_nibble(cmd & 0x0F);
    delay_ms(2);
}

void lcd_data(unsigned char data)
{
    LCD_RS_PORT |= (1 << LCD_RS_PIN);
    delay_us(100);
    lcd_write_nibble(data >> 4);
    lcd_write_nibble(data & 0x0F);
    delay_us(50);
}

void lcd_init(void)
{
    // Set all pins as output
    LCD_RS_DDR |= (1 << LCD_RS_PIN);
    LCD_EN_DDR |= (1 << LCD_EN_PIN);
    LCD_D4_DDR |= (1 << LCD_D4_PIN);
    LCD_D5_DDR |= (1 << LCD_D5_PIN);
    LCD_D6_DDR |= (1 << LCD_D6_PIN);
    LCD_D7_DDR |= (1 << LCD_D7_PIN);
    
    // Ensure RS and EN are low
    LCD_RS_PORT &= ~(1 << LCD_RS_PIN);
    LCD_EN_PORT &= ~(1 << LCD_EN_PIN);
    
    // CRITICAL: Wait for LCD to power up (minimum 50ms)
    delay_ms(100);
    
    // Force LCD into 8-bit mode first by sending 0x03 three times
    // This works regardless of what state the LCD is in
    lcd_write_nibble(0x03);
    delay_ms(10);
    
    lcd_write_nibble(0x03);
    delay_ms(10);
    
    lcd_write_nibble(0x03);
    delay_ms(10);
    
    // Now switch to 4-bit mode
    lcd_write_nibble(0x02);
    delay_ms(10);
    
    // Now we're in 4-bit mode, send proper commands
    lcd_cmd(0x28);  // 4-bit, 2 lines, 5x7 dots
    lcd_cmd(0x0C);  // Display ON, cursor OFF, blink OFF
    lcd_cmd(0x06);  // Entry mode: increment, no shift
    lcd_cmd(0x01);  // Clear display
    delay_ms(5);    // Clear command needs extra time
}

void lcd_clear(void)
{
    lcd_cmd(0x01);
    delay_ms(5);
}

void lcd_gotoxy(unsigned char x, unsigned char y)
{
    unsigned char addr;
    if (y == 0)
        addr = 0x80 + x;
    else
        addr = 0xC0 + x;
    lcd_cmd(addr);
}

void lcd_puts(char *str)
{
    while (*str)
    {
        lcd_data(*str);
        str++;
    }
}