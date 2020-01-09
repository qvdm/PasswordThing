#ifndef UTILS_H
#define UTILS_H

#include <avr/sleep.h>

#include "hardware.h"

void scani2c();
void sleepcpu();
bool recvWithEndMarker(char endMarker, int *maxLen, char receivedChars[]);

#endif