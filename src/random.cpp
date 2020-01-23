/*
 * Name: Random.cpp
 * 
 * Purpose: Provides the random string generation service
 * 
 * Provides:
 *   genPw - Generate a random password
 *   getEntropy - returns current entropy
 *   vTaskRandomGen - Periodic task for harvesting entropy
 * 
 * Private: 
 *  getByte
 *  rotl
 *
 * Operation:
 *   Random generator task collects one random bit from a free running timer every time it runs.  
 *   Randomness is stored in an entropy buffer
 *   genPw pulls bytes from the entropy buffer as a string
 * 
 */

#include "random.h"

Random::Random() 
{
}

Random::~Random() { }

// Rotate bits to the left
byte Random::rotl(const byte value, int shift) 
{
  if ((shift &= sizeof(value)*8 - 1) == 0)
    return value;
  return (value << shift) | (value >> (sizeof(value)*8 - shift));
}

// Grab a single byte from the entropy pool.  
byte Random::getbyte()
{
  if( !q.isEmpty() )
  {
    return q.dequeue();
  }
  return (byte) 0;
}

// Get bytes from the entropy pool and create a password according to the selected rules
bool Random::genPw(char *buf, byte len, byte mode)
{
  byte range, irange, startc, eneeded;
  int numchars=0, numbytes=0, i;
  
  switch (mode)
  {
    case PWM_NUM :
      eneeded = len*2; // Minimum entropy needed - conservative estimate
      irange = 250;    // Range out of wich we pick our random byte - must be a multiple of range and < 255
      range=10;        // Actual range of random values
      startc='0';      // Starting value - there may be gaps that need further processing
      break;
    case PWM_ALPHA :
      eneeded = len*2;
      irange = 208;
      range=26*2;
      startc='A';
      break;
    case PWM_ANUM :
      eneeded = len*2;
      irange = 248;
      range=26*2+10;
      startc='0';
      break;
    case PWM_SPEC :
    default :
      eneeded = len*3;
      irange = 180;
      range=90;
      startc=35;
      break;
  }

  // Make sure we have comfortably enough entropy to proceed
  int e = getEntropy();
  if (e < eneeded)
    return false;

  while (numchars <  len)
  {
    if (numbytes > e)
      return false;  // Entropy exhausted
    byte b = getbyte();  // Get a byte from the entropy queue
    numbytes++;

    if (b < irange) // Check if byte in needed range
    {
       b = b % range; // Get byte in actual range after removing modulo bias
       buf[numchars++] = b + startc;
    }

    // else get another byte
  }

  // Fix up charset for Alpha aand Alnum modes  
  if (mode == PWM_ANUM)
  {
    for (i=0; i < numchars; i++)
    {
      if (buf[i] > '9')   // Skip the characters between '9' and 'A'
        buf[i] += 7;
    }
  }

  if ( (mode == PWM_ANUM) || (mode == PWM_ALPHA) )
  {
    for (i=0; i < numchars; i++)
    {
      if (buf[i] > 'Z')   // Skip the characters between 'Z' and 'a'
        buf[i] += 6;
    }
  }
  return true;
}

// Get current entropy
int Random::getEntropy()
{
  return q.size();
}

// Generate one bit of randomness every  systick.  startc a byte to the entropy pool once we have 8 bits
void Random::vTaskRandomGen()  
{
  byte randsample=0;
  static byte current_byte=0;
  static byte current_bit=0;

   // Sample timer, ignore higher bits
  randsample = TCNT1L; 

  // Spread randomness around, XOR preserves randomness 
  current_byte = rotl(current_byte, 1); 
  current_byte ^= randsample; 

  current_bit++;

  if (current_bit > 7)
  {
    // Have 8 bits - startc to random Queue
    current_bit = 0;
    q.enqueue(current_byte);
  }
}

