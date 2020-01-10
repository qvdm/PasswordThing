/*
 * Name: Led.cpp
 * 
 * Purpose: Drives the onboard and colour LEDs
 * 
 * Provides:
 *     vTaskManageLeds - Periodic Led management task - turns them on an off for blinking
 *     ledcolor - set color LED color as code or rgb and set blink rate
 *     ob_blink - set onboard LED blink rate
 *     cl_blink - set color LED blink rate
 *     push - save current led color
 *     pop - restore pushed color
 *     settimeout - set LED timeout
 *     unblank - unblank LED
 *     isblank - is LED blank due to timeout?
 *
 * Private:
 *     cledout - actual output to color led
 *
 * Operation:
 *    Task counts cycles to toggle LEDs on an off
 * 
 */

#include "led.h"

Led::Led() 
{
  // Set onboard and colour led pins to OUTPUT
  pinMode(OLR_PIN, OUTPUT);
  pinMode(OLG_PIN, OUTPUT);
  pinMode(OLB_PIN, OUTPUT);
  pinMode(OLO_PIN, OUTPUT);

  // Turn off onboard led and set to nonblinking
  digitalWrite(OLO_PIN, LOW);
  ob_blink(BLNK_SLOW);

  // Set colour led to white and set to nonblinking
  ledcolor(COL_GRN, BLNK_ON);
}

Led::~Led()  { }

// Actual output to RGB LED.  
void Led::cledout(byte r, byte g, byte b)
{
  digitalWrite(OLR_PIN, r);
  digitalWrite(OLG_PIN, g);
  digitalWrite(OLB_PIN, b);  
}


// Set led color from color code table
void Led::ledcolor(byte colorcode, byte mode)
{
   byte r=BRI_OFF, g=BRI_OFF, b=BRI_OFF;
  
  if (colorcode & RMASK) r=BRI_ON;
  if (colorcode & GMASK) g=BRI_ON;
  if (colorcode & BMASK) b=BRI_ON;

  cl_blink(mode);
  lr=r; lg=g; lb=b; lk=mode;
  unblank();
}

// Set led color using rgb values
void Led::ledcolor(byte r, byte g, byte b, byte mode)
{
   cl_blink(mode);
  lr=r; lg=g; lb=b; lk=mode;
  unblank();

}


// Save current LED state
void Led::push()
{
  slr=lr; slg=lg; slb=lb; slk=lk;
}

// Restore saved LED state
void Led::pop()
{
  ledcolor(slr, slg, slb, slk);
}

// Toggle enable state of onboard loop blinky
void Led::ob_enable(byte state)
{
  obenabled = false;
  ob_blink(BLNK_OFF);
  if (state != CBLNK_OFF) 
  {
    obenabled = true;
    if (state == CBLNK_BLNK)
      ob_blink(BLNK_MED);
    else
      ob_blink(BLNK_ON);
  }
}


// Set onboard led blink rate - switching managed by task
void Led::ob_blink(byte rate)
{
  obmode=rate;
  if (obmode == BLNK_FAST)
    maxobcycle=BT_FAST;
  else if (obmode == BLNK_MED)
    maxobcycle=BT_MED;
  else if (obmode == BLNK_SLOW)
    maxobcycle=BT_SLOW;
  else if (obmode == BLNK_MOST)
    maxobcycle=BT_MOST;
  else if (obmode == BLNK_LEAST)
    maxobcycle=BT_LEAST;

}

// Set colour led blink rate - switching managed by task
void Led::cl_blink(byte rate)
{
  clmode=rate;
  if (clmode == BLNK_FAST)
    maxclcycle=BT_FAST;
  else if (clmode == BLNK_MED)
    maxclcycle=BT_MED;
  else if (clmode == BLNK_SLOW)
    maxclcycle=BT_SLOW;
  else if (clmode == BLNK_MOST)
    maxclcycle=BT_MOST;
  else if (clmode == BLNK_LEAST)
    maxclcycle=BT_LEAST;
}


void Led::settimeout(byte timeout)
{
  if (timeout < (byte) MAXLTO)
    led_timeout = timeout * 10;
  else
    led_timeout = MAXLTO * 10;
  blanktime = (unsigned long) led_timeout * 1000L / LOOP_MS;
}

void Led::unblank()
{
  blanktime = (unsigned long) led_timeout * 1000L / LOOP_MS;
}

bool Led::isblank()
{
  return ((led_timeout > 0) && (clmode == BLNK_ON) && (blanktime == 0) );
}

// LED task - blinks the LEDs as needed based on blink rate set above
void Led::vTaskManageLeds( )  
{
  // cycle is a rough proxy for time elapsed - may stutter during eeprom write and so on
  if (++obcycle > maxobcycle) {obcycle = 0; }
  if (++clcycle > maxclcycle) {clcycle = 0; }

  // Onboard LED
  switch (obmode)
  {
    case BLNK_OFF :
      obstate = false;
      break;
    case BLNK_SLOW :
    case BLNK_MED :
    case BLNK_FAST :
      if (obcycle < maxobcycle/2) obstate = true; else obstate = false; 
      break;
    case BLNK_LEAST :
      if (obcycle < BT_LEAST_ON) obstate = true; else obstate = false; 
      break;
    case BLNK_MOST :
      if (obcycle < BT_MOST_ON) obstate = true; else obstate = false; 
      break;
    case BLNK_ON :
      obstate = true;
      break;
  }
  // Write on/off state to pin every time
  if (obenabled)
    obstate ? digitalWrite(OLO_PIN, HIGH) : digitalWrite(OLO_PIN, LOW);
  else
    digitalWrite(OLO_PIN, LOW);

  // Colour LED
  switch (clmode)
  {
    case BLNK_OFF :
      clstate = false;
      break;
    case BLNK_SLOW :
    case BLNK_MED :
    case BLNK_FAST :
      if (clcycle < maxclcycle/2) clstate = true; else clstate = false; 
      break;
    case BLNK_LEAST :
      if (clcycle < BT_LEAST_ON) clstate = true; else clstate = false; 
      break;
    case BLNK_MOST :
      if (clcycle < BT_MOST_ON) clstate = true; else clstate = false; 
      break;
    case BLNK_ON :
      clstate = true;
      break;
  }

 // Blank led on timeout
  if ((led_timeout > 0) && (clmode == BLNK_ON) )
  {
    if (blanktime > 0) 
      blanktime--;
    else
      clstate = false;
  }


  // Write on/off state with current colour values with cledout - which only writes to pin on change 
    clstate ? cledout(lr, lg, lb) : cledout(BRI_OFF, BRI_OFF, BRI_OFF);
}
