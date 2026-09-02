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

extern void DrawSplash(TEffectSlot *ef);

void SetSplash(VECTOR *pos, short sx, short sy, int speed)
{
    long z;
    int idx;
    TEffectSlot *slot;
    int count;
    SplashType *fp;

    FIND_EFFECT_SLOT(idx, count, slot, found);
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
