#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "conflict.h"
#include "humanoid.h"
#include "game_globals.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GotoPosition(long vx, long vz);
 *     THINK.C:254, 11 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long vx
 *     param $a1       long vz
 * END PSX.SYM */

extern Humanoid *Me_THINK_C;
extern s32 ProbeLevelLow;

s16 GotoPosition(s32 vx, s32 vz)
{
    u16 dir;
    s32 turn;
    s32 result;
    s32 adir;

    result = 0;
    if (vx != 0 || vz != 0)
    {
        dir = GetDirection(vx, vz,
                           Me_THINK_C->rotate->vy);
    }
    else
    {
        dir = Degree;
    }
    turn = Me_THINK_C->turn;
    if (turn < (s16)dir)
    {
        result = PADLright;
    }
    else if ((s16)dir < -turn)
    {
        result = (s16)PADLleft;
    }
    adir = (s16)dir;
    if (adir < 0)
    {
        adir = -adir;
    }
    if (adir < 500)
    {
        result |= PADLup;
    }
    if (!(Attrib & ATTR_PHASE))
    {
        if (Attrib & ATTR_WALL)
        {
            s32 cached;

            cached = ProbeLevelLow;
            if (cached != LEVEL_NONE)
            {
                return result;
            }
            if (Me_THINK_C->pad_hold == 0)
            {
                SVECTOR local;
                s32 d1, d2;

                GetMoveSpeed(&local,
                             Me_THINK_C->rotate->vy,
                             0, Me_THINK_C->width);
                d1 = GetAreaMapLevel(GlobalAreaMap,
                                     Me_THINK_C->locate->vx + local.vx,
                                     Me_THINK_C->locate->vy - 500,
                                     Me_THINK_C->locate->vz + local.vz,
                                     AREA_LEVEL_RETURN_DELTA |
                                         AREA_LEVEL_FIRST_HIT |
                                         AREA_LEVEL_REUSE_CACHED);
                d2 = GetAreaMapLevel(GlobalAreaMap,
                                     Me_THINK_C->locate->vx - local.vx,
                                     Me_THINK_C->locate->vy - 500,
                                     Me_THINK_C->locate->vz - local.vz,
                                     AREA_LEVEL_RETURN_DELTA |
                                         AREA_LEVEL_FIRST_HIT |
                                         AREA_LEVEL_REUSE_CACHED);
                /* pad_hold packs (button << 16) | frames: latch a 30-frame
                 * sidestep toward the clearer flank. */
                if ((result & PADLright) && (d1 != cached))
                {
                    d2 = PADLright << 16;
                    goto apply;
                }
                if ((result & PADLleft) && (d2 != (u32)LEVEL_NONE))
                {
                    d2 = (u32)PADLleft << 16;
                apply:
                    d2 |= 30;
                    Me_THINK_C->pad_hold = d2;
                }
                else
                {
                    result = 0;
                }
            }
        }
    }
    return result;
}
