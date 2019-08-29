/*
 * Name: SerialUi.cpp
 * 
 * Purpose: Provides a UI via the serial port for entering UIDS and Passwords, or generating PWDs
 * 
 * Provides:
 *    toggle_sio_menu(); - Turn the Serial menu on or off
 *    vTaskSerialUi(); - Periodic task for processing serial input
 * 
 * Private: 
 *     help - display help
 *     kbd_on - return to keyboard mode
 *     kbd_off - turn off keyboard mode
 *     printcurpw - print pwd
 *
 * Operation:
 */

#include "serialui.h"
#include "menu.h"

extern unsigned long getTime(void);
extern int Secseq[];


// CTOR
SerialUi::SerialUi(Led& rl, Display &rd, Random& rr, Eeprom& ee) : led(rl), disp(rd), rand(rr), eeprom(ee)
{
  pwgenlen = MAXPW;
  pwgenmode = PWM_SPEC;
}

SerialUi::~SerialUi() { }

// Post CTOR initialization
void SerialUi::init(int sseq)
{
  while (Serial.available() > 0) 
    Serial.read();

  waitforseq=sseq;
}

// Turn SIO menu on
void SerialUi::sio_menu_on()
{
  menurunning = true;
  Keyboard.end(); // Turn off kbd
  Serial.begin(115200);  while (!Serial); 
  SUICRLF;

  if (waitforseq == 0)
  {
    SUIPROMPT;
  }
  else
    disp.displaylarge((char *) "LOCKED"); 
}

// Turn SIO menu off
void SerialUi::sio_menu_off()
{
  menurunning = false;
  Serial.end();
  Keyboard.begin(); // Turn on kbd
}


// SIO running?
bool SerialUi::running()
{
  return menurunning;
}


// Print pwd and/or UID
void SerialUi::printcurpw()
{
  struct pwvalid ev = eeprom.entryvalid(curslot);
  struct eepw pw;

  eeprom.getpw(curslot, &pw);
  Serial.print(F("UID: "));
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
  
  Serial.print(F("PWD: "));
  if (ev.pwdvalid)
  {
    Serial.write('\"');
    for (int i=0; i < pw.pwdlen; i++)
      Serial.write(pw.pwd[i]);
    Serial.write('\"');
  }
  else
    Serial.print(F("None "));
  Serial.flush();
  SUICRLF;

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
      printcurpw();
    }
    else
    {
      SUICRLF;
      Serial.print(F("Error generating password - maybe not enough entropy - try again later"));
      SUICRLF;
    }
    
  }
  else
  {
    SUICRLF;
    Serial.print(F("Not enough entropy yet - try again later"));
    SUICRLF;
  }
}


// Main task
void SerialUi::vTaskSerialUi()
{

  // flush the input if not running
  if (!menurunning) 
  {
    if (Serial.available() > 0) 
      Serial.read();
    return;
  }

  // PW locked?  (not currently implemented)
  if (waitforseq > 0)
  {
    st_mode = SM_WAIT_DATA;
    waitfor = WD_SECP;
    led.ledcolor(COL_YEL, BLNK_MOST);
    if (!displck)
    {
      disp.displaylarge((char *) "LOCKED"); 
      SUICRLF;
      Serial.print(F("Password: "));  Serial.flush();
      displck=true;
    }
  }
  else
  {
    led.ledcolor(COL_WHT, BLNK_MOST);
  }
  

  // Read input
  if (Serial.available() > 0) 
  {
    st_inchar = Serial.read();
    Serial.write(st_inchar); Serial.flush();

    handle_input();
  }
}

// Process a single menu input char
void SerialUi::handle_input()
{

  switch (st_mode) 
  {
    case SM_WAIT_CMD : // Waiting for a single-key command
      handle_cmd();
      break;
    case SM_WAIT_DATA : // Waiting for user input / cmd argument
      handle_data();
      break;
  }
}

// Process a command char
void SerialUi::handle_cmd()
{
  int e;

  switch (st_inchar)
  {
    case '?' : // help
    case 'h' : 
      help();
      SUICRLF;
      SUIPROMPT;
      break;

    case 's' : // slot
      st_mode = SM_WAIT_DATA;
      waitfor = WD_SLOT;
      Serial.print(F("\nSlot [0-")); Serial.print(MAXSLOTS-1); Serial.print(F("]: ")); Serial.flush();
      break;

    case '0' : // Select slot directly with numeric keys
    case '1' :
    case '2' :
    case '3' :
    case '4' :
    case '5' :
      curslot = st_inchar-'0';
      SUICRLF;
      SUIPROMPT;
      break;

    case 'n' : // next slot
      curslot++; if (curslot >= MAXSLOTS) curslot=0;
      SUICRLF;
      SUIPROMPT;
      break;

    case 'r' : // prev slot
      curslot--; if (curslot >=  MAXSLOTS) curslot=MAXSLOTS-1;
      SUICRLF;
      SUIPROMPT;
      break;

    case 'p' :
      SUICRLF;
      printcurpw();
      SUICRLF;
      SUIPROMPT;
      break;

    case 'g' : // generate password 
      genpw();            
      SUICRLF;
      SUIPROMPT;
      break;

    case 'u' : // enter uid
      st_mode = SM_WAIT_DATA;
      waitfor = WD_UID;
      st_ptr=0;
      SUICRLF;
      Serial.print(F("Userid for Slot ")); Serial.print(curslot); Serial.print(F(": ")); Serial.flush();
      break;

    case 'o' : // enter pwd
      st_mode = SM_WAIT_DATA;
      waitfor = WD_PWD;
      st_ptr=0;
      SUICRLF;
      Serial.print(F("Password for Slot ")); Serial.print(curslot); Serial.print(F(": ")); Serial.flush();
      break;

    case 'm' : // generator mode
      st_mode = SM_WAIT_DATA;
      waitfor = WD_MODE;
      st_ptr=0;
      Serial.print(F("\nMode (A)lpha (N)umeric a(L)phanumeric (S)pecial : ")); Serial.flush();
      break;

    case 'l' : // generator length
      st_mode = SM_WAIT_DATA;
      waitfor = WD_PLEN;
      st_ptr=0;
      SUICRLF;
      Serial.print(F("\nPassword length [4-")); Serial.print(EE_PWLEN-3); Serial.print(F("] : ")); Serial.flush();
      break;

    case 'a' : // Duplicate slot
      st_mode = SM_WAIT_DATA;
      waitfor = WD_DUP;
      st_ptr=0;
      SUICRLF;
      Serial.print(F("\nDestination Slot [0-")); Serial.print(MAXSLOTS-1); Serial.print(F("]: ")); Serial.flush();
      break;

    case 'c' : // clear slot
      eeprom.clearslot(curslot);
      SUICRLF;
      SUIPROMPT;
      break;

    case 'b' : // Toggle loop blink
      toggle_blink();
      SUICRLF;
      SUIPROMPT;
      break;

    case 'i' : // Set privacy timeout
      st_mode = SM_WAIT_DATA;
      waitfor = WD_TLEN;
      st_ptr=0;
      SUICRLF;
      Serial.print(F("\nOLED Timeout (x 10s) [0-")); Serial.print(MAXPTO); Serial.print(F("] 0=None : ")); Serial.flush();
      break;

    case 'j' : // Set led timeout
      st_mode = SM_WAIT_DATA;
      waitfor = WD_LLEN;
      st_ptr=0;
      SUICRLF;
      Serial.print(F("\nCLED Timeout (x 10s) [0-")); Serial.print(MAXPTO); Serial.print(F("] 0=None : ")); Serial.flush();
      break;

    case 'f' : // Toggle display flip
      toggle_flip();
      SUICRLF;
      SUIPROMPT; 
      break;

    case 't' : // button config
      st_mode = SM_WAIT_DATA;
      waitfor = WD_BTTN;
      st_ptr=0;
      menu_buttonconfig();
      break;

    case 'k' : // led config
      st_mode = SM_WAIT_DATA;
      waitfor = WD_COL;
      st_ptr=0;
      menu_ledconfig();
      break;

    case 'w' : // security sequence
      st_mode = SM_WAIT_DATA;
      waitfor = WD_SECC;
      st_ptr=0;
      Serial.print(F("\nSecurity Sequence (exactly 4 buttons, each labelled 1,2 or 3, e.g. '1231', 0 for None ) : ")); Serial.flush();
      break;

    case 'v' : // show eeprom variables
      show_eevars();
      SUICRLF;
      SUIPROMPT;
      break;

    case 'd' : // dump eeprom
      SUICRLF;
      eeprom.dump();
      SUIPROMPT;
      break;

    case 'z' : // zero eeprom
      eeprom.zero();
      SUIPROMPT;
      break;

    case 'e' : // show entropy
      e = rand.getEntropy();
      SUICRLF;
      Serial.print(F("Entropy: ")); Serial.print(e); Serial.flush();
      SUICRLF;
      SUIPROMPT;
      break;

    case '\r' : // Ignore
      break;

    case '\n' :
      SUICRLF;
      SUIPROMPT;
      break;

    default:
      SUICRLF;
      Serial.print(F("Invalid command")); Serial.flush();
      SUICRLF;
      SUIPROMPT;
      break;

  }
}


// Display help
void SerialUi::help(void)
{
  // abcdefghijklmnopqrstuvwxyz
  // xxxxxxxxxxxxxxxxxxxxxxx  x
  SUICRLF;
  Serial.println(F("? or h - help")); Serial.flush();
  Serial.println(F("s - Select (S)lot")); Serial.flush();
  Serial.println(F("n - (N)ext slot")); Serial.flush();
  Serial.println(F("r - p(R)evious slot")); Serial.flush();
  Serial.println(F("p - (P)rint uid/pwd")); Serial.flush();
  Serial.println(F("u - Enter (U)id")); Serial.flush();
  Serial.println(F("o - Enter (O)wn pwd")); Serial.flush();
  Serial.println(F("m - Set generator (M)ode")); Serial.flush();
  Serial.println(F("l - Set generator (L)ength")); Serial.flush();
  Serial.println(F("g - (G)enerate pwd")); Serial.flush();
  Serial.println(F(" ")); Serial.flush();
  Serial.println(F("c - (C)lear Slot")); Serial.flush();
  Serial.println(F("a - duplic(A)te Slot")); Serial.flush();
  Serial.println(F(" ")); Serial.flush();
  Serial.println(F("b - Toggle (B)linking indicator")); Serial.flush();
  Serial.println(F("i - Set pr(I)vacy (display autoblank) timeout")); Serial.flush();
  Serial.println(F("j - Set colour led timeout")); Serial.flush();
  Serial.println(F("f - (F)lip display")); Serial.flush();
  Serial.println(F("t - Reconfigure bu(T)tons")); Serial.flush();
  Serial.println(F("k - Set LED colors")); Serial.flush();
  Serial.println(F("w - Set security sequence")); Serial.flush();
  Serial.println(F(" ")); Serial.flush();
  Serial.println(F("v - Show EEPROM (V)ariables")); Serial.flush();
  Serial.println(F("d - (D)ump EEPROM")); Serial.flush();
  Serial.println(F("z - (Z)ero EEPROM")); Serial.flush();
  Serial.println(F(" ")); Serial.flush();
  Serial.println(F("e - Show available (E)ntropy")); Serial.flush();
}

// Toggle LED blink state
void SerialUi::toggle_blink()
{
  bool blnk = (bool) eeprom.getvar(EEVAR_LBLINK);
  blnk = !blnk;
  eeprom.storevar(EEVAR_LBLINK, (byte) blnk);
  led.ob_enable(blnk);
  Serial.print(F("\nBlink ")); if (blnk) Serial.print(F("ON")); else Serial.print(F("OFF")); Serial.flush();
}

// Toggle display flip state
void SerialUi::toggle_flip()
{
      byte b = eeprom.getvar(EEVAR_DFLIP);
      bool flip = (bool) (b & 0x01);
      flip = !flip;
      disp.setflip(flip);
      b &= 0xfe;
      if (flip) b |= 0x01;
      eeprom.storevar(EEVAR_DFLIP, (byte) b);
      Serial.print(F("\nDisplay Flip ")); if (flip) Serial.print(F("ON")); else Serial.print(F("OFF")); Serial.flush();
}

// Display button config menu
void SerialUi::menu_buttonconfig()
{
  Serial.print(F("\nButton configuration")); Serial.flush();
  Serial.print(F("\n1- Generate Next Select")); Serial.flush();
  Serial.print(F("\n2- Generate Select Next")); Serial.flush();
  Serial.print(F("\n3- Next Generate Select")); Serial.flush();
  Serial.print(F("\n4- Next Select Generate")); Serial.flush();
  Serial.print(F("\n5- Select Generate Next")); Serial.flush();
  Serial.print(F("\n6- Select Next Generate ")); Serial.flush();
  Serial.print(F("\nChoice: ")); Serial.flush();
}

// Display led config menu
void SerialUi::menu_ledconfig()
{
  Serial.print(F("\nLED configuration:")); Serial.flush();
  Serial.print(F("\n1- R Y G Ma B Cy")); Serial.flush();
  Serial.print(F("\n2- Y R G Ma B Cy")); Serial.flush();
  Serial.print(F("\n3- Ma R B Y G Cy")); Serial.flush();
  Serial.print(F("\n4- Cy B Ma G Y R")); Serial.flush();
  Serial.print(F("\n5- R Y G B Ma Cy")); Serial.flush();
  Serial.print(F("\n6- B G Y R Cy Ma")); Serial.flush();
  Serial.print(F("\nChoice: ")); Serial.flush();
}

// Display all EEprom variables
void SerialUi::show_eevars()
{
  bool blnk = (bool) eeprom.getvar(EEVAR_LBLINK);
  Serial.print(F("\nBlink ")); if (blnk) Serial.print("ON"); else Serial.print("OFF"); Serial.flush();
  bool flip = (bool) (eeprom.getvar(EEVAR_DFLIP) & 0x01);
  Serial.print(F("\nDisplay Flip ")); if (flip) Serial.print(F("ON")); else Serial.print(F("OFF")); Serial.flush();
  byte ss = (eeprom.getvar(EEVAR_DFLIP) >> 1);
  Serial.print(F("\nSecurity Seq: ")); if (ss > 0) Serial.print(Secseq[ss+1]); else Serial.print(F("None")); Serial.flush();
  byte priv = (eeprom.getvar(EEVAR_PRIV) & 0x0F) * 10;
  Serial.print(F("\nDisplay Timeout ")); Serial.print(priv); Serial.print(F("s")); Serial.flush();
  priv = ( (eeprom.getvar(EEVAR_PRIV) & 0xF0) >> 4) * 10;
  Serial.print(F("\nLED Timeout ")); Serial.print(priv); Serial.print(F("s")); Serial.flush();
  byte b = eeprom.getvar(EEVAR_BUTTONS);
  Serial.print(F("\nButton assignment ")); 
  switch (b & 0x0F)
  {
    case 0 : // GNS
      Serial.print(F("\nGenerate Next Select")); 
      break;
    case 1 : // GSN
      Serial.print(F("\nGenerate Select Next "));
      break;
    case 2 : // NGS
      Serial.print(F("\nNext Generate Select"));
      break;
    case 3 : // NSG
      Serial.print(F("\nNext Select Generate"));
      break;
    case 4 : // SGN
      Serial.print(F("\nSelect Generate Next "));
      break;
    case 5 : // SNG
      Serial.print(F("\nSelect Next Generate "));
      break;
    default : 
      Serial.print(F("\nInvalid"));
      break;
  }
  Serial.flush();
  Serial.print(F("\nLed slot colors ")); 
  switch ( (b & 0xF0) >> 4)
  {
    case 0 : 
      Serial.print(F("\nR Y G Ma B Cy")); 
      break;
    case 1 : // GSN
      Serial.print(F("\nY R G Ma B Cy"));
      break;
    case 2 : // NGS
      Serial.print(F("\nMa R B Y G Cy"));
      break;
    case 3 : // NSG
      Serial.print(F("\nCy B Ma G B Y R"));
      break;
    case 4 : // SGN
      Serial.print(F("\nR Y G B Ma Cy"));
      break;
    case 5 : // SNG
      Serial.print(F("\nB G Y R Cy Ma"));
      break;
    default : 
      Serial.print(F("\nInvalid"));
      break;
  }
  Serial.flush();
}

// Process a data char
void SerialUi::handle_data()
{
  switch (waitfor)
  {
    case WD_SLOT : // Expect a single-key slot #
    {
      set_slot(st_inchar);
      st_mode = SM_WAIT_CMD;
      SUICRLF;
      SUIPROMPT;
    }
    break;

    case WD_UID : // Expect a UID
    {
      if (get_string(EE_PWLEN))
      {
        set_eeuid();
        printcurpw();
        st_mode = SM_WAIT_CMD;
        SUIPROMPT;
      }
    }
    break;

    case WD_PWD : // Expect a Pwd
    {
      if (get_string(EE_PWLEN))
      {
        set_eepw();
        printcurpw();
        st_mode = SM_WAIT_CMD;
        SUIPROMPT;
      }
    }
    break;

    case WD_MODE : // Expect a single-key mode #
    {
      set_pwgmode(st_inchar);
      st_mode = SM_WAIT_CMD;
      SUICRLF;
      SUIPROMPT;
    }
    break;

    case WD_PLEN : // Expect 1-2 digit length between 4 and EE_PWLEN-3
    {
      if (get_string(2))
      {
        set_pwglen();        
        st_mode = SM_WAIT_CMD;
        SUICRLF;
        SUIPROMPT;
      }
    }
    break;

    case WD_TLEN : // Expect 1-2 digit length between 0 and MAXPTO
    {
      if (get_string(2))
      {
        set_dispto();        
        st_mode = SM_WAIT_CMD;
        SUICRLF;
        SUIPROMPT;
      }
    }
    break;

    case WD_LLEN : // Expect 1-2 digit length between 0 and MAXLTO
    {
      if (get_string(2))
      {
        set_ledto();        
        st_mode = SM_WAIT_CMD;
        SUICRLF;
        SUIPROMPT;
      }
    }
    break;

    case WD_DUP : // Expect a single-key destination slot #
    {
      dup_slot(st_inchar);
      st_mode = SM_WAIT_CMD;
      SUICRLF;
      SUIPROMPT;
    }
    break;

    case WD_BTTN : // Expect a single key button mode
    {
      set_btnmode(st_inchar);
      st_mode = SM_WAIT_CMD;
      SUICRLF;
      SUIPROMPT;
    }
    break;

    case WD_COL : // Expect a single key LED color selection
    {
      set_colmode(st_inchar);
      st_mode = SM_WAIT_CMD;
      SUICRLF;
      SUIPROMPT;
    }
    break;

    case WD_SECC : // Expect exactly 4 characters from '1' to '3'
    {
      if (get_string(SSEQL))
      {
        set_secseq();
        st_mode = SM_WAIT_CMD;
        SUICRLF;
        SUIPROMPT;
      }
    }
    break;

    case WD_SECP : // Initial PWD: Expect exactly 4 characters from '1' to '3'
    {
      if (get_string(SSEQL))
      {
        get_initialpw();
      }
    }
    break;
  }
}

// Add inchar to buf and check if we have a string of length len yet.  Return true if len reached or CR/LF entered
bool SerialUi::get_string(int len)
{
  if ( (st_inchar != '\r') && (st_inchar != '\n') )
  {
    st_buf[st_ptr++]=st_inchar; 
  }

  if ( (st_ptr > len) || (st_inchar == '\r') || (st_inchar == '\n') )
    return true;

  return false; 
}

// Write uid in buf to eeprom
void SerialUi::set_eeuid()
{
  struct eepw pw;

  eeprom.getpw(curslot, &pw);
  for (int i=0; i < st_ptr; i++)
  {
    pw.uid[i]=st_buf[i];
  }
  pw.uidlen = st_ptr; 
  eeprom.storepw(curslot, &pw);
}

// Write pw in buf to eeprom
void SerialUi::set_eepw()
{
  struct eepw pw;

  eeprom.getpw(curslot, &pw);
  for (int i=0; i < st_ptr; i++)
  {
    pw.pwd[i]=st_buf[i];
  }
  pw.pwdlen = st_ptr;            
  eeprom.storepw(curslot, &pw);
}

// Set pw generator mode
void SerialUi::set_pwgmode(char m)
{
  switch (m)
  {
    case 'a' : // Alpha
      pwgenmode = PWM_ALPHA;
      break;
    case 'n' : // Numeric
      pwgenmode = PWM_NUM;
      break;
    case 'l' : // Alphanumeric
      pwgenmode = PWM_ANUM;
      break;
    case 's' : // Special
      pwgenmode = PWM_SPEC;
      break;
    default : 
      SUICRLF;
      Serial.print(F("Invalid mode"));
      break;
  }
}

// Set current pw slot
void SerialUi::set_slot(char s)
{
  s -= '0';
  if ( (s >= 0) && (s < MAXSLOTS) )
  {
    curslot = s;
  }
}

// Duplicate current pw slot to destination in s
void SerialUi::dup_slot(char s)
{
  s -= '0';
  if ( (s >= 0) && (s < MAXSLOTS) && (s != curslot) )
  {
    Serial.print(F("\nCopying slot ")); Serial.print(curslot); Serial.print(F(" to ")); Serial.print((int) s);
    SUICRLF;
    eeprom.dupslot(curslot, (int) s);
  }
  else
  {
    SUICRLF;
    Serial.print(F("Invalid slot"));
  }
}

// Convert unterminated string in buf to a positive integer within limits, returns -1 for invalid
int SerialUi::buf_to_int(int min, int max)
{
  st_buf[st_ptr]=0;
  int d = atoi(st_buf);
  if ( (d >= min) && (d <= max) )
    return d;
  else
    return -1;
}

// Set pw generator length from string in buf
void SerialUi::set_pwglen()
{
  int d = buf_to_int(4, EE_PWLEN-3);
  if ( d > 0 )
  {            
    pwgenlen = d;
  }
  else
  {
    SUICRLF;
    Serial.print(F("Invalid length"));
  }
}

// Set display timeout from string in buf
void SerialUi::set_dispto()
{
  int d = buf_to_int(0, MAXPTO);
  if ( d >= 0 )
  {            
    byte priv = (byte) d;
    byte v = eeprom.getvar(EEVAR_PRIV); 
    v &= 0xF0; priv &= 0x0F;
    v |= priv;
    eeprom.storevar(EEVAR_PRIV, (byte) v);
    disp.setprivacy(priv);
  }
  else
  {
    SUICRLF;
    Serial.print(F("Invalid timeout"));
  }
}


// Set led timeout from string in buf
void SerialUi::set_ledto()
{
  int d = buf_to_int(0, MAXLTO);
  if ( d >= 0 )
  {            
    byte priv = (byte) d;
    byte v = eeprom.getvar(EEVAR_PRIV); 
    v &= 0x0F; priv &= 0x0F; priv <<= 4;
    v |= priv;
    eeprom.storevar(EEVAR_PRIV, (byte) v);
    led.settimeout(priv);
  }
  else
  {
    SUICRLF;
    Serial.print(F("Invalid timeout"));
  }
}

// Set button mode
void SerialUi::set_btnmode(char m)
{
  byte v = eeprom.getvar(EEVAR_BUTTONS); // button selection in lower nibble
  byte b;

  switch (m)
  {
    case '1' : // GNS
      Serial.print(F("\nGNS Selected")); 
      b = 0;
      break;
    case '2' : // GSN
      Serial.print(F("\nGSN Selected"));
      b = 1;
      break;
    case '3' : // NGS
      Serial.print(F("\nNGS Selected"));
      b = 2;
      break;
    case '4' : // NSG
      Serial.print(F("\nNSG Selected"));
      b = 3;
      break;
    case '5' : // SGN
      Serial.print(F("\nSGN Selected"));
      b = 4;
      break;
    case '6' : // SNG
      Serial.print(F("\nSNG Selected"));
      b = 5;
      break;
    default : 
      Serial.print(F("\nInvalid configuration"));
      b=0X0F;
      break;
  }
  v &= 0xF0;
  v |= b;
  eeprom.storevar(EEVAR_BUTTONS, v);
}

// Set led colormode
void SerialUi::set_colmode(char m)
{
  byte v = eeprom.getvar(EEVAR_BUTTONS); // led color in upper nibble
  byte b; 
  switch (st_inchar)
  {
    case '1' : 
    case '2' : 
    case '3' : 
    case '4' : 
    case '5' : 
    case '6' : 
      b = st_inchar - '1';
      break;
    default : 
      Serial.print(F("\nInvalid configuration"));
      b=0x0F;
      break;
  }
  v &= 0x0F;
  v |= (b<< 4);
  eeprom.storevar(EEVAR_BUTTONS, v);
}


// Set led security button sequence
void SerialUi::set_secseq()
{
  int i, d, ss;
  byte v, s;

  st_buf[st_ptr]=0;
  bool ok=true;
  if (st_ptr < SSEQL)
  {
    ok=false;
  }
  for (i=0; i < st_ptr; i++)
    if ( (st_buf[i] < '1') || (st_buf[i] > '3') )
      ok=false;
  if (ok)
  {
    d = atoi(st_buf);
    ss=NSSEQ+1;
    for (i=1; i < NSSEQ+1; i++)
    {
      if (Secseq[i] == d)
      {
        ss=i-1;
        break;
      }
    }
    v = eeprom.getvar(EEVAR_DFLIP); 
    v &= 0x01; s = (byte) ss <<  1; 
    v |= s;
    eeprom.storevar(EEVAR_DFLIP, (byte) v);
  }
  else
  {
    if (st_buf[0] == '0')
    {
      v = eeprom.getvar(EEVAR_DFLIP); 
      v &= 0x01;
      eeprom.storevar(EEVAR_DFLIP, (byte) v);
    }
    else
    {
      SUICRLF;
      Serial.print(F("Invalid sequence"));
    }
  }
}

// Get 'serial login' pw (same as sec seq)
void SerialUi::get_initialpw()
{
  int d, ss, i;

  st_buf[st_ptr]=0;
  bool ok=true;
  if (st_ptr < SSEQL)
  {
    ok=false;
  }
  for (i=0; i < st_ptr; i++)
    if ( (st_buf[i] < '1') || (st_buf[i] > '3') )
      ok=false;
  if (ok)
  {
    d = atoi(st_buf);
    ss=NSSEQ+1;
    for (i=1; i < NSSEQ+1; i++)
    {
      if (Secseq[i] == d)
      {
        ss=i-1;
        break;
      }
    }
    if (ss == waitforseq)
    {
      waitforseq=0;
      disp.displaylarge((char *) "SERIAL"); 
      st_mode = SM_WAIT_CMD;
      SUICRLF;
      SUIPROMPT;
    }
  }
  else
  {
    Serial.print(F("Invalid"));
    st_ptr=0;
    displck=false;
  }
}