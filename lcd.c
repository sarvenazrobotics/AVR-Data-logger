#include "lcd.h"

// Wait for LCD to be ready
void lcd_wait(void)
{
    delay_us(50);
}

// Pulse enable pin
void lcd_enable(void)
{
    LCD_EN_PORT |= (1 << LCD_EN_PIN);
    delay_us(1);
    LCD_EN_PORT &= ~(1 << LCD_EN_PIN);
    delay_us(50);
}

// Send 4-bit nibble to LCD
void lcd_write_nibble(unsigned char nibble)
{
    // Clear data pins first
    LCD_D4_PORT &= ~((1 << LCD_D4_PIN) | (1 << LCD_D5_PIN) | (1 << LCD_D6_PIN) | (1 << LCD_D7_PIN));
    
    // Set data pins according to nibble
    if (nibble & 0x01) LCD_D4_PORT |= (1 << LCD_D4_PIN);
    if (nibble & 0x02) LCD_D5_PORT |= (1 << LCD_D5_PIN);
    if (nibble & 0x04) LCD_D6_PORT |= (1 << LCD_D6_PIN);
    if (nibble & 0x08) LCD_D7_PORT |= (1 << LCD_D7_PIN);
    
    // Pulse enable
    lcd_enable();
}

// Send command to LCD
void lcd_cmd(unsigned char cmd)
{
    LCD_RS_PORT &= ~(1 << LCD_RS_PIN);  // RS = 0 for command
    
    // Send high nibble first
    lcd_write_nibble(cmd >> 4);
    // Send low nibble
    lcd_write_nibble(cmd & 0x0F);
    
    delay_ms(2);  // Command execution time
}

// Send data to LCD
void lcd_data(unsigned char data)
{
    LCD_RS_PORT |= (1 << LCD_RS_PIN);  // RS = 1 for data
    
    // Send high nibble first
    lcd_write_nibble(data >> 4);
    // Send low nibble
    lcd_write_nibble(data & 0x0F);
    
    delay_us(50);  // Data write time
}

// Initialize LCD in 4-bit mode
void lcd_init(void)
{
    // Set all LCD pins as output
    LCD_RS_DDR |= (1 << LCD_RS_PIN);
    LCD_EN_DDR |= (1 << LCD_EN_PIN);
    LCD_D4_DDR |= (1 << LCD_D4_PIN);
    LCD_D5_DDR |= (1 << LCD_D5_PIN);
    LCD_D6_DDR |= (1 << LCD_D6_PIN);
    LCD_D7_DDR |= (1 << LCD_D7_PIN);
    
    // Initial delay for LCD power-up
    delay_ms(50);
    
    // RS = 0 for all commands during init
    LCD_RS_PORT &= ~(1 << LCD_RS_PIN);
    
    // Initialization sequence for 4-bit mode
    // Send 0x03 three times with proper delays
    lcd_write_nibble(0x03);
    delay_ms(10);
    
    lcd_write_nibble(0x03);
    delay_ms(10);
    
    lcd_write_nibble(0x03);
    delay_ms(10);
    
    // Set to 4-bit mode
    lcd_write_nibble(0x02);
    delay_ms(10);
    
    // Now send commands using 4-bit mode
    lcd_cmd(LCD_4BIT_2LINE);    // 4-bit, 2 lines, 5x7 font
    lcd_cmd(LCD_DISPLAY_ON);    // Display on, cursor off, blink off
    lcd_cmd(LCD_CLEAR);         // Clear display
    delay_ms(2);
    lcd_cmd(LCD_ENTRY_MODE);    // Entry mode: increment, no shift
    delay_ms(2);
}

// Clear LCD display
void lcd_clear(void)
{
    lcd_cmd(LCD_CLEAR);
    delay_ms(2);
}

// Return cursor to home position
void lcd_home(void)
{
    lcd_cmd(LCD_HOME);
    delay_ms(2);
}

// Set cursor position
void lcd_gotoxy(unsigned char x, unsigned char y)
{
    unsigned char addr;
    
    if (y == 0)
        addr = 0x80 + x;  // First line
    else
        addr = 0xC0 + x;  // Second line
    
    lcd_cmd(addr);
}

// Print string to LCD
void lcd_puts(char *str)
{
    while (*str)
    {
        lcd_data(*str);
        str++;
    }
}

// Print character to LCD
void lcd_putch(unsigned char ch)
{
    lcd_data(ch);
}