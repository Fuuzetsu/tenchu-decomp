#include "common.h"
#include "main.exe.h"
#include "effect.h"

/*
 * MATCHED. Spawns `count` smoke particles around `pos`, gives each a random
 * horizontal spread, adds that displacement to the starting position, and
 * divides the particle velocity by `divisor` before handing it to DrawSmoke.
 *
 * Matching notes:
 *  - The outer infinite loop and the bottom-tested EffectSlot search mirror
 *    the PSX.SYM-backed SetSmoke sibling. Direct `EffectSlot[idx]` expressions
 *    let loop strength reduction create the target's pool walk; this complete
 *    loop/array shape also keeps the spread and divisor extensions at their
 *    use sites instead of hoisting them into the prologue.
 *  - Keeping the pool-full fallback after the scan reproduces the target's
 *    delay-slot increment and compensating decrement.
 *  - Compute the first spread width before taking `&slot->param.smoke`, then put
 *    the positive-width body first. This places the pointer formation in the
 *    branch delay slot and gives both spread arms their target layout.
 *  - Sony's `copyVector` macro copies x/y/z without the pad word. The quotient
 *    values are separate shorts so all three guarded divisions precede that
 *    position copy as in the target.
 *  - `r` stays unsigned for the target allocation, but the `% 15` operand is
 *    explicitly cast signed. Naming `m = smoke->time - 8` prevents fold from
 *    migrating the constant onto the remainder.
 */
extern void DrawSmoke(TEffectSlot *ef);

void spawn_smoke_burst_(VECTOR *pos, u16 spread, s16 divisor, s16 count)
{
    int i;
    int idx;
    TEffectSlot *slot;
    int searched;
    SmokeType *smoke;
    short vx;
    short vy;
    short vz;
    u32 r;
    int m;

    i = 0;
    do
    {
        searched = 0;
        if (i >= count)
        {
            return;
        }
        idx = EFFECT_CURSOR_;
        do
        {
            idx++;
            if (idx >= N_EFFECT_SLOTS)
            {
                idx = 0;
            }
            if (EffectSlot[idx].proc == 0)
            {
                EFFECT_CURSOR_ = idx + 1;
                if (EFFECT_CURSOR_ >= N_EFFECT_SLOTS)
                {
                    EFFECT_CURSOR_ = 0;
                }
                slot = &EffectSlot[idx];
                goto found;
            }
            searched++;
        } while (searched < N_EFFECT_SLOTS);
        slot = &dmy;
    found:
        {
            int width;

            width = (s16)spread * 2;
            smoke = &slot->param.smoke;
            if (width > 0)
            {
                smoke->vec.vx = rand() % width - spread;
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
        copyVector(&smoke->pos, pos);
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
        smoke->sprite = SMOKE_SPRITE_ALT;
        smoke->evtime = m - ((s32)r % 15);
        slot->proc = DrawSmoke;
    } while (1);
}
