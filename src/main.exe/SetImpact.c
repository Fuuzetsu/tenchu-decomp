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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 *    EffectSlot[200] search is a REAL `do { ... } while (count < N_EFFECT_SLOTS);`
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
 *  - The randomized speed (`spd`) and the two packed colour constants
 *    `start_color`/`end_color`
 *    (COLOR_GRAY each), are all named locals assigned BEFORE the loop and
 *    held live across the whole search (no calls run inside it) — not
 *    literals at their point of use. All three floated only after the
 *    magic-multiply div-by-90 expression for `spd` was written FIRST in
 *    source, then the two colours;
 *    the compiler's scheduler places their independent lui/ori pairs
 *    ahead of the still-latency-bound `mult`/`mfhi` chain regardless, so
 *    getting the register (t1/t2/t3) assignment right needed this order,
 *    not just declaring them anywhere before the loop.
 *  - The offset-zero `px` store goes through the slot directly (the impact
 *    pointer isn't computed yet), and only the `pos->vz` capture is delayed
 *    to the very end. Every other impact field, including the two COLOR_GRAY
 *    colour words, stores immediately in offset order.
 */
extern void DrawImpact(TEffectSlot *ef);

void SetImpact(VECTOR *pos, short size, short type)
{
    short spd;
    long start_color;
    long end_color;
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    ImpactType *param;
    long pz;

    spd = rand() % 90 + 90;
    start_color = COLOR_GRAY;
    end_color = COLOR_GRAY;
    count = 0;
    base = EffectSlot;
    idx = EFFECT_CURSOR_;
    slot = base + idx;
    do
    {
        idx++;
        slot++;
        if (idx > N_EFFECT_SLOTS - 1)
        {
            slot = base;
            idx = 0;
        }
        if (slot->proc == 0)
        {
            EFFECT_CURSOR_ = idx + 1;
            if (N_EFFECT_SLOTS - 1 < idx + 1)
            {
                EFFECT_CURSOR_ = 0;
            }
            ef = slot;
            goto found;
        }
        count++;
    } while (count < N_EFFECT_SLOTS);
    ef = &dmy;
found:
    ef->proc = (void (*)())DrawImpact;
    ef->param.impact.px = pos->vx;
    param = &ef->param.impact;
    param->py = pos->vy;
    pz = pos->vz;
    param->super = 0;
    param->rotate = 0;
    param->rotate_speed = spd;
    param->start_color.word = start_color;
    param->end_color.word = end_color;
    param->start_size = size;
    param->end_size = 0;
    param->time = 15;
    param->count = 0;
    param->type = type;
    param->pz = pz;
}
