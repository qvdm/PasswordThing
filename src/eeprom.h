#ifndef EEPROM_H
#define EEPROM_H

#include <EEPROMex.h>
#include <Arduino.h>
#include "hardware.h"

#define EE_PWLEN  31 // Max space for uid/pwd
#define EE_STRLEN (EE_PWLEN*2+2) 
#define EE_MAXSTR  8
#define EE_VARS    4
#define EE_VARLOC EE_STRLEN * EE_MAXSTR
#define EE_CRCLOC EE_STRLEN * EE_MAXSTR + EE_VARS

struct eepw {
  byte uidlen;
  byte pwdlen;
  byte uid[EE_PWLEN];
  byte pwd[EE_PWLEN];
};

struct pwvalid {
  bool uidvalid;
  bool pwdvalid;
};

class Eeprom {
  public:

    Eeprom();
    ~Eeprom();
    bool valid();
    void zero();
    struct pwvalid entryvalid (int slot);
    void storepw(byte slot, struct eepw* pw);
    void getpw(byte slot, struct eepw* pw);
    void storevar(int var, byte value);
    byte getvar(int var);
    void clearslot(int slot);
    void dupslot(int source, int dest);
    void dump();


  private:
    bool eevalid;
    unsigned long calc_crc(void);
    void update_crc(void);

};

#endif 