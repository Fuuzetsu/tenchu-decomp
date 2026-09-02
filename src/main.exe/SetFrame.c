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

/*
 * Matching notes (all verified against the original bytes; the same pool
 * search shape recurs in SetSplash/SetBleed and every other EffectSlot
 * inserter — see effect.h and this function's comments for the reusable
 * idioms):
 *  - FIND_EFFECT_SLOT is a real bottom-tested do-while over
 *    `EffectSlot[idx]`. Loop strength reduction creates the target's scan
 *    pointer and wrap reset; `slot` is only the found/fallback result.
 *  - The free-slot cursor-update store lives INSIDE
 *    `if (EffectSlot[idx].proc == 0) { ... }`, not after a bare
 *    `if (proc==0) break;` with the update code after the loop — only the
 *    former gives the occupied path (not the found path) the branch-away
 *    polarity the target has (a bare `if(cond) break;`'s jump always goes
 *    with cond-true, i.e. the wrong path here).
 *  - A param-union write to a NONZERO field offset goes through a cached
 *    typed pointer (`fp = &slot->param.frame;`); the very first field written,
 *    if it sits at a nonzero offset itself (frame.px here), still wants fp —
 *    only an offset-ZERO field (frame.super) is written through a fresh
 *    `slot->param.frame.super = ...` recast instead of `fp->super`.
 *  - `z = pos->vz;` (captured before the mode/size/count stores, stored via
 *    `fp->pz = z;` after them) reproduces the original's delayed store —
 *    inlining `fp->pz = pos->vz;` in position would read pos->vz too late.
 */
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
