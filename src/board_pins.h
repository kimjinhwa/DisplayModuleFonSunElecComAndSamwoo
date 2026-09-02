#ifndef BOARD_PINS_H
#define BOARD_PINS_H

// I2C: DS1307Z + GT911 공유
#define PIN_I2C_SDA 19
#define PIN_I2C_SCL 20
#define DS1307_I2C_ADDR 0x68

// THVD1406DR 자동방향 RS485 (DE/RE GPIO 없음)
#define PIN_RS485_TX 17
#define PIN_RS485_RX 18
#ifndef RS485_UART_BAUD
#define RS485_UART_BAUD 9600
#endif

// WIZ610 / W6100 SPI. RST·INT GPIO 없음 (모듈 H/W 리셋만)
#define PIN_W610_MOSI 11
#define PIN_W610_SCLK 12
#define PIN_W610_MISO 13
#define PIN_W610_CS 10

#endif
