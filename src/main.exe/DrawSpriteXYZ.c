#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawSpriteXYZ(struct GsSPRITE *sprt, long x, long y, long z, long scale);
 *     EFFECT.C:204, 11 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct GsSPRITE * sprt
 *     param $a1       long x
 *     param $a2       long y
 *     param $a3       long z
 *     param stack+16  long scale
 *     stack sp+16     struct SVECTOR scr
 * END PSX.SYM */

void DrawSpriteXYZ(GsSPRITE *sprt, s32 x, s32 y, s32 z, s32 scale)
{
    SVECTOR scr;
    s32 otz;
    s32 t;
    s32 pri;

    GetScreenPosition(x, y, z, &scr);
    otz = scr.vz;
    if (otz > NEAR_DEPTH)
    {
        sprt->scalex = sprt->scaley =
            (s16)((scale * PROJECTION_DISTANCE) / otz) + 1;
        sprt->x = scr.vx;
        sprt->y = scr.vy;
        t = (s16)(u16)scr.vz >> 2;
        CLAMP_SORT_DEPTH(pri, t);
        GsSortSprite(sprt, OTablePt, (u16)pri);
    }
}
