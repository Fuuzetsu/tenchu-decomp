#include "common.h"
#include "tuning.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemKawarimi(struct tag_TItem *item);
 *     ITEM.C:1571, 35 src lines, frame 80 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_TItem * item
 *     reg   $s4       struct param_drop * param
 *     reg   $s2       int i
 *     stack sp+16     struct VECTOR pos
 *     stack sp+32     struct SVECTOR vec
 *     reg   $s0       struct tag_TItem * item
 * END PSX.SYM */

#include "item.h"

void ProcItemKawarimi(TItem *item)
{
    enum
    {
        KAWARIMI_MODE_START = 0,
        KAWARIMI_MODE_BLEED = 1,
        KAWARIMI_MODE_FINISH = 2,
        KAWARIMI_BLEED_FRAMES = 0x1f
    };
    param_drop *param;
    s32 particle_index;

    param = &item->param.drop;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = KAWARIMI_MODE_START;
        return;
    }
    switch (item->mode)
    {
    case KAWARIMI_MODE_START:
        param->count = 0;
        item->mode++;
        return;

    case KAWARIMI_MODE_BLEED:
        particle_index = 0;
        while (1)
        {
            if (particle_index >= 0x14)
                break;
            {
                VECTOR position = {
                    .vx = item->owner->model->locate.coord.t[0] +
                        (rand() % 1000 - 500),
                    .vy = item->owner->model->locate.coord.t[1] +
                        (rand() % 1000 - 1200),
                    .vz = item->owner->model->locate.coord.t[2] +
                        (rand() % 1000 - 500)
                };
                SVECTOR velocity = {
                    .vx = 0,
                    .vy = rand() % 10 - 30,
                    .vz = 0
                };

                SetBleed(&position, &velocity,
                         rand() % 16 + 15, RGB24(100, 200, 220));
            }
            particle_index++;
        }
        {
            u8 frame_count;

            frame_count = param->count + 1;
            param->count = frame_count;
            if (frame_count < KAWARIMI_BLEED_FRAMES)
                return;
        }
        item->mode++;
        return;

    case KAWARIMI_MODE_FINISH:
        if (item->proc == 0)
            return;
        DISPOSE_ITEM(item);
        return;
    }
}
