
/*
 * Systick related definitions
 */

#ifndef TICK_H
#define TICK_H

#define TICK_US        100 // Systick in microseconds
#define LOOP_MS        100 // Main loop time in milliseconds - adjust debounce time in Input when changing
#define LOOPS_PERSEC   (1000/LOOP_MS) // # of loops per second
#define TICKS_PERMS    (1000/TICK_US) // # of ticks per millisecond
#define TICKS_PERLOOP  (LOOP_MS*1000/TICK_US)
#define WD_TOS         10 // watchdog timeout in seconds
#define WD_TIMEOUT     (WD_TOS*1000000L/TICK_US) // Watchdog timeout in ticks

#endif