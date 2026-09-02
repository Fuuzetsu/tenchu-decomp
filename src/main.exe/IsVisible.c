#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int IsVisible(long x, long y, long z, long s);
 *     WORLD.C:685, 63 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long x
 *     param $a1       long y
 *     param $a2       long z
 *     param $a3       long s
 * END PSX.SYM */

extern s32 abs(s32 x);

int IsVisible(s32 x, s32 y, s32 z, s32 s)
{
    enum
    {
        SXW = 160,
        SYW = 120
    };
    enum
    {
        NEAR = 150
    };
    GsRVIEW2 *view;
    VECTOR *view_space;
    s32 dx, dy, dz;
    s32 zs;
    s32 aq;
    s32 qs;
    s32 q0, q2;
    s32 fail;

    view = CONSTRUCTION_VISIBILITY_VIEW;

    dx = x - view->vpx;
    if (30000 < abs(dx))
        return 0;

    dy = y - view->vpy;
    if (30000 < abs(dy))
        return 0;

    dz = z - view->vpz;
    if (30000 < abs(dz))
        return 0;

    CONSTRUCTION_VISIBILITY_RELATIVE->vx = (s16)dx;
    CONSTRUCTION_VISIBILITY_RELATIVE->vy = (s16)dy;
    CONSTRUCTION_VISIBILITY_RELATIVE->vz = (s16)dz;
    ApplyRotMatrix(CONSTRUCTION_VISIBILITY_RELATIVE,
                   CONSTRUCTION_VISIBILITY_VIEW_SPACE);

    view_space = CONSTRUCTION_VISIBILITY_VIEW_SPACE;
    zs = view_space->vz + s;
    if (zs <= NEAR)
        return 0;
    if (17000 < view_space->vz - s)
        return 0;

    q0 = (view_space->vx * PROJECTION_DISTANCE) / zs;
    qs = (s * PROJECTION_DISTANCE) / zs;
    q2 = (view_space->vy * PROJECTION_DISTANCE) / zs;
    fail = 0;
    aq = abs(q0);
    if (qs + SXW < aq)
        goto failed;

    zs = abs(q2);
    if (qs + SYW < zs)
        goto failed;
    goto done;

failed:
    fail = 1;
done:
    return !fail;
}
