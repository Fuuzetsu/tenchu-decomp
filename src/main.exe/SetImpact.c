#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetImpact(struct VECTOR *pos, short size, short type);
 *     EFFECT.C:893, 13 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
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
 *     param $a0       struct VECTOR * pos
 *     param $a1       short size
 *     param $a2       short type
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

/*
 * Matching notes (all verified against the original bytes):
 *  - Unlike SetFrame/SetSplash/SetBleed's hand-rolled `goto loop;`, this
 *    EffectSlot[200] search is a REAL `do { ... } while (count < 200);`
 *    with `ef = &dmy;` placed AFTER the loop (fallthrough on exhaustion),
 *    not a while(1)+break with `ef = &dmy;` inside the loop body. The
 *    target's give-up path has a tell-tale `addiu idx,idx,1` / `addiu
 *    idx,idx,-1` pair: reorg steals the loop head's `idx = idx + 1` into
 *    the backjump's delay slot (retargeting the branch to skip it) and
 *    patches the fallthrough (loop-exhausted) path with a compensating
 *    decrement — the "wrap-around search loop" idiom (cookbook, Loops),
 *    which only appears with a genuine bottom-tested do-while (loop
 *    notes), not the hand-rolled goto shape. Since &dmy here is used
 *    strictly AFTER the loop (not on a conditional path INSIDE it), a
 *    real loop doesn't risk loop.c hoisting its address either — the
 *    hoisting hazard that forced SetFrame/SetSplash/SetBleed's goto shape
 *    doesn't apply once `ef = &dmy;` moves outside the loop body.
 *  - rand()'s result (`spd`), and the two packed colour constants
 *    `start_color`/`end_color`
 *    (0x808080 each), are all named locals assigned BEFORE the loop and
 *    held live across the whole search (no calls run inside it) — not
 *    literals at their point of use. All three floated only after the
 *    magic-multiply div-by-90 sequence for `spd` was written FIRST in
 *    source (immediately after `r = rand();`), then the two colours;
 *    the compiler's scheduler places their independent lui/ori pairs
 *    ahead of the still-latency-bound `mult`/`mfhi` chain regardless, so
 *    getting the register (t1/t2/t3) assignment right needed this order,
 *    not just declaring them anywhere before the loop.
 *  - The offset-zero `px` store goes through the slot directly (the impact
 *    pointer isn't computed yet), and only the `pos->vz` capture is delayed
 *    to the very end. Every other impact field, including the two 0x808080
 *    colour words, stores immediately in offset order.
 */
extern void DrawImpact(TEffectSlot *ef);

void SetImpact(VECTOR *pos, short size, short type)
{
    int r;
    short spd;
    long start_color;
    long end_color;
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    ImpactType *param;
    long py;

    r = rand();
    spd = r % 90 + 90;
    start_color = 0x808080;
    end_color = 0x808080;
    count = 0;
    base = EffectSlot;
    idx = CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_;
    slot = base + idx;
    do
    {
        idx = idx + 1;
        slot = slot + 1;
        if (199 < idx)
        {
            slot = base;
            idx = 0;
        }
        if (slot->proc == 0)
        {
            CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = idx + 1;
            if (199 < idx + 1)
            {
                CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = 0;
            }
            ef = slot;
            goto found;
        }
        count = count + 1;
    } while (count < 200);
    ef = &dmy;
found:
    ef->proc = (void (*)())DrawImpact;
    ef->param.impact.px = pos->vx;
    param = &ef->param.impact;
    param->py = pos->vy;
    py = pos->vz;
    param->super = 0;
    param->rotate = 0;
    param->rotate_speed = spd;
    param->start_color.word = start_color;
    param->end_color.word = end_color;
    param->start_size = size;
    param->end_size = 0;
    param->time = 0xf;
    param->count = 0;
    param->type = type;
    param->pz = py;
}
