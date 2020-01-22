#ifndef INPUT_H
#define INPUT_H

#include <Arduino.h>
#include "hardware.h"
#include "tick.h"

#include "menu.h"


#define DEBOUNCE_C 1 // # of cycles for debounce

#define NBUTTONS 3

#define BUT0 0x01
#define BUT1 0x02
#define BUT2 0x03

// Events
#define NOTPRESSED    0
#define PRESSED       1
#define PRESSINGLONG  2
#define PRESSINGVLONG 3

#define SHORTPRESS  0
#define LONGPRESS   1
#define LONGPRESS_T (TICKS_PERMS)*1000 
#define VLONGPRESS_T (TICKS_PERMS)*3000 

class Input {
  public:
    Input(Menu&);
    ~Input();
    bool anyPressed();
    void vTaskDigitalRead();

  private:
    byte bstate[NBUTTONS];
    const int bport[NBUTTONS] = {IB0_PIN, IB1_PIN, IB2_PIN};
    byte bctr[NBUTTONS] =  {0,0,0};
    unsigned long downtime[NBUTTONS];
    bool lpnotify[NBUTTONS]={false, false, false};
    bool vlpnotify[NBUTTONS]={false, false, false};

    Menu &menu;

    void sendevent(byte button, byte event);

};

#endif
