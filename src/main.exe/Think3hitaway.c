#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "game_globals.h"
#include "item.h"
#include "humanoid.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3hitaway(void);
 *     THINK_3.C:172, 33 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a1       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short SR;
 *     extern short (*AttackFunc[4])();
 *     extern short Degree;
 *     extern short Attrib;
 * END PSX.SYM */

extern Humanoid *Me_THINK_C;

extern s32 rand(void);
extern s16 ChasetoTarget(s32 length);
extern s16 SuccessionAttack(s32 dist, s16 deg);
extern s16 GotoPosition(s32 vx, s32 vz);

s16 Think3hitaway(void)
{
    u16 pad;

    if (Distance < SR_CLEAR_RANGE && SR != SR_GONE)
    {
        SR = SR_NONE;
    }
    if (Me_THINK_C->status == STAT_ATTACK)
    {
        Me_THINK_C->actflg = 0;
        Me_THINK_C->chase[HUMANOID_CHASE_Z] = 0;
        Me_THINK_C->chase[HUMANOID_CHASE_X] = 0;
        return SuccessionAttack(3000, 1500);
    }
    else if (Me_THINK_C->actflg != 0)
    {
        pad = AttackFunc[WEAPON_ATTACK_CLASS(Me_THINK_C->wpatk)]();
    }
    else
    {
        if (__builtin_abs(Degree) < 1000)
        {
            pad = GotoPosition(0, 0);
            pad = (pad & (PADLleft | PADLdown | PADLright)) | PADLdown;
        }
        else
        {
            pad = ChasetoTarget(5000);
        }
        if (Distance < 2000)
        {
            if (rand() % 30 == 0)
            {
                pad |= PADRdown;
            }
        }
        if (Distance > 4000 || (Attrib & ATTR_WALL))
        {
            Me_THINK_C->actflg = 1;
        }
    }
    return pad;
}
