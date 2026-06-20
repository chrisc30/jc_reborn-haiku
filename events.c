/*
 *  events.c  —  Haiku port
 *
 *  The original events.c drove the main loop via SDL_PollEvent and SDL_Delay.
 *  In the Haiku screensaver model there is no window to poll — the BScreenSaver
 *  framework calls Draw() at the configured tick interval.
 *
 *  eventsWaitTick() still exists so the game logic (ttm.c, story.c, etc.)
 *  compiles unchanged, but it now just yields for the requested time using
 *  snooze() (Haiku's fine-grained sleep, resolution ~1 ms).
 *
 *  The original delay unit is 1/50 s (20 ms per tick unit), matching the
 *  original Windows screensaver's WM_TIMER at 20 ms.
 */

#include <OS.h>          /* snooze()                         */
#include <stdio.h>
#include <stdlib.h>

#include "mytypes.h"
#include "events.h"
#include "graphics.h"   /* grOutputSfc (for future use)     */


int evHotKeysEnabled = 0;
int evQuitRequested  = 0;   /* set to 1 by jc_saver.cpp StopSaver() */

static bigtime_t lastTick = 0;


void eventsInit()
{
    lastTick = system_time();   /* Haiku: microseconds since boot */
}


/*
 * delay is in units of 20 ms (same as original).
 * We sleep until the wall-clock target time to avoid drift.
 * If evQuitRequested is set, return immediately so the story
 * loop can check it and break out cleanly.
 */
void eventsWaitTick(uint16 delay)
{
    if (evQuitRequested)
        return;

    bigtime_t target = lastTick + (bigtime_t)delay * 20000LL; /* µs */
    bigtime_t now    = system_time();

    if (target > now)
        snooze(target - now);

    lastTick = system_time();
}
