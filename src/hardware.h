/*
 * Hardware definitions for AtMega 32U4 'Beetle' board
 *
 *              A2   A2   A0   D9   D10  D11
 *               +    +    +    +    +    +
 *               |    |    |    |    |    |
 *       +-------+----+----+----+----+----+---------+
 * 5V  +-+                                          |
 *       |                                          |
 *       |                                          |
 * GND +-+                                          +-+ TX
 *       |                                          |
 *       |                                          |
 * RES +-+                                          +-+ RX
 *       |                                          |
 *       |                                          |
 * SCK +-+                                          +-+SDA
 *       |                                          |
 *       |                                          |
 * MOSI+-+                                          +-+SCL
 *       |                                          |
 *       |                                          |
 * MISO+-+                                          |
 *       |                                          |
 *       +-----+-----------------------------+------+
 *             |                             |
 *             |                             |
 *             +-----------------------------+
 *
 * 
 * RGB LED connected to A0-A2, active LOW
 * 3 buttons (+, -, SEL) connected to D9-D11
 * 
 * RX/TX is D0/D1 (separate from USB Rx/Tx)
 * SDA/SCL is D2/D3
 * 
 * MISO/SCK/MOSI is D14/D15/D16
 * 
 * On-Board LED is on D13, active HIGH (USB-A board) 
 *                    D6, active HIGH (microUSB board)
 * 
 * USBA RD+/RD- is connected to D+/D-
 * 
 */ 

#ifndef HARDWARE_H
#define HARDWARE_H

typedef unsigned char byte;

// Button pins
#ifdef USBA_32U4
#define IB0_PIN 9
#define IB1_PIN 10
#define IB2_PIN 11
#endif

#ifdef MICROUSB_32U4
#define IB0_PIN 9
#define IB1_PIN 8
#define IB2_PIN 7
#endif


// RGB LED pins
#define OLR_PIN A2
#define OLG_PIN A1
#define OLB_PIN A0

// On board LED
#ifdef MICROUSB_32U4
#define OLO_PIN 6
#else
#define OLO_PIN 13
#endif

// OLED
#define I2C_OLED_ADDRESS 0x3C 

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define SDBG(S)   Serial.println(S); Serial.flush();

// EEPROM var offsets
#define EEVAR_LBLINK  0 // Onboard Led blink        - boolean
#define EEVAR_DFLIP   1 // Flip display             - boolean in bot bit, security seq in top 7 bits
#define EEVAR_PRIV    2 // Privacy (turn OLED | LED off)  - timeout in secs/10, 0 = none, OLED in bot nibble, CLED in top
#define EEVAR_BUTTONS 3 // Button defs - see menu.h - button sequence in bot nibble, led seq in top
  
// # of possible security seqs and seq length
#define NSSEQ 81 
#define SSEQL 4

#endif