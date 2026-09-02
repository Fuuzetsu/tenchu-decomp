#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetSmoke(struct VECTOR *pos, struct SVECTOR *vect, short n, short time);
 *     EFFECT.C:835, 21 src lines, frame 56 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s5       struct VECTOR * pos
 *     param $s6       struct SVECTOR * vect
 *     param $s7       short n
 *     param $s3       short time
 *     reg   $s4       short i
 *     reg   $s1       struct tag_EffectSlot * slot
 *     reg   $s0       struct SmokeType * smoke
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

extern void DrawSmoke(TEffectSlot *ef);

void SetSmoke(VECTOR *pos, SVECTOR *vect, short n, short time)
{
    short i;
    int idx;
    TEffectSlot *slot;
    int count;
    SmokeType *smoke;
    int r;
    int m;

    i = 0;
    do
    {
        if (i >= n)
        {
            return;
        }
        FIND_EFFECT_SLOT(idx, count, slot, found);
    found:
        smoke = &slot->param.smoke;
        r = rand();
        smoke->scale = r % SMOKE_SCALE_SPREAD + SMOKE_SCALE_MIN;
        smoke->rotate = (rand() % 360) * FIXED_ONE;
        smoke->pos.vx = pos->vx;
        smoke->pos.vy = pos->vy;
        smoke->pos.vz = pos->vz;
        smoke->vec.vx = vect->vx + (rand() % 100 - 50);
        smoke->vec.vy = vect->vy + (rand() % 100 - 50);
        smoke->vec.vz = vect->vz + (rand() % 100 - 50);
        smoke->time = time + rand() % 160;
        r = rand();
        i++;
        smoke->sprite = SMOKE_SPRITE_NORMAL;
        m = smoke->time - 1;
        smoke->evtime = m - (time / 2 + r % time);
        slot->proc = DrawSmoke;
    } while (1);
}
