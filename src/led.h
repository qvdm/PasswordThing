#ifndef LED_H
#define LED_H

#include <Arduino.h>
#include "hardware.h"
#include "tick.h"

// BLink states
#define BLNK_OFF   0 // Always off
#define BLNK_LEAST 1 // 100ms on, 900ms off
#define BLNK_SLOW  2 // 1s on, 1s off
#define BLNK_MED   3 // 500ms on, 500ms off 
#define BLNK_FAST  4 // 200ms on, 200ms off 
#define BLNK_MOST  5 // 900ms on, 100ms off 
#define BLNK_ON    6 // Always on


// Blink state to Time
#define BT_SLOW LOOPS_PERSEC  
#define BT_MED  (LOOPS_PERSEC)/2
#define BT_FAST (LOOPS_PERSEC)/5
#define BT_MOST (LOOPS_PERSEC)
#define BT_MOST_ON (LOOPS_PERSEC)*9/10
#define BT_LEAST (LOOPS_PERSEC)
#define BT_LEAST_ON (LOOPS_PERSEC)/10

#define RMASK 0x01
#define GMASK 0x02
#define BMASK 0x04

// Color table
                        // BGR
#define COL_OFF   0x00  // 000
#define COL_RED   0x01  // 001
#define COL_GRN   0x02  // 010
#define COL_YEL   0x03  // 011
#define COL_BLU   0x04  // 100
#define COL_MAG   0x05  // 101
#define COL_CYA   0x06  // 110
#define COL_WHT   0x07  // 111

// Translate on/off to LED active-low  logic
#define BRI_ON  LOW
#define BRI_OFF HIGH

// On-timeout
#define MAXLTO 15 // 150s


class Led {
  public:
    Led();
    ~Led();
    void set_colmap(byte map);
    void ledcolor(byte colorcode, byte mode);
    void ledcolor(byte r, byte g, byte b, byte mode);
    void cl_blink(byte rate);
    void push(void);
    void pop(void);
    void settimeout(byte timeout);
    void unblank(void);
    bool isblank(void);
    void cledout(byte r, byte g, byte b);
    void vTaskManageLeds();

  private:
    byte lr=BRI_OFF,lg=BRI_OFF,lb=BRI_OFF, lk=BLNK_OFF;  // Current RGB led color settings
    byte slr, slg, slb, slk;  // Saved RGB led color and blink settings
    byte clcycle = 0;       // color blink cycle counter
    byte maxclcycle=(LOOPS_PERSEC)*2;
    bool clstate=false; // colour led state (on/off) 
    byte clmode=BLNK_ON;  // colour led blink mode
    unsigned long blanktime=0;
    byte led_timeout=60; //s



};

#endif 