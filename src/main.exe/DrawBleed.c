#include "common.h"
#include "main.exe.h"
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
 *     extern struct tag_TItem items[30];
 *     extern struct POLY_F4 plyBleed;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 47 of 532 bytes differ (same instruction COUNT —
 * length matches exactly; every differing instruction is a genuine
 * instruction the draft already has elsewhere, only its SCHEDULED position
 * differs — no wrong value, no missing/extra logic).
 *
 * DrawBleed (0x8003437c, EFFECT.C:910) — the blood-drip effect's per-frame
 * draw: while `mode==0` and `time!=0`, advances the drip position by its
 * velocity (`pos += vec`) and drifts `vec.vy` by +1 (gravity-ish), or kills
 * the slot (`ef->proc = 0`) once `time` runs out; every frame regardless
 * decrements `time`, then projects `pos` (camera-relative, via the
 * DrawTarget-style Scratchpad SetTransMatrix/SetRotMatrix/RotTransPers
 * idiom) and, if visible (`otz > 0x24`), fills the shared `plyBleed` POLY_F4
 * quad (a diagonal streak from `(x,y)` to `(x+sz,y+sz)`, `sz` a distance-
 * scaled length) and GsSortPoly's it into the OT with the same
 * `[0, 0x4e1]` OTZ-derived priority clamp as DrawSpriteXYZ/FUN_8003a148.
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
 *    (`scr.vz = (s16)RotTransPers(...)`), matching FUN_8003a148's `scr`
 *    convention exactly (x/y filled via the `sxy` out-param, z assigned
 *    separately from the call result).
 *  - `t = (s32)((u32)(u16)scr.vz << 16);` must be its own NAMED variable,
 *    reused for BOTH `otz = t >> 16` (the `>0x24` test + the `900/otz`
 *    divisor) AND the tail's OTZ clamp (`t >> 18`, i.e. `>> 0x12`) — the
 *    target computes the `(u16)x<<16` pattern only ONCE (one `lhu`) and
 *    keeps it alive in a register across the whole draw body (div, all the
 *    POLY_F4 field stores) to the clamp at the very end. Unlike
 *    DrawSpriteXYZ/FUN_8003a148 (which each re-read `scr.vz` fresh for
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
 *    MASPSX_EXTRA), same as DrawSprite/DrawSpriteXYZ/FUN_8003a148.
 *
 * RESIDUAL (47 bytes, same class as DrawHinoko's own parked residual): the
 * target's SCHEDULER hoists `ViewInfo`'s `%hi` materialization (`lui
 * v1,%hi(ViewInfo)`) all the way into the FIRST branch's (`mode==0` test)
 * delay slot, and `pos.vy`/`pos.vz`'s FULL-WORD loads right after `time`'s
 * own reload — both well before their own statement's source position —
 * while this draft's schedule keeps them near their point of use. Confirmed
 * NOT a source-order issue: moving the `dy=pos.vy;dz=pos.vz;` statements
 * earlier in the C (before `time -= 1`) produced BYTE-IDENTICAL output,
 * proving cc1's list scheduler, not source position, decides how far to
 * hoist an independent load — same mechanism DrawHinoko.c's own residual
 * documents (a read hoisted far from its write, right instruction SET,
 * wrong SLOT). Also confirmed the LOAD WIDTH lever separately: `pos.vx/vy/vz`
 * (VECTOR, `long` fields) must be captured into plain `s32` temps (`dx`,
 * `dy`, `dz`) BEFORE the narrowing scratchpad store, or cc1's "narrowing
 * store" optimization narrows the LOAD itself to `lhu` (a valid modular-
 * arithmetic transform for the truncated subtraction, but 4 bytes short of
 * the target's `lw`) — the temps fixed the WIDTH; only the hoist DISTANCE
 * remains open. Tried and rejected: an explicit `TViewInfo *vp = &ViewInfo;`
 * declared at the very top (to bias the scheduler toward an early `lui`) —
 * this over-hoisted `%hi(ViewInfo)` even further (before the prologue's own
 * `sw ra`!) and regressed to 128 differing bytes; reverted. `tools/autorules.py`
 * found one real win (`pri: s32->s16`, 1004->48 differing bytes) already
 * applied; no further mechanical win. One bounded `tools/permute.py` run
 * (~300 s, `-j4 --stop-on-zero`) plateaued around score 175-760 with a
 * consistently high (100+) "errors" count that never reached 0 — consistent
 * with the cookbook's characterization: this divergence is chosen by a
 * later RTL pass (scheduling) the permuter's C-AST mutations cannot reach.
 */
typedef struct
{
    u_long tag;
    u8 r0, g0, b0, code;
    s16 x0, y0;
    s16 x1, y1;
    s16 x2, y2;
    s16 x3, y3;
} POLY_F4;

typedef struct
{
    s32 vpx;
    s32 vpy;
    s32 vpz;
    s32 vrx;
    s32 vry;
    s32 vrz;
} TViewInfo;

extern TViewInfo ViewInfo;
extern POLY_F4 plyBleed;
extern GsOT *OTablePt;
extern MATRIX GsWSMATRIX;
extern void SetTransMatrix(MATRIX *m);
extern void SetRotMatrix(MATRIX *m);
extern s32 RotTransPers(SVECTOR *v0, s32 *sxy, void *p, void *flg);
extern void GsSortPoly(POLY_F4 *p, GsOT *ot, s32 pri);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DrawBleed", DrawBleed);
#else
void DrawBleed(TEffectSlot *ef)
{
    BleedType *param = &ef->param.bleed;
    SVECTOR scr;
    SVECTOR *scrp;
    s32 dx, dy, dz;
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
            param->vec.vy = param->vec.vy + 1;
        }
    }
    dy = param->pos.vy;
    dz = param->pos.vz;
    param->time = param->time - 1;

    dx = param->pos.vx;
    *(s32 *)0x1F800014 = 0;
    *(s32 *)0x1F800018 = 0;
    *(s32 *)0x1F80001C = 0;
    *(s16 *)0x1F800020 = dx - (short)ViewInfo.vpx;
    *(s16 *)0x1F800022 = dy - (short)ViewInfo.vpy;
    *(s16 *)0x1F800024 = dz - (short)ViewInfo.vpz;
    SetTransMatrix((MATRIX *)0x1F800000);
    SetRotMatrix(&GsWSMATRIX);
    scrp = &scr;
    scrp->vz = (s16)RotTransPers((SVECTOR *)0x1F800020, (s32 *)scrp, (void *)0x1F800028, (void *)0x1F80002C);

    t = (s32)((u32)(u16)scr.vz << 16);
    otz = t >> 16;
    if (otz > 0x24)
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
        if (pri < 0)
        {
            goto zero;
        }
        pri = 0x4e1;
        if ((t >> 18) < 0x4e2)
        {
            pri = t >> 18;
        }
        goto done;
    zero:
        pri = 0;
    done:
        GsSortPoly(&plyBleed, OTablePt, (u16)pri);
    }
}
#endif
