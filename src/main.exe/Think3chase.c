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

extern Humanoid *Me_THINK_C;
extern s32 AttackActionCount; /* next GameClock tick an attack action may fire */

s16 Think3chase(void)
{
    s32 degree;
    u16 result;

    if (Distance < SR_CLEAR_RANGE && SR != SR_GONE)
    {
        SR = SR_NONE;
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
            if (Distance > 4000)
            {
                result = Me_THINK_C->pad.data;
            }
            else if (Distance > 3000)
            {
                result = SetCommand(&Me_THINK_C->pad, CMD_LUNGE);
            }
            else
            {
                result = PADRleft;
                if (Distance < 2000)
                {
                    result = PADRleft | PADRright;
                }
            }
            AttackActionCount = GameClock + EngageLevel * ATTACK_COOLDOWN_PER_LEVEL;
            goto return_result;
        }
    }
    result = AttackFunc[WEAPON_ATTACK_CLASS(Me_THINK_C->wpatk)]();
return_result:
    return result;
}
