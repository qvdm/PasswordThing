/*
 * Name: Display.cpp
 * 
 * Purpose: Manages OLED I/O
 * 
 * Provides:
 *  init - post instantiation initialization
 *  isblank - is the display currently blanked?
 *  displaylarge - display a string in large font
 *  displaysmall - display a string in small font
 *  setprivacy - sets pricacy timeout on oled
 *  setflip - flips the oled display
 *  setpwrevert - sets pw mode to Revert
 *  void vTaskManageDisplay - periodic task
 * 
 * Operation: Utilizes the small ASCCI SSD1306 library to drive the OLED display.  Provides a periodic task for display timeout and so on
 * 
 */

#include "display.h"

extern char Version[];
extern byte slot;

Display::Display(SSD1306AsciiWire &rd, Eeprom &re) : oled(rd), eeprom(re)
{
}

Display::~Display() { }

void Display::init()
{
  // Initialize Wire I2C library
  Wire.begin();
  Wire.setClock(400000L);

  // Initialize display
  oled.begin(&Adafruit128x32, I2C_OLED_ADDRESS);
  oled.setFont(Adafruit5x7);
  oled.set2X();
  oled.clear();

  displaylarge(Version);
  delay(DVDELAY);
}


bool Display::isblank()
{
  return ((privacy_timeout > 0) &&  (blanktime == 0) );
}


// Display large chars on Oled
void Display::displaylarge(char *str)
{
  oled.set2X();
  oled.clear();
  oled.setRow(1);       
  oled.println(str); 

  // Display bottom help string
  oled.set1X();

  byte b=  eeprom.getvar(EEVAR_BUTSEQ);
  switch (b) 
  {
    case 0 : oled.print("P N S  G R N  X S R"); break;
    case 1 : oled.print("P S N  G N R  X R S"); break;
    case 2 : oled.print("N P S  R G N  S X R"); break;
    case 3 : oled.print("N S P  R N G  S R X"); break;
    case 4 : oled.print("S P N  N G R  R X S"); break;
    case 5 : oled.print("S N P  N R G  R S X"); break;
  }
  
  // set up for blanking after PTO s
  blanktime = (unsigned long) privacy_timeout * 1000L / LOOP_MS;
}

// Display small chars on Oled
void Display::displaysmall(char *str1, char *str2, char *str3)
{
  oled.clear();
  oled.set1X();

  if (str1) oled.println(str1); 
  if (str2) oled.println(str2); 
  if (str3) oled.print(str3); 

  // set up for blanking after PTO s
  blanktime = (unsigned long) privacy_timeout * 1000L / LOOP_MS;
}


void Display::setprivacy(byte timeout)
{
  if (timeout < (byte) MAXPTO)
    privacy_timeout = timeout * 10;
  else
    privacy_timeout = MAXPTO * 10;
  blanktime = (unsigned long) privacy_timeout * 1000L / LOOP_MS;
}

void Display::setflip(bool state)
{
  oled.displayRemap(state);
}

void Display::setpwrevert(bool state)
{
  pwrevert = state;
}


void Display::vTaskManageDisplay()
{
 // Blank display on timeout
  if (blanktime > 0)
  {
    if (--blanktime == 0) 
    {
      oled.clear();
      // Revert the pwd at the same time if flag set
      if (pwrevert) slot = 0;
    }
  }
}

