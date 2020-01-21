#ifndef SERIALUI_H
#define SERIALUI_H

#include <Arduino.h>
#include <avr/wdt.h>
#include "hardware.h"
#include "tick.h"

#include "led.h"
#include "display.h"
#include "random.h"
#include "eeprom.h"
#include "menu.h"

#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__) 
#include <Keyboard.h>
#endif

// GLobal modes
#define KM_KBD    0
#define KM_SERIAL 1

// Command length
#define MAXCMD 64

// Macros
#define SUIPROMPT  Serial.println(">"); Serial.flush()
#define SUICRLF    Serial.println(" "); Serial.flush()
#define SUICLS     Serial.write(0x1B); Serial.write('['); Serial.write('2'); Serial.write('J'); Serial.flush();

class SerialUi {
  public:
#ifndef MAINT
    SerialUi(Led&, Display&, Random&, Eeprom&);
#else
    SerialUi(Led&, Eeprom&);
#endif    
    ~SerialUi();
    void init(int sseq);
    void sio_menu_on();
    void sio_menu_off();
    bool running();
    void vTaskSerialUi();

  private:
    bool menurunning=false;
    byte waitfor;
    byte curslot=0;
    byte pwgenlen;
    byte pwgenmode;
    int mem;
    int waitforseq=0;    
    bool displck=false;

    int  st_inchar;
    byte st_ptr=0;
    char st_buf[MAXCMD];


    Led &led;
#ifndef MAINT
    Display &disp;
    Random &rand;
#endif    
    Eeprom &eeprom;

    void help(void);
    void printcurpw(void);
    void printcurname(void);
#ifndef MAINT  
    void genpw(void);
#endif
    void handle_input(void);
    void parse_input(void);
    void toggle_blink(void);
    void toggle_flip(void);
    void toggle_pwdisp(void);
    void menu_buttonconfig(void);
    void menu_ledconfig(void);
    void show_eevars(void);
    bool get_string(int len);
    void set_eeuid(void);
    void set_eepw(void);
    void set_eename(void);
    int buf_to_int(int min, int max);
#ifndef MAINT
    void set_pwgmode(char m);
    void set_pwglen(void);
    void set_dispto(void);
    void set_ledto(void);
    void set_lockto(void);
    void set_btnmode(char m);
    void set_colmode(char m);
#endif
    void set_slot(char s);
    void dup_slot(char s);
    void toggle_prto(void);
    void set_secseq(void);
    void get_initialpw(void);
};


#endif
