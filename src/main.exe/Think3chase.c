#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "game_globals.h"
#include "item.h"
#include "padcmd.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3chase(void);
 *     THINK_3.C:60, 5 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short SR;
 *     extern short EngageLevel;
 *     extern long GameClock;
 *     extern long AttackActionCount;
 *     extern short Degree;
 *     extern short (*AttackFunc[4])();
 * END PSX.SYM */

/*
 * Think3chase clears SR at close range. When an attack action is due and the
 * target is aligned, it selects a pad command by distance and schedules the
 * next action; otherwise it dispatches the current weapon class through
 * AttackFunc. The indirect callback takes no arguments.
 *
 * `result` deliberately spans both paths and has one return label. Expanding
 * two source returns lets sched/reload rescue the pad value into $v0 before
 * the AttackActionCount update, duplicating its short conversion. The shared
 * source tail keeps result in $a0, leaves the update in $v0/$v1, and emits the
 * single target conversion at the join.
 */

extern Humanoid *Me_THINK_C;
extern s16 Degree;
extern s32 AttackActionCount; /* next GameClock tick an attack action may fire */

s16 Think3chase(void)
{
    s32 degree;
    u16 result;

    if (Distance < 10000 && SR != -2)
    {
        SR = 0;
    }
    if (AttackActionCount + EngageLevel * 30 < GameClock)
    {
        degree = Degree;
        if (degree < 0)
        {
            degree = -degree;
        }
        if (degree < 500)
        {
            if (Distance >= 0xFA1)
            {
                result = Me_THINK_C->pad.data;
            }
            else if (Distance >= 0xBB9)
            {
                result = SetCommand(&Me_THINK_C->pad, 0x21);
            }
            else
            {
                result = 0x80;
                if (Distance < 2000)
                {
                    result = 0xA0;
                }
            }
            AttackActionCount = GameClock + EngageLevel * 10;
            goto return_result;
        }
    }
    result = AttackFunc[Me_THINK_C->wpatk >> 4]();
return_result:
    return result;
}
