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
 * Matching notes (see SetFrame.c for the full writeup of the shared
 * EffectSlot pool-search idioms — goto loop instead of while(1)+break so
 * loop.c doesn't hoist &dmy's address, idx-computed-before-slot so idx/slot
 * land in the target's t0/v1 pair, and the cursor-update store living inside
 * `if (slot->proc==0){...break;}` for the right branch polarity):
 *  - splash.px is this struct's offset-ZERO field and is the first one
 *    written, so it goes through a fresh `ef->param.splash.px = ...` recast;
 *    `fp = &ef->param.splash;` is only introduced for the second field
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
    TEffectSlot *ef;
    SplashType *fp;

    idx = EFFECT_CURSOR_;
    count = 0;
    base = EffectSlot;
    slot = base + idx;
loop:
    idx++;
    slot++;
    if (idx > N_EFFECT_SLOTS - 1)
    {
        slot = base;
        idx = 0;
    }
    if (slot->proc == 0)
    {
        EFFECT_CURSOR_ = idx + 1;
        if (N_EFFECT_SLOTS - 1 < idx + 1)
        {
            EFFECT_CURSOR_ = 0;
        }
        ef = slot;
        goto found;
    }
    count++;
    if (count > N_EFFECT_SLOTS - 1)
    {
        ef = &dmy;
        goto found;
    }
    goto loop;
found:
    ef->param.splash.px = pos->vx;
    fp = &ef->param.splash;
    fp->py = pos->vy;
    z = pos->vz;
    fp->mode = SPLASH_MODE_SPAWN;
    fp->sx = sx;
    fp->sy = sy;
    fp->speed = speed;
    fp->pz = z;
    ef->proc = (void (*)())DrawSplash;
}
