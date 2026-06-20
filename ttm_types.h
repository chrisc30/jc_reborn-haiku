/*
 *  ttm_types.h  —  Haiku port
 *
 *  Defines TTtmSlot, TTtmThread, TTtmTag, TAdsScene and the associated
 *  constants that the original codebase left undefined (used implicitly
 *  by ads.c and ttm.c as an in-progress project).
 *
 *  Include this header in every .c/.cpp file that references these types,
 *  BEFORE any other project header (after mytypes.h).
 *
 *  Constant rationale:
 *    MAX_BMP_SLOTS       – number of BMP image-set slots per TTtmSlot
 *    MAX_SPRITES_PER_SLOT – max individual frames in one BMP slot
 *    MAX_TTM_SLOTS       – simultaneous loaded TTM animations
 *    MAX_TTM_THREADS     – simultaneous running scene threads
 */

#ifndef TTM_TYPES_H
#define TTM_TYPES_H

#include "mytypes.h"
#include "graphics_haiku.h"   /* GrSurface */

/* ------------------------------------------------------------------ */
/* Tuneable constants                                                   */
/* ------------------------------------------------------------------ */

#define MAX_BMP_SLOTS        10
#define MAX_SPRITES_PER_SLOT 200
#define MAX_TTM_SLOTS        20
#define MAX_TTM_THREADS      16

/* ------------------------------------------------------------------ */
/* TTtmTag — one labelled jump-point inside a TTM byte-stream          */
/* ------------------------------------------------------------------ */

struct TTtmTag {
    uint16 id;
    uint32 offset;
};

/* ------------------------------------------------------------------ */
/* TTtmSlot — one loaded TTM resource with its sprite sheets           */
/* ------------------------------------------------------------------ */

struct TTtmSlot {
    /* Byte-code stream */
    uint8  *data;
    uint32  dataSize;

    /* Jump tags parsed from the stream */
    uint16          numTags;
    struct TTtmTag *tags;

    /* Sprite sheets: [bmpSlotNo][spriteNo] */
    uint16      numSprites[MAX_BMP_SLOTS];
    GrSurface  *sprites[MAX_BMP_SLOTS][MAX_SPRITES_PER_SLOT];
};

/* ------------------------------------------------------------------ */
/* TTtmThread — one running playback context for a TTtmSlot            */
/* ------------------------------------------------------------------ */

struct TTtmThread {
    struct TTtmSlot *ttmSlot;   /* which slot this thread plays        */
    GrSurface       *ttmLayer;  /* off-screen layer for this thread    */

    /* Playback state */
    uint32  ip;                 /* instruction pointer into slot->data */
    uint32  nextGotoOffset;     /* pending GOTO target (0 = none)      */
    int     isRunning;          /* 0=stopped, 1=running, 2=done        */

    /* Timing */
    uint16  delay;              /* frame delay in tick units           */
    uint16  timer;              /* countdown to next frame             */

    /* Scene identity */
    uint16  sceneSlot;
    uint16  sceneTag;
    sint16  sceneTimer;         /* ms remaining (-ve = count up)       */
    int     sceneIterations;    /* repeat count (0 = infinite)         */

    /* Drawing state */
    uint8   fgColor;
    uint8   bgColor;
    uint8   selectedBmpSlot;
};

/* ------------------------------------------------------------------ */
/* TAdsScene — slot+tag pair identifying one scene in an ADS script    */
/* ------------------------------------------------------------------ */

struct TAdsScene {
    uint16 slot;
    uint16 tag;
};

#endif /* TTM_TYPES_H */
