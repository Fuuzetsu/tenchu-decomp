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
 *  - This uses the shared FIND_EFFECT_SLOT operation. Its bottom-tested loop,
 *    direct `EffectSlot[idx]` access, and post-loop fallback give loop.c the
 *    pointer walk and exhausted-pool path visible in the target. The former
 *    apparent difference in counter/test order was only instruction
 *    scheduling; the common expansion matches this function exactly.
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

    FIND_EFFECT_SLOT(idx, count, slot, found);
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
