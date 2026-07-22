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
 * HISTORICAL 47-BYTE PARK (superseded) was ONE clean mirror swap: sched1 (the
 * PRE-RA scheduler) dragged `dy`/`dz`'s loads from the top of the merge block
 * to the bottom, shortening their live range so global-alloc gave them v0/v1
 * while `lui %hi(ViewInfo)` took the block-leader slot and a1/a2 instead of
 * `dy`/`dz`. That park correctly proved `.flow` already matches the target (so no
 * STATEMENT reorder inside the block can fix it — sched1 overrides source
 * order) and that the lever must be sched1's PRIORITY model. It did not have
 * `tools/sched-deps.py`/`tools/rtlguide.py` yet (both landed after that
 * park) and its one permuter run predated `d02da20` (permute.py's own
 * CC_FLAGS mirror still had `-fno-builtin` in the wrong flag group, so that
 * negative was searching a DIFFERENT PROGRAM than the build — void).
 *
 * The historical 47 -> 8 checkpoint introduced a genuinely SEPARATE pseudo
 * for the value `param` already holds, introduced right where sched1 was
 * choosing to sink `dy`/`dz`. A fresh bounded permuter run (post-fno-builtin-
 * fix) found it census-first as a random-variable mutation; the win was
 * verified by porting the semantic delta and re-measuring with
 * `tools/matchdiff.py`, never trusting the permuter's own proxy score:
 *
 *   1. `param2 = param;` seeded at the tail of BOTH inner branches (the
 *      `time==0` -> `ef->proc=0` arm and the velocity-update `else` arm),
 *      then `dy = param2->pos.vy; dz = param2->pos.vz;` after the if. Having
 *      a second RTL identity for the same pointer value changes sched1's
 *      priority computation enough that `dy`/`dz` land early again (47->12).
 *      Per `tools/regalloc.py`'s own diagnosis ("gcc-2.8.1 has NO coalescing
 *      pass ... An alias copy survives ONLY if the alias CONFLICTS with its
 *      source"): `param` is still live after the copy (needed for `dx` and
 *      later `r`/`g`/`b`), so the copy DOES conflict and survives as a real
 *      `move` — that's the residual's fixed cost from here on.
 *   2. `savedTime = param->time;` seeded BEFORE the `dy`/`dz` reads, with
 *      `param->time = savedTime - 1;` at the original position — the
 *      "compute into a named temp before an intervening read" idiom
 *      (§3.13, AfsGetHeader family) fixed the remaining `time`-vs-`dy`
 *      ordering tie (12->8).
 *
 * Its former residual (8 bytes) was all one issue — `param2` needed its OWN
 * register because it conflicts with `param` (proven above), and the allocator
 * picks a2 instead of coalescing with s1:
 *
 *              TARGET                          OURS
 *   0x3ac  nop                             move  a2,s1        (param2=param)
 *   0x400  lw a1,4(s1)  pos.vy             lw a1,4(a2)
 *   0x404  lw a2,8(s1)  pos.vz             lw a2,8(a2)
 *
 * `tools/rtlguide.py DrawBleed` named this precisely: owner
 * cse/coalescing+regalloc, register goal "a2 -> s1 x2".  The previous round
 * called that unreachable, but only after holding the invented alias structure
 * fixed.  The exact source above disproves the conclusion; these three tests
 * remain useful only as a record of that local minimum:
 *   - `tools/autorules.py DrawBleed --guided` (the rules rtlguide names for
 *     this exact signature: ptr-base-split, deref-address-split,
 *     disjoint-local-alias, identical-arm-fence, etc., beam depth 2, 160
 *     compiled candidates): no improving edit.
 *   - Covering the THIRD path too (`param2 = param;` also on the implicit
 *     `mode!=0` fast path, as an `else` on the outer `if`) costs +2 bytes
 *     (8->10): that path's `bnez` delay slot is already spoken for by the
 *     ViewInfo `lui` in the target, with provably zero spare room.
 *   - Making `param` fully dead after the copy (routing `dx` and the
 *     `r`/`g`/`b` reads through `param2` too, so the alias no longer
 *     conflicts, satisfying regalloc's own coalescing condition) does not
 *     coalesce the copy away — it costs a LENGTH MISMATCH (536 vs 532)
 *     instead. The register pressure moves, it does not disappear.
 *
 * A REJECTED further permuter find (8 -> 4, found twice independently by
 * fresh bounded runs seeded from the 12- and 8-byte checkpoints): deleting
 * the `param2 = param;` assignment ENTIRELY (from both branches, leaving
 * `param2` read via `dy = param2->pos.vy` with no reaching definition on ANY
 * path) links to 4 whole-image differing bytes. This is a genuinely
 * uninitialized local — gcc-2.8.1 has no def for `param2`'s pseudo, so
 * global-alloc records no conflict against it and happens to hand it s1,
 * making the read coincidentally correct FOR THIS EXACT COMPILATION. It is
 * not adopted: every well-defined attempt to reproduce the same "no
 * conflict" state (see above) either failed to move the bytes or cost length,
 * and a construct whose correctness depends on an absent conflict edge
 * rather than a proven value is exactly the "scores better while being
 * further from the target's shape" trap the permuter contract warns about.
 * Left here as a lead: if a later round finds a WELL-DEFINED source shape
 * that reaches 4 (or 0), the candidate is banked at
 * `.shake/permuter/DrawBleed/output-10-1/source.c` (regenerated per-run, not
 * committed) — the transformation to reproduce is
 * `git diff` against this file with both `param2 = param;` lines removed
 * and `param->vec.vy = param2->vec.vy + 1;` reverted to `param->vec.vy += 1`.
 *
 * LOAD WIDTH is separately settled and must not be regressed: `pos.vx/vy/vz`
 * (VECTOR, `long`) must be captured full-width before the narrowing
 * scratchpad store; the y/z scalar `s32` views are also what prevent sched1
 * from sinking those loads.  An early narrowing cast instead emits `lhu`.
 *
 * Also measured and rejected inside the original 47-byte decomposition (not
 * claims about other source structures): struct-typed scratchpad casts (CSE
 * folds the base, -> 496);
 * plain statement reorders inside the merge block (sched1 overrides them,
 * -> 512); x/y/z holding differences instead of raw positions (-> 512);
 * dropping the ViewInfo `(short)` casts (-> 512); volatile scratchpad
 * stores (536 / 524 / 524, non-monotonic, no form found that lands 532).
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
                                 (void *)TENCHU_SCRATCHPAD(0x28),
                                 (void *)TENCHU_SCRATCHPAD(0x2c));

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
