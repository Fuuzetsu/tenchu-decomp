#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetScreenPositionS(long x, long y, long z, struct SVECTOR *scr);
 *     EFFECT.C:588, 11 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long x
 *     param $a1       long y
 *     param $a2       long z
 *     param $a3       struct SVECTOR * scr
 * END PSX.SYM */

void GetScreenPositionS(s32 x, s32 y, s32 z, SVECTOR *scr)
{
    SVECTOR *point = (SVECTOR *)TENCHU_SCRATCHPAD(0x80);

    point->vx = x - (short)ViewInfo.vpx;
    point->vy = y - (short)ViewInfo.vpy;
    point->vz = z - (short)ViewInfo.vpz;
    scr->vz = RotTransPers(
        point, (s32 *)scr, (s32 *)TENCHU_SCRATCHPAD_ADDRESS,
        (s32 *)TENCHU_SCRATCHPAD(0x10));
}
