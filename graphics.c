#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#include "mytypes.h"
#include "utils.h"
#include "graphics.h"
#include "resource.h"
#include "events.h"
#include "ttm.h"

static uint8 ttmPalette[16][4];
static GrSurface *grSavedZonesLayer = NULL;

GrSurface *grBackgroundSfc = NULL;
GrSurface *grOutputSfc     = NULL;

int grDx          = 0;
int grDy          = 0;
int grWindowed    = 0;
int grUpdateDelay = 0;

static void grReleaseScreen() { gr_free_surface(grBackgroundSfc); grBackgroundSfc = NULL; }
static void grReleaseSavedLayer() { gr_free_surface(grSavedZonesLayer); grSavedZonesLayer = NULL; }

static void grPutPixel(GrSurface *sfc, int x, int y, uint8 color)
{
    if (x < sfc->clip_x || y < sfc->clip_y ||
        x >= sfc->clip_x + sfc->clip_w ||
        y >= sfc->clip_y + sfc->clip_h) return;
    unsigned int *pixel = (unsigned int*)(sfc->pixels + y * sfc->pitch) + x;
    *pixel = gr_pack_bgra(ttmPalette[color][0], ttmPalette[color][1], ttmPalette[color][2], 0);
}

static void grDrawHorizontalLine(GrSurface *sfc, int x1, int x2, int y, uint8 color)
{
    if (y < 0 || y >= sfc->h) return;
    if (x1 < 0) x1 = 0;
    if (x2 >= sfc->w) x2 = sfc->w - 1;
    unsigned int packed = gr_pack_bgra(ttmPalette[color][0], ttmPalette[color][1], ttmPalette[color][2], 0);
    unsigned int *row = (unsigned int*)(sfc->pixels + y * sfc->pitch);
    for (int x = x1; x <= x2; x++) row[x] = packed;
}

void grLoadPalette(struct TPalResource *palResource)
{
    if (palResource == NULL) fatalError("NULL palette\n");
    for (int i = 0; i < 16; i++) {
        ttmPalette[i][0] = palResource->colors[i].b << 2;
        ttmPalette[i][1] = palResource->colors[i].g << 2;
        ttmPalette[i][2] = palResource->colors[i].r << 2;
        ttmPalette[i][3] = 0;
    }
}

void graphicsInit()
{
    grOutputSfc = gr_create_surface(SCREEN_WIDTH, SCREEN_HEIGHT);
    grLoadPalette(palResources[0]);
    srand(time(NULL));
    eventsInit();
}

void graphicsEnd()
{
    gr_free_surface(grOutputSfc);  grOutputSfc = NULL;
    gr_free_surface(grBackgroundSfc); grBackgroundSfc = NULL;
    gr_free_surface(grSavedZonesLayer); grSavedZonesLayer = NULL;
}

void grRefreshDisplay() {}
void grToggleFullScreen() { grWindowed = !grWindowed; }

void grStartFrame()
{
    if (grBackgroundSfc != NULL) {
        gr_blit(grOutputSfc, grBackgroundSfc, 0, 0);
    }
    if (grSavedZonesLayer != NULL)
        gr_blit(grOutputSfc, grSavedZonesLayer, 0, 0);
}

void grBlitLayer(GrSurface *layer)
{
    if (layer != NULL) {
        gr_blit(grOutputSfc, layer, 0, 0);
    }
}

void grFlushDisplay()
{
    eventsWaitTick(grUpdateDelay);
}

GrSurface *grNewLayer()
{
    GrSurface *sfc = gr_create_surface(640, 480);
    gr_fill_rect(sfc, 0, 0, 640, 480, GR_COLORKEY);
    sfc->colorkey_enabled = 1;
    sfc->colorkey = GR_COLORKEY;
    return sfc;
}

void grFreeLayer(GrSurface *sfc) { gr_free_surface(sfc); }

void grSetClipZone(GrSurface *sfc, sint16 x1, sint16 y1, sint16 x2, sint16 y2)
{
    x1 += grDx; y1 += grDy; x2 += grDx; y2 += grDy;
    gr_set_clip(sfc, x1, y1, x2-x1, y2-y1);
}

void grCopyZoneToBg(GrSurface *sfc, uint16 x, uint16 y, uint16 width, uint16 height)
{
    x += grDx; y += grDy;
    if (grSavedZonesLayer == NULL) grSavedZonesLayer = grNewLayer();
    for (int row = 0; row < height; row++) {
        int src_y = (int)y + row;
        if (src_y < 0 || src_y >= sfc->h) continue;
        if (src_y >= grSavedZonesLayer->h) continue;
        int x1 = (int)x;
        if (x1 < 0) x1 = 0;
        int copy_w = (int)width + 2;
        if (x1 + copy_w > sfc->w) copy_w = sfc->w - x1;
        if (x1 + copy_w > grSavedZonesLayer->w) copy_w = grSavedZonesLayer->w - x1;
        if (copy_w <= 0) continue;
        unsigned int *src = (unsigned int*)(sfc->pixels + src_y*sfc->pitch) + x1;
        unsigned int *dst = (unsigned int*)(grSavedZonesLayer->pixels + src_y*grSavedZonesLayer->pitch) + x1;
        memcpy(dst, src, copy_w * 4);
    }
}

void grSaveImage1(GrSurface *sfc, uint16 a, uint16 b, uint16 c, uint16 d) { (void)sfc;(void)a;(void)b;(void)c;(void)d; }
void grSaveZone(GrSurface *sfc, uint16 x, uint16 y, uint16 w, uint16 h) { (void)sfc;(void)x;(void)y;(void)w;(void)h; }
void grRestoreZone(GrSurface *sfc, uint16 x, uint16 y, uint16 w, uint16 h) { (void)sfc;(void)x;(void)y;(void)w;(void)h; grReleaseSavedLayer(); }

void grDrawPixel(GrSurface *sfc, sint16 x, sint16 y, uint8 color)
{ grPutPixel(sfc, x+grDx, y+grDy, color); }

void grDrawLine(GrSurface *sfc, sint16 x1, sint16 y1, sint16 x2, sint16 y2, uint8 color)
{
    x1+=grDx; y1+=grDy; x2+=grDx; y2+=grDy;
    int dx=abs(x2-x1), dy=abs(y2-y1);
    int xinc=(x2>x1)?1:-1, yinc=(y2>y1)?1:-1;
    int x=x1, y=y1;
    if (dy < dx) {
        int cumul=(dx+1)>>1;
        for (int i=0;i<dx;i++) { grPutPixel(sfc,x,y,color); x+=xinc; cumul+=dy; if(cumul>dx){cumul-=dx;y+=yinc;} }
    } else {
        int cumul=(dy+1)>>1;
        for (int i=0;i<dy;i++) { grPutPixel(sfc,x,y,color); y+=yinc; cumul+=dx; if(cumul>dy){cumul-=dy;x+=xinc;} }
    }
}

void grDrawRect(GrSurface *sfc, sint16 x, sint16 y, uint16 width, uint16 height, uint8 color)
{
    x+=grDx; y+=grDy;
    unsigned int packed = gr_pack_bgra(ttmPalette[color][0],ttmPalette[color][1],ttmPalette[color][2],0);
    gr_fill_rect(sfc, x, y, width, height, packed);
}

void grDrawCircle(GrSurface *sfc, sint16 x1, sint16 y1, uint16 width, uint16 height, uint8 fgColor, uint8 bgColor)
{
    x1+=grDx; y1+=grDy;
    if (width != height) { fprintf(stderr,"Warning: grDrawCircle: no ellipse\n"); return; }
    if (width%2) { fprintf(stderr,"Warning: grDrawCircle: odd diameter\n"); return; }
    int r=(width>>1)-1, xc=x1+r, yc=y1+r, cx=0, cy=r, d=1-r;
    while(1) {
        grDrawHorizontalLine(sfc,xc-cx,xc+cx+1,yc+cy+1,bgColor);
        grDrawHorizontalLine(sfc,xc-cx,xc+cx+1,yc-cy,bgColor);
        grDrawHorizontalLine(sfc,xc-cy,xc+cy+1,yc+cx+1,bgColor);
        grDrawHorizontalLine(sfc,xc-cy,xc+cy+1,yc-cx,bgColor);
        if(cy-cx<=1) break;
        if(d<0) d+=(cx<<1)+3; else {d+=((cx-cy)<<1)+5;cy--;} cx++;
    }
    if (fgColor != bgColor) {
        cx=0; cy=r; d=1-r;
        while(1) {
            grPutPixel(sfc,xc-cx,yc+cy+1,fgColor); grPutPixel(sfc,xc+cx+1,yc+cy+1,fgColor);
            grPutPixel(sfc,xc-cx,yc-cy,fgColor);   grPutPixel(sfc,xc+cx+1,yc-cy,fgColor);
            grPutPixel(sfc,xc-cy,yc+cx+1,fgColor); grPutPixel(sfc,xc+cy+1,yc+cx+1,fgColor);
            grPutPixel(sfc,xc-cy,yc-cx,fgColor);   grPutPixel(sfc,xc+cy+1,yc-cx,fgColor);
            if(cy-cx<=1) break;
            if(d<0) d+=(cx<<1)+3; else {d+=((cx-cy)<<1)+5;cy--;} cx++;
        }
    }
}

void grDrawSprite(GrSurface *sfc, struct TTtmSlot *ttmSlot, sint16 x, sint16 y, uint16 spriteNo, uint16 imageNo)
{
    if (spriteNo >= ttmSlot->numSprites[imageNo]) {
        fprintf(stderr,"Warning: grDrawSprite(): less than %d sprites in slot %d\n", imageNo, spriteNo);
        return;
    }
    x+=grDx; y+=grDy;
    gr_blit(sfc, ttmSlot->sprites[imageNo][spriteNo], x, y);
}

void grDrawSpriteFlip(GrSurface *sfc, struct TTtmSlot *ttmSlot, sint16 x, sint16 y, uint16 spriteNo, uint16 imageNo)
{
    if (spriteNo >= ttmSlot->numSprites[imageNo]) {
        fprintf(stderr,"Warning: grDrawSpriteFlip(): less than %d sprites in slot %d\n", imageNo, spriteNo);
        return;
    }
    x+=grDx; y+=grDy;
    GrSurface *srcSfc = ttmSlot->sprites[imageNo][spriteNo];
    int sw=srcSfc->w, sh=srcSfc->h;
    for (int col=0;col<sw;col++) {
        int dx2=x+sw-1-col;
        for (int row=0;row<sh;row++) {
            unsigned int *spx=(unsigned int*)(srcSfc->pixels+row*srcSfc->pitch)+col;
            if (srcSfc->colorkey_enabled && *spx==srcSfc->colorkey) continue;
            int dy2=y+row;
            if (dx2<sfc->clip_x||dy2<sfc->clip_y||dx2>=sfc->clip_x+sfc->clip_w||dy2>=sfc->clip_y+sfc->clip_h) continue;
            unsigned int *dpx=(unsigned int*)(sfc->pixels+dy2*sfc->pitch)+dx2;
            *dpx=*spx;
        }
    }
}

void grClearScreen(GrSurface *sfc)
{
    int cx=sfc->clip_x,cy=sfc->clip_y,cw=sfc->clip_w,ch=sfc->clip_h;
    gr_set_clip(sfc,0,0,sfc->w,sfc->h);
    gr_fill_rect(sfc,0,0,sfc->w,sfc->h,GR_COLORKEY);
    gr_set_clip(sfc,cx,cy,cw,ch);
}

void grLoadScreen(char *strArg)
{
    if (grBackgroundSfc != NULL) grReleaseScreen();
    if (grSavedZonesLayer != NULL) grReleaseSavedLayer();
    struct TScrResource *scrResource = findScrResource(strArg);
    if ((scrResource->width % 2) == 1)
        fprintf(stderr,"Warning: grLoadScreen(): odd width\n");
    if (scrResource->width > 640 || scrResource->height > 480)
        fatalError("grLoadScreen(): can't manage more than 640x480 resolutions");
    uint16 width=scrResource->width, height=scrResource->height;
    grBackgroundSfc = gr_create_surface(width, height);
    uint8 *inPtr=scrResource->uncompressedData;
    uint8 *outPtr=grBackgroundSfc->pixels;
    for (int i=0;i<width*height/2;i++) {
        uint8 idx0=(inPtr[0]&0xf0)>>4, idx1=(inPtr[0]&0x0f);
        outPtr[0]=ttmPalette[idx0][0]; outPtr[1]=ttmPalette[idx0][1]; outPtr[2]=ttmPalette[idx0][2]; outPtr[3]=0; outPtr+=4;
        outPtr[0]=ttmPalette[idx1][0]; outPtr[1]=ttmPalette[idx1][1]; outPtr[2]=ttmPalette[idx1][2]; outPtr[3]=0; outPtr+=4;
        inPtr++;
    }
}

void grInitEmptyBackground()
{
    if (grBackgroundSfc != NULL) grReleaseScreen();
    if (grSavedZonesLayer != NULL) grReleaseSavedLayer();
    grBackgroundSfc = gr_create_surface(640, 480);
    memset(grBackgroundSfc->pixels, 0, 640*480*4);
}

void grReleaseBmp(struct TTtmSlot *ttmSlot, uint16 bmpSlotNo)
{
    for (int i=0;i<ttmSlot->numSprites[bmpSlotNo];i++)
        gr_free_surface(ttmSlot->sprites[bmpSlotNo][i]);
    ttmSlot->numSprites[bmpSlotNo]=0;
}

void grLoadBmp(struct TTtmSlot *ttmSlot, uint16 slotNo, char *strArg)
{
    if (ttmSlot->numSprites[slotNo]) grReleaseBmp(ttmSlot, slotNo);
    struct TBmpResource *bmpResource=findBmpResource(strArg);
    uint8 *inPtr=bmpResource->uncompressedData;
    ttmSlot->numSprites[slotNo]=bmpResource->numImages;
    for (int image=0;image<bmpResource->numImages;image++) {
        if ((bmpResource->widths[image]%2)==1) fatalError("grLoadBmp(): odd width");
        uint16 width=bmpResource->widths[image], height=bmpResource->heights[image];
        GrSurface *surface=gr_create_surface(width,height);
        surface->colorkey_enabled=1; surface->colorkey=GR_COLORKEY;
        uint8 *outPtr=surface->pixels;
        for (int i=0;i<width*height/2;i++) {
            uint8 idx0=(inPtr[0]&0xf0)>>4, idx1=(inPtr[0]&0x0f);
            unsigned int p0=gr_pack_bgra(ttmPalette[idx0][0],ttmPalette[idx0][1],ttmPalette[idx0][2],0);
            unsigned int p1=gr_pack_bgra(ttmPalette[idx1][0],ttmPalette[idx1][1],ttmPalette[idx1][2],0);
            *(unsigned int*)outPtr=(p0==gr_pack_bgra(0xa8,0,0xa8,0))?GR_COLORKEY:p0; outPtr+=4;
            *(unsigned int*)outPtr=(p1==gr_pack_bgra(0xa8,0,0xa8,0))?GR_COLORKEY:p1; outPtr+=4;
            inPtr++;
        }
        ttmSlot->sprites[slotNo][image]=surface;
    }
}

void grFadeOut()
{
    static int fadeOutType=0;
    grDx=grDy=0;
    switch(fadeOutType) {
        case 0: for(int r=20;r<=400;r+=20){grDrawCircle(grOutputSfc,320-r,240-r,r<<1,r<<1,5,5);eventsWaitTick(1);} break;
        case 1: for(int i=1;i<=20;i++){grDrawRect(grOutputSfc,320-i*16,240-i*12,i*32,i*24,5);eventsWaitTick(1);} break;
        case 2: for(int i=600;i>=0;i-=40){grDrawRect(grOutputSfc,i,0,40,480,5);eventsWaitTick(1);} break;
        case 3: for(int i=0;i<640;i+=40){grDrawRect(grOutputSfc,i,0,40,480,5);eventsWaitTick(1);} break;
        case 4: for(int i=0;i<320;i+=20){grDrawRect(grOutputSfc,320+i,0,20,480,5);grDrawRect(grOutputSfc,300-i,0,20,480,5);eventsWaitTick(1);} break;
    }
    fadeOutType=(fadeOutType+1)%5;
}
