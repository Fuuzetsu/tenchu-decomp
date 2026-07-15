#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetBleedsDir(struct VECTOR *pos, struct SVECTOR *vec, short grange, short n, int time, long col);
 *     EFFECT.C:1115, 12 src lines, frame 88 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
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
 *     param $s7       struct VECTOR * pos
 *     param $fp       struct SVECTOR * vec
 *     param $a2       short grange
 *     param $s5       short n
 *     param stack+16  int time
 *     param stack+20  long col
 *     reg   $s4       int time
 *     reg   $s6       long col
 *     stack sp+16     struct VECTOR npos
 *     stack sp+32     struct SVECTOR v
 *     reg   $a3       struct VECTOR * pos
 *     reg   $t0       int time
 *     reg   $s6       long col
 *     reg   $v1       struct BleedType * param
 *     reg   $a2       struct tag_EffectSlot * slot
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — links 808 bytes vs the 876-byte target (17
 * instructions / 68 bytes SHORT). Default build keeps the byte-identical
 * INCLUDE_ASM stub.
 *
 * Matching notes (against SetBleed.c/SetBleeds.c, the same EffectSlot[200]
 * pool spawner family — see their headers for the idiom writeups):
 *  - `SetBleedsDir` is the "given direction" variant: `pos` is jittered by
 *    `grange` (same 3-axis default-then-conditionally-overwritten idiom as
 *    SetBleeds' `grange`/`srange` jitter), but `vec` (the direction) is
 *    copied VERBATIM into `v` — no srange/jitter step for it at all (that's
 *    the whole "Dir" distinction from SetBleeds, which jitters both).
 *  - The two `memset(&npos/&v, 0, sizeof(...))` calls are load-bearing, same
 *    as SetBleeds (zero the padding/unused fields before the block-copies).
 *  - `time` splits as `half = time / 8; rem = time - half; btime = half +
 *    (rem>0 ? rand()%rem : 0);` — note the DIVISOR is 8 here, not 2 like
 *    SetBleeds' half-split; confirmed by the raw `.s`'s `sra ,3`/`+7`
 *    round-toward-zero shift sequence (the standard signed-divide-by-
 *    power-of-2 compilation), not a `sra ,1`.
 *  - The inner EffectSlot[200] pool scan is SetBleed.c's own hand-rolled
 *    `goto loop;` shape verbatim (count++ only on the NOT-found path, &dmy
 *    assigned after the loop, cursor-store inside the found branch, `n--`
 *    on both exits before the shared tail).
 *  - Field store order into BleedType: pos (block copy), vec (block copy),
 *    r, g, time, b, mode, proc — same order as SetBleed.c/SetBleeds.c, not
 *    struct-offset order.
 *  - `grange * 2`/`rand() % grange2` needs maspsx `--expand-div` for this
 *    file (division by a variable) — added to Build.hs/permute.py.
 *
 * RESIDUAL (68 bytes / 17 instructions short): the SAME unresolved
 * "throwaway stack scratch, then a SECOND block-copy" shape as SetBleeds'
 * own parked residual (see its header) — target computes each of npos's
 * THREE jitter fields into its own memset'd stack scratch, then reloads
 * and re-stores them into npos's real stack slot (8 extra instructions: 4
 * lw + 4 sw) before npos ever gets block-copied again into
 * `ef->param.bleed.pos`. A plain `long px, py, pz;` (SetBleeds.c's own
 * idiom, tried first) let cc1 keep the jitter values resident in
 * REGISTERS and store straight into npos's fields with no intermediate
 * stack bounce at all (692 bytes short residual: 788 vs 876). Declaring
 * them as one array instead — `long p[3];`, `p[0]/p[1]/p[2] = ...; npos.vx
 * = p[0];` etc. — is a REAL, reusable lever: it forced genuine stack
 * residency (arrays don't get promoted to registers as readily as scalars
 * in this pipeline) and cut the residual from 88 to 68 bytes, though it
 * still doesn't reproduce the exact double-copy-through-TWO-different-
 * scratch-addresses shape target uses (target's third jitter value's
 * scratch slot gets REUSED as `v`'s own scratch immediately after, a
 * stack-slot-numbering coincidence downstream of whatever register
 * pressure the real source had that this draft doesn't reach). Consistent
 * with SetBleeds' own conclusion: this is the cookbook's allocator-
 * cost-tie class, not a source-shape bug — parked per the attempt-cap
 * guidance.
 *
 * CAUTION for future attempts: `tools/autorules.py SetBleedsDir` produced
 * actively WRONG local types when run on this draft (narrowed `p`'s
 * 32-bit position-jitter values to `s16`, and `idx`/`count` — both real
 * 0..200 loop counters — to `s16`/`s8`). Because `p` feeds a VECTOR field
 * and idx/count are compared against 200, these aren't just "different
 * register class" ties like the cookbook's usual autorules wins; they're
 * outright semantically wrong despite autorules reporting them as
 * score-improving (and the resulting file failed to even build cleanly
 * once). Sanity-check every autorules-suggested width against what the
 * variable actually holds before accepting it, not just the score delta.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetBleedsDir", SetBleedsDir);
#else
extern void DrawBleed(TEffectSlot *ef);
extern void *memset(void *s, int c, u32 n);
extern int rand(void);

void SetBleedsDir(VECTOR *pos, SVECTOR *vec, short grange, short n, int time, long col)
{
    VECTOR npos;
    SVECTOR v;
    long p[3];
    int grange2;
    long d;
    int half;
    int rem;
    int btime;
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    BleedType *fp;

    grange2 = grange * 2;
    do
    {
        if (n <= 0)
        {
            return;
        }
        memset(&npos, 0, sizeof(npos));
        d = -grange;
        if (grange2 > 0)
        {
            d = rand() % grange2 - grange;
        }
        p[0] = pos->vx + d;

        d = -grange;
        if (grange2 > 0)
        {
            d = rand() % grange2 - grange;
        }
        p[1] = pos->vy + d;

        d = -grange;
        if (grange2 > 0)
        {
            d = rand() % grange2 - grange;
        }
        p[2] = pos->vz + d;
        npos.vx = p[0];
        npos.vy = p[1];
        npos.vz = p[2];

        memset(&v, 0, sizeof(v));
        v = *vec;

        half = time / 8;
        rem = time - half;
        btime = half;
        if (rem > 0)
        {
            btime = rand() % rem + half;
        }

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
            n = n - 1;
            goto found;
        }
        count = count + 1;
        if (199 < count)
        {
            ef = &dmy;
            n = n - 1;
            goto found;
        }
        goto loop;
    found:
        fp = &ef->param.bleed;
        fp->pos = npos;
        fp->vec = v;
        fp->r = col >> 16;
        fp->g = col >> 8;
        fp->time = btime;
        fp->b = col;
        fp->mode = 0;
        ef->proc = (void (*)())DrawBleed;
    } while (1);
}
#endif
