#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1ninja(void);
 *     THINK_1.C:99, 2 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

extern s16 Think1random(void);

s16 Think1ninja(void)
{
    u8 actscnt;
    s16 result;

    result = 0;
    if (Me_THINK_C->status == STAT_JUMP)
    {
        return 0;
    }
    actscnt = Me_THINK_C->actscnt;
    Me_THINK_C->actscnt++;
    if (actscnt > 30)
    {
        result = Think1random();
        if (Me_THINK_C->motion->mid == MOT_MOVE &&
            Me_THINK_C->motion->count == 0)
        {
            SVECTOR move;
            s32 d1;
            s32 d2;

            GetMoveSpeed(&move, Me_THINK_C->rotate->vy,
                         (s16)(Me_THINK_C->width * 5), 0);
            d1 = GetAreaMapLevel(GlobalAreaMap, Me_THINK_C->locate->vx,
                                 Me_THINK_C->locate->vy - EYE_HEIGHT,
                                 Me_THINK_C->locate->vz,
                                 AREA_LEVEL_STEP_DOWN | AREA_LEVEL_FIRST_HIT |
                                     AREA_LEVEL_REUSE_CACHED);
            d2 = GetAreaMapLevel(GlobalAreaMap, Me_THINK_C->locate->vx + move.vx,
                                 Me_THINK_C->locate->vy - EYE_HEIGHT,
                                 Me_THINK_C->locate->vz + move.vz,
                                 AREA_LEVEL_RETURN_DELTA | AREA_LEVEL_FIRST_HIT |
                                     AREA_LEVEL_REUSE_CACHED);
            if (d1 == Me_THINK_C->map.level)
            {
                s32 abs_d2;

                abs_d2 = (d2 >= 0) ? d2 : -d2;
                if (abs_d2 < 500)
                {
                    goto set_1040;
                }
            }
            if (d2 <= 6100)
            {
                return result;
            }
        set_1040:
            result = PADLup | PADRdown;
        }
    }
    return result;
}
