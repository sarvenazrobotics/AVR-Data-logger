# ATmega328P Multi-Function Clock, Thermometer & UART Monitor

![Proteus Schematic](schematic.png) <!-- Replace with actual screenshot if hosting on Git -->

## 📌 Overview
This project is a comprehensive embedded systems application built around the **ATmega328P** microcontroller. It serves as a multi-function digital clock, temperature monitor, and serial data transmitter. The system features a menu-driven user interface displayed on a 16x2 LCD, controlled via a 4x4 matrix keypad, with a multiplexed 4-digit 7-segment display for quick data reading.

## ✨ Features
*   **Real-Time Clock (RTC):** Reads time (HH:MM:SS) from a DS1307 RTC via I2C (TWI).
*   **Temperature Monitoring:** Reads ambient temperature using an LM35 sensor via ADC.
*   **Dual Display System:** 
    *   16x2 LCD for menu navigation and detailed data.
    *   4-Digit 7-Segment display (multiplexed) for clock and temperature.
*   **UART Serial Communication:** Transmits time and temperature data to a PC/Virtual Terminal.
*   **Menu System:** State-machine based menu to switch between Clock, Temperature, and UART modes.
*   **Efficient Multiplexing:** Uses Timer0 interrupt for 7-segment multiplexing and keypad scanning, reducing main loop overhead.
*   **SPI Communication:** Utilizes hardware SPI to drive cascaded 74HC595 shift registers for the 7-segment display.

## 🛠️ Hardware Requirements
*   **Microcontroller:** ATmega328P
*   **Display:** LM016L (16x2 Character LCD)
*   **Display:** 4-Digit 7-Segment Display (Multiplexed)
*   **Shift Registers:** 2x 74HC595
*   **RTC:** DS1307 (with 32.768kHz crystal)
*   **Temperature Sensor:** LM35
*   **Input:** 4x4 Matrix Keypad
*   **Misc:** Resistors, Capacitors, Potentiometer (for LCD contrast), 5V Power Supply.

## 🔌 Pin Mapping
Based on the `lcd.h` and main application code:

### LCD (LM016L)
| LCD Pin | ATmega328P Pin | Function |
| :--- | :--- | :--- |
| RS | PD2 | Register Select |
| EN | PD3 | Enable |
| D4 | PD4 | Data 4 |
| D5 | PD5 | Data 5 |
| D6 | PD6 | Data 6 |
| D7 | PD7 | Data 7 |
| RW | GND | Write Mode only |

### 74HC595 & 7-Segment (SPI)
| 74HC595 | ATmega328P Pin | Function |
| :--- | :--- | :--- |
| LATCH (ST_CP) | PB2 | Latch Clock |
| MOSI (DS) | PB3 | Serial Data In |
| SCK (SH_CP) | PB5 | Shift Clock |

### Keypad Matrix
| Keypad Column | ATmega328P Pin |
| :--- | :--- |
| C1 | PB0 |
| C2 | PB1 |
| C3 | PC3 |
| C4 | PB4 |
*(Keypad rows are driven by the 74HC595 outputs)*

### Other Peripherals
*   **DS1307 (TWI):** SCL, SDA (Hardware I2C)
*   **LM35 (ADC):** ADC0 (or as configured in `adc.h`)
*   **UART:** RXD, TXD (Hardware USART)

## 📂 Project Structure
```text
├── main.c          # Main application logic, ISRs, and initialization
├── lcd.h           # LCD function prototypes and pin definitions
├── lcd.c           # Custom LCD driver implementation
├── twi.h           # I2C (TWI) driver prototypes
├── twi.c           # I2C (TWI) driver implementation for DS1307
├── adc.h           # ADC driver prototypes
├── adc.c           # ADC driver implementation for LM35
├── uart.h          # UART driver prototypes
├── uart.c          # UART driver implementation
├── menu.h          # Menu state machine prototypes
├── menu.c          # Menu state machine implementation
└── README.md       # Project documentation
