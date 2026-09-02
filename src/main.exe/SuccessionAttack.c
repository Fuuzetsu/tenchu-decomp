#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "game_globals.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short SuccessionAttack(long dist, short deg);
 *     THINK_3.C:247, 15 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long dist
 *     param $a1       short deg
 *
 * Globals it touches, as the original declared them:
 *     extern struct BattleType BattleDB[78];
 *     extern long Distance;
 *     extern short Degree;
 *     extern short EngageLevel;
 * END PSX.SYM */

extern Humanoid *Me_THINK_C;
extern int rand(void);

short SuccessionAttack(long dist, short deg)
{
    int t;
    int lev;
    s16 buttons;

    buttons = 0;
    if (Me_THINK_C->motion->count !=
        BattleDB[Me_THINK_C->warid].contfrm)
    {
        return 0;
    }
    if (Distance < dist)
    {
        int d;
        int raw;

        d = deg;
        raw = (int)Degree;
        raw = __builtin_abs(raw);
        t = raw < d;
        if (t)
            goto in_range;
    }
    t = rand();
    lev = EngageLevel + 1;
    if (t % lev != 0)
    {
        goto ret;
    }
in_range:
    if (Degree > 300)
    {
        buttons = PADLright;
    }
    else
    {
        buttons |= PADRleft;
        if (Degree < -300)
        {
            /* The signed view keeps PADLleft in addiu's immediate range. */
            buttons = (s16)PADLleft;
        }
        else
        {
            goto ret;
        }
    }
    buttons |= PADRleft;
ret:
    return buttons;
}
