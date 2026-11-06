/* Vsync-paced animation ticks. A worker thread waits for every DWM
 * composition (DwmFlush) and posts one message per frame to the UI
 * thread, so frames line up with the monitor (60/120/144 Hz) instead of
 * beating against the ~15.6 ms system timer. */
#ifndef FRAMECLOCK_H
#define FRAMECLOCK_H

#include "app.h"

int frameclock_start(HWND target, UINT msg);   /* 0 on success */
void frameclock_stop(void);
void frameclock_ack(void);   /* UI thread consumed a frame message */

#endif
