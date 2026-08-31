#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawBleed(struct tag_EffectSlot *ef);
 *     EFFECT.C:910, 34 src lines, frame 40 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct tag_EffectSlot * ef
 *     reg   $s1       struct BleedType * param
 *     stack sp+16     struct SVECTOR scr
 *     reg   $v0       long x
 *     reg   $a1       long y
 *     reg   $a2       long z
 *     reg   $s0       struct SVECTOR * scr
 *     reg   $v1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct POLY_F4 plyBleed;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * STATUS: MATCHING — exact 532-byte pure C.
 *
 * DrawBleed (0x8003437c, EFFECT.C:910) — the blood-drip effect's per-frame
 * draw: while `mode==0` and `time!=0`, advances the drip position by its
 * velocity (`pos += vec`) and drifts `vec.vy` by +1 (gravity-ish), or kills
 * the slot (`ef->proc = 0`) once `time` runs out; every frame regardless
 * decrements `time`, then projects `pos` (camera-relative, via the
 * DrawTarget-style Scratchpad SetTransMatrix/SetRotMatrix/RotTransPers
 * idiom) and, if visible (`otz > NEAR_DEPTH`), fills the shared `plyBleed` POLY_F4
 * quad (a diagonal streak from `(x,y)` to `(x+sz,y+sz)`, `sz` a distance-
 * scaled length) and GsSortPoly's it into the OT with the same
 * `[0, 0x4e1]` OTZ-derived priority clamp as DrawSpriteXYZ/draw_sprite_coord_.
 *
 * Matching constraints:
 *  - Use PSX.SYM's long x, y, and z captures, with y and z read through
 *    scalar s32 lvalues. Direct nested-VECTOR reads carry a structure-memory
 *    marker and sched1 sinks them; the scalar views preserve the target's
 *    early $a1/$a2 loads. No param2 or savedTime identities are needed.
 *  - param is the proven BleedType at ef+4. Decrement time with an actual
 *    -1; Ghidra's displayed +0xff is not the encoded operation here.
 *  - Keep position integration and vec.vy update as four statements.
 *    vec.vy is loaded signed for the s32 accumulator and unsigned for its
 *    narrowing self-store, so sharing the read changes the code.
 *  - Preserve the DrawTarget scratchpad projection and assign the
 *    RotTransPers result to scr.vz through s16.
 *  - t = (s32)((u32)(u16)scr.vz << 16) is one named value reused for the
 *    visibility/division path and the final >>18 priority clamp. Re-reading
 *    scr.vz adds an lhu that the target does not contain.
 *  - Keep the POLY_F4 store order x0,y0,y1,x2; compute sz; x1,y2,x3,y3;
 *    then r0,g0,b0. Repeat scr.vx + sz and scr.vy + sz so the second pair
 *    reuses the live values rather than reloading fields.
 *  - The priority clamp retains DrawSpriteXYZ's goto-zero topology, and this
 *    runtime division file requires maspsx --expand-div.
 */
extern MATRIX GsWSMATRIX;

void DrawBleed(TEffectSlot *ef)
{
    BleedType *param = &ef->param.bleed;
    SVECTOR scr;
    SVECTOR *scrp;
    long x, y, z;
    s32 t;
    s32 otz;
    s16 pri;
    s16 sz;

    if (param->mode == 0)
    {
        if (param->time == 0)
        {
            ef->proc = 0;
        }
        else
        {
            param->pos.vx += param->vec.vx;
            param->pos.vy += param->vec.vy;
            param->pos.vz += param->vec.vz;
            param->vec.vy += 1;
        }
    }
    x = param->pos.vx;
    y = *(s32 *)&param->pos.vy;
    z = *(s32 *)&param->pos.vz;
    param->time -= 1;

    *(s32 *)TENCHU_SCRATCHPAD(0x14) = 0;
    *(s32 *)TENCHU_SCRATCHPAD(0x18) = 0;
    *(s32 *)TENCHU_SCRATCHPAD(0x1c) = 0;
    *(s16 *)TENCHU_SCRATCHPAD(0x20) = x - (short)ViewInfo.vpx;
    *(s16 *)TENCHU_SCRATCHPAD(0x22) = y - (short)ViewInfo.vpy;
    *(s16 *)TENCHU_SCRATCHPAD(0x24) = z - (short)ViewInfo.vpz;
    SetTransMatrix((MATRIX *)TENCHU_SCRATCHPAD_ADDRESS);
    SetRotMatrix(&GsWSMATRIX);
    scrp = &scr;
    scrp->vz = (s16)RotTransPers((SVECTOR *)TENCHU_SCRATCHPAD(0x20),
                                 (s32 *)scrp,
                                 (s32 *)TENCHU_SCRATCHPAD(0x28),
                                 (s32 *)TENCHU_SCRATCHPAD(0x2c));

    t = (s32)((u32)(u16)scr.vz << 16);
    otz = t >> 16;
    if (otz > NEAR_DEPTH)
    {
        plyBleed.x0 = scr.vx;
        plyBleed.y0 = scr.vy;
        plyBleed.y1 = scr.vy;
        plyBleed.x2 = scr.vx;
        sz = (s16)(900 / otz) + 1;
        plyBleed.x1 = scr.vx + sz;
        plyBleed.y2 = scr.vy + sz;
        plyBleed.x3 = scr.vx + sz;
        plyBleed.y3 = scr.vy + sz;
        plyBleed.r0 = param->r;
        plyBleed.g0 = param->g;
        plyBleed.b0 = param->b;
        pri = t >> 18;
        if (pri >= 0)
        {
            pri = DEPTH_LIMIT - 1;
            if ((t >> 18) < DEPTH_LIMIT)
            {
                pri = t >> 18;
            }
        }
        else
        {
            pri = 0;
        }
        GsSortPoly(&plyBleed, OTablePt, (u16)pri);
    }
}
