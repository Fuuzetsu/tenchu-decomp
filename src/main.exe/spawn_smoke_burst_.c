#include "common.h"
#include "main.exe.h"
#include "effect.h"

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
        if (i >= count)
        {
            return;
        }
        FIND_EFFECT_SLOT(idx, searched, slot, found);
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
