#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long ComputeAreaLevel(struct AreaNodeType *node, long x, long z);
 *     CONFLICT.C:83, 20 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $t0       struct AreaNodeType * node
 *     param $a1       long x
 *     param $a2       long z
 *     reg   $a0       short yy
 * END PSX.SYM */

long ComputeAreaLevel(AreaNodeType *node, long x, long z)
{
    short dz, zspan;
    short dx, xspan;
    short yy;
    int mask;

    dz = z - (u16)node->z1;
    zspan = (u16)node->z2 - (u16)node->z1 + 1;
    dx = x - (u16)node->x1;
    xspan = (u16)node->x2 - (u16)node->x1 + 1;

    mask = 1 << (((dz << 2) / zspan) * 4 + ((dx << 2) / xspan));
    if (((u16)node->division & mask) != 0)
    {
        yy = node->y;

        switch (node->attribute & (MAP_SLOPE_X | MAP_SLOPE_Z))
        {
        case MAP_SLOPE_X:
            yy = yy + dx * node->dy / xspan;
            break;
        case MAP_SLOPE_Z:
            yy += dz * node->dy / zspan;
            break;
        }
        return yy;
    }
    return LEVEL_NONE;
}
