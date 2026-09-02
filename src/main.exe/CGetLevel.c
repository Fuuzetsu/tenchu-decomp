#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long CGetLevel(struct AreaNodeType **hint, long x, long y, long z, unsigned long flag);
 *     EFFECT.C:220, 34 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct AreaNodeType ** hint
 *     param $a1       long x
 *     param $a2       long y
 *     param $a3       long z
 *     param stack+16  unsigned long flag
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct AreaNodeType *FieldArea;
 * END PSX.SYM */

/* Retail narrows flag to s16 at the map query despite the public u_long
 * parameter. */

extern long ComputeAreaLevel(AreaNodeType *node, long x, long z);
long CGetLevel(AreaNodeType **hint, long x, long y, long z, unsigned long flag)
{
    long x10;
    long y10;
    long z10;
    AreaNodeType *node;
    long ret;

    x10 = x / 10;
    y10 = y / 10;
    node = *hint;
    z10 = z / 10;
    if (node == 0 || node->y - 200 > y10 || y10 > node->y || x10 < node->x1 ||
        z10 < node->z1 || node->x2 < x10 || node->z2 < z10)
    {
        ret = GetAreaMapLevel(GlobalAreaMap, x, y - 300, z, (short)flag);
        if (y <= ret && FieldArea->division == AREA_DIVISION_ALL)
        {
            *hint = FieldArea;
        }
    }
    else if (node->dy != 0)
    {
        ret = ComputeAreaLevel(node, x10, z10);
        if (ret == (u32)LEVEL_NONE)
        {
            goto done;
        }
        ret = ret * 10;
    }
    else
    {
        ret = node->y * 10;
    }
done:
    return ret;
}
