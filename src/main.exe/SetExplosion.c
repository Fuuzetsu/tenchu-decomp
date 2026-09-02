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
