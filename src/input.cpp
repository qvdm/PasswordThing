/*
 * Name: Input.cpp
 * 
 * Purpose: Reads the buttons
 * 
 * Provides:
 *   anyPressed - Checks if any key is being pressed
 *   vTaskDigitalRead - Task function - to be called periodically to read and debounce inputs
 * 
 * Private: 
 *   sendevent - Dispatches key press and release events to handlers
 *
 * Operation:
 *   Task polls the buttons every tick and, after debouncing, sends changed state as event to handler functions
 * 
 */

#include "input.h"

#ifndef MAINT

extern unsigned long getTime(void);
extern unsigned long lastkeypress;


Input::Input(Menu &rm) : menu(rm)
{
  for (int i=0; i < NBUTTONS; ++i)
  {
    pinMode(bport[i], INPUT_PULLUP);
    bstate[i] = NOTPRESSED;
  }
}

Input::~Input() { }

// Is any key pressed? Does not require Task to be running, but is not debounced
bool Input::anyPressed()
{
  bool rv=false;
  for (int i=0; i < NBUTTONS; ++i)
  {
    // read the button state
    byte bst = digitalRead(bport[i]);
    if (bst == LOW) rv = true;
  }
  return rv;
}


// Send a key press event to the handler function
void Input::sendevent(byte button, byte event)
{
  if (event == PRESSED)
  {
    downtime[button] = lastkeypress = getTime();
  }
  else if (event == PRESSINGLONG) // Pre-notify menu that button is being pressed a long time, in order to give user feedback
  {
    menu.pressinglong(button);
  }
  else if (event == PRESSINGVLONG) // Pre-notify menu that button is being pressed a very long time
  {
    menu.pressingverylong(button);
  }
  else // RELEASED
  {
    unsigned long d = getTime() - downtime[button];
    if (d > VLONGPRESS_T)
    {
      menu.verylongpress(button);
    }
    else if (d > LONGPRESS_T)
    {
      menu.longpress(button);
    }
    else
    {
      menu.shortpress(button);
    }
  }
}

// Read the button states and debounce them. Send events
void Input::vTaskDigitalRead() 
{
  for (int i=0; i < NBUTTONS; ++i)
  {
    // read the button state
    byte bst = digitalRead(bport[i]);
    // LOW state = pressed
    if ( (bst == LOW) && (bctr[i] < DEBOUNCE_C) ) 
    { 
      if (++bctr[i] >= DEBOUNCE_C) 
      {
        bstate[i] = PRESSED;
        sendevent(i, PRESSED);
      }
    }
    // HIGH state = not pressed
    else if ( (bst == HIGH) && (bctr[i] > 0) ) 
    { 
      if (--bctr[i] <= 0) 
      {
        bstate[i] = NOTPRESSED;
        sendevent(i, NOTPRESSED);
      }
    }
    // Send notification while pressing longer than LONGPRESS_T to provide user feedback
    if ( (bstate[i] == PRESSED) && (!lpnotify[i]) )
    {
      unsigned long d = getTime() - downtime[i];
      if (d > LONGPRESS_T)
      {
        sendevent(i, PRESSINGLONG);
        lpnotify[i] = true;
      }
    }
    else if (bstate[i] != PRESSED)
    {
      lpnotify[i] = false;
    }

    // Send notification while pressing longer than VLONGPRESS_T to provide user feedback
    if ( (bstate[i] == PRESSED) && (!vlpnotify[i]) )
    {
      unsigned long d = getTime() - downtime[i];
      if (d > VLONGPRESS_T)
      {
        sendevent(i, PRESSINGVLONG);
        vlpnotify[i] = true;
      }
    }
    else if (bstate[i] != PRESSED)
    {
      vlpnotify[i] = false;
    }
  }
}
#endif