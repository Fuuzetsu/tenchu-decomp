#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetBlood(struct VECTOR *pos, struct SVECTOR *vect, short n, short time);
 *     EFFECT.C:745, 45 src lines, frame 64 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct VECTOR * pos
 *     param $s5       struct SVECTOR * vect
 *     param $s7       short n
 *     param $fp       short time
 *     reg   $s4       short i
 *     reg   $s6       struct AreaNodeType * hint
 *     reg   $s1       struct tag_EffectSlot * slot
 *     reg   $s0       struct BloodType * blood
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct tag_EffectSlot EffectSlot[200];
 *     extern struct AreaNodeType *FieldArea;
 * END PSX.SYM */

/*
 * MATCHED. The pool-scan cursor's initial address computation `slot = base +
 * idx;` needed the INTEGER-SUM spelling, not pointer arithmetic:
 *     slot = (TEffectSlot *)(idx * sizeof(TEffectSlot) + (int)base);
 * Plain `base + idx` pointer arithmetic folds to a base-first `addu $a0,
 * $s6,$v0` regardless of source spelling (`base+idx`, `idx+base`,
 * `&base[idx]` all identical post-fold); the target has index-first `addu
 * $a0,$v0,$s6`. Cookbook rule confirmed here: "Pointer arithmetic normalises
 * to base+index; only INTEGER addition keeps operand order" — spelling the
 * same address as an explicit `idx*sizeof + (int)base` integer sum (instead
 * of `base[idx]`/`base+idx`) reaches the index-first `addu` because integer
 * PLUS doesn't get the pointer-arithmetic canonicalization fold does to
 * array/pointer addition. This was previously (wrongly) parked as
 * "permuter-immune, sub-C-level, no source lever" — it had a lever, just not
 * the one tried (reordering statements/loop shape instead of the operand's
 * own type). SetHinoko (same TU, identical residual) fixed the same way.
 *
 * Matching notes (all verified against the original bytes):
 *  - The demo's PSX.SYM records a FOUR-argument prototype (pos, vect, n,
 *    time). Retail's three callers (ActDAMAGE, CVAupdate, DamageControl x2)
 *    each set only $a0/$a1/$a2 before `jal SetBlood`, and the callee itself
 *    never reads $a3 — the SVECTOR* "spread" parameter was dropped between
 *    the demo and retail builds (every per-axis jitter below now comes
 *    straight from rand(), with no live use of a spread vector anywhere in
 *    the body). Retail SetBlood is a plain THREE-argument
 *    `void SetBlood(VECTOR *pos, short n, short time)` — verified from the
 *    callers' own register setup, not assumed from PSX.SYM.
 *  - GetAreaMapLevel's return is discarded (called for a side effect only);
 *    `hint = FieldArea;` is a separate global read right after, not
 *    GetAreaMapLevel's result. Ghidra drops GetAreaMapLevel's stack-passed
 *    5th arg (the `0` mode flag) — cookbook: Ghidra undercounts stack args.
 *  - The whole per-particle fill (spawn `n` particles) is a HAND-ROLLED
 *    `outer: if (!(i < n)) goto end; ...; i++; goto outer; end:` — not a
 *    real while/for and not even while(1)+break. A genuine loop here lets
 *    loop.c hoist the shared %120/%60 magic-multiply constant AND the
 *    `time/2` split (both loop-invariant across outer iterations, neither
 *    depends on `i`) all the way to the function's prologue; the target
 *    recomputes both fresh every outer iteration, right where they're first
 *    used. Only the hand-rolled goto form (no loop notes at all) suppresses
 *    that invariant motion (cookbook: "a top-test loop that never hoists its
 *    invariants is a hand-rolled goto loop, not while(1)+break").
 *  - The inner EffectSlot[200] search is the same round-robin do-while as
 *    SetExplosion/SetImpact (`ef = &dmy;` sits AFTER the loop). Verified
 *    from THIS function's own asm (don't assume a sibling's shape): the
 *    "occupied" branch's delay slot unconditionally increments `count`
 *    regardless of outcome, so `count = count + 1;` sits BEFORE the
 *    `if (slot->proc == 0)` test — SetExplosion's order, not SetImpact's.
 *  - Field store order follows Ghidra's own rendering exactly: sprite,
 *    scale, rotate, px, py, pz, vx, vy, vz, time (branch), `i++`, a
 *    brightness halfword store, hint, mode, and `proc` last (it lands in
 *    the closing loop-jump's delay slot).
 *  - Retail replaced the demo's adjacent `mode`/`bright` bytes with the
 *    halfword `brightness` at +0x20, so the 0x80 initialization is one `sh`.
 *  - `time`'s jitter needs a genuine variable division (`rand() % half2`),
 *    guarded by ASPSX's break 7/break 6 — needs `--expand-div`
 *    (Build.hs/permute.py).
 */
extern void DrawBlood(TEffectSlot *ef);

void SetBlood(VECTOR *pos, short n, short time)
{
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    BloodType *blood;
    struct AreaNodeType *hint;
    short i;
    int half;
    int half2;

    GetAreaMapLevel(GlobalAreaMap, pos->vx, pos->vy, pos->vz,
                    AREA_LEVEL_DEFAULT);
    hint = FieldArea;
    base = EffectSlot;
    i = 0;
outer:
    if (i >= n)
    {
        goto end;
    }
    {
        count = 0;
        idx = EFFECT_CURSOR_;
        slot = (TEffectSlot *)(idx * sizeof(TEffectSlot) + (int)base);
        do
        {
            idx++;
            slot++;
            if (idx > N_EFFECT_SLOTS - 1)
            {
                slot = base;
                idx = 0;
            }
            count++;
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
        } while (count < N_EFFECT_SLOTS);
        ef = &dmy;
    found:
        blood = &ef->param.blood;
        do
        {
            blood->sprite = rand() % N_AIRBORNE_BLOOD_SPRITES;
            blood->scale = rand() % FIXED_ONE + 2 * FIXED_ONE;
            blood->rotate = (rand() % 360) * FIXED_ONE;
            blood->px = pos->vx;
            blood->py = pos->vy;
            blood->pz = pos->vz;
            blood->vx = rand() % 120 - 60;
            blood->vy = rand() % 60 - 120;
            blood->vz = rand() % 120 - 60;
            half = time / 2;
            half2 = time - half;
            if (half2 > 0)
            {
                blood->time = rand() % half2 + half;
            }
            else
            {
                blood->time = half;
            }
            i++;
            blood->brightness = 0x80;
            blood->hint = hint;
            blood->mode = BLOOD_MODE_AIRBORNE;
            ef->proc = DrawBlood;
        } while (0);
    }
    goto outer;
end:
    return;
}
