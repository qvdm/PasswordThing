#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

#include "hardware.h"
#include "tick.h"
#include "eeprom.h"

// OLED Top/Btm
#define DISP_TOP 1
#define DISP_MID 2
#define DISP_BOT 3

// Privacy timeout
#define MAXPTO 15 // 150s

// Version display delay
#define DVDELAY 2000

// Max length of large string
#define MAXLDSTR 10

class Display {
  public:
    Display(SSD1306AsciiWire &rd, Eeprom &re);
    ~Display();
    void init();
    bool isblank();
    void displaylarge(char *str, bool showhelp);
    void displaysmall(char *str1, char *str2, char *str3);
    void setprivacy(byte timeout);
    void setflip(bool state);
    void setpwrevert(bool state);
    void vTaskManageDisplay();

  private:
    SSD1306AsciiWire &oled;
    Eeprom &eeprom;
    unsigned long blanktime=0;
    byte privacy_timeout=10; //s
    bool pwrevert=false;
};

#endif
