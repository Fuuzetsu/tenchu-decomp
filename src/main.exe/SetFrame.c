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
 *  - The pool search is a hand-rolled `goto loop;`, not `while(1){...break;}`:
 *    the give-up path takes `&dmy`'s address (a compile-time constant), and a
 *    real loop shape would let loop.c hoist that lui/addiu into the preheader
 *    — the target only materializes it at its own use, right before "found".
 *  - `idx = CURSOR; slot = base + idx;` (idx assigned first, slot computed
 *    FROM idx) lands idx/slot in the target's t0/v1 register pair; computing
 *    slot first and re-reading the cursor global for idx (relying on cse to
 *    fold the loads) swaps them to v1/t0 instead.
 *  - The free-slot cursor-update store lives INSIDE
 *    `if (slot->proc == 0) { ...; ef = slot; break; }`, not after a bare
 *    `if (proc==0) break;` with the update code after the loop — only the
 *    former gives the occupied path (not the found path) the branch-away
 *    polarity the target has (a bare `if(cond) break;`'s jump always goes
 *    with cond-true, i.e. the wrong path here).
 *  - A param-union write to a NONZERO field offset goes through a cached
 *    typed pointer (`fp = &ef->param.frame;`); the very first field written,
 *    if it sits at a nonzero offset itself (frame.px here), still wants fp —
 *    only an offset-ZERO field (frame.super) is written through a fresh
 *    `ef->param.frame.super = ...` recast instead of `fp->super`.
 *  - `tmp = pos->vz;` (captured before the mode/size/count stores, stored via
 *    `fp->pz = tmp;` after them) reproduces the original's delayed store —
 *    inlining `fp->pz = pos->vz;` in position would read pos->vz too late.
 */
extern void DrawFrame(TEffectSlot *ef);

void SetFrame(VECTOR *pos, short size, short time, GsCOORDINATE2 *super)
{
    long tmp;
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    FrameType *fp;

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
    fp = &ef->param.frame;
    fp->px = pos->vx;
    fp->py = pos->vy;
    tmp = pos->vz;
    fp->mode = 0;
    fp->size = size;
    fp->count = time;
    fp->pz = tmp;
    ef->param.frame.super = super;
    ef->proc = (void (*)())DrawFrame;
}
