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
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct VECTOR * pos
 *     param $a1       struct SVECTOR * vec
 *     param $a2       int time
 *     param $a3       long col
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

/*
 * Matching notes (all verified against the original bytes):
 *  - The effect-slot pool search is a hand-rolled `goto loop;` (NOT
 *    while(1)+break): the give-up path takes `&dmy`'s address, a compile-time
 *    constant, and a real loop shape would let loop.c hoist that lui/addiu to
 *    the preheader (wrong — target only materializes &dmy at its own use).
 *  - `idx` must be assigned before `slot = base + idx;` (not the other way
 *    round with a second read of the cursor global) to land idx/slot in the
 *    target's t0/v1 pair instead of the swapped v1/t0.
 *  - The free-slot cursor-update code (store back to the pool cursor) lives
 *    INSIDE the `if (slot->proc == 0) { ...; ef = slot; break; }` body, not
 *    after a bare `if (proc==0) break;` — that's what gives the occupied path
 *    (not the found path) the branch-away polarity the original has.
 *  - `ef->param.bleed.pos = *pos;` / `.vec = *vec;` are plain whole-struct
 *    assignments: VECTOR (align 4) block-moves as 4 lw+4 sw, SVECTOR (align 2)
 *    as lwl/lwr+swl/swr pairs — no manual field-by-field copy needed.
 *  - `fp = &ef->param.bleed;` must be computed BEFORE `r = col >> 16;` (both
 *    textually and hence in the RTL) even though r's value is stored later:
 *    with r first, cc1 duplicates r's independent `sra` onto both merge-entry
 *    paths and leaves fp's address undupped, backwards from the target (which
 *    duplicates the necessarily-path-dependent fp address and computes r's
 *    path-invariant value exactly once). Reordering the two flips it back.
 *  - `ef->proc = ...;` last, after time/b/mode, lets its store fall into the
 *    final jr's delay slot like the original.
 */
extern void DrawBleed(TEffectSlot *ef);

void SetBleed(VECTOR *pos, SVECTOR *vec, int time, long col)
{
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    BleedType *fp;
    u8 r;

    base = EffectSlot;
    idx = CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_;
    slot = base + idx;
    count = 0;
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
    fp = &ef->param.bleed;
    r = col >> 16;
    ef->param.bleed.pos = *pos;
    ef->param.bleed.vec = *vec;
    fp->r = r;
    fp->g = col >> 8;
    fp->time = time;
    fp->b = col;
    fp->mode = 0;
    ef->proc = (void (*)())DrawBleed;
}
