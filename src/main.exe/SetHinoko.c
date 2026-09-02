#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetHinoko(struct VECTOR *pos, struct SVECTOR *power, int n);
 *     EFFECT.C:1321, 21 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * Matching notes (all verified against the original bytes):
 *  - Unlike SetBlood in the same TU, retail keeps PSX.SYM's exact
 *    THREE-argument prototype (pos, power, n) — all three registers
 *    ($a0/$a1/$a2) are read at entry and no parameter was dropped.
 *  - FIND_EFFECT_SLOT is the same round-robin search used by SetBlood,
 *    SetExplosion, and SetImpact. Its direct `EffectSlot[idx]` access lets
 *    loop strength reduction generate the target's scan pointer instead of
 *    exposing that compiler cursor in the source.
 *  - The outer "spawn n particles" fill is `while (1) { if (!(i < n)) break;
 *    ...; i++; }`, NOT a hand-rolled goto (that put the generated pool base in
 *    $s3 and shifted every parameter register up by one — 55 bytes of cascade).
 *    The while(1)+break form lets loop.c place that base in $s5 exactly as the
 *    target does; loop.c
 *    still does NOT hoist the %15 magic-multiply constant (it stays inside the
 *    loop, recomputed per iteration, matching the target).
 *  - `time`'s rand() must be a SEPARATE `r = rand();` statement BEFORE `i++`
 *    and `mode = 0` (not inlined into `param->time = rand()%15+15`): the target
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
    TEffectSlot *slot;
    int count;
    ExplosionType *param;
    short i;
    int r;

    i = 0;
    while (1)
    {
        if (i >= n)
        {
            break;
        }
        FIND_EFFECT_SLOT(idx, count, slot, found);
    found:
        param = &slot->param.hinoko;
        param->scale = rand() % FIXED_ONE + FIXED_ONE;
        param->rotate = (rand() % 360) * FIXED_ONE;
        param->pos.vx = pos->vx;
        param->pos.vy = pos->vy;
        param->pos.vz = pos->vz;
        param->vec.vx = rand() % power->vx - power->vx / 2;
        param->vec.vy = -(rand() % power->vy + power->vy / 2);
        param->vec.vz = rand() % power->vz - power->vz / 2;
        r = rand();
        i++;
        param->mode = EXPLOSION_MODE_FLASH;
        param->time = r % 15 + 15;
        slot->proc = DrawHinoko;
    }
}
