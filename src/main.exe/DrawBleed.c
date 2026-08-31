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
 * The 8-byte park was a local minimum created by its own `param2` and
 * `savedTime` scaffolding.  Restoring the PSX.SYM `long x, y, z` captures and
 * the ordinary `param->time -= 1` removes both invented identities.  The two
 * later coordinates deliberately read through scalar `s32` lvalues, matching
 * the already-proven DrawSplash source shape: direct nested-VECTOR member
 * reads carry cc1's structure-memory marker and sched1 sinks them, whereas the
 * scalar views let the loads remain early in `$a1/$a2`, exactly as both the
 * retail target and same-sized demo build show.  The demo's 532-byte body is
 * otherwise instruction-for-instruction identical through this whole state
 * update and projection setup, making this a source reconstruction rather than
 * an allocation nudge.
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
 * Matching notes (docs/matching-cookbook.md):
 *  - `param = &ef->param.bleed;` (BleedType* at ef+4, matching PSX.SYM's
 *    own `reg $s1 struct BleedType * param`) — `pos` (VECTOR, s32 fields)
 *    and `vec` (SVECTOR, s16 fields) are BleedType's own proven layout
 *    (effect.h), no truncated per-TU redeclaration needed.
 *  - `param->time = param->time - 1;` is a REAL `addiu -1` (not the
 *    `+0xff` Ghidra's decompile TEXT shows) — the countdown-decrement
 *    idiom is per-function, decode the raw immediate (0xFFFF = -1), don't
 *    trust Ghidra's rendered constant.
 *  - The velocity integration needs FOUR independent statements, not a
 *    struct copy: `pos.vx+=vec.vx; pos.vy+=vec.vy; pos.vz+=vec.vz;
 *    vec.vy=vec.vy+1;` — `vec.vy` is read TWICE by the target, once
 *    SIGNED (`lh`, widening into the `pos.vy` s32 accumulator) and once
 *    UNSIGNED (`lhu`, the narrowing self-store `vec.vy=vec.vy+1`) — two
 *    un-CSE'd loads of one field, same family as DeleteConflict's
 *    ConflictObjects (a narrowing use of a signed field always loads
 *    `lhu`, a widening use loads `lh`; don't collapse to one shared read).
 *  - The camera-relative Scratchpad projection is DrawTarget's own
 *    idiom verbatim (zero the 3-word rotation, store `pos - (short)View`
 *    per axis, `SetTransMatrix`/`SetRotMatrix`/`RotTransPers`).
 *  - `scr.vz` gets RotTransPers's return value truncated in by the caller
 *    (`scr.vz = (s16)RotTransPers(...)`), matching draw_sprite_coord_'s `scr`
 *    convention exactly (x/y filled via the `sxy` out-param, z assigned
 *    separately from the call result).
 *  - `t = (s32)((u32)(u16)scr.vz << 16);` must be its own NAMED variable,
 *    reused for BOTH `otz = t >> 16` (the `>0x24` test + the `900/otz`
 *    divisor) AND the tail's OTZ clamp (`t >> 18`, i.e. `>> 0x12`) — the
 *    target computes the `(u16)x<<16` pattern only ONCE (one `lhu`) and
 *    keeps it alive in a register across the whole draw body (div, all the
 *    POLY_F4 field stores) to the clamp at the very end. Unlike
 *    DrawSpriteXYZ/draw_sprite_coord_ (which each re-read `scr.vz` fresh for
 *    their own clamp — verified by their own asm), DrawBleed's target has
 *    NO second load at all: an independent re-read of `scr.vz` here costs
 *    an extra `lhu` (4 bytes) the target doesn't spend. Same shift-reuse
 *    idiom, opposite lever from the sibling functions — read the actual
 *    asm, don't assume the family's usual re-read.
 *  - POLY_F4 field STORE ORDER is the raw `.s`'s physical order, not
 *    Ghidra's rendered statement order: `x0,y0,y1,x2` first, THEN `sz`
 *    computed, THEN `x1,y2,x3,y3` (x3/y3 immediately follow x1/y2 — NOT
 *    deferred to just before GsSortPoly the way Ghidra's decompile
 *    renders them; m2c's raw-offset dump agrees with the asm), THEN
 *    `r0,g0,b0`. `x3`/`y3` reuse the SAME `scr.vx+sz`/`scr.vy+sz`
 *    expression as `x1`/`y2` (repeat the expression; the target's `sh`
 *    for x3/y3 reuses the still-live registers, not a fresh reload of
 *    `plyBleed.x1`/`.y2`).
 *  - The `[0, 0x4e1]` clamp is DrawSpriteXYZ's exact `goto zero;` shape
 *    (the trivial `pri=0` body must be the branch TARGET, not the
 *    fall-through — see that file's header for the reorg mechanics).
 *  - This TU divides by a runtime value (`900 / otz`): needs
 *    `--expand-div` (Build.hs maspsxGpExterns' `extra` list + permute.py's
 *    MASPSX_EXTRA), same as DrawSprite/DrawSpriteXYZ/draw_sprite_coord_.
 *
 * The superseded round-by-round investigation log for this function lives
 * in docs/matching-archive.md.
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
