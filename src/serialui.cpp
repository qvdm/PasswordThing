/*
 * Name: SerialUi.cpp
 * 
 * Purpose: Provides a UI via the serial port for entering UIDS and Passwords, or generating PWDs
 * 
 * Provides:
 *    sio_menu_on/off; - Turn the Serial menu on or off
 *    running - is serial menu running?
 *    vTaskSerialUi(); - Periodic task for processing serial input
 * 
 * Private: 
 *     help - display help
 *     kbd_on - return to keyboard mode
 *     kbd_off - turn off keyboard mode
 *     printcurpw - print pwd
 *     genpw - generate pwd
 *     printcurname - print slot name
 *     handle_input - input handler
 *     handle_cmd - command handler
 *     toggle_flip - toggle disp flip
 *     toggle_pwdisp - toggle pw disp
 *     toggle_prto - toggle disp timeout
 *     menu_buttonconfig - get new button config
 *     menu_ledconfig - get new led config
 *     show_eevars - show ee vars
 *     handle_data - handle command data
 *     get_string - utility
 *     set_eeuid 
 *     set_eepw
 *     set_eename
 *     set_pwgmode
 *     set_slot
 *     dup_slot
 *     buf_to_int
 *     set_pwglen
 *     set_dispto
 *     set_ledto
 *     set_lockto
 *     set_btnmode
 *     set_colmode
 *     set_secseq
 *     get_initialpw
 *
 * Operation:
 *   Parses sequences in one of the followinf formats:
 * <slot><cmd>[arg]
 * 
 * 1Opasswd - Set S1 pwd
 * 1Uuserid - Set S1 uid
 * 1Nname - Set S1 name
 * 1P - Display S1 uid pwd name
 * 1G Generate pwd for S1
 * 1C Clear S1
 * 1D2 - Duplicate S1 to S2
 * 
 * <group><cmd>[arg]
 * 
 * GM<A|N|L|S> - Generator Mode
 * GM - Show generator mode
 * GL<nn> : L10 - Pwd length
 * GL - Show pwd length
 * GE - show entropy
 * 
 * SFT - Toggle display flip 
 * SF - Show
 * SRT - Toggle pwd revert
 * SR - show
 * 
 * TD<nnn> : D030 - Set display timeout to 30s
 * TD - Show
 * TI<nnn> : I030 - Set LED timeout to 30s
 * TI - show
 * TL<nnn> : L030 - set lock timeout to 30 mins
 * TL - show
 * 
 * QB<bbb> : TSNG - Set button sequence to SNG
 * QB - show
 * QL<llllll> : TRGBCMY - Set LED sequence to RGBCMY
 * QL - show
 * 
 * ED - Dump eeprom in friendly mode
 * EB - Dump eeprom in backup mode
 * ER - Restore eeprom
 * EZ - Zero eeprom
 * 
 * <cmd>
 * MR - Reset
 * ML - Toggle logo display at startup
 * MS<nnnn> : W1231 - Set security seq to 1231
 * MS - show security seq
 * 
 * V - Show EEprom version
 * R - Show Software release
 */

#include "serialui.h"

// External refs
extern unsigned long getTime(void);
extern int Secseq[];
extern char Version[];
extern byte eevVer;
extern byte eedVer;

// CTOR
SerialUi::SerialUi(Led& rl, Display &rd, Random& rr, Eeprom& ee) : led(rl), disp(rd), rand(rr), eeprom(ee)
{
  pwgenlen = MAXPW;
  pwgenmode = PWM_SPEC;
}

SerialUi::~SerialUi() { }

// Initialize SIO menu 
void SerialUi::sio_init(int sseq)
{
  waitforsec=sseq;
  disp.displaylarge((char *) "R-SERIAL", false); 
  led.ledcolor(COL_WHT, BLNK_ON, true);
  Keyboard.end(); // Turn off kbd
  Serial.begin(115200);  while (!Serial); 
  Serial.println((char *) Version);
  if (waitforsec == 0)
  {
    disp.displaylarge((char *) "SERIAL", false); 
    led.ledcolor(COL_WHT, BLNK_MOST, false);
  }
  else
  {
    disp.displaylarge((char *) "LOCKED", false); 
    disp.setprivacy(0);
    led.ledcolor(COL_YEL, BLNK_ON, true);
  }
  SUIPROMPT;
}

// Periodic 'Task' handling serial UI
void SerialUi::vTaskSerialUi()
{

  // Read input
  if (Serial.available() > 0) 
  {
    st_inchar = Serial.read();
    if (waitforsec == 0)
      Serial.write(st_inchar); Serial.flush();
    handle_input();
  }
}

// Process a single menu input char
void SerialUi::handle_input()
{
  if ( st_inchar == '\r' ) // CMD received - process
  {
    SUICRLF;
    parse_input();
    st_ptr=0;
    SUIPROMPT;
  }
  else if (st_inchar == '\b') // Backspace
  {
    if (st_ptr > 0) 
    {
      Serial.write(" \b"); Serial.flush();
      st_ptr--;
    }
  }
  else if ( ((st_inchar >= ' ') && (st_inchar <= '~') )  || (st_inchar == '\t') ) // buffer input char
  {
    st_buf[st_ptr++] = st_inchar;
  }
}

// Parse the input line
void SerialUi::parse_input()
{
  st_buf[st_ptr]=0;

  if ( st_buf[0] == 'V' ) // EE var version req
  {
    handle_ver();
  }
  else if ( st_buf[0] == 'S' ) // EE schema/layout version request
  {
    handle_sch();
  }
  else if ( st_buf[0] == 'R' ) // SW release req
  {
    handle_rel();
  }
  else if (waitforsec != 0) // Security sequence active? 
  {
    handle_sec();
  }
  else if ( (st_buf[0] >= '0') && (st_buf[0] < '0' + MAXSLOTS) ) // Fist char is a digit --> slot
  {
    curslot = st_buf[0] - '0';
    handle_slot();
  }
  else
  {
    switch (toupper(st_buf[0]))
    {
        case 'G' : handle_gen(); break;
        case 'S' : handle_set(); break;
        case 'T' : handle_to(); break;
        case 'Q' : handle_seq(); break;
        case 'E' : handle_eep(); break;
        case 'M' : handle_cmd(); break;
        case 'H' :
        case '?' : Serial.println("#GSTQEM");
     }
  }
}

// Show EE var version
void SerialUi::handle_ver()
{
  if (waitforsec != 0)
    Serial.write('L');
  else 
    Serial.write('E');
  if (eevVer < 10) Serial.write('0');
  Serial.println(eevVer);
}

// Show SW version
void SerialUi::handle_rel()
{
  Serial.write('R');
  Serial.println(Version);
}

// Show EE schema version
void SerialUi::handle_sch()
{
  Serial.write('V');
  if (eedVer < 10) Serial.write('0');
  Serial.println(eedVer);
}


// Parse security sequence
void SerialUi::handle_sec()
{
  if (st_buf[0] == '?') 
  {
    Serial.println("#Z");
    return;
  }
  else if (st_buf[0] == 'Z')
  {
    eeprom.zero();
    waitforsec=0;
    return;
  }

  if (st_ptr != SSEQL)
  {
    byte b = eeprom.getsema(EESEM_BADLCK);
    eeprom.storesema(EESEM_BADLCK, b+1);
    return; 
  }
  for (int i=0; i < SSEQL; i++)
    if (!isdigit(st_buf[i]))
    {
      byte b = eeprom.getsema(EESEM_BADLCK);
      eeprom.storesema(EESEM_BADLCK, b+1);
      return;
    }
  int d = buf_to_int(0, 1, 3333);
  if ( d > 0 )
  {
    for (int i=1; i < NSSEQ+1; i++)
    {
      if (Secseq[i] == d)
      {
        if ((i-1) == waitforsec)
        {
          // Success
          waitforsec=0;
          eeprom.storesema(EESEM_BADLCK, 0);
          disp.displaylarge((char *) "SERIAL", false); 
          led.ledcolor(COL_WHT, BLNK_MOST, false);
          break;
        }
      }
    }
  }
}

// Handle slot subcommands
void SerialUi::handle_slot()
{

  switch (toupper(st_buf[1]))
  {
    // Set new password for slot
    case 'S' : if ((st_ptr > MINPW+2) && (st_ptr < EE_PWLEN+2))  
               { 
                 set_eepw(); 
                 printcurpw(); 
               }
               else Serial.println("LENGTH");
               break;
    // Set new userid for slot
    case 'U' : if ((st_ptr > 3) && (st_ptr < EE_PWLEN+2)) 
               { 
                 set_eeuid(); 
                 printcurpw(); 
               }
               else Serial.println("LENGTH");
               break;
    // Set new name for slot
    case 'N' : if ((st_ptr > 3) && (st_ptr < EE_PWLEN+2)) 
               { 
                 set_eename(); 
                 printcurpw(); 
               }
               else Serial.println("LENGTH");
               break;
    // Print
    case 'P' : printcurpw(); break;
    // Generate
    case 'G' : genpw(); printcurpw(); break;
    // Clear
    case 'C' : eeprom.clearslot(curslot); printcurpw();break;
    // Duplicate
    case 'D' : if ( (st_buf[2] >= '0') && (st_buf[2] < '0' + MAXSLOTS) ) dup_slot(st_buf[2]); printcurpw(); break;
    case 'H' :
    case '?' : Serial.println("SUNPGCD");
    
  }
}

// Handle generator subcommands
void SerialUi::handle_gen()
{
  switch (toupper(st_buf[1]))
  {
    // Set mode
    case 'M' : if (st_ptr > 1) set_pwgmode(st_buf[2]); break;
    // Set length
    case 'L' : if (st_ptr > 1) set_pwglen(); break;
    // Show entropy
    case 'E' : showentropy(); break;
    case 'H' :
    case '?' : Serial.println("MLE");
  }
}

// Handle Set subommands
void SerialUi::handle_set()
{
  switch (toupper(st_buf[1]))
  {
    // Display flip
    case 'F' : if ( (st_ptr > 1) && (toupper(st_buf[2]) == 'T') ) toggle_flip(); show_flip(); break;
    // Pwd revert
    case 'R' : if ( (st_ptr > 1) && (toupper(st_buf[2]) == 'T') ) toggle_prto(); show_prto(); break;
    case 'H' :
    case '?' : Serial.println("FR");
  }
}

// Handle timeout subcommands
void SerialUi::handle_to() 
{
  switch (toupper(st_buf[1]))
  {
    // Display
    case 'D' : if (st_ptr > 2) set_dispto(); show_dispto(); break;
    // LED
    case 'I' : if (st_ptr > 2) set_ledto(); show_ledto(); break;
    // Lock
    case 'L' : if (st_ptr > 2) set_lockto(); show_lockto(); break;
    case 'H' :
    case '?' : Serial.println("DIL");
  }
}

// Handle Sequence subcommands
void SerialUi::handle_seq()
{
  switch (toupper(st_buf[1]))
  {
    // Buttons
    case 'B' : if (st_ptr > 2) set_butseq(); show_butseq(); break;
    // Led colors
    case 'L' : if (st_ptr > 2) set_ledseq(); show_ledseq(); break; 
    // Bad sequence action
    case 'U' : if (st_ptr > 2) set_badseq(); show_badseq(); break; 
    case 'H' :
    case '?' : Serial.println("BLU");
  }
}

// Handle eeprom subcommands
void SerialUi::handle_eep()
{
  switch (toupper(st_buf[1]))
  {
    // Human readable dump
    case 'D' : eeprom.dump(); break;
    // Backup
    case 'B' : eeprom.backup(); break;
    // Restore
    case 'R' : Serial.setTimeout(30000); eeprom.restore(); break;
    // Zero
    case 'Z' : eeprom.zero(); break;
    case 'H' :
    case '?' : Serial.println("DBRZ");
  }
}

// Handle management subcommands
void SerialUi::handle_cmd() 
{
  switch (toupper(st_buf[1]))
  {
    // Reset
    case 'R' : reset(); break;
    // Toggle logo display
    case 'L' : toggle_logo(); break;
    // Lock sequence
    case 'S' : if (st_ptr > 2) set_lockseq(); show_lockseq(); break;
    case 'H' :
    case '?' : Serial.println("RLS");
  }
}

// Print pwd and/or UID
void SerialUi::printcurpw()
{
  struct pwvalid ev = eeprom.entryvalid(curslot);
  struct eepw pw;
  eeprom.getpw(curslot, &pw);
  eeprom.getname(curslot, st_buf);
  if (strlen(st_buf) > 0)
  {
    for (unsigned int n=0; n < strlen(st_buf); n++)
      Serial.write(st_buf[n]);
    Serial.println("");
    Serial.flush();
  }
  if (ev.uidvalid)
  {
    Serial.write('\"');
    for (int i=0; i < pw.uidlen; i++)
    {
      if (pw.uid[i] == '\t')
        Serial.write("<TAB>");
      else
        Serial.write(pw.uid[i]);
    }
    Serial.write('\"');
  }
  else
    Serial.print(F("None "));
  Serial.println("");
  Serial.flush();
  
  if (ev.pwdvalid)
  {
    Serial.write('\"');
    for (int i=0; i < pw.pwdlen; i++)
      Serial.write(pw.pwd[i]);
    Serial.write('\"');
  }
  else
    Serial.print(F("None "));
}

// Show entropy
void SerialUi::showentropy()
{
  int e = rand.getEntropy();
  Serial.print(e); 
}

// Generate a password
void SerialUi::genpw()
{
  struct eepw pw;

  int e = rand.getEntropy(); // check if we have enough entropy
  if (e >= pwgenlen)
  {
    eeprom.getpw(curslot, &pw);
    if (rand.genPw((char *) &(pw.pwd), pwgenlen, pwgenmode))
    {
      pw.pwdlen=pwgenlen;

      // Store generated password in EEprom
      eeprom.storepw(curslot, &pw);
    }
  }
}

// Reset
void SerialUi::reset()
{
  eeprom.storesema(EESEM_SERMODE, 0); // get out of wserial mode after reset
  WDRESET;
}


// Toggle display flip state
void SerialUi::toggle_flip()
{
  bool flip = (bool) eeprom.getvar(EEVAR_DFLP);
  flip = !flip;
  disp.setflip(flip);
  eeprom.storevar(EEVAR_DFLP, (byte) flip);
}

// Show display flip state
void SerialUi::show_flip()
{
  bool flip = (bool) eeprom.getvar(EEVAR_DFLP);
  if (flip) Serial.print(F("ON")); else Serial.print(F("OFF")); 
}

// Toggle PWD revert state
void SerialUi::toggle_prto()
{
  bool prto = eeprom.getvar(EEVAR_PRTO);
  prto = !prto;
  disp.setpwrevert(prto);
  eeprom.storevar(EEVAR_PRTO, (byte) prto);
}

// Show PWD revert state
void SerialUi::show_prto()
{
  bool prto = (bool) eeprom.getvar(EEVAR_PRTO);
  if (prto) Serial.print(F("ON")); else Serial.print(F("OFF")); 
}


// Toggle logo display state
void SerialUi::toggle_logo()
{
  bool logo = (bool) eeprom.getvar(EEVAR_AYB);
  logo = !logo;
  eeprom.storevar(EEVAR_AYB, (byte) logo);
  if (logo) Serial.print(F("OFF")); else Serial.print(F("ON")); 
}


// Write uid in buf to eeprom
void SerialUi::set_eeuid()
{
  struct eepw pw;

  eeprom.getpw(curslot, &pw);
  for (int i=2; i < st_ptr+2; i++)
  {
    pw.uid[i-2]=st_buf[i];
  }
  pw.uidlen = st_ptr-2; 
  eeprom.storepw(curslot, &pw);
}

// Write pw in buf to eeprom
void SerialUi::set_eepw()
{
  struct eepw pw;

  eeprom.getpw(curslot, &pw);
  for (int i=2; i < st_ptr; i++)
  {
    pw.pwd[i-2]=st_buf[i];
  }
  pw.pwdlen = st_ptr-2;            
  eeprom.storepw(curslot, &pw);
}

// Write slot name in buf to eeprom
void SerialUi::set_eename()
{
  st_buf[st_ptr]=0;
  eeprom.storename(curslot, st_buf+2);
}

// Set pw generator mode
void SerialUi::set_pwgmode(char m)
{
  switch (toupper(m))
  {
    case 'A' : // Alpha
      pwgenmode = PWM_ALPHA;
      Serial.println("Alpha");
      break;
    case 'N' : // Numeric
      pwgenmode = PWM_NUM;
      Serial.println("Num");
      break;
    case 'L' : // Alphanumeric
      pwgenmode = PWM_ANUM;
      Serial.println("Anum");
      break;
    case 'S' : // Special
      pwgenmode = PWM_SPEC;
      Serial.println("Special");
      break;
  }
}


// Duplicate current pw slot to destination in s
void SerialUi::dup_slot(char s)
{
  s -= '0';
  if ( (s >= 0) && (s < MAXSLOTS) && (s != curslot) )
  {
    eeprom.dupslot(curslot, (int) s);
  }
}

// Convert unterminated string in buf to a positive integer within limits, returns -1 for invalid
int SerialUi::buf_to_int(int start, int min, int max)
{
  st_buf[st_ptr]=0;
  int d = atoi(st_buf+start);
  if ( (d >= min) && (d <= max) )
    return d;
  else
    return -1;
}

// Set pw generator length from string in buf
void SerialUi::set_pwglen()
{
  int d = buf_to_int(2, MINPW, EE_PWLEN-3);
  if ( d > 0 )
  {            
    pwgenlen = d;
  }
  Serial.println(pwgenlen);
}

// Set display timeout from string in buf
void SerialUi::set_dispto()
{
  int d = buf_to_int(2, 0, MAXPTO*10);
  if ( d >= 0 )
  {            
    byte priv = (byte) (d/10);
    eeprom.storevar(EEVAR_OPRIV, priv);
    disp.setprivacy(priv);
  }
}

void SerialUi::show_dispto()
{
  byte priv = eeprom.getvar(EEVAR_OPRIV);
  Serial.print((int) priv * 10 );
}

// Set led timeout from string in buf
void SerialUi::set_ledto()
{
  int d = buf_to_int(2, 0, MAXLTO*10);
  if ( d >= 0 )
  {            
    byte priv = (byte) (d/10);
    eeprom.storevar(EEVAR_LPRIV, priv);
    led.settimeout(priv);
  }
}

void SerialUi::show_ledto()
{
  byte priv = eeprom.getvar(EEVAR_LPRIV);
  Serial.print((int) priv * 10 );

}

// Set lock timeout from string in buf
void SerialUi::set_lockto()
{
  int d = buf_to_int(2, 0, MAXLOCKTO*10);
  Serial.println(d );
  if ( d >= 0 )
  {            
    byte lck = (byte) (d/10);
    eeprom.storevar(EEVAR_LOCK, lck);
  }
}

void SerialUi::show_lockto()
{
  byte lck = eeprom.getvar(EEVAR_LOCK);
  Serial.print((int)lck * 10 );
}

// Set button mode
void SerialUi::set_butseq()
{
  char *s = st_buf+2;
  byte b=0;

  if (strcmp(strupr(s), "GNS") == 0) b=0;
  else if (strcmp(strupr(s), "GSN") == 0) b=1;
  else if (strcmp(strupr(s), "NGS") == 0) b=2;
  else if (strcmp(strupr(s), "NSG") == 0) b=3;
  else if (strcmp(strupr(s), "SGN") == 0) b=4;
  else if (strcmp(strupr(s), "SNG") == 0) b=5;

  eeprom.storevar(EEVAR_BUTSEQ, b);
}

void SerialUi::show_butseq()
{
  byte b=  eeprom.getvar(EEVAR_BUTSEQ);

  switch (b) 
  {
    case 0 : Serial.println("GNS"); break;
    case 1 : Serial.println("GSN"); break;
    case 2 : Serial.println("NGS"); break;
    case 3 : Serial.println("NSG"); break;
    case 4 : Serial.println("SGN"); break;
    case 5 : Serial.println("SNG"); break;
  }
}

// Set led sequence
void SerialUi::set_ledseq()
{
  char *s = st_buf+2;
  byte b=0;

  if (strcmp(strupr(s), "RYGMBC") == 0) b=0;
  else if (strcmp(strupr(s), "YRGMBC") == 0) b=1;
  else if (strcmp(strupr(s), "MRBYGC") == 0) b=2;
  else if (strcmp(strupr(s), "CBMGYR") == 0) b=3;
  else if (strcmp(strupr(s), "RYGBMC") == 0) b=4;
  else if (strcmp(strupr(s), "BGYRCM") == 0) b=5;

  eeprom.storevar(EEVAR_LEDSEQ, b);
}

void SerialUi::show_ledseq()
{
  byte b=  eeprom.getvar(EEVAR_LEDSEQ);

  switch (b) 
  {
    case 0 : Serial.println("RYGMBC"); break;
    case 1 : Serial.println("YRGMBC"); break;
    case 2 : Serial.println("MRBYGC"); break;
    case 3 : Serial.println("CBMGYR"); break;
    case 4 : Serial.println("RYGBMC"); break;
    case 5 : Serial.println("BGYRCM"); break;
  }
}

// Set bad sec entry sequence
void SerialUi::set_badseq()
{
  int d = buf_to_int(2, 0, 255);
  if ((d >= 0) && (d <= 255))
  {
    eeprom.storevar(EEVAR_TRIES, d);
  }
}

void SerialUi::show_badseq()
{
  byte d =  eeprom.getvar(EEVAR_TRIES);

  switch (d) 
  {
    case 0 : Serial.println("Off"); break;
    case 1 : Serial.println("3T"); break;
    case 2 : Serial.println("10T"); break;
    default : Serial.print(d) ; Serial.println("sD"); break;
  }
}


// Set security button sequence
void SerialUi::set_lockseq()
{
  bool ok=true;
  if (st_ptr < SSEQL+2)
    ok=false;
  for (int i=2; i < st_ptr; i++)
    if ( (st_buf[i] < '1') || (st_buf[i] > '3') )
      ok=false;
  if (ok)
  {
    int d = atoi(st_buf+2);
    int ss=NSSEQ+1;
    for (int i=1; i < NSSEQ+1; i++)
    {
      if (Secseq[i] == d)
      {
        ss=i-1;
        break;
      }
    }
    eeprom.storevar(EEVAR_SEC, (byte) ss);
  }
  else
  {
    if (st_buf[2] == '0')
    {
      eeprom.storevar(EEVAR_SEC, 0);
    }
  }
}

void SerialUi::show_lockseq()
{
  byte ss = eeprom.getvar(EEVAR_SEC) ;
  if (ss > 0) Serial.print(Secseq[ss+1]); 
}

