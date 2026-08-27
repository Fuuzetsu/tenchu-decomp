#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetSpline(struct SVECTOR *vect, struct SplineControlType *spc, short cnt);
 *     ACTION.C:388, 34 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $s2       struct SVECTOR * vect
 *     param $s0       struct SplineControlType * spc
 *     param $s1       short cnt
 * END PSX.SYM */

/*
 * GetSpline (0x8001c170, 0x150 bytes) — advance a bone's spline bracket
 * (spc->key0/key1) so it straddles `cnt`, re-deriving the per-frame deltas
 * (UpdateSplineControl) whenever the bracket actually moved, then evaluate
 * the spline at `cnt` via eval_spline_gte_ (independently guarded), using a
 * memoized fixed-point interpolation fraction (SplineFrac, cached in
 * SplineFracOld so SplineRow's table-row address is only recomputed when the
 * fraction changes). PSX.SYM's globals (StageMotion/FieldIndex/Command)
 * belong to the demo build's differently-shaped callee — this retail body
 * doesn't reference them directly.
 *
 * STATUS: MATCHED — exact retail bytes (336 bytes, 84 instructions).
 *
 * Matching notes: each do-while keeps the current `key` and derived `next`
 * as distinct source identities. Assigning `key = next` after publishing
 * `next` into the control block becomes the loop branch's delay-slot move;
 * it also frees the incoming `$a0` early enough for cc1 to place `key` there
 * and save `vect` into `$s2` at the target's second instruction. Collapsing
 * both identities into one incremented pointer produces the right behavior
 * but a different loop schedule.
 *
 * The fraction is one direct `/` expression. Build.hs's per-file
 * --expand-div supplies ASPSX's bnez/break 7/break 6 guards; they are not
 * hand-written C. Updating SplineFracOld before SplineRow preserves the target
 * store order. SplineTable is a large, non-`-G8`-small table, so its address
 * fully materializes with `lui/addiu`. eval_spline_gte_'s separate GTE-backed
 * implementation remains guarded; this matched caller contains only C.
 */
extern void UpdateSplineControl(SplineControlType *spc);
extern void eval_spline_gte_(SVECTOR *vect, SplineControlType *spc, s32 row);
extern s16 SplineFracOld;
extern s16 SplineFrac;
extern s32 SplineRow;
extern u8 SplineTable[];

void GetSpline(SVECTOR *vect, SplineControlType *spc, short cnt)
{
    MotionElementType *key;
    MotionElementType *next;

    key = spc->key1;
    if (key->time < cnt) {
        do {
            next = key + 1;
            spc->key1 = next;
            key = next;
        } while (next->time < cnt);
        spc->key0 = next - 1;
    } else {
        key = spc->key0;
        if (key->time <= cnt) {
            goto skip;
        }
        do {
            next = key - 1;
            spc->key0 = next;
            key = next;
        } while (cnt < next->time);
        spc->key1 = next + 1;
    }
    UpdateSplineControl(spc);
skip:
    SplineFrac = (s16)(((cnt - spc->key0->time) * 0x20) /
                      (spc->key1->time - spc->key0->time));
    if ((s32)SplineFracOld != (s32)SplineFrac) {
        SplineFracOld = SplineFrac;
        SplineRow = (s32)(SplineTable + SplineFrac * 8);
    }
    eval_spline_gte_(vect, spc, SplineRow);
}
