#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct VECTOR * GetAreaMapPassage(unsigned long *area, struct VECTOR *pos, struct SVECTOR *vect, short n);
 *     CONFLICT.C:223, 31 src lines, frame 72 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s4       unsigned long * area
 *     param $a1       struct VECTOR * pos
 *     param $s1       struct SVECTOR * vect
 *     param $s3       short n
 *     reg   $a0       struct AreaNodeType * node
 *     stack sp+24     long [2] x
 *     stack sp+32     long [2] z
 *     stack sp+40     long [2] y
 *
 * Globals it touches, as the original declared them:
 *     extern struct AreaNodeType *FieldArea;
 *     extern struct NodeIndexType *FieldIndex;
 * END PSX.SYM */

extern VECTOR cv;

VECTOR *GetAreaMapPassage(AreaMapType *area, VECTOR *pos, SVECTOR *vect, short n)
{
    long x[2], z[2], y[2];
    long xmin, xmax, ymin, ymax;
    short initial;
    short count;
    AreaNodeType *node;

    cv = *pos;
    initial = n;
    if (n <= 0)
    {
        initial = 100;
    }
    count = initial;

    for (;;)
    {
        y[0] = GetAreaMapLevel(area, cv.vx, cv.vy, cv.vz,
                               AREA_LEVEL_DEFAULT);
        if (y[0] == LEVEL_NONE)
        {
            break;
        }
        node = FieldArea;
        x[0] = node->x1 * 10;
        x[1] = node->x2 * 10;
        z[0] = node->z1 * 10;
        z[1] = node->z2 * 10;
        if (FieldIndex != (NodeIndexType *)area)
        {
            ymax = FieldIndex[-1].y * 10;
        }
        else
        {
            ymax = -1000000;
        }
        xmin = x[0];
        xmax = x[1];
        ymin = y[0];
        y[1] = ymax;
    inner:
        cv.vx += vect->vx;
        cv.vy += vect->vy;
        cv.vz += vect->vz;
        count--;
        if (count == 0)
        {
            return 0;
        }
        if (xmin <= cv.vx && cv.vx <= xmax && ymin <= cv.vy && cv.vy <= y[1] && z[0] <= cv.vz && cv.vz <= z[1])
        {
            goto inner;
        }
    }
    cv.vx -= vect->vx;
    cv.vy -= vect->vy;
    cv.vz -= vect->vz;
    return &cv;
}
