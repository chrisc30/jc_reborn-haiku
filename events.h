/*
 *  events.h  —  Haiku port
 *
 *  SDL event loop replaced with a simple Haiku-compatible timing mechanism.
 *  The screensaver framework owns the window; we have no SDL_Window to poll.
 *  eventsWaitTick() now sleeps using snooze() (Haiku's usleep equivalent).
 */

#ifndef EVENTS_H
#define EVENTS_H

#include "mytypes.h"

extern int evHotKeysEnabled;   /* unused in screensaver mode, kept for compat */
extern int evQuitRequested;    /* set to 1 by jc_saver.cpp on StopSaver()    */

void eventsInit(void);
void eventsWaitTick(uint16 delay);

#endif /* EVENTS_H */
