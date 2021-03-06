/*
 * Name: Menu.cpp
 * 
 * Purpose: Provides the pushbutton user interface
 * 
 * Provides:
 *    init - Initialize menu
 *    shortpress - handler for short button press
 *    longpress  - handler for long button press
 *    pressinglong - handler for long press pre-notification
 *    verylongpress - handler for very long button press
 *    pressingverylong - handler for very long press pre-notification
 *    set_buttonmode - sets the custom button assignment
 *    set_slotcolors - set slot color assignment
 * 
 * Private: 
 *    next - next key handler
 *    select - select key handler
 *    generate - generate key handler
 *    sendpw - send password 
 *    displaylarge - display large chars on oled
 *    displaysmall - display small chars on oled
 *    displayslot - display slot info on oled
 *    showslotled -  set led color according to slot
 *    displayleds - actual output to all leds
 *    indicate_slot - slot indicator
 * 
 * Operation:
 *   Handles short and long press on Next, Select and Generate buttons
 * 
 *     Select button sends current PWD (and UID if defined)
 *     Generate button sends current PWD without CR (for changing pwds)
 *     Next button switches to next PWD slot
 *     Long press on Select performs Next
 * 
 *     Long press Generate generates a new PWD
 *
 */


#include "menu.h"

extern int Secseq[];
extern byte slot;

// Constructor
Menu::Menu(Led& rl, Display& rd, Random& rr, Eeprom& ee) : led(rl), disp(rd), rand(rr), eeprom(ee)
{
  set_slotcolors(0);
}
 
// Destructor
Menu::~Menu() { }

// Menu initialization that needs to be done outside constructor because of startup prereqs
void Menu::init(int sseq)
{
  slot = -1; 

  if (sseq == 0)
    next();
  else
  {
    waitforseq=sseq;
    // Set led to white in serial mode
    led.ledcolor(COL_YEL, BLNK_ON, true);
    disp.displaylarge((char *) "LOCKED", false); 
    disp.setprivacy(0);
  }
}

// Set the custom button assignments
void Menu::set_buttonmode(byte mode)
{
  // TBD refactor everywhere to use arrays / maps
  if (mode > BDEF_SNG) mode = BDEF_GNS;
  switch(mode)
  {
    case BDEF_GNS : // GEN NXT SEL
      gen_button=B_GENERATE;
      nxt_button=B_NEXT;
      sel_button=B_SELECT;
      break;
    case BDEF_GSN : // GEN SEL NXT 
      gen_button=B_GENERATE;
      sel_button=B_NEXT;
      nxt_button=B_SELECT;
      break;
    case BDEF_NGS : // NXT GEN SEL
      nxt_button=B_GENERATE;
      gen_button=B_NEXT;
      sel_button=B_SELECT;
      break;
    case BDEF_NSG : // NXT SEL GEN
      nxt_button=B_GENERATE;
      sel_button=B_NEXT;
      gen_button=B_SELECT;
      break;
    case BDEF_SGN : // SEL GEN NXT
      sel_button=B_GENERATE;
      gen_button=B_NEXT;
      nxt_button=B_SELECT;
      break;
    case BDEF_SNG : // SEL NXT GEN
      sel_button=B_GENERATE;
      nxt_button=B_NEXT;
      gen_button=B_SELECT;
      break;
  }
}

// Set the custom button assignments
void Menu::set_slotcolors(byte cols)
{
  byte *p=slotcolors;
  
  if (cols > 5) cols = 0;
  switch(cols)
  {
    case 0 : 
      *p++=COL_RED; *p++=COL_YEL; *p++= COL_GRN; *p++= COL_MAG; *p++= COL_BLU; *p++= COL_CYA;
      break;
    case 1 : 
      *p++=COL_YEL; *p++=COL_RED; *p++= COL_GRN; *p++= COL_MAG; *p++= COL_BLU; *p++= COL_CYA;
      break;
    case 2 : 
      *p++=COL_MAG; *p++=COL_RED; *p++= COL_BLU; *p++= COL_YEL; *p++= COL_GRN; *p++= COL_CYA;
      break;
    case 3 : 
      *p++=COL_CYA; *p++=COL_BLU; *p++= COL_MAG; *p++= COL_GRN; *p++= COL_YEL; *p++= COL_RED;
      break;
    case 4 : 
      *p++=COL_RED; *p++=COL_YEL; *p++= COL_GRN; *p++= COL_BLU; *p++= COL_MAG; *p++= COL_CYA;
      break;
    case 5 : 
      *p++=COL_BLU; *p++=COL_GRN; *p++= COL_YEL; *p++= COL_RED; *p++= COL_CYA; *p++= COL_MAG;
      break;
  }
}

// Handle press pre-notification for a button
void Menu::pressing(byte button)
{
  if (waitforseq > 0)
    return;

  if (button == nxt_button)
    disp.displaylarge((char *) "Nxt|Rvt|Ser", false); 
  else if(button == sel_button)
    disp.displaylarge((char *) "UP|Nxt|Rst", false); 
  else if (button == gen_button)  
    disp.displaylarge((char *) "Pwd|Gen|VP", false); 
}

// Handle short press of a button
void Menu::shortpress(byte button)
{
 
  // Locked & waiting for security sequence?
  if (waitforseq > 0)
  {
    //Translate button to seq
    if (button == B_GENERATE)
      seqbuf[seqptr++]='1';
    else if (button == B_NEXT)
      seqbuf[seqptr++]='2';
    else if (button == B_SELECT)
      seqbuf[seqptr++]='3';

    // Check if sequence valid
    if (seqptr > (SSEQL-1))
    {
      seqbuf[seqptr]=0;
      int s = atoi(seqbuf);
      byte secok=false;
      if (s > 0)
      {
        for (int i=1; i < NSSEQ+1; i++)
        {
          if (Secseq[i] == s)
          {
            if ((i-1) == waitforseq)
            {
              // Success!
              secok = true;
              waitforseq=0;
              eeprom.storesema(EESEM_BADLCK, 0);
              // Reinstate privacy timeout
              byte priv = eeprom.getvar(EEVAR_OPRIV); 
              disp.setprivacy(priv);
              next();
              break;
            }
          }
        }
      }
      if (!secok)
      {
        disp.displaylarge((char *) "INVALID", false); 
        byte b = eeprom.getsema(EESEM_BADLCK);
        eeprom.storesema(EESEM_BADLCK, b+1);
        WDRESET;
      }
    }
  }
  else
  {
    if (led.isblank())
      indicate_slot();
    else
    {
      if (button == nxt_button)
        next();
      else if(button == sel_button)
        select();
      else if (button == gen_button)  
        sendpw(false, false); // Send pwd (only), no CR
    }
  }
}

// Handle long press of a button
void Menu::longpress(byte button)
{
  if (waitforseq > 0) //  Bail if waiting for security sequence
    return;
  if (button == sel_button)  // Long press on select is the same as short press on next
  {
    led.unblank();
    next();
  }
  else if (button == nxt_button) // Long press on Next returns to slot 0
  {
    slot = -1; 
    next();
  }
  else if (button == gen_button) 
  {
    led.unblank();
    generate();
  }
}


// Handle long press pre-notification for a button
void Menu::pressinglong(byte button)
{
  if (waitforseq > 0)
    return;
  if (button == sel_button) 
  {
    byte nxts = (slot+1) % MAXSLOTS; 
    displayleds(slotcolors[(int)nxts], BLNK_ON);  
    disp.displaylarge((char *) "Nxt|Rst", false); 
  }
  else if ( button == nxt_button)
  {
    displayleds(slotcolors[0], BLNK_ON);  
    disp.displaylarge((char *) "Rvt|Ser", false); 
  }
  else if (button == gen_button)
  {
    displayleds(COL_WHT, BLNK_ON);  
    disp.displaylarge((char *) "Gen|View", false); 
  }
}

// Handle very long press of a button
void Menu::verylongpress(byte button)
{
  if (waitforseq > 0)
    return;
  if (button == sel_button) // Reset
  {
     disp.displaylarge((char *) "RESET", false); 
     displayleds(COL_YEL, BLNK_ON);
     WDRESET;
  }
  else if (button == gen_button) // View Pwd
  {
    //indicate_slot();
    displaypw();
  }
  else // next - reset to Serial mode
  {
     // Set serial boot flag
     eeprom.storesema(EESEM_SERMODE, 1);
     WDRESET;
  }
}

// Handle very long press pre-notification for a button
void Menu::pressingverylong(byte button)
{
  if (waitforseq > 0)
    return;
  if (button == sel_button) 
  {
    displayleds(COL_YEL, BLNK_ON);  
    disp.displaylarge((char *) "Reset", false); 
  }
  else if (button == gen_button) 
  {
    displayleds(slotcolors[slot], BLNK_ON);  
    disp.displaylarge((char *) "View", false); 
  }
  else
  {
    displayleds(COL_WHT, BLNK_ON);  
    disp.displaylarge((char *) "Serial", false); 
  }
}


// Next function 
void  Menu::next()
{
  if (++slot >= MAXSLOTS) 
    slot= 0;

  // Show where we are
  indicate_slot();
}

// Select function
void  Menu::select()
{
  // Send the password
  sendpw(true, true);
  indicate_slot();
}

void  Menu::generate()
{
  int e = rand.getEntropy(); // check if we have enough entropy
  if (e >= MAXPW)
  {
    eeprom.getpw(slot, &pwbuf);
    if (rand.genPw((char *) &(pwbuf.pwd), MAXPW, PWM_SPEC) )
    {
      pwbuf.pwdlen=MAXPW;

      // Store generated password in EEprom
      eeprom.storepw(slot, &pwbuf);
      pwbuf.pwd[MAXPW]=0;

      indicate_slot();
    }
    else
    {
      disp.displaylarge((char *) "ERROR", true);
    }
  }
  else
  {
    disp.displaylarge((char *) "Entropy", true);
  }
  
  // Update LED
  indicate_slot();
}

// Send current password to the host, if valid.  Sndcr and Snduid toggles sending of CR and UID respectively
void Menu::sendpw(bool sndcr, bool snduid)
{
  byte lastc;

  // Get PWD and UID validity
  struct pwvalid v=eeprom.entryvalid(slot);
  if (v.pwdvalid)
  {
    // Read the PWD/UID from EEPROM
    eeprom.getpw(slot, &pwbuf);
    if ( (v.uidvalid) && snduid)
    {
      // Valid UID - send it
      pwbuf.uid[pwbuf.uidlen]=0;
      for (int i=0; i < pwbuf.uidlen; i++)
      {
        Keyboard.write(pwbuf.uid[i]);
        lastc=pwbuf.uid[i];
        delay(10);
      }
      if (lastc != '\t')
      {
        delay(50);
        Keyboard.press(KEY_RETURN);
        delay(50);
        Keyboard.release(KEY_RETURN);
      }
      delay(50);
    }
    // Send the PWD
    for (int i=0; i < pwbuf.pwdlen; i++)
    {
      Keyboard.write(pwbuf.pwd[i]);
      delay(10);
    }

    if (sndcr)
    {
      delay(50);
      Keyboard.press(KEY_RETURN);
      delay(50);
      Keyboard.release(KEY_RETURN);
      delay(50);
    }
  }
  indicate_slot();
}

// Sets the color and blink state of the LEDs
void Menu::displayleds(byte color, byte clmode)
{
  led.ledcolor(color, clmode, false);
}

// Set color of LED according to slot and blink rate according to validity
void Menu::showslotled(struct pwvalid *v)
{
  if (v->pwdvalid)
  { 
    if (v->uidvalid)
    {
      displayleds(slotcolors[(int)slot], BLNK_ON); 
    }
    else
    {
      displayleds(slotcolors[(int)slot], BLNK_ON); 
    }
  }
  else
  {
    displayleds(slotcolors[(int)slot], BLNK_FAST); 
  }
}



// Display slot # and info on OLED
void Menu::displayslot(struct pwvalid *v)
{
  eeprom.getname(slot, snbuf);
  if (strlen(snbuf) > 0)
    sprintf(dispbuf, "%s", snbuf);
  else
    sprintf(dispbuf, "Slot %d", slot);
  if (!v->pwdvalid) strcat(dispbuf, " INV");
  else if (v->uidvalid) strcat(dispbuf, " UP");
  else strcat(dispbuf, " P");

  disp.displaylarge(dispbuf, true);

}

// Check entry validity and display slot on Oled and LED accordingly
void Menu::indicate_slot()
{
  struct pwvalid v= eeprom.entryvalid(slot);
  displayslot(&v);
  showslotled(&v);
  prevslot = slot;
}

// Display pwd on OLED
void Menu::displaypw()
{
  struct eepw pw;
  eeprom.getpw(slot, &pw);

  if (pw.pwdlen > 20)
  {
    strncpy((char *) d2buf, (char *) pw.pwd, 20);  d2buf[20]=0;
    strcpy((char *) dispbuf, (char *) (pw.pwd+20));  
  }


  if (pw.uidlen > 0)
    if (pw.pwdlen <= 20)
      disp.displaysmall((char *) pw.uid, "", (char *) pw.pwd);
    else
      disp.displaysmall((char *) pw.uid, (char *) d2buf, (char *) dispbuf);
  else
    if (pw.pwdlen <= 20)
      disp.displaysmall((char *) pw.pwd, "", "");
    else
      disp.displaysmall((char *) d2buf, (char *) dispbuf, "");
}
