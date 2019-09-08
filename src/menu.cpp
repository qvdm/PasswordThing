/*
 * Name: Menu.cpp
 * 
 * Purpose: Provides the pushbutton user interface
 * 
 * Provides:
 *    init - Initialize menu
 *    shortpress - handler for short button press
 *    longpress  - handler for long button press
 *    pressinglong - hanlder for long press pre-notification
 *    set_buttonmode - sets the custom button assignment
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
    disp.displaylarge((char *) "LOCKED"); 
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

// Handle short press of a button
void Menu::shortpress(byte button)
{
  int i;
  
  if (waitforseq > 0)
  {
    if (button == B_GENERATE)
      seqbuf[seqptr++]='1';
    else if (button == B_NEXT)
      seqbuf[seqptr++]='2';
    else if (button == B_SELECT)
      seqbuf[seqptr++]='3';

    if (seqptr > (SSEQL-1))
    {
      seqbuf[seqptr]=0;
      int s = atoi(seqbuf);
      byte secok=false;
      if (s > 0)
      {
        for (i=1; i < NSSEQ+1; i++)
        {
          if (Secseq[i] == s)
          {
            if ((i-1) == waitforseq)
            {
              secok = true;
              waitforseq=0;
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
        disp.displaylarge((char *) "INVALID"); 
        for (;;);
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
  if ( (button == nxt_button) || (button == sel_button) ) // Long press on select is the same as short press on next
  {
    led.unblank();
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
  byte nxts;
  
  if (( button == nxt_button) || (button == sel_button) )
  {
    nxts = (slot+1) % MAXSLOTS; 
    displayleds(BLNK_SLOW, slotcolors[(int)nxts], BLNK_ON);  
  }
  else if (button == gen_button)
  {
    displayleds(BLNK_SLOW, COL_WHT, BLNK_ON);  
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

      // Show PW on Oled
      disp.displaysmall((char *) pwbuf.pwd, NULL, NULL);
    }
    else
    {
      disp.displaylarge((char *) "ERROR");
    }
  }
  else
  {
    disp.displaysmall((char *) "Entropy", NULL, NULL);
  }
  
  // Update LED
  indicate_slot();
}

// Send current password to the host, if valid.  Sndcr and Snduid toggles sending of CR and UID respectively
void Menu::sendpw(bool sndcr, bool snduid)
{
  byte lastc;
  char *s1=NULL, *s2=NULL, *s3=NULL;

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
      s1 = (char *) pwbuf.uid;
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


    // Display on OLED
    pwbuf.pwd[pwbuf.pwdlen]=0;
    if (pwbuf.pwdlen >= MAXPW) // long user entered pw - split
    {
      strcpy(dispbuf, (char *) &(pwbuf.pwd[MAXPW]));
      pwbuf.pwd[MAXPW] = 0;
    }
    if (s1) // UID exists
    {
      s2 = (char *) pwbuf.pwd;
      if (pwbuf.pwdlen >= MAXPW)
        s3 = dispbuf;
    }
    else 
    {
      s1 = (char *) pwbuf.pwd;
      if (pwbuf.pwdlen >= MAXPW)
        s2 = dispbuf;
    }
    disp.displaysmall(s1, s2, s3);

  }
}

// Sets the color and blink state of the LEDs
void Menu::displayleds(byte obmode, byte color, byte clmode)
{
  led.ob_blink(obmode);
  led.ledcolor(color, clmode);
}

// Set color of LED according to slot and blink rate according to validity
void Menu::showslotled(struct pwvalid *v)
{
  if (v->pwdvalid)
  { 
    if (v->uidvalid)
    {
      displayleds(BLNK_SLOW, slotcolors[(int)slot], BLNK_ON); 
    }
    else
    {
      displayleds(BLNK_SLOW, slotcolors[(int)slot], BLNK_ON); 
    }
  }
  else
  {
    displayleds(BLNK_SLOW, slotcolors[(int)slot], BLNK_FAST); 
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
  else strcat(dispbuf, " PW");

  disp.displaylarge(dispbuf);
}

// Check entry validity and display slot on Oled and LED accordingly
void Menu::indicate_slot()
{
  struct pwvalid v= eeprom.entryvalid(slot);

  displayslot(&v);
  showslotled(&v);
}
