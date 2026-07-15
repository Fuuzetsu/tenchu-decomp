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
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
    long tmp;
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    SplashType *fp;

    idx = CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_;
    count = 0;
    base = EffectSlot;
    slot = base + idx;
loop:
    idx = idx + 1;
    slot = slot + 1;
    if (199 < idx)
    {
        slot = base;
        idx = 0;
    }
    if (slot->proc == 0)
    {
        CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = idx + 1;
        if (199 < idx + 1)
        {
            CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = 0;
        }
        ef = slot;
        goto found;
    }
    count = count + 1;
    if (199 < count)
    {
        ef = &dmy;
        goto found;
    }
    goto loop;
found:
    ef->param.splash.px = pos->vx;
    fp = &ef->param.splash;
    fp->py = pos->vy;
    tmp = pos->vz;
    fp->mode = 0;
    fp->sx = sx;
    fp->sy = sy;
    fp->speed = speed;
    fp->pz = tmp;
    ef->proc = (void (*)())DrawSplash;
}
