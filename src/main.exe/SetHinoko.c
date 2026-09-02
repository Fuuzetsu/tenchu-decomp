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
