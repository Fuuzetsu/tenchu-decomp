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
 *  - `param = &ef->param.bleed;` must be computed BEFORE `r = col >> 16;` (both
 *    textually and hence in the RTL) even though r's value is stored later:
 *    with r first, cc1 duplicates r's independent `sra` onto both merge-entry
 *    paths and leaves param's address undupped, backwards from the target
 *    (which duplicates the necessarily-path-dependent param address and
 *    computes r's path-invariant value exactly once). Reordering the two flips
 *    it back.
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
    BleedType *param;
    u8 r;

    base = EffectSlot;
    idx = EFFECT_CURSOR_;
    slot = base + idx;
    count = 0;
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
    param = &ef->param.bleed;
    r = col >> 16;
    ef->param.bleed.pos = *pos;
    ef->param.bleed.vec = *vec;
    param->r = r;
    param->g = col >> 8;
    param->time = time;
    param->b = col;
    param->mode = 0;
    ef->proc = DrawBleed;
}
