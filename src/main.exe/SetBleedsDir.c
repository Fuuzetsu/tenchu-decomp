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
 * STATUS: NON_MATCHING — the guarded C now has the target's exact 876-byte /
 * 219-instruction extent and linked-image layout. Thirteen bytes remain in
 * the six-instruction store tail; the default build keeps the byte-identical
 * INCLUDE_ASM stub.
 *
 * Matching notes (against SetBleed.c/SetBleeds.c, the same EffectSlot[200]
 * pool spawner family — see their headers for the idiom writeups):
 *  - `SetBleedsDir` is the "given direction" variant: `pos` is jittered by
 *    `grange` (same 3-axis default-then-conditionally-overwritten idiom as
 *    SetBleeds' `grange`/`srange` jitter), but `vec` (the direction) is
 *    copied VERBATIM into `v` — no srange/jitter step for it at all (that's
 *    the whole "Dir" distinction from SetBleeds, which jitters both).
 *  - Stack layout is the structural key (the now-matched SetBleeds.c proves
 *    the idiom): npos@sp+0x10, v@sp+0x20, scratch t@sp+0x28. Position jitter
 *    is built as a VECTOR through `(VECTOR *)&v`, spanning v and t after a
 *    16-byte memset, then block-copied into npos. The direction is built in
 *    t after an 8-byte memset and block-copied into v. This recovered the
 *    target's two throwaway-scratch copies and the exact 0x58-byte frame.
 *  - Each position axis is a full if/else with its base load before the
 *    branch. Naming `g = grange` shares the one target sign extension across
 *    grange2 and all six arms.
 *  - `time` splits as `half = time / 8; rem = time - half; btime = half +
 *    (rem>0 ? rand()%rem : 0);` — note the DIVISOR is 8 here, not 2 like
 *    SetBleeds' half-split; confirmed by the raw `.s`'s `sra ,3`/`+7`
 *    round-toward-zero shift sequence (the standard signed-divide-by-
 *    power-of-2 compilation), not a `sra ,1`.
 *  - The inner shadow block (`VECTOR *pos = &npos`, `int time = btime`) and
 *    SetBleed.c's hand-rolled pool scan recover the target a3/t0 carriers,
 *    count/update branches, and single source `n--` duplicated by reorg.
 *  - `grange * 2`/`rand() % grange2` needs maspsx `--expand-div` for this
 *    file (division by a variable) — added to Build.hs/permute.py.
 *
 * RESIDUAL: the target materializes DrawBleed in v0 before the time/b/mode
 * byte stores, then places the proc store in the loop-back jump's delay slot.
 * Putting the assignment last preserves that delay-slot store but schedules
 * the address after the byte stores (19 bytes); putting it before those
 * fields, retained below, improves to 13 bytes but stores the proc early and
 * puts mode in the delay slot. A named proc, one-shot fence, and identical-arm
 * carrier either stayed at 19 or disturbed the otherwise-exact allocation.
 * This is the remaining sched/reload tie; do not regress the proven scratch
 * layout or narrow the VECTOR values / 0..200 scan counters to chase it.
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
    SVECTOR t;
    int grange2;
    long b;
    int g;
    int half;
    int rem;
    int btime;

    g = grange;
    grange2 = g * 2;
    do
    {
        if (n <= 0)
        {
            return;
        }
        memset(&v, 0, sizeof(VECTOR));
        b = pos->vx;
        if (grange2 > 0)
        {
            ((VECTOR *)&v)->vx = b + (rand() % grange2 - g);
        }
        else
        {
            ((VECTOR *)&v)->vx = b - g;
        }
        b = pos->vy;
        if (grange2 > 0)
        {
            ((VECTOR *)&v)->vy = b + (rand() % grange2 - g);
        }
        else
        {
            ((VECTOR *)&v)->vy = b - g;
        }
        b = pos->vz;
        if (grange2 > 0)
        {
            ((VECTOR *)&v)->vz = b + (rand() % grange2 - g);
        }
        else
        {
            ((VECTOR *)&v)->vz = b - g;
        }
        npos = *(VECTOR *)&v;
        memset(&t, 0, sizeof(SVECTOR));
        t.vx = vec->vx;
        t.vy = vec->vy;
        t.vz = vec->vz;
        v = t;

        half = time / 8;
        rem = time - half;
        if (rem > 0)
        {
            btime = rand() % rem + half;
        }
        else
        {
            btime = half;
        }
        {
            VECTOR *pos = &npos;
            int time = btime;
            int idx;
            TEffectSlot *base;
            TEffectSlot *slot;
            int count;
            TEffectSlot *ef;
            BleedType *fp;
            u8 r;

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
            n = n - 1;
            fp = &ef->param.bleed;
            r = col >> 16;
            ef->param.bleed.pos = *pos;
            ef->param.bleed.vec = v;
            fp->r = r;
            fp->g = col >> 8;
            ef->proc = (void (*)())DrawBleed;
            fp->time = time;
            fp->b = col;
            fp->mode = 0;
        }
    } while (1);
}
#endif
