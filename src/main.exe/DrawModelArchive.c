#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawModelArchive(struct ModelArchiveType *mad, long gap);
 *     3DCTRL.C:393, 28 src lines, frame 88 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $s2       struct ModelArchiveType * mad
 *     param $s3       long gap
 *     reg   $s0       struct ModelType * objp
 *     stack sp+16     struct MATRIX mat
 *     stack sp+48     struct SVECTOR pos
 *     reg   $s1       short i
 *     reg   $s2       struct ModelType * objp
 *     reg   $v1       long sz
 *     reg   $s0       short atr
 *     stack sp+56     short [2] rxy
 *
 * Globals it touches, as the original declared them:
 *     extern short SkipFrame;
 *     extern struct SVECTOR UnitVector;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * DrawModelArchive (0x8001768c) — MATCHED.
 *
 * Same TU as DrawModel.c/DrawSprite.c (3DCTRL.C): the ModelArchiveType twin
 * of DrawModel's visibility gauntlet (same GsGetLs+GsSetLsMatrix /
 * attribute&1/2/4/8/0x10 clip test / UnitVector RotTransPers), gated by a
 * SkipFrame early-out and a `gap<0` bypass, and instead of a single DrawTMD
 * call it walks `mad->object[0..n)` drawing each visible (attribute&1==0)
 * sub-model with `gap` as the DrawTMD mode.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `sz`/`result` follow DrawSprite's two-variable shape: `sz` ($v1) holds
 *    the OTZ from either RotTransPers (re-tested by the attribute&4/&0x10
 *    guards), `result` ($v0) is the "-1 = reject / else the accepted OTZ"
 *    value the tail sums with `gap` — assigned only at each exit edge, never
 *    once up front, so cc1 rematerialises `li v0,-1` per reject.
 *  - There is exactly ONE shared `reject:` block. Every reject site reaches
 *    it with `goto reject;` — including attribute&1, exactly as the matched
 *    sibling DrawSprite spells it. The asm LOOKS like four independent
 *    "direct to tail" rejects (`bcond tail` + own `li v0,-1` in the delay
 *    slot) plus one branch to a shared stub, but that is reorg's doing, not
 *    the source's: reorg steals reject's own `li v0,-1` into each eligible
 *    branch's delay slot and retargets the branch THROUGH reject's `j tail`
 *    to tail itself. The iv2>=0xb5 site (0x8001775c) still points at the real
 *    `reject:` only because its delay slot was already taken by
 *    `andi v0,s0,0x10`, so reorg had nothing to steal with. The five
 *    `li v0,-1` in the target are one real + four stolen copies.
 *    Do NOT spell any reject as its own `result = -1; goto tail;` body: a
 *    trailing `result=-1` block adjacent to `tail:` gets a free fallthrough,
 *    which is an identical instruction stream to reject's `li v0,-1; j tail`,
 *    so it absorbs reject and swaps reject with the DrawTMDmode==0 arm
 *    (44 bytes).
 *  - `if (SkipFrame == 1) goto ret1;` must be a GOTO to the function's single
 *    trailing `return 1;`, not an inline `return 1;`. An inline early return
 *    leaves a join CODE_LABEL (RTL `code_label 25`) between the `mad`
 *    parameter copy and the first `GsGetLs` call. cse's basic block ends at
 *    every CODE_LABEL, so the a0 == mad equivalence dies there and cc1 emits
 *    a redundant `move a0,s2`, which reorg then hoists into the `bltz s3`
 *    delay slot the target leaves as a bare `nop`. Spelled as a goto, the
 *    body is a lone jump, so jump.c folds the guard and deletes the label
 *    (exactly as it already folds `if (gap < 0) goto loop;`); cse's block
 *    then spans the prologue through the call and deletes the copy, leaving
 *    a0 holding the incoming `mad`. The `nop` is the SYMPTOM — with the copy
 *    gone the fallthrough block starts with the `jal`, which is ineligible
 *    for a delay slot. Verified with `tools/rtldump.py --pass rtl,jump,cse`.
 *  - `RotTransPers(&UnitVector, ...)` passes a literal NULL sxy pointer here
 *    (no sprite-relative xy to fill, unlike DrawSprite's `xy`), so there is
 *    no `if (xy != 0)` tail arm — `result = sz;` sits directly after the
 *    DrawTMDmode if/else and is the last block before `tail:`.
 *  - The tail test is `result + gap`, not `result == -1`: DrawModelArchive
 *    folds the caller's `gap` into the same sentinel arithmetic.
 *  - The sub-model loop is a plain `for (i = 0; i < mad->n; i++)` over
 *    `mad->object[i]`; cc1's own loop rotation duplicates the `0 < mad->n`
 *    entry test for free. `i` is `short`: the per-iteration address is
 *    recomputed via the fused `sll 16/sra 14` sign-extend+scale (the
 *    short-counter idiom that suppresses loop.c's strength reduction).
 */

extern void DrawTMD(GsDOBJ2 *obj, GsOT *ot, s32 mode);
extern s32 RotTransPers(SVECTOR *v0, s32 *sxy, void *p, void *flg);
extern short SkipFrame;
extern SVECTOR UnitVector;
extern GsOT *OTablePt;
extern s32 DrawTMDmode;

short DrawModelArchive(ModelArchiveType *mad, long gap)
{
    MATRIX mat;
    SVECTOR pos;
    u16 attr;
    long sz;
    long result;
    s32 iv;
    short i;
    ModelType *objp;
    short rxy[2];

    if (SkipFrame == 1)
        goto ret1;
    if (gap < 0)
        goto loop;
    GsGetLs(&mad->locate, &mat);
    GsSetLsMatrix(&mat);
    attr = mad->attribute;
    if ((attr & 1) != 0)
        goto reject;
    if ((attr & 2) == 0)
    {
        sz = RotTransPers(&mad->clip, (s32 *)rxy, 0, 0) >> 2;
        if ((attr & 4) != 0 && sz == 0)
        {
            result = -1;
            goto tail;
        }
        if ((attr & 8) != 0)
        {
            iv = rxy[0];
            if (iv < 0)
            {
                iv = -iv;
            }
            if (iv < 0xf1)
            {
                iv = rxy[1];
                if (iv < 0)
                {
                    iv = -iv;
                }
                if (iv < 0xb5)
                {
                    goto reject_check;
                }
                goto reject;
            }
            result = -1;
            goto tail;
        }
    reject_check:
        if ((attr & 0x10) != 0 && 0x4e2 < sz)
        {
            result = -1;
            goto tail;
        }
        goto unit_vector;
    }
    else
    {
    unit_vector:
        sz = RotTransPers(&UnitVector, 0, 0, 0) >> 2;
        if (sz >= 0x4e3)
        {
        reject:
            result = -1;
            goto tail;
        }
        if (sz >= 300)
            DrawTMDmode = 0x20;
        else
            DrawTMDmode = 0;
        result = sz;
    }
tail:
    if (result + gap < 0)
    {
        return 0;
    }
loop:
    for (i = 0; i < mad->n; i++)
    {
        objp = mad->object[i];
        if ((objp->attribute & 1) == 0)
        {
            GsGetLs(&objp->locate, &mat);
            GsSetLsMatrix(&mat);
            DrawTMD(&objp->object, OTablePt, gap);
        }
    }
ret1:
    return 1;
}
