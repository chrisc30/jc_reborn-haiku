#ifndef GRAPHICS_H
#define GRAPHICS_H
#include "mytypes.h"
#include "graphics_haiku.h"
#include "ttm_types.h"
struct TPalResource;
extern GrSurface *grBackgroundSfc;
extern GrSurface *grOutputSfc;
extern int grDx;
extern int grDy;
extern int grWindowed;
extern int grUpdateDelay;
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480
void graphicsInit(void);
void graphicsEnd(void);
void grRefreshDisplay(void);
void grToggleFullScreen(void);
void grLoadPalette(struct TPalResource *palResource);
void grStartFrame(void);
void grBlitLayer(GrSurface *layer);
void grFlushDisplay(void);
GrSurface *grNewLayer(void);
void grFreeLayer(GrSurface *sfc);
void grSetClipZone(GrSurface *sfc, sint16 x1, sint16 y1, sint16 x2, sint16 y2);
void grCopyZoneToBg(GrSurface *sfc, uint16 x, uint16 y, uint16 width, uint16 height);
void grSaveImage1(GrSurface *sfc, uint16 arg0, uint16 arg1, uint16 arg2, uint16 arg3);
void grSaveZone(GrSurface *sfc, uint16 x, uint16 y, uint16 width, uint16 height);
void grRestoreZone(GrSurface *sfc, uint16 x, uint16 y, uint16 width, uint16 height);
void grDrawPixel(GrSurface *sfc, sint16 x, sint16 y, uint8 color);
void grDrawLine(GrSurface *sfc, sint16 x1, sint16 y1, sint16 x2, sint16 y2, uint8 color);
void grDrawRect(GrSurface *sfc, sint16 x, sint16 y, uint16 width, uint16 height, uint8 color);
void grDrawCircle(GrSurface *sfc, sint16 x1, sint16 y1, uint16 width, uint16 height, uint8 fgColor, uint8 bgColor);
void grDrawSprite(GrSurface *sfc, struct TTtmSlot *ttmSlot, sint16 x, sint16 y, uint16 spriteNo, uint16 imageNo);
void grDrawSpriteFlip(GrSurface *sfc, struct TTtmSlot *ttmSlot, sint16 x, sint16 y, uint16 spriteNo, uint16 imageNo);
void grClearScreen(GrSurface *sfc);
void grLoadScreen(char *strArg);
void grInitEmptyBackground(void);
void grReleaseBmp(struct TTtmSlot *ttmSlot, uint16 bmpSlotNo);
void grLoadBmp(struct TTtmSlot *ttmSlot, uint16 slotNo, char *strArg);
void grFadeOut(void);
#endif
