#include "Arduino.h"
#include <Wire.h>

#include "utils.h"

#ifndef MAINT

// Scans the i2c bus for devices and dumps to the serial terminal
// Not multitasking safe - call before creating tasks
void scani2c()
{
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");
 
  nDevices = 0;
  for(address = 1; address < 127; address++ )
  {
    if (address<16)
      Serial.print("0");
    Serial.print(address,HEX);
    if ((address % 32) == 0)
      Serial.println(" ");
    else
      Serial.print(" ");
    Serial.flush();

    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
      Serial.flush();
 
      nDevices++;
    }
    else if (error==4)
    {
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
      Serial.flush();
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done scanning\n");
  Serial.flush();

}

#endif

// Read chars from serial until end char found
bool recvWithEndMarker(char endMarker, int *maxLen, char receivedChars[]) 
{
  static byte ndx = 0;
  char rc;
  
  while (Serial.available() > 0 ) 
  {
    rc = Serial.read();

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

