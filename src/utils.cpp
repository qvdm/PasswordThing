#include "Arduino.h"
#include <Wire.h>

#include "utils.h"

// Read chars from serial until end char found
bool recvWithEndMarker(char endMarker, int *maxLen, char receivedChars[]) 
{
  static byte ndx = 0;
 
  while (Serial.available() > 0 ) 
  {
    char rc = Serial.read();

    if (rc != endMarker) 
    {
      receivedChars[ndx++] = rc;
      if (ndx > *maxLen) 
      {
        return false;
      }
    }
    else 
    {
      receivedChars[ndx] = '\0'; // terminate the string
      *maxLen = ndx-1;
      return true;
    }
  }
  return false;
}

byte hextobyte(char hex[])
{
  byte v=0;
  for (int i=0; i < 2 ; i++)
  {
    if (hex[i] >= '0' && hex[i] <= '9')
      v = (v << 4) + (hex[i] - '0');
    else if (hex[i] >= 'A' && hex[i] <= 'F')
      v = (v << 4) + (hex[i] - 'A' + 10);
  }
  return(v);
}

