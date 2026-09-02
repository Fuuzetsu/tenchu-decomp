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

extern void DrawImpact(TEffectSlot *ef);

void SetImpact(VECTOR *pos, short size, short type)
{
    short spd;
    long start_color;
    long end_color;
    int idx;
    TEffectSlot *slot;
    int count;
    ImpactType *param;
    long pz;

    spd = rand() % 90 + 90;
    start_color = COLOR_GRAY;
    end_color = COLOR_GRAY;
    FIND_EFFECT_SLOT(idx, count, slot, found);
found:
    slot->proc = DrawImpact;
    slot->param.impact.px = pos->vx;
    param = &slot->param.impact;
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
