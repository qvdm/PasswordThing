#ifndef RANDOM_H
#define RANDOM_H

#include <Arduino.h>
#include "hardware.h"

#include "queue.h"

#define MAXENTROPY 256

// Generator modes
#define PWM_ANUM  0 // Alphanumeric
#define PWM_ALPHA 1 // Alpha only
#define PWM_NUM   2 // Numeric only
#define PWM_SPEC  3 // Special chars

class Random {
  public:
    Random();
    ~Random();
    void vTaskRandomGen();
    bool genPw(char *buf, byte len, byte mode);
    int getEntropy(void);


  private:
    byte getbyte(void);
    byte rotl(const byte value, int shift);

    Queue q;
};


#endif 
