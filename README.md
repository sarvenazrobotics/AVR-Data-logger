# ATmega328P Embedded Monitoring & Control System

A complete embedded-systems project based on the **ATmega328P**, developed in **CodeVisionAVR** and simulated in **Proteus**.

The project integrates:

- ATmega328P microcontroller
- 4-digit 7-segment display
- Two cascaded 74HC595 shift registers
- 4×4 matrix keypad
- Character LCD
- DS1307 Real-Time Clock (RTC)
- LM35 temperature sensor
- ADC temperature measurement
- Hardware UART communication
- Custom hardware TWI/I²C communication
- Menu-driven user interface
- Timer0 interrupt-based display refresh and keypad scanning

---

## 1. Project Overview

The purpose of this project is to create a small embedded monitoring system capable of displaying and communicating:

1. Current time from the DS1307 RTC
2. Temperature measured using an LM35 sensor
3. Data transmitted through UART
4. User-selected functions through a 4×4 keypad
5. Information on both an LCD and a 4-digit 7-segment display

The system uses the ATmega328P as the central controller.

The user interacts with the system through the keypad, while the LCD provides a text-based menu and status information.

The UART interface can be connected to the **Proteus Virtual Terminal** for monitoring transmitted data.

---

# 2. Main Features

## Clock

The system communicates with the DS1307 RTC and reads:

- Hours
- Minutes
- Seconds

The time is stored in the DS1307 in BCD format and converted to decimal before being displayed.

Example:

```text
Time: 14:30:25
