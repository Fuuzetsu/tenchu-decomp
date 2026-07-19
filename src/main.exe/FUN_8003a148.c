#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * MATCH.
 *
 * FUN_8003a148 (0x8003a148, 0x160 bytes) — same "project a 3D point and
 * sort-draw a sprite there" tail as DrawSpriteXYZ (the [0, 0x4e1] clamp +
 * GsSortSprite), but with a CHOICE of projection: when `coord` (param_6) is
 * non-NULL it goes through the GsGetLs/SetLsMatrix/RotTransPers path (same
 * scratchpad idiom as DrawOrnament/DrawModel — install `coord`'s local
 * system as the GTE's matrix, then rotate-translate-perspective the
 * (x,y,z) point written into the scratchpad SVECTOR @ 0x1F800020);
 * otherwise it falls back to GetScreenPosition (the already-matched
 * camera-relative-transform + RotTransPers helper). Either path leaves the
 * projected (x,y) and OTZ in one local SVECTOR `scr` (x,y from the
 * RotTransPers/GetScreenPosition out-param, z/otz narrowed from the return
 * value) exactly like DrawSpriteXYZ's `scr`. The `pri` clamp additionally
 * folds in `param_7` (added to otz before the `>>2`), unlike DrawSpriteXYZ's
 * plain otz.
 *
 * Matching notes (see DrawSpriteXYZ.c for the shared idioms in full):
 *  - This TU divides by a runtime value (`size*300/otz`): needs
 *    `--expand-div` (Build.hs maspsxGpExterns' `extra` list + permute.py's
 *    MASPSX_EXTRA), same as DrawSpriteXYZ. Ghidra's `trap(0x1c00)`/
 *    `trap(0x1800)` are ASPSX's automatic divide-by-zero/overflow guards,
 *    not literal source calls — plain `/` reproduces them once the
 *    function is on the expand-div list.
 *  - `if (coord != 0) {GsGetLs path} else {GetScreenPosition}` — the opposite
 *    polarity of Ghidra's literal `if (coord == 0) {GetScreenPosition} else
 *    {GsGetLs}` rendering: the target's branch is `beqz coord, ...` with
 *    the GsGetLs block as the FALL-THROUGH, so the C needs the condition
 *    that puts GsGetLs first (the `if (cond) A; else B;`-makes-A-the-
 *    fallthrough rule).
 *  - The [0, 0x4e1] clamp reuses DrawSpriteXYZ's exact `goto zero;` shape,
 *    comparing `(scr.vz + d) >> 2` instead of the twin's `(u16)otz << 16 >>
 *    0x12`. The RHS must re-read the struct field `scr.vz`, not the
 *    already-live `otz` local: the target's asm re-loads it fresh (`lh`)
 *    right there even though `otz` (from the `if (otz > 0x24)` test) is
 *    numerically identical and already in a register — same "target's own
 *    SSA rendering tells you it reloads" tell as `vrealloc`/DrawSpriteXYZ's
 *    own `scr.vz` re-read for `t`. Reusing `otz` here is 4 bytes short
 *    (cc1 doesn't refetch a value it can already see live).
 */
extern GsOT *OTablePt;
extern void GetScreenPosition(s32 x, s32 y, s32 z, s32 *out);

void FUN_8003a148(GsSPRITE *sp, s32 x, s32 y, s32 z, s32 size, GsCOORDINATE2 *coord, short d)
{
    SVECTOR scr;
    s32 otz;
    s16 sc;
    s32 t;
    s32 pri;

    if (coord != 0)
    {
        SVECTOR *sv = (SVECTOR *)TENCHU_SCRATCHPAD(0x20);
        sv->vx = x;
        sv->vy = y;
        sv->vz = z;
        GsGetLs(coord, (MATRIX *)TENCHU_SCRATCHPAD_ADDRESS);
        GsSetLsMatrix((MATRIX *)TENCHU_SCRATCHPAD_ADDRESS);
        scr.vz = (s16)RotTransPers(
            sv, (s32 *)&scr, (void *)TENCHU_SCRATCHPAD(0x28),
            (void *)TENCHU_SCRATCHPAD(0x2c));
    }
    else
    {
        GetScreenPosition(x, y, z, (s32 *)&scr);
    }
    otz = scr.vz;
    if (otz > 0x24)
    {
        sc = (s16)((size * 300) / otz) + 1;
        sp->scaley = sc;
        sp->scalex = sc;
        sp->x = scr.vx;
        sp->y = scr.vy;
        t = (scr.vz + (s32)d) >> 2;
        if (t < 0)
        {
            goto zero;
        }
        pri = 0x4e1;
        if (t < 0x4e2)
        {
            pri = t;
        }
        goto done;
    zero:
        pri = 0;
    done:
        GsSortSprite(sp, OTablePt, (u16)pri);
    }
}
