/*
 * Name: PwGen - Cooperative multitasking version
 *
 * Purpose: Provides a password generator and typer
 * 
 * Features:
 *   Password select - 1-6
 *   Password 'type'
 *   Password generate
 *   EEPROM storage of passwords
 *   OLED and RGB/onboard LED display
 *   Simple 'lock' mechanism
 * 
 * Operation:
 *   See "hardware.h" for physical description
 *   See documentation for user guide
 * 
 * Functions:
 * sysTick - Interrupt driven time counter
 * setup - Arduino setup
 * loop - Arduino loop
 * 
 * TBD
<<<<<<< HEAD
 *      Add UUSB diagram in hardware.h
 *		Add build and test instrcutions to docs
=======
 *      Firmware upload for pwtgui
>>>>>>> 0a952e91e60a0f3e14bf94149c5b49cc1bfb509c
 * 
 * BUGS:
 * 
 */

#include <Arduino.h>
#include <avr/wdt.h>

#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

#include <TimerOne.h>

// Class headers
#include "version.h"
#include "hardware.h"
#include "tick.h"
#include "utils.h"
#include "display.h"
#include "input.h"
#include "led.h"
#include "random.h"
#include "eeprom.h"
#include "menu.h"
#include "serialui.h"

char Version[]=VERSION_SHORT; // PWT s/w version
byte eedVer = 4; // eeprom dump format version
byte eevVer = 2; // eeprom vars schema version, stored in EE semaphore area

// Forward declare systick function
void sysTick();

// Create utility classes
SSD1306AsciiWire cOled;
Random cRandom;
Eeprom cEeprom;
Led cLed;
Display cDisp(cOled, cEeprom);
Menu cMenu(cLed, cDisp, cRandom, cEeprom);
SerialUi cSui(cLed, cDisp, cRandom, cEeprom);
Input cInput(cMenu);

// Global timers
volatile unsigned long Time=0;

// Global Mode
byte kbmode = KM_KBD;

// Global slot
byte slot=0;

// Security Sequence array (button presses to get out of LOCKED state)
int Secseq[] = { 0,
      1111,1112,1113,1121,1122,1123,1131,1132,1133,1211,1212,1213,1221,1222,1223,1231,1232,1233,1311,1312,1313,1321,1322,1323,1331,1332,1333,
      2111,2112,2113,2121,2122,2123,2131,2132,2133,2211,2212,2213,2221,2222,2223,2231,2232,2233,2311,2312,2313,2321,2322,2323,2331,2332,2333,
      3111,3112,3113,3121,3122,3123,3131,3132,3133,3211,3212,3213,3221,3222,3223,3231,3232,3233,3311,3312,3313,3321,3322,3323,3331,3332,3333
    };

// Lock variables
unsigned long locktimeout=0;
unsigned long lastkeypress=0;
byte sseq=0;

// "Initial Task"
void setup() 
{
   // Initialize Oled display
  cDisp.init(); 

  // Start LED @ green
  cLed.ledcolor(COL_GRN, BLNK_ON, true);

  // Initialize Serial if a button is pressed at startup
  if ( cInput.anyPressed() )
  {
    delay(200); // Wait 200 ms to see if still pressed
    if (cInput.anyPressed())
    {
      kbmode = KM_SERIAL;
      // Set led to white in serial mode
      cLed.ledcolor(COL_WHT, BLNK_ON, true);
    }
  }
  else if (cEeprom.getsema(EESEM_SERMODE) > 0)
  {
    cEeprom.storesema(EESEM_SERMODE, 0);
    kbmode = KM_SERIAL;
    // Set led to white in serial mode
    cLed.ledcolor(COL_WHT, BLNK_ON, true);
  }
  
  // Check EEPROM signature if not valid, zero EEPROM and write signature
  if (!cEeprom.check_signature())
  {
    // Cyan led indicates eeprom kerfuffling
    cLed.ledcolor(COL_CYA, BLNK_ON, true);
    cEeprom.zero();
    cLed.ledcolor(COL_BLU, BLNK_ON, true);
  }

  int eesver = cEeprom.getsema(EESEM_EEVER);
  if (eesver < eevVer)
  {
    cEeprom.upgrade_schema();
  }

  if (!cEeprom.valid() )
  {
    cLed.ledcolor(COL_CYA, BLNK_ON, true);
    cEeprom.zero();
    cLed.ledcolor(COL_BLU, BLNK_ON, true);
  }


  // Get security seq
  sseq = cEeprom.getvar(EEVAR_SEC); // Security Seq

  if (sseq > 0)
  {
    byte badseq = cEeprom.getvar(EEVAR_TRIES);
    byte badtries = cEeprom.getsema(EESEM_BADLCK);
    switch (badseq)
    {
       case 0 : break; // infinite retries
       case 1 : if ( badtries > 3) { cEeprom.zero(); WDRESET; } break;
       case 2 : if ( badtries > 10) { cEeprom.zero(); WDRESET; } break;
       default: if (badtries) delay((1 << (badtries-1)) * badseq * 1000); 
    }
  }

  if (kbmode == KM_SERIAL)
  {
    // Early init to allow programming (!! Needs to be up here - do not move down)
    cSui.sio_init(sseq);
  }

  // Initialize EEPROM vars
  byte priv = cEeprom.getvar(EEVAR_OPRIV); // Display timeout
  cDisp.setprivacy(priv);
  priv = cEeprom.getvar(EEVAR_LPRIV); // LED timeout
  cLed.settimeout(priv);
  byte lck = cEeprom.getvar(EEVAR_LOCK); // Autolock timeout
  locktimeout = (unsigned long) lck * 10L * 60L * 1000 * TICKS_PERMS;
  byte flip = cEeprom.getvar(EEVAR_DFLP); // Display flip
  cDisp.setflip((bool) flip);
  byte pwrevert = cEeprom.getvar(EEVAR_PRTO); // PW Revert
  cDisp.setpwrevert((bool) pwrevert);
  byte btnmode = cEeprom.getvar(EEVAR_BUTSEQ); // Button assignments
  cMenu.set_buttonmode(btnmode);
  byte ledcols = cEeprom.getvar(EEVAR_LEDSEQ); // LED color assignments
  cMenu.set_slotcolors(ledcols);

  cDisp.displaylarge(Version, false);
  delay(DVDELAY);

  if (cEeprom.getvar(EEVAR_AYB) == 0)
  {
    cDisp.displaylarge(LOGO, false);
    delay(DVDELAY);
  }

  // Initialize button 'menu' or serial menu depending in global mode
  if (kbmode == KM_KBD)
  {
      Keyboard.begin(); // Turn on kbd
#ifdef TEST
    testhw();
#else
    wdt_enable(WDTO_4S);  
#endif    
    cMenu.init(sseq);  
  }
  

  // Initialize systick timer to generate an interrupt every 10us
  Timer1.initialize(TICK_US);
  Timer1.attachInterrupt(sysTick); 
}

// Increment system time 
void sysTick()
{
  Time++;
}

// Retrieves current system time
unsigned long getTime()
{
  unsigned long T;
  noInterrupts();
  T=Time;
  interrupts();
  return T; 
}

// Main processing loop
void loop()
{
  static unsigned long loopcount=0; 
  unsigned long loopstart, loopend, loopduration, ldms, delaytime;

  // Measure loop start time
  loopstart = getTime();
  loopcount++;

  // Kick watchdog
#ifndef TEST
  wdt_reset();
#endif  

  // Execute periodic 'Tasks' (naming convention remains from FreeRTOS days, now we just call them in sequence)
  cLed.vTaskManageLeds();      // LED manager
  cDisp.vTaskManageDisplay();  // Display manager
  cRandom.vTaskRandomGen();    // Random # entropy harvester

  if (kbmode == KM_SERIAL)
  {
    cSui.vTaskSerialUi();      // Serial UI
  }
  else
  {
    cInput.vTaskDigitalRead(); // Digital input
    // Check for lock timeout
    if ( (locktimeout > 0) && ((getTime()-lastkeypress) > locktimeout)  && (sseq > 0) )
    {
      WDRESET;
    }
  }


  // Measure elapsed time
  loopend = getTime();
  loopduration = loopend-loopstart;
  ldms = loopduration / (TICKS_PERMS);

  // delay for (loop period - time elapsed) 
  if (ldms >= (LOOP_MS-1)) 
    delaytime = 1;
  else
    delaytime = LOOP_MS-ldms;


  // Harvest extra entropy during idle time when low, else just delay
  if ( (delaytime > 2) && (cRandom.getEntropy() < (MAXENTROPY/2) ) )
  {
    for (int i=0; i < (int) delaytime; i++)
    {
      cRandom.vTaskRandomGen();  // Random # entropy harvester
      delay(1);
    }
  }
  else 
    delay(delaytime);
}

// HW test
void testhw(void)
{
  cLed.cledout(HIGH, HIGH, HIGH); // OFF
  delay(1000);
  cLed.cledout(LOW, HIGH, HIGH); // Red
  delay(1000);
  cLed.cledout(HIGH, LOW, HIGH); // Green
  delay(1000);
  cLed.cledout(HIGH, HIGH, LOW); //Blue
  delay(1000);
  cLed.cledout(LOW, HIGH, LOW); // Magenta
  delay(1000);
  cLed.cledout(LOW, LOW, HIGH); // Yellow
  delay(1000);
  cLed.cledout(HIGH, LOW, LOW); // Cyan
  delay(1000);
  cLed.cledout(LOW, LOW, LOW); // White
  delay(1000);

  while (1)
  {
    if (digitalRead(IB0_PIN) == LOW)
      cLed.cledout(LOW, HIGH, HIGH); // Red
    if (digitalRead(IB1_PIN) == LOW)
      cLed.cledout(HIGH, LOW, HIGH); // Green
    if (digitalRead(IB2_PIN) == LOW)
      cLed.cledout(HIGH, HIGH, LOW); // Blue
  }
}
