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
#define IB0_PIN 10
#define IB1_PIN 11
#define IB2_PIN 9
#endif

#ifdef MICROUSB_32U4
#define IB0_PIN 9
#define IB1_PIN 8
#define IB2_PIN 7
#endif


// RGB LED pins
#ifdef USBA_32U4
#define OLR_PIN A0
#define OLG_PIN A1
#define OLB_PIN A2
#endif

#ifdef MICROUSB_32U4
#define OLR_PIN A2
#define OLG_PIN A1
#define OLB_PIN A0
#endif

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
#define EEVAR_AYB     0 // Display logo at startup - boolean, 0-yes, other=no
#define EEVAR_DFLP    1 // Flip display        - boolean 
#define EEVAR_SEC     2 // Security seq #      - byte seq
#define EEVAR_OPRIV   3 // OLED  Privacy       - byte timeout in secs/10, 0 = none
#define EEVAR_LPRIV   4 // LED  Privacy        - byte timeout in secs/10, 0 = none
#define EEVAR_BUTSEQ  5 // Button seq          - byte index, see menu.h
#define EEVAR_LEDSEQ  6 // LED seq             - byte index, see menu.h
#define EEVAR_PRTO    7 // PWD Revert          - boolean
#define EEVAR_TRIES   8 // # of lock code tries before erase - byte 0=infinite, 1=3, 2=10, >2 = 20 w/incremental timeout: N
#define EEVAR_LOCK    9 // Autolock            - byte timeout in minutes/10, 0 = none

// EEPROM semaphores
#define EESEM_SERMODE  0 // Reboot into serial mode
#define EESEM_BADLCK   1 // # of bad unlock attempts
#define EESEM_EEVER    2 // EEprom schema version
  
// # of possible security seqs and seq length
#define NSSEQ 81 
#define SSEQL 4

// Global maximums
#define MAXSLOTS   6  // Max # of used PW slots
#define MAXPW      20 // Max generated PWD length
#define MAXLOCKTO  144 // 1440 min


// Reset
#define WDRESET cli(); wdt_enable(WDTO_15MS);  while (1)

// Logo
#define LOGO (const char *) "q.uent.in/pwt"

#endif