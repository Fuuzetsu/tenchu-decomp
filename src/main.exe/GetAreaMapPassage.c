#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct VECTOR * GetAreaMapPassage(unsigned long *area, struct VECTOR *pos, struct SVECTOR *vect, short n);
 *     CONFLICT.C:223, 31 src lines, frame 72 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

/*
 * GetAreaMapPassage (0x8001a0c4, 0x244 bytes) — advance the global probe
 * position `cv` along `vect` until it leaves the current map cell, returning
 * the last valid point when the map query fails or null when `count` expires.
 *
 * Matching notes:
 *  - The real outer `for (;;)` loop keeps `%hi(cv)` and `&cv` in saved
 *    registers across repeated GetAreaMapLevel calls. The inner back-edge is
 *    hand-written: only x[0], x[1], and y[0] are cached in registers; y[1],
 *    z[0], and z[1] remain lazy stack reloads in the six-way bounds test.
 *  - `initial` separates the conditional default from the live counter. The
 *    empty one-shot loop after their copy is a weight-free allocation boundary:
 *    it preserves the target's `v0 -> s3` copy without giving `count` enough
 *    loop weight to exchange registers with the cached `%hi(cv)` value.
 *  - `ymax` delays the y[1] stack store until after the three cached-bound
 *    loads, matching the join schedule. The final x/y/z subtraction order
 *    makes the return-path `&cv` materialization match independently.
 *  - AreaNodeType and NodeIndexType use the recovered map layouts;
 *    FieldIndex[-1].y is the target's -0x10 halfword
 *    load. GetAreaMapLevel's fifth argument is the stack-passed u16 zero.
 */

extern AreaNodeType *FieldArea;
extern NodeIndexType *FieldIndex;
extern VECTOR cv;

extern long GetAreaMapLevel(unsigned long *area, long x, long y, long z, u16 flag);

VECTOR *GetAreaMapPassage(unsigned long *area, VECTOR *pos, SVECTOR *vect, short n)
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
        y[0] = GetAreaMapLevel(area, cv.vx, cv.vy, cv.vz, 0);
        if (y[0] == 0x80000000)
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
        cv.vx = cv.vx + vect->vx;
        cv.vy = cv.vy + vect->vy;
        count--;
        cv.vz = cv.vz + vect->vz;
        if (count == 0)
        {
            return 0;
        }
        if (xmin <= cv.vx && cv.vx <= xmax && ymin <= cv.vy
            && cv.vy <= y[1] && z[0] <= cv.vz && cv.vz <= z[1])
        {
            goto inner;
        }
    }
    cv.vx = cv.vx - vect->vx;
    cv.vy = cv.vy - vect->vy;
    cv.vz = cv.vz - vect->vz;
    return &cv;
}
