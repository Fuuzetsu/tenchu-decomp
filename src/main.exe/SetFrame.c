#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetFrame(struct VECTOR *pos, short size, short time, struct _GsCOORDINATE2 *super);
 *     EFFECT.C:1095, 17 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * pos
 *     param $a1       short size
 *     param $a2       short time
 *     param $a3       struct _GsCOORDINATE2 * super
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

extern void DrawFrame(TEffectSlot *ef);

void SetFrame(VECTOR *pos, short size, short time, GsCOORDINATE2 *super)
{
    long z;
    int idx;
    TEffectSlot *slot;
    int count;
    FrameType *fp;

    FIND_EFFECT_SLOT(idx, count, slot, found);
found:
    fp = &slot->param.frame;
    fp->px = pos->vx;
    fp->py = pos->vy;
    z = pos->vz;
    fp->mode = FRAME_MODE_FLASH;
    fp->size = size;
    fp->count = time;
    fp->pz = z;
    slot->param.frame.super = super;
    slot->proc = DrawFrame;
}
