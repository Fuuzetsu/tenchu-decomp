#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void RestoreItemLayout(void *buf);
 *     ITEM.C:507, 22 src lines, frame 288 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       void * buf
 *     reg   $s3       struct TItemLayout * slot
 *     stack sp+16     unsigned char [200] fn
 *     reg   $s1       int i
 *     reg   $s1       int i
 *     stack sp+216    struct PARAM_ITEM_STAY param
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

/* The loop consumes four {dx,dz} pairs despite this four-short declaration.
 * Keep the historical bound until the symbol can be typed without changing
 * its small-data addressing. */
extern short DropOffsets[4]; /* {dx,dz} probe pairs x1000 */

extern s32 abs(s32 x);
extern void *memset(void *s, int c, u32 n);

void RestoreItemLayout(void *buf)
{
    TItem *it;
    TItemLayout *slot;
    s32 i;
    PARAM_ITEM_STAY param;
    s32 level;
    s32 level_mode;
    s32 sentinel;
    s32 x;
    s32 z;

    {
        s32 i;

        i = 0;
        it = items;
    loop1:
        if (i >= MAX_ITEMS)
            goto loop1_end;
        if (it->proc != 0)
        {
            DISPOSE_ITEM(it);
        }
        it++;
        i++;
        goto loop1;
    loop1_end:;
    }

    i = 0;
    level_mode = AREA_LEVEL_STEP_DOWN;
    sentinel = LEVEL_NONE;
    slot = buf;
    for (;;)
    {
        if (i >= MAX_ITEMS)
            return;
        if (slot->type != ITEM_NONE)
        {
            PARAM_ITEM_STAY tmp;

            memset(&tmp, 0, sizeof(PARAM_ITEM_STAY));
            tmp.type = slot->type;
            tmp.locate = slot->locate;
            param = tmp;

            level = GetAreaMapLevel(GlobalAreaMap, param.locate.vx, param.locate.vy, param.locate.vz, level_mode);
            if (level == sentinel || abs(level - param.locate.vy) >= 1000)
            {
                s32 k;
                short *offs;

                k = 0;
                offs = DropOffsets;
                for (;;)
                {
                    if (k < 4)
                    {
                        x = param.locate.vx + offs[0] * 1000;
                        z = param.locate.vz + offs[1] * 1000;
                        level = GetAreaMapLevel(GlobalAreaMap, x,
                                                param.locate.vy, z, level_mode);
                        if (level != sentinel &&
                            abs(level - param.locate.vy) < 1000)
                        {
                            goto search_success;
                        }
                        offs += 2;
                        k++;
                        continue;
                    }
                search_check:
                    if (k == 4)
                    {
                        goto skip_stay;
                    }
                    break;
                }
            }
            param.locate.vy = level;
            ReqItemStay(&param);
        skip_stay:;
        }
        slot++;
        i++;
        continue;

    search_success:
        param.locate.vx = x;
        param.locate.vz = z;
        goto search_check;
    }
}
