#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetSplash(struct VECTOR *pos, short sx, short sy, int speed);
 *     EFFECT.C:1023, 17 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * pos
 *     param $a1       short sx
 *     param $a2       short sy
 *     param $a3       int speed
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

/*
 * Matching notes (see SetFrame.c for the shared indexed do-while pool scan):
 *  - splash.px is this struct's offset-ZERO field and is the first one
 *    written, so it goes through a fresh `slot->param.splash.px = ...` recast;
 *    `fp = &slot->param.splash;` is only introduced for the second field
 *    onward (all nonzero offsets), matching the target's t0-direct first
 *    store followed by a v1=t0+4 computed just before the second.
 */
extern void DrawSplash(TEffectSlot *ef);

void SetSplash(VECTOR *pos, short sx, short sy, int speed)
{
    long z;
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    SplashType *fp;

    idx = EFFECT_CURSOR_;
    count = 0;
    base = EffectSlot;
    do
    {
        idx++;
        if (idx >= N_EFFECT_SLOTS)
        {
            idx = 0;
        }
        if (base[idx].proc == 0)
        {
            EFFECT_CURSOR_ = idx + 1;
            if (EFFECT_CURSOR_ >= N_EFFECT_SLOTS)
            {
                EFFECT_CURSOR_ = 0;
            }
            slot = &base[idx];
            goto found;
        }
        count++;
    } while (count < N_EFFECT_SLOTS);
    slot = &dmy;
found:
    slot->param.splash.px = pos->vx;
    fp = &slot->param.splash;
    fp->py = pos->vy;
    z = pos->vz;
    fp->mode = SPLASH_MODE_SPAWN;
    fp->sx = sx;
    fp->sy = sy;
    fp->speed = speed;
    fp->pz = z;
    slot->proc = DrawSplash;
}
