#ifndef MENU_H
#define MENU_H

#include <Arduino.h>

#include "hardware.h"

#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__) 
#include <Keyboard.h>
#endif

#include "led.h"
#include "display.h"
#include "random.h"
#include "eeprom.h"
#include "serialui.h"

// Default Buttons
#define B_GENERATE 0 
#define B_NEXT     1 
#define B_SELECT   2

// Selectable button definitions
#define BDEF_GNS 0 // Default GEN NXT SEL
#define BDEF_GSN 1 // GEN SEL NXT 
#define BDEF_NGS 2 // NXT GEN SEL
#define BDEF_NSG 3 // NXT SEL GEN
#define BDEF_SGN 4 // SEL GEN NXT
#define BDEF_SNG 5 // SEL NXT GEN

// Maximums
#define MAXSLOTS   6
#define MAXPW      20 // Max generated PWD length

// Keyboard Enter
#define KENTER 176

class Menu {
  public:
    Menu(Led&, Display&, Random&, Eeprom&);
    ~Menu();
    void init(int sseq);
    void shortpress(byte button);
    void longpress(byte button);
    void pressinglong(byte button);
    void set_buttonmode(byte mode);
    void set_slotcolors(byte cols);
    void setprto(byte to);

  private:
//    byte slot=0;
    byte prevslot=99;
    byte prto=0;
    unsigned long rtcount=0;
    struct eepw pwbuf;
    char dispbuf[64];
    char snbuf[EE_SNLEN];
    byte gen_button=B_GENERATE;
    byte nxt_button=B_NEXT;
    byte sel_button=B_SELECT;
    int waitforseq=0;
    int seqptr=0;
    char seqbuf[5];

    Led &led;
    Display &disp;
    Random &rand;
    Eeprom &eeprom;

    byte slotcolors[MAXSLOTS]; 

    void next(void);
    void select(void);
    void generate(void);
    void sendpw(bool sndcr, bool snduid);
    void displayslot(struct pwvalid *v);
    void showslotled(struct pwvalid *v);
    void displayleds(byte obmode, byte color, byte clmode);
    void indicate_slot(void);
};

#endif 