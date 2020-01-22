#ifndef EEPROM_H
#define EEPROM_H

#include <EEPROMex.h>
#include <Arduino.h>
#include "hardware.h"
#include "utils.h"

#define EE_PWLEN   30 // Max space for uid/pwd
#define EE_SNLEN    8 // Slot name
#define EE_ULLEN    1 // Space for UID length
#define EE_PLLEN    1 // Space for PWD length
#define EE_HDRLEN   (EE_ULLEN+EE_PLLEN+EE_SNLEN)
#define EE_SLOTLEN  (EE_HDRLEN+EE_PWLEN*2) 
#define EE_PWOFS    (EE_HDRLEN+EE_PWLEN)
#define EE_NUMSLOTS 8
#define EE_VARS    16
#define EE_VARLOC (EE_SLOTLEN*EE_NUMSLOTS)
#define EE_CRCLOC (EE_VARLOC+EE_VARS)
#define EE_SEMALOC (EE_VARLOC+EE_VARS+4)

struct eepw {
  byte uidlen;
  byte pwdlen;
  byte slotname[EE_SNLEN];
  byte uid[EE_PWLEN];
  byte pwd[EE_PWLEN];
};

struct pwvalid {
  bool uidvalid;
  bool pwdvalid;
};

union eeentry
{
    struct eepw eep;
    byte b[EE_SLOTLEN];
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
    void storename(byte slot, char* name);
    void getname(byte slot, char* name);
    void storevar(int var, byte value);
    byte getvar(int var);
    void clearslot(int slot);
    void dupslot(int source, int dest);
    void dump();
    void backup();
    void restore();


  private:
    bool eevalid=false;
    unsigned long calc_crc(void);
    void update_crc(void);
    union eeentry eee;
    char parsebuf[EE_SLOTLEN*2+2];
    byte vbuf[EE_SLOTLEN];
};

#endif 