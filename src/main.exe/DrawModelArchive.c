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
 * STATUS: NON_MATCHING — 48 of 488 bytes differ (same instruction COUNT —
 * length matches exactly; every differing instruction is a duplicate/
 * reordering of an instruction the draft already has elsewhere, never a
 * wrong value or missing logic).
 *
 * DrawModelArchive (0x8001768c) — same TU as DrawModel.c/DrawSprite.c
 * (3DCTRL.C): the ModelArchiveType twin of DrawModel's visibility gauntlet
 * (same GsGetLs+GsSetLsMatrix / attribute&1/2/4/8/0x10 clip test / UnitVector
 * RotTransPers), gated by a demo-only `SkipFrame` early-out and a `gap<0`
 * bypass, and instead of a single DrawTMD call it walks `mad->object[0..n)`
 * drawing each visible (attribute&1==0) sub-model with `gap` as the DrawTMD
 * mode.
 *
 * Residual root cause (RTL-verified, see docs/matching-cookbook.md "Reading
 * cc1's RTL dumps"): TWO independent ties, both downstream of source, both
 * confirmed by `tools/rtldump.py DrawModelArchive --draft --pass all`:
 *  1. (4 bytes) The `if (gap < 0) goto loop;` guard's delay slot: target
 *     leaves it a bare `nop`; this draft's reorg fills it with the
 *     GsGetLs first-argument `move a0,s2` (harmless either way — a0 already
 *     equals mad from the caller's own convention). This is the cookbook's
 *     NAMED "guard's delay-slot fill tie" class (Iteration protocol §5,
 *     StickonCheck) — reorg picks between two equally-valid fillers with no
 *     source lever.
 *  2. (44 bytes) The `reject`/DrawTMDmode-arm block ORDER at the tail: target
 *     places `reject` (2 predecessors: the box check's iv2>=0xb5 fail, and
 *     unit_vector's own sz>=0x4e3 fail) EARLY, right after the sz>=0x4e3
 *     test, needing its own `j tail` — and the DrawTMDmode==0 arm LAST,
 *     falling straight into `tail:` with no jump. This draft's cc1 instead
 *     merges the shared `reject` body to be LAST/adjacent-to-tail, forcing
 *     the DrawTMDmode==0 arm to need its own jump instead — same
 *     instructions, same length, swapped position. Traced via
 *     `tools/rtldump.py`: the `.jump`-pass RTL already has target's own
 *     order (`if_then_else (eq reg 0) (pc) (label_ref DrawTMDmode)` — reject
 *     is the fallthrough, matching target); the swap happens LATER, between
 *     `.sched2` and `.dbr` (confirmed absent in `.sched2`, present in
 *     `.dbr`) — i.e. it's reorg/jump2's OWN cross-jump merge-candidate
 *     choice among the several byte-identical `result = -1; goto tail;`
 *     sites, not a source-level decision. Read gcc-2.8.1's own `jump.c`
 *     (`find_cross_jump`/`do_cross_jump`, the `simplejump_p` "cross jumping
 *     of unconditional jumps" path with its `jump_chain` same-target scan):
 *     candidate jumps whose OWN preceding block is a lone `if(cond){A;goto
 *     L;}` guard get folded into a compact conditional branch EARLIER in the
 *     SAME pass (dropping out of the jump_chain entirely, matching why
 *     attribute&1/&4/iv1/reject_check each keep their OWN separate `li
 *     v0,-1` — confirmed, none of them merge with anything); `reject`
 *     survives as a genuine standalone `simplejump` (it has 2 predecessors,
 *     not foldable into one guard) and is the one instruction stream cross-
 *     jump's minimum-length scan finds a match against. Tried and disproved
 *     as REAL levers (all byte-IDENTICAL output, confirming the tie is
 *     below the C AST): (a) `goto reject;` vs a literal duplicated
 *     `result=-1;goto tail;` at the iv2>=0xb5 site — same bytes; (b) an
 *     explicit trailing `goto tail;` after `result = sz;` vs relying on
 *     implicit fallthrough — same bytes; (c) inverting the whole `if
 *     (sz>=0x4e3) {reject} DrawTMDmode-if/else` to `if (sz<0x4e3)
 *     {DrawTMDmode-if/else; goto tail;} reject:` (textually swapping which
 *     block is "last") — same bytes; (d) reverting attribute&1 to its
 *     un-optimized direct-literal form (confirming issue 1 and issue 2 are
 *     independent — reject's mis-position persists either way, and with
 *     attribute&1 broken it instead merges into attribute&1's OWN leftover
 *     `j;li` island, further evidence this is pure cross-jump candidate
 *     selection, not a source shape). `tools/permute.py` ran one bounded
 *     (300 s, -j4) foreground pass; no candidate reported (permuter
 *     transforms the C AST — this divergence is chosen in `jump2`, a later
 *     RTL pass, so it cannot reach it, consistent with the cookbook's
 *     characterization of this residual class).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - The reject-site split follows DrawSprite's `sz`/`result` two-variable
 *    shape: `sz` ($v1) holds the OTZ from either RotTransPers (re-tested by
 *    the attribute&4/&0x10 guards), `result` ($v0) is the "-1 = reject / else
 *    the accepted OTZ" value the tail sums with `gap` — assigned only at
 *    each exit edge (never once up front), so cc1 rematerialises `li v0,-1`
 *    at each direct reject and leaves `result` caller-saved.
 *  - FOUR reject sites are DIRECT branches straight to the tail's sum-test
 *    (own `result = -1;` in the branch's own delay slot): attribute&1 set,
 *    (attribute&4 set && sz==0), the box check's FIRST threshold
 *    (iv1>=0xf1), and reject_check (attribute&0x10 set && sz>0x4e2). TWO
 *    reject sites SHARE one physical stub (`li v0,-1; j tail`) placed inside
 *    unit_vector: the box check's SECOND threshold (iv2>=0xb5) via an
 *    explicit `goto reject;`, and unit_vector's own sz>=0x4e3 fail (the
 *    guard's fallthrough, which the `reject:` label sits on). This is a
 *    slightly different direct/shared split than DrawSprite (whose
 *    attribute&1 test shares the same stub) — read off the actual branch
 *    TARGETS, not assumed from the sibling.
 *  - `RotTransPers(&UnitVector, ...)` passes a literal NULL sxy pointer here
 *    (there's no sprite-relative xy address to fill, unlike DrawSprite's
 *    `xy`), so there is no `if (xy != 0)` tail arm — `result = sz;` sits
 *    directly after the DrawTMDmode if/else.
 *  - The tail test is `result + gap`, not `result == -1`: DrawModel/
 *    DrawSprite's callers pass a fixed mode, but DrawModelArchive folds the
 *    caller's `gap` into the same sentinel arithmetic (`-1 + gap < 0` is
 *    true for any `gap >= 0`, so a genuine reject always returns 0; a
 *    accepted OTZ only rejects if it's very negative relative to `gap`,
 *    which never happens for a valid OTZ — the two-variable split still
 *    matters for the internal &4/&0x10/unit_vector re-tests).
 *  - The sub-model loop is a plain `for (i = 0; i < mad->n; i++)` over
 *    `mad->object[i]` — cc1's own loop rotation duplicates the `0 < mad->n`
 *    entry test for free (no hand-written outer `if`, the KillHumanoid
 *    lever). `i` is `short`: the per-iteration address is recomputed from
 *    `mad->object` + `i*4` via the fused `sll 16/sra 14` sign-extend+scale
 *    (the short-counter idiom that suppresses loop.c's strength reduction —
 *    same family as SetupMotionRegist's `(i<<16)>>13`, here `>>14` for a
 *    4-byte pointer element).
 */

extern void GsGetLs(GsCOORDINATE2 *coord, MATRIX *m);
extern void GsSetLsMatrix(MATRIX *m);
extern void DrawTMD(GsDOBJ2 *obj, GsOT *ot, s32 mode);
extern s32 RotTransPers(SVECTOR *v0, s32 *sxy, void *p, void *flg);
extern short SkipFrame;
extern SVECTOR UnitVector;
extern GsOT *OTablePt;
extern s32 DrawTMDmode;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DrawModelArchive", DrawModelArchive);
#else
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
        return 1;
    if (gap < 0)
        goto loop;
    GsGetLs(&mad->locate, &mat);
    GsSetLsMatrix(&mat);
    attr = mad->attribute;
    if ((attr & 1) == 0)
    {
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
    }
    else
    {
        result = -1;
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
    return 1;
}
#endif
