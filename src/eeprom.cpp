/*
 * Name: Eeprom.cpp
 * 
 * Purpose: Manages EEPROM I/O
 * 
 * Provides:
 *   valid?     - Eeprom CRC valid?
 *   zero       - Zero EEPROM 
 *   entryvalid - Specific entry valid?
 *   storepw    - Store a PWD and optional UID
 *   getpw      - Get a PWD and optional UID
 *   storename  - Store a slot name
 *   getname    - Get slot name
 *   storevar   - Store a value to Variables area
 *   getvar     - Get a value from Variables area
 *   clearslot  - Clear specified PWD slot
 *   dupslot    - duplicate the content of a slot
 *   dump       - Debug: dump eeprom contents to Serial
 *   backup     - Dump contents in backup format to Serial
 *   restore    - Restore from backup dump via serial
 *   check_signature
 *   write_signature
 * 
 * Private: 
 *   calc_crc
 *   update_crc
 * 
 * Operation:
 *   Eeprom is organized as follows:
 *   0000:                      <UID0_Len> <PWD0_Len> <SLOT0 NAME> <UID0_B0> <UID0_B1> ... <UID0_Bn> <PAD>x(EE_PWLEN - UID_0Len) <PWD0_B0> <PWD0_B1> ... <PWD0_Bn> <PAD>x(EE_PWLEN - PWD_0Len)
 *   EE_SLOTLEN:                <UID1_Len> <PWD1_Len> <SLOT0 NAME> <UID1_B0> <UID1_B1> ... <UID1_Bn> <PAD>x(EE_PWLEN - UID_1Len) <PWD1_B0> <PWD1_B1> ... <PWD1_Bn> <PAD>x(EE_PWLEN - PWD_1Len)
 *   ...
 *   EE_SLOTLEN x (EE_NUMSLOTS-1): <UIDn_Len> <PWDn_Len> <UIDn_B0> ...
 *   EE_SLOTLEN x EE_NUMSLOTS:     <NVBYTE0> <NVBYTE1> <NVBYTE2> ... <NVBYTE15> <CRC0> <CRC1> <CRC2> <CRC3>
 * 
 */

#include "eeprom.h"

extern byte eedVer;
extern byte eevVer;

Eeprom::Eeprom()
{
}

Eeprom::~Eeprom() { }

// Is ee state valid?
bool Eeprom::valid()
{
  unsigned long eecrc, crc;

  // Check Eeprom CRC  and set EE state to invalid if wrong
  crc = calc_crc();
  eecrc = EEPROM.readLong(EE_CRCLOC);
  eevalid = (crc == eecrc);

  return eevalid;
}

// Update CRC in EEprom with current data
void Eeprom::update_crc()
{
   unsigned long crc = calc_crc();

  EEPROM.updateLong(EE_CRCLOC, crc );
  eevalid = true;
}

// Check signature
bool Eeprom::check_signature()
{
   unsigned long sig;
   unsigned long csig = EE_SIG;

  sig = EEPROM.readLong (EE_SIGLOC);
  return (sig == csig);
}


// Write signature
void Eeprom::write_signature()
{
   unsigned long sig = EE_SIG;

  EEPROM.updateLong(EE_SIGLOC, sig );
}

// Zero out the EEPROM and initialize the CRC - takes a long time 
void Eeprom::zero()
{
  int i;
  
  for (i = 0 ; i < EE_USED  ; i++)
  {
    EEPROM.update(i, (byte) 0 );
  }
  
  update_crc();
  storesema(EESEM_EEVER, eevVer);
  write_signature();

}

void Eeprom::upgrade_schema(void)
{
  byte oldver = getsema(EESEM_EEVER);
  
  if ( (oldver < OLDCRCVER) && (eevVer >= OLDCRCVER) )
  {
    update_crc();
  }

  storesema(EESEM_EEVER, eevVer);
}

// Calculate CRC
unsigned long Eeprom::calc_crc(void) 
{
  const unsigned long crc_table[16] = 
  {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };


  unsigned long crc = ~0L;
  
  for (int index = 0 ; index < EE_CRCLOC  ; ++index) {
    byte eebyte = EEPROM.readByte(index);

    crc ^= eebyte; 
    crc = crc_table[crc & 0x0f] ^ (crc >> 4);
    crc = crc_table[crc & 0x0f] ^ (crc >> 4);
  }
  return (crc ^ 0xFFFFFFFF);
}

// Checks if a password and/or UID entry is valid
struct pwvalid Eeprom::entryvalid (int slot)
{
  struct pwvalid res;
  int addr = slot * EE_SLOTLEN;
  
  res.uidvalid = false;
  res.pwdvalid = false;
  
  byte uidlen = EEPROM.readByte(addr);
  byte pwdlen = EEPROM.readByte(addr+1);

  if ( (uidlen != 0) && (uidlen <= EE_PWLEN) )
    res.uidvalid = true;
  if ( (pwdlen != 0) && (pwdlen <= EE_PWLEN) )
    res.pwdvalid = true;
  
  return res;
}

// Store a pwd entry in EEprom
void Eeprom::storepw(byte slot, struct eepw* pw)
{
  int addr = slot*EE_SLOTLEN;
  if (pw->uidlen > 0)
  {
    EEPROM.updateByte(addr, pw->uidlen);
    for (int i=0; i < pw->uidlen; i++)
    {
      EEPROM.updateByte(addr+EE_HDRLEN+i, pw->uid[i]);
    }
  }
  else
  {
    EEPROM.updateByte(addr, 0);
  }
  
  if (pw->pwdlen > 0)
  {
    EEPROM.updateByte(addr+1, pw->pwdlen);
    for (int i=0; i < pw->pwdlen; i++)
    {
      EEPROM.updateByte(addr+EE_PWOFS+i, pw->pwd[i]);
    }
  }
  else
  {
    EEPROM.updateByte(addr+1, 0);
  }
  update_crc();
}

// Get a Pwd entry from EEPROM
void Eeprom::getpw(byte slot, struct eepw* pw)
{
  int addr = slot*EE_SLOTLEN;

  pw->uidlen = EEPROM.readByte(addr);
  pw->pwdlen = EEPROM.readByte(addr+1);

  if (pw->uidlen > 0)
  {
    for (int i=0; i < pw->uidlen; i++)
    {
      pw->uid[i] = EEPROM.readByte(addr+EE_HDRLEN+i);
    }
  }
  
  if (pw->pwdlen > 0)
  {
    for (int i=0; i < pw->pwdlen; i++)
    {
      pw->pwd[i] = EEPROM.readByte(addr+EE_PWOFS+i);
    }
  }
}

// Store a slot name to EEprom
void Eeprom::storename(byte slot, char* name)
{
  int addr = slot*EE_SLOTLEN;
  for (int i=0; i < EE_SNLEN; i++)
  {
    EEPROM.updateByte(addr+EE_ULLEN+EE_PLLEN+i, name[i]);
  }
  update_crc();
}

// Retrieve a slot name from EEprom
void Eeprom::getname(byte slot, char* name)
{
  int addr = slot*EE_SLOTLEN;
  for (int i=0; i < EE_SNLEN; i++)
  {      
    name[i] = EEPROM.readByte(addr+EE_ULLEN+EE_PLLEN+i);
  }
}

// Store a variable in the EEPROM variable space
void Eeprom::storevar(int var, byte value)
{
  int addr = EE_VARLOC+var;
 
  EEPROM.updateByte(addr, value);
  update_crc();
}

// Read a variable from the EEPROM variable space
byte Eeprom::getvar(int var)
{
  int addr = EE_VARLOC+var;

  return EEPROM.readByte(addr);
}

// Store a semaphore in the EEPROM semaphore space
void Eeprom::storesema(int sema, byte value)
{
  int addr = EE_SEMALOC+sema;
 
  EEPROM.updateByte(addr, value);
}

// Read a semaphore from the EEPROM semaphore space
byte Eeprom::getsema(int sema)
{
  int addr = EE_SEMALOC+sema;

  return EEPROM.readByte(addr);
}



// Clears a UID/PWD slot
void Eeprom::clearslot(int slot)
{
  int addr = slot*EE_SLOTLEN;
  EEPROM.updateByte(addr, 0); // zero uid length
  EEPROM.updateByte(addr+1, 0); // zero pwd length
  EEPROM.updateByte(addr+EE_ULLEN+EE_PLLEN, 0); // clear name
  update_crc();
}

void Eeprom::dupslot(int source, int dest)
{
  int src_addr = source*EE_SLOTLEN;
  int dst_addr = dest*EE_SLOTLEN;
  for (int i=0; i < EE_SLOTLEN; i++)
  {
    EEPROM.updateByte(dst_addr+i, EEPROM.readByte(src_addr+i));
  }
  update_crc();
}


// Utility function to dump entire EEPROM to Serial
void Eeprom::dump()
{
  Serial.begin(115200);  while (!Serial);
  Serial.println("EEPROM Dump");
  Serial.println("SLOTS");
  for (int i = 0 ; i < EE_NUMSLOTS  ; ++i)
  {
    sprintf(eo_buf, "S%dUPL %04x", i, (i*EE_SLOTLEN));
    Serial.println(eo_buf);
    Serial.flush();
    for (int j=0; j < EE_SLOTLEN; j++)
    {
      if ( (j==EE_HDRLEN) || (j==EE_PWOFS) )
        Serial.println("");
      sprintf(eo_buf, "%02X ", EEPROM.readByte(i*EE_SLOTLEN+j));
      Serial.print(eo_buf);
      Serial.flush();
    }
    Serial.println("");
    Serial.flush();
  }

  Serial.println(F("VARS"));
  for (int k = 0 ; k < EE_VARS  ; ++k)
  {
    sprintf(eo_buf, "%02X ", EEPROM.readByte(EE_VARLOC+k));
    Serial.print(eo_buf);
    Serial.flush();
  }
  Serial.println("");
  Serial.flush();
  Serial.println(F("CRC"));
  for (int l = 0 ; l < 4  ; ++l)
  {
    sprintf(eo_buf, "%02X ", EEPROM.readByte(EE_CRCLOC+l));
    Serial.print(eo_buf);
    Serial.flush();
  }
  Serial.println("");
  Serial.flush();

  Serial.println(F("SEMAS"));
  for (int m= 0 ; m < EE_SEMAS  ; ++m)
  {
    sprintf(eo_buf, "%02X ", EEPROM.readByte(EE_SEMALOC+m));
    Serial.print(eo_buf);
    Serial.flush();
  }
  Serial.println("");
  Serial.flush();

  Serial.println(F("SIGNATURE"));
  for (int n= 0 ; n < EE_SIGLEN  ; ++n)
  {
    sprintf(eo_buf, "%02X ", EEPROM.readByte(EE_SIGLOC+n));
    Serial.print(eo_buf);
    Serial.flush();
  }


  Serial.println("");
  Serial.println("");
  Serial.flush();
}

// Utility function to dump entire EEPROM to Serial in backup format
void Eeprom::backup()
{
  Serial.begin(115200);  while (!Serial);
  sprintf(eo_buf, "V%02d", eedVer);
  Serial.println(eo_buf); 

  for (int i = 0 ; i < EE_USED  ; ++i)
  {
    sprintf(eo_buf, "%02X", EEPROM.readByte(i));
    Serial.print(eo_buf);
  }
  Serial.println("");
  Serial.println("");
  Serial.flush();
}



void Eeprom::restore()
{
  int ptr;
  byte ee;

  Serial.println("");
  Serial.print("R>");
  Serial.flush();

  while (Serial.available() == 0 ); // wait for input
  parsebuf[3]=0;
  parsebuf[0] = Serial.read();
  parsebuf[1] = Serial.read();
  parsebuf[2] = Serial.read();
  Serial.read();

  sprintf(eo_buf, "V%02d", eedVer);
  
  if (strcmp(parsebuf, eo_buf) != 0)
  {
    Serial.print("Wrong Version, got ");
    Serial.print(parsebuf);
    Serial.print(" expected ");
    Serial.println(eo_buf);
    Serial.flush();
    return;
  }

  while (Serial.available() == 0 ); // wait for input
  
  parsebuf[2]=0;
  ptr=0;
  while (Serial.available() > 0 ) 
  {
    parsebuf[0] = Serial.read();
    parsebuf[1] = Serial.read();
    ee = hextobyte(parsebuf);
    EEPROM.updateByte(ptr, ee);
    if (++ptr > EE_USED)
      break;
  }
  
  if (valid())
    Serial.println(F("Restored"));
  else
  {
    Serial.println(F("Bad Restore"));
    zero();
  }



}


