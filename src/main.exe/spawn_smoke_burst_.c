#include "common.h"
#include "main.exe.h"
#include "effect.h"

/*
 * MATCHED. Spawns `count` smoke particles around `pos`, gives each a random
 * horizontal spread, adds that displacement to the starting position, and
 * divides the particle velocity by `divisor` before handing it to DrawSmoke.
 *
 * Matching notes:
 *  - The outer repetition is a literal label/back-edge. A recognized C loop
 *    lets loop.c hoist the spread and divisor extensions into the prologue,
 *    enlarging the frame and rotating every saved register. The hand-written
 *    back-edge keeps those casts at their two target sites while `base` is
 *    explicitly initialized once before the loop.
 *  - The EffectSlot search is the usual bottom-tested do-while. Keeping the
 *    pool-full fallback after it reproduces the target's delay-slot increment
 *    and compensating decrement.
 *  - Compute the first spread width before taking `&ef->param.smoke`, then put
 *    the positive-width body first. This places the pointer formation in the
 *    branch delay slot and gives both spread arms their target layout.
 *  - `pos` is copied as three scalar words, not as a VECTOR aggregate (which
 *    would also copy the pad word). The quotient values are separate shorts so
 *    all three guarded divisions precede the position copy as in the target.
 *  - `r` stays unsigned for the target allocation, but the `% 15` operand is
 *    explicitly cast signed. Naming `m = smoke->time - 8` prevents fold from
 *    migrating the constant onto the remainder.
 *  - The identical negative-spread stores under `if (pos)` are a zero-code
 *    allocation fence. They add one weighted RTL reference to `pos`, raising
 *    its global-allocation priority above `base` and producing target homes
 *    pos=$s5/base=$s6. Jump/cross-jump remove the redundant condition and
 *    duplicate store completely; a bounded permuter found the shape, and A/B
 *    testing proved this fence alone is load-bearing.
 */
extern void DrawSmoke(TEffectSlot *ef);

void spawn_smoke_burst_(VECTOR *pos, u16 spread, s16 divisor, s16 count)
{
    int i;
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int searched;
    TEffectSlot *ef;
    SmokeType *smoke;
    short vx;
    short vy;
    short vz;
    u32 r;
    int m;

    i = 0;
    base = EffectSlot;
loop:
    searched = 0;
    if (i >= count)
    {
        return;
    }
    idx = EFFECT_CURSOR_;
    /* Offset spelling: byte-required (base + idx flips the addu operand
         * order; measured — same class as UpdateEvent's walk). */
        slot = (TEffectSlot *)((idx * sizeof(TEffectSlot)) + (s32)base);
    do
    {
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
        searched++;
    } while (searched < N_EFFECT_SLOTS);
    ef = &dmy;
found:
    {
        int width;

        width = (s16)spread * 2;
        smoke = &ef->param.smoke;
        if (width > 0)
        {
            smoke->vec.vx = rand() % width - spread;
        }
        else if (pos)
        {
            smoke->vec.vx = -spread;
        }
        else
        {
            smoke->vec.vx = -spread;
        }
    }
    smoke->vec.vy = -5;
    {
        int width;

        width = (s16)spread * 2;
        if (width > 0)
        {
            smoke->vec.vz = rand() % width - spread;
        }
        else
        {
            smoke->vec.vz = -spread;
        }
    }

    {
        int div;

        div = (s16)divisor;
        vx = smoke->vec.vx / div;
        vy = smoke->vec.vy / div;
        vz = smoke->vec.vz / div;
    }
    smoke->pos.vx = pos->vx;
    smoke->pos.vy = pos->vy;
    smoke->pos.vz = pos->vz;
    smoke->pos.vx += smoke->vec.vx;
    smoke->pos.vy += smoke->vec.vy;
    smoke->pos.vz += smoke->vec.vz;
    smoke->vec.vx = vx;
    smoke->vec.vy = vy;
    smoke->vec.vz = vz;

    smoke->scale = rand() % SMOKE_SCALE_SPREAD + SMOKE_SCALE_MIN;
    smoke->rotate = 0;
    smoke->time = 15;
    r = rand();
    i++;
    m = smoke->time - 8;
    smoke->sprite = 1;
    smoke->evtime = m - ((s32)r % 15);
    ef->proc = (void (*)())DrawSmoke;
    goto loop;
}
