#ifndef UTILS_H
#define UTILS_H

#include <avr/sleep.h>

#include "hardware.h"

#ifndef MAINT
void scani2c();
#endif
void sleepcpu();
bool recvWithEndMarker(char endMarker, int *maxLen, char receivedChars[]);
byte hextobyte(char hex[]);

#endif