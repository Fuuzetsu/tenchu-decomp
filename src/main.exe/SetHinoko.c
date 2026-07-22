#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetHinoko(struct VECTOR *pos, struct SVECTOR *power, int n);
 *     EFFECT.C:1321, 21 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
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
 *     param $s3       struct VECTOR * pos
 *     param $s4       struct SVECTOR * power
 *     param $s5       int n
 *     reg   $s2       short i
 *     reg   $s1       struct tag_EffectSlot * slot
 *     reg   $s0       struct ExplosionType * param
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

/*
 * MATCHED. Same TU, identical residual/fix as SetBlood: the pool-scan
 * cursor init `slot = base + idx;` needs the INTEGER-SUM spelling —
 *     slot = (TEffectSlot *)(idx * sizeof(TEffectSlot) + (int)base);
 * — to get the target's index-first `addu $a0,$v0,$s5` instead of the
 * base-first `addu $a0,$s5,$v0` that plain pointer arithmetic (`base+idx`,
 * `idx+base`, `&base[idx]`) always folds to. See SetBlood.c's header for
 * the full cookbook-rule writeup ("Pointer arithmetic normalises to
 * base+index; only INTEGER addition keeps operand order").
 *
 * Matching notes (all verified against the original bytes):
 *  - Unlike SetBlood in the same TU, retail keeps PSX.SYM's exact
 *    THREE-argument prototype (pos, power, n) — all three registers
 *    ($a0/$a1/$a2) are read at entry and no parameter was dropped.
 *  - Same EffectSlot[200] round-robin search as SetBlood/SetExplosion/
 *    SetImpact (do-while, `ef = &dmy;` sits AFTER the loop). This
 *    function's own asm has `count = count + 1;` BEFORE the
 *    `if (slot->proc == 0)` test (the "occupied" branch's delay slot
 *    unconditionally increments count) — SetExplosion's order, not
 *    SetImpact's.
 *  - The outer "spawn n particles" fill is `while (1) { if (!(i < n)) break;
 *    ...; i++; }`, NOT a hand-rolled goto (that put `base` in $s3 and shifted
 *    every param register up by one — 55 bytes of cascade). The while(1)+break
 *    form lets loop.c place `base` in $s5 exactly as the target does; loop.c
 *    still does NOT hoist the %15 magic-multiply constant (it stays inside the
 *    loop, recomputed per iteration, matching the target).
 *  - `time`'s rand() must be a SEPARATE `r = rand();` statement BEFORE `i++`
 *    and `mode = 0` (not inlined into `fp->time = rand()%15+15`): the target
 *    calls rand for time first, then does i++/mode=0, then the %15 arithmetic.
 *    Inlining floats the `mode = 0` store early (into the vec.vz division) —
 *    extracting the call to its own statement fixes 37 bytes of scheduling.
 *  - Fields store in the exact order Ghidra shows: scale, rotate, pos.vx,
 *    pos.vy, pos.vz, vec.vx, vec.vy, vec.vz, `i++`, mode, time (mode and
 *    time are two SEPARATE `sb` stores, unlike SetBlood's combined `sh` —
 *    here `time` is a runtime rand() value, not a constant, so there's
 *    nothing for cc1 to fold together).
 *  - `vec.vx`/`vec.vy`/`vec.vz` each divide by the matching `power->vx/vy/vz`
 *    component (a genuine variable division, guarded by ASPSX's
 *    break 7/break 6 — needs `--expand-div`). `vec.vy` is the odd one out:
 *    it NEGATES `(rem + half)` where the other two SUBTRACT `half` from
 *    `rem` — verified from the raw asm (`negu` only appears on the vy
 *    path), not a naming/rendering artifact.
 */
extern void DrawHinoko(TEffectSlot *ef);

void SetHinoko(VECTOR *pos, SVECTOR *power, int n)
{
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    ExplosionType *fp;
    short i;
    int r;

    i = 0;
    base = EffectSlot;
    while (1)
    {
        if (!(i < n))
        {
            break;
        }
        count = 0;
        idx = CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_;
        slot = (TEffectSlot *)(idx * sizeof(TEffectSlot) + (int)base);
        do
        {
            idx = idx + 1;
            slot = slot + 1;
            if (199 < idx)
            {
                slot = base;
                idx = 0;
            }
            count = count + 1;
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
        } while (count < 200);
        ef = &dmy;
    found:
        fp = &ef->param.hinoko;
        fp->scale = rand() % 4096 + 0x1000;
        fp->rotate = (rand() % 360) * 0x1000;
        fp->pos.vx = pos->vx;
        fp->pos.vy = pos->vy;
        fp->pos.vz = pos->vz;
        fp->vec.vx = rand() % power->vx - power->vx / 2;
        fp->vec.vy = -(rand() % power->vy + power->vy / 2);
        fp->vec.vz = rand() % power->vz - power->vz / 2;
        r = rand();
        i = i + 1;
        fp->mode = 0;
        fp->time = r % 15 + 15;
        ef->proc = (void (*)())DrawHinoko;
    }
}
