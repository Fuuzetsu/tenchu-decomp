#ifndef AFTERIMAGE_H
#define AFTERIMAGE_H

#include <psxsdk/libgpu.h>
#include "item.h"

/* EFFECT.C's weapon-trail state. PSX.SYM supplies the complete record and
 * original names; SetupAfterimage, DrawAfterimage, and DisposeAfterimage
 * independently confirm every field and the 0x58-byte allocation. */
typedef struct AfterimageType
{
    ModelType *model;             /* 0x00 */
    SVECTOR vector1;              /* 0x04 */
    SVECTOR vector2;              /* 0x0C */
    s16 maxn;                     /* 0x14 */
    s16 n;                        /* 0x16 */
    long *p1;                     /* 0x18 */
    long *p2;                     /* 0x1C */
    long sz;                      /* 0x20 */
    POLY_GT4 poly;                /* 0x24 */
} AfterimageType;                 /* 0x58 */

AfterimageType *SetupAfterimage(ModelType *model, short len);
void DisposeAfterimage(AfterimageType *afi);
short DrawAfterimage(AfterimageType *afi, short disp);

#endif
