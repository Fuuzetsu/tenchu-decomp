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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * Matched. Notes (against SetBleed.c/SetBleeds.c, the same EffectSlot[200]
 * pool spawner family — see their headers for the idiom writeups):
 *  - `SetBleedsDir` is the "given direction" variant: `pos` is jittered by
 *    `grange` (same 3-axis default-then-conditionally-overwritten idiom as
 *    SetBleeds' jitter), but `vec` (the direction) is copied VERBATIM into
 *    `v` — no srange/jitter step for it at all (that's the whole "Dir"
 *    distinction from SetBleeds, which jitters both).
 *  - Stack layout: npos@sp+0x10, v@sp+0x20, scratch t@sp+0x28. Position
 *    jitter spans both short-vector halves of BleedSpawnVectors after a
 *    16-byte memset, then is block-copied into npos. The direction is built
 *    in the upper half after an 8-byte memset and copied into the lower.
 *    This gives
 *    the target's two throwaway-scratch copies and the exact 0x58 frame.
 *  - The `time` split's DIVISOR is 8 here, not 2 like SetBleeds' half-split:
 *    the `.s` shows the `bgez`/`+7`/`sra ,3` round-toward-zero sequence.
 *    `time / 8` is spelled out at all three sites (cse1 folds them into the
 *    one `sra`); a named `half` temp also matches but is not needed.
 *  - `rand() % (grange * 2)` needs maspsx `--expand-div` for this file (division
 *    by a variable) — already in Build.hs/permute.py.
 *
 *  - THE KEY, and why this sat parked at 13/19 bytes: `grange * 2` MUST be
 *    spelled inline at its three loop-body sites rather than hoisted into a
 *    named `grange2` temp before the loop. Not for the multiply itself —
 *    cse/loop still hoist it back out to the single `sll s2,s3,0x1` in the
 *    prologue — but purely for its effect on loop.c's INSN COUNT.
 *
 *    cc1's loop.c hoists a loop-invariant only when
 *        threshold * savings * lifetime >= insn_count      (loop.c:1640)
 *    with threshold = (loop_has_call ? 1 : 2) * (1 + n_non_fixed_regs)
 *    = 1 * (1 + 28) = 29 here. The `high`/`lo_sum` pair for DrawBleed's
 *    address gets savings 2 / lifetime 2 from force_movables (loop.c:1200),
 *    which rewards a high->lo_sum chain, so its score is 29*2*2 = 116.
 *    With `grange2` precomputed the loop is 114 real insns, 116 >= 114 holds,
 *    and loop.c hoists DrawBleed's address to the function entry. It is then
 *    live across the whole loop (across rand()/memset()) with no callee-saved
 *    register free, so it gets no hard reg and reload REMATERIALIZES it from
 *    its REG_EQUAL note at the use site — emitting lui/addiu glued to the sw,
 *    after sched1 has already run. That single cause produced all three
 *    symptoms of the old park: address in the wrong register (a reload temp,
 *    t1, not v0), address not hoisted above the byte stores, and the proc
 *    store not sinking into the delay slot.
 *
 *    Inlining `grange * 2` at 3 sites lifts the loop to 117 real insns, 116 >= 117
 *    is false, loop.c leaves the address in the tail block as ordinary insns,
 *    and sched1 then schedules them into the target's exact order. The margin
 *    is TWO insns: 115 still hoists, 117 does not. Verified against the
 *    matched sibling SetBleeds, whose loop is 135 real insns and which
 *    therefore never hoists (its `.loop` log says "not desirable"), and
 *    against the demo build, whose SetBleedsDir has the identical tail.
 */
extern void DrawBleed(TEffectSlot *ef);
extern void *memset(void *s, int c, u32 n);
extern int rand(void);

void SetBleedsDir(VECTOR *pos, SVECTOR *vec, short grange, short n, int time, long col)
{
    VECTOR npos;
    BleedSpawnVectors work;
    long b;
    int btime;

    do
    {
        if (n <= 0)
        {
            return;
        }
        memset(&work, 0, sizeof(VECTOR));
        b = pos->vx;
        if (grange * 2 > 0)
        {
            work.position.vx =
                b + (rand() % (grange * 2) - grange);
        }
        else
        {
            work.position.vx = b - grange;
        }
        b = pos->vy;
        if (grange * 2 > 0)
        {
            work.position.vy =
                b + (rand() % (grange * 2) - grange);
        }
        else
        {
            work.position.vy = b - grange;
        }
        b = pos->vz;
        if (grange * 2 > 0)
        {
            work.position.vz =
                b + (rand() % (grange * 2) - grange);
        }
        else
        {
            work.position.vz = b - grange;
        }
        npos = work.position;
        memset(&work.vector.temporary, 0, sizeof(SVECTOR));
        work.vector.temporary.vx = vec->vx;
        work.vector.temporary.vy = vec->vy;
        work.vector.temporary.vz = vec->vz;
        work.vector.velocity = work.vector.temporary;

        if (time - time / 8 > 0)
        {
            btime = rand() % (time - time / 8) + time / 8;
        }
        else
        {
            btime = time / 8;
        }
        {
            VECTOR *pos = &npos;
            int time = btime;
            int idx;
            TEffectSlot *base;
            TEffectSlot *slot;
            int count;
            TEffectSlot *ef;
            BleedType *param;
            u8 r;

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
            n--;
            param = &ef->param.bleed;
            r = col >> 16;
            ef->param.bleed.pos = *pos;
            ef->param.bleed.vec = work.vector.velocity;
            param->r = r;
            param->g = col >> 8;
            param->time = time;
            param->b = col;
            param->mode = 0;
            ef->proc = (void (*)())DrawBleed;
        }
    } while (1);
}
