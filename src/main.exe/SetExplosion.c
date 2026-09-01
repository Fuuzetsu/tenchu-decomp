#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetExplosion(struct VECTOR *pos, struct SVECTOR *vect);
 *     EFFECT.C:1236, 18 src lines, frame 48 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct VECTOR * pos
 *     param $s3       struct SVECTOR * vect
 *     reg   $s1       struct tag_EffectSlot * slot
 *     reg   $s0       struct ExplosionType * param
 *     reg   $v1       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

/*
 * Matching notes (all verified against the original bytes):
 *  - Same EffectSlot[200] pool search as SetImpact (see its header): a
 *    real `do { ... } while (count < N_EFFECT_SLOTS);`, not a hand-rolled goto — the
 *    give-up path's `slot = &dmy;` sits AFTER the loop, not inside it, so
 *    loop.c doesn't get a chance to hoist that address. The source indexes
 *    `EffectSlot[idx]`; loop strength reduction creates the target's scan
 *    pointer.
 *  - UNLIKE SetImpact, `count = count + 1;` here comes BEFORE the
 *    `if (slot->proc == 0)` test, not after (both Ghidra's own rendering
 *    and the raw asm's delay-slot fill agree: the branch testing
 *    `EffectSlot[idx].proc` has `count++` in its delay slot, executed regardless of
 *    outcome — only possible if count++ is the statement immediately
 *    preceding the if in source). Each EffectSlot-pool inserter in this TU
 *    apparently wrote this test/increment order slightly differently;
 *    don't assume one sibling's shape for another without checking.
 *  - `slot->param` is `ExplosionType` (see DrawExplosion.c/DrawHinoko.c):
 *    Ghidra's `blood.py/pz/scale` and `smoke.*` names are its own wrong
 *    union guess for the same proven offsets (pos@0x8, vec@0x0, time@0x20,
 *    mode@0x21).
 *  - `param->scale = FIXED_ONE;` is a plain independent constant store that
 *    floats into the `jal rand`'s delay slot (unrelated to the call);
 *    write it as the first statement, before `rand()`, matching Ghidra.
 *  - `vect->vz` is captured into a temp and stored to `param->vec.vz`
 *    LAST (after time/mode), the same delayed-store idiom as
 *    SetFrame/SetSplash/SetBleed's `pos->vz` capture.
 *  - `vect->vx/vy/vz` read via `lhu` despite SVECTOR's fields being
 *    signed `short`: a pure narrowing copy (read then immediately written
 *    back at the same 16-bit width, no arithmetic) doesn't need the sign
 *    bits, so cc1 picks the cheaper unsigned load.
 *  - `SetBleeds` takes SIX arguments (`VECTOR *pos, short grange, short
 *    srange, short n, int time, long col` — reference/psxsym-protos.h);
 *    Ghidra's call rendering drops the two stack-passed args past the
 *    4 register ones (cookbook: Ghidra under-counts stack args). The raw
 *    `.s` shows all six: pos, 200, 0x96, 0x14, 0x1e, 0xFFFF00.
 */
extern void DrawExplosion(TEffectSlot *ef);

void SetExplosion(VECTOR *pos, SVECTOR *vect)
{
    int idx;
    TEffectSlot *slot;
    int count;
    ExplosionType *param;
    int r;
    short vz;

    count = 0;
    idx = EFFECT_CURSOR_;
    do
    {
        idx++;
        if (idx >= N_EFFECT_SLOTS)
        {
            idx = 0;
        }
        count++;
        if (EffectSlot[idx].proc == 0)
        {
            EFFECT_CURSOR_ = idx + 1;
            if (EFFECT_CURSOR_ >= N_EFFECT_SLOTS)
            {
                EFFECT_CURSOR_ = 0;
            }
            slot = &EffectSlot[idx];
            goto found;
        }
    } while (count < N_EFFECT_SLOTS);
    slot = &dmy;
found:
    param = &slot->param.explosion;
    param->scale = FIXED_ONE;
    r = rand();
    param->rotate = (r % 360) * FIXED_ONE;
    param->pos.vx = pos->vx;
    param->pos.vy = pos->vy;
    param->pos.vz = pos->vz;
    param->vec.vx = vect->vx;
    param->vec.vy = vect->vy;
    vz = vect->vz;
    param->time = 5;
    param->mode = EXPLOSION_MODE_FLASH;
    param->vec.vz = vz;
    slot->proc = DrawExplosion;
    SetBleeds(pos, 200, 150, 20, 30, COLOR_YELLOW);
}
