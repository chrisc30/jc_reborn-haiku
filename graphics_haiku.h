/*
 *  graphics_haiku.h
 *
 *  Replaces SDL2 types and functions for the Haiku screensaver port.
 *  GrSurface is a simple BGRA pixel buffer that mirrors what SDL_Surface
 *  provided. All drawing in graphics.c targets GrSurface; the final
 *  composite is blitted to a BBitmap in jc_saver.cpp.
 */

#ifndef GRAPHICS_HAIKU_H
#define GRAPHICS_HAIKU_H

/*
 * Do NOT include <stdint.h> here — mytypes.h owns uint8/uint16/uint32
 * and on Haiku <stdint.h> is pulled in transitively, causing -Wpedantic
 * "redefinition of typedef" warnings for every type in mytypes.h.
 * Callers must include mytypes.h before this header.
 */
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* GrSurface — replaces SDL_Surface                                    */
/* ------------------------------------------------------------------ */

typedef struct GrSurface {
    unsigned char  *pixels;     /* BGRA, 4 bytes per pixel            */
    int             w, h;
    int             pitch;      /* bytes per row = w * 4              */

    /* Clip rectangle (replaces SDL_SetClipRect) */
    int       clip_x, clip_y, clip_w, clip_h;

    /* Colour key (replaces SDL_SetColorKey) */
    int          colorkey_enabled;
    unsigned int colorkey;  /* BGRA packed                            */
} GrSurface;


/* ------------------------------------------------------------------ */
/* Colour key / magic transparent colour                               */
/* 0xa8, 0x00, 0xa8 in RGB → packed BGRA = 0x00a800a8                 */
/* ------------------------------------------------------------------ */
#define GR_COLORKEY  0x00a800a8u


static inline unsigned int gr_pack_bgra(unsigned char b, unsigned char g,
                                        unsigned char r, unsigned char a)
{
    return ((unsigned int)a << 24) | ((unsigned int)r << 16) |
           ((unsigned int)g <<  8) | (unsigned int)b;
}

static inline GrSurface *gr_create_surface(int w, int h)
{
    GrSurface *s = (GrSurface*)malloc(sizeof(GrSurface));
    s->w      = w;
    s->h      = h;
    s->pitch  = w * 4;
    s->pixels = (unsigned char*)malloc(w * h * 4);
    memset(s->pixels, 0, w * h * 4);

    /* Default clip = whole surface */
    s->clip_x = 0; s->clip_y = 0;
    s->clip_w = w; s->clip_h = h;

    s->colorkey_enabled = 0;
    s->colorkey         = 0;
    return s;
}

static inline void gr_free_surface(GrSurface *s)
{
    if (!s) return;
    free(s->pixels);
    free(s);
}

/* Set the clip rectangle (w==0 resets to full surface) */
static inline void gr_set_clip(GrSurface *s, int x, int y, int w, int h)
{
    if (w == 0 || h == 0) {
        s->clip_x = 0; s->clip_y = 0;
        s->clip_w = s->w; s->clip_h = s->h;
    } else {
        s->clip_x = x; s->clip_y = y;
        s->clip_w = w; s->clip_h = h;
    }
}

/* Fill a rectangle on the surface (respects clip) */
static inline void gr_fill_rect(GrSurface *s,
                                 int rx, int ry, int rw, int rh,
                                 unsigned int color)
{
    /* Intersect with clip */
    int x1 = rx < s->clip_x ? s->clip_x : rx;
    int y1 = ry < s->clip_y ? s->clip_y : ry;
    int x2 = rx + rw; if (x2 > s->clip_x + s->clip_w) x2 = s->clip_x + s->clip_w;
    int y2 = ry + rh; if (y2 > s->clip_y + s->clip_h) y2 = s->clip_y + s->clip_h;

    for (int y = y1; y < y2; y++) {
        unsigned int *row = (unsigned int*)(s->pixels + y * s->pitch);
        for (int x = x1; x < x2; x++)
            row[x] = color;
    }
}

/*
 * Blit src onto dst at (dx, dy).
 * Respects dst clip rectangle and src colorkey (transparent pixels skipped).
 */
static inline void gr_blit(GrSurface *dst, GrSurface *src, int dx, int dy)
{
    /* Source region */
    int sx1 = 0, sy1 = 0;
    int sx2 = src->w, sy2 = src->h;

    /* Clip destination */
    int cx1 = dst->clip_x,  cy1 = dst->clip_y;
    int cx2 = cx1 + dst->clip_w, cy2 = cy1 + dst->clip_h;

    /* Translate and clip */
    if (dx < cx1) { sx1 += cx1 - dx; dx = cx1; }
    if (dy < cy1) { sy1 += cy1 - dy; dy = cy1; }
    if (dx + (sx2-sx1) > cx2) sx2 = sx1 + (cx2 - dx);
    if (dy + (sy2-sy1) > cy2) sy2 = sy1 + (cy2 - dy);

    if (sx1 >= sx2 || sy1 >= sy2) return;

    int copy_w = sx2 - sx1;
    int copy_h = sy2 - sy1;

    for (int row = 0; row < copy_h; row++) {
        unsigned int *srow = (unsigned int*)(src->pixels + (sy1+row) * src->pitch) + sx1;
        unsigned int *drow = (unsigned int*)(dst->pixels + (dy +row) * dst->pitch) + dx;

        if (src->colorkey_enabled) {
            for (int col = 0; col < copy_w; col++) {
                if (srow[col] != src->colorkey)
                    drow[col] = srow[col];
            }
        } else {
            memcpy(drow, srow, copy_w * 4);
        }
    }
}

#endif /* GRAPHICS_HAIKU_H */
