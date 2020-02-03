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

// Min
#define MINPW 6 // min pw length

// Macros
#define SUIPROMPT Serial.println(" "); if (waitforsec == 0) Serial.print(">"); else Serial.print("#"); Serial.flush()
#define SUICRLF  Serial.println(" "); Serial.flush()

class SerialUi {
  public:
    SerialUi(Led&, Display&, Random&, Eeprom&);
    ~SerialUi();
    void sio_init(int sseq);
    void vTaskSerialUi();

  private:
    int waitforsec=0;
    byte curslot=0;
    byte pwgenlen;
    byte pwgenmode;
    bool displck=false;

    int  st_inchar;
    byte st_ptr=0;
    char st_buf[MAXCMD];


    Led &led;
    Display &disp;
    Random &rand;
    Eeprom &eeprom;

    void reset(void);
    void printcurpw(void);
    void showentropy(void);
    void genpw(void);
    void handle_input(void);
    void parse_input(void);

    void handle_sec(void);
    void handle_slot(void);
    void handle_gen(void);
    void handle_set(void);
    void handle_to(void); 
    void handle_seq(void);
    void handle_eep(void);
    void handle_cmd(void);

    void toggle_flip(void);
    void show_flip(void);
    void toggle_prto(void);
    void show_prto(void);
    void toggle_logo(void);
    void menu_buttonconfig(void);
    void menu_ledconfig(void);
    void show_eevars(void);
    bool get_string(int len);
    void set_eeuid(void);
    void set_eepw(void);
    void set_eename(void);
    int buf_to_int(int start, int min, int max);
    void set_pwgmode(char m);
    void set_pwglen(void);
    void set_dispto(void);
    void show_dispto(void);
    void set_ledto(void);
    void show_ledto(void);
    void set_lockto(void);
    void show_lockto(void);
    void set_butseq(void);
    void show_butseq(void);
    void set_ledseq(void);
    void show_ledseq(void);
    void set_lockseq(void);
    void show_lockseq(void);
    void set_slot(char s);
    void dup_slot(char s);
};


#endif
