#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetBleed(struct VECTOR *pos, struct SVECTOR *vec, int time, long col);
 *     EFFECT.C:946, 14 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * pos
 *     param $a1       struct SVECTOR * vec
 *     param $a2       int time
 *     param $a3       long col
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

extern void DrawBleed(TEffectSlot *ef);

void SetBleed(VECTOR *pos, SVECTOR *vec, int time, long col)
{
    int idx;
    TEffectSlot *slot;
    int count;
    BleedType *param;
    u8 r;

    FIND_EFFECT_SLOT(idx, count, slot, found);
found:
    param = &slot->param.bleed;
    r = col >> 16;
    slot->param.bleed.pos = *pos;
    slot->param.bleed.vec = *vec;
    param->r = r;
    param->g = col >> 8;
    param->time = time;
    param->b = col;
    param->mode = 0;
    slot->proc = DrawBleed;
}
