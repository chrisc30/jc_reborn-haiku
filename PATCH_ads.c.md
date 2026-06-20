# ads.c — required changes for Haiku port

## 1. Remove the SDL include

Remove:
```c
#include <SDL2/SDL.h>
```

## 2. Replace grUpdateDisplay() calls with the new three-call API

`ads.c` calls `grUpdateDisplay(&ttmBackgroundThread, ttmThreads, &ttmHolidayThread)`
in one or more places.  Replace each call with:

```c
grStartFrame();
for (int i = 0; i < MAX_TTM_THREADS; i++)
    if (ttmThreads[i].isRunning)
        grBlitLayer(ttmThreads[i].ttmLayer);
if (ttmHolidayThread.isRunning)
    grBlitLayer(ttmHolidayThread.ttmLayer);
grFlushDisplay();
```

If the background thread layer also needs blitting, add:
```c
if (ttmBackgroundThread.isRunning)
    grBlitLayer(ttmBackgroundThread.ttmLayer);
```
between grStartFrame() and the threads loop.

## 3. Add to the #include block (after existing includes)

```c
#include "graphics.h"   /* already there — grStartFrame/grBlitLayer/grFlushDisplay */
```
(No new include needed — graphics.h is already included.)
