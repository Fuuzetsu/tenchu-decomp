#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetSmokeS(struct VECTOR *pos, short vx, short vy, short vz, int time);
 *     EFFECT.C:858, 14 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * pos
 *     param $a1       short vx
 *     param $a2       short vy
 *     param $a3       short vz
 *     param stack+16  int time
 * END PSX.SYM */

/* Retail narrows the demo build's int time parameter to u16. */
extern void DrawSmoke(TEffectSlot *ef);

void SetSmokeS(VECTOR *pos, short vx, short vy, short vz, unsigned short time)
{
    int idx;
    TEffectSlot *slot;
    int count;
    SmokeType *smoke;
    int r;
    int m;

    FIND_EFFECT_SLOT(idx, count, slot, found);
found:
    smoke = &slot->param.smoke;
    smoke->scale = rand() % SMOKE_SCALE_SPREAD + SMOKE_SCALE_MIN;
    smoke->rotate = (rand() % 360) * FIXED_ONE;
    smoke->pos.vx = pos->vx;
    smoke->pos.vy = pos->vy;
    smoke->pos.vz = pos->vz;
    smoke->vec.vx = vx;
    smoke->vec.vy = vy;
    smoke->vec.vz = vz;
    smoke->time = time;
    r = rand();
    smoke->sprite = SMOKE_SPRITE_NORMAL;
    m = smoke->time - 1;
    smoke->evtime = m - ((short)time / 2 + r % (short)time);
    slot->proc = DrawSmoke;
}
