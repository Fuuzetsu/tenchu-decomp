#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

extern u16 DeathIndex;
extern long EmergencyNotice;

/*
 * register_character_death (0x8002bcb8) periodically chooses another live
 * humanoid and starts its alert/chase state when the dead actor is close and
 * an area-map passage exists between them.
 *
 * Matching notes:
 *  - The three long deltas at sp+0x10/sp+0x14/sp+0x18 are one VECTOR.  Keeping
 *    them as independent scalars allocates them to saved registers instead of
 *    reproducing the target's 0x38-byte frame and s0-s2 register set.
 *  - The repeated component-halving block is one `while` over three inline
 *    absolute-value tests.  cc1 duplicates its exit test at entry and emits
 *    the target's shared loop body without source labels.
 *  - Publishing the incremented selection cursor before the modulo keeps both
 *    target stores to DeathIndex; writing the two stores adjacently lets dead
 *    store elimination discard the first one.
 */

void register_character_death(Humanoid *dead)
{
    VECTOR delta;
    SVECTOR passage;
    Humanoid *human;
    s16 scale;
    s16 next;
    s16 index;
    s32 alert_time;
    s32 chase_z;

    scale = 1;
    if ((dead->attribute & ATTR_SEARCH) == 0 && gNannido != DIFFICULTY_EASY)
    {
        next = DeathIndex + 1;
        DeathIndex = next;
        index = next % Humans;
        human = HumanGroup[index];
        DeathIndex = index;

        if ((human->attribute & (ATTR_SUSPEND | ATTR_PHASE)) == 0 &&
            human->status != STAT_DEAD && human->status != STAT_DAMAGE &&
            human != StagePlayer)
        {
            delta.vx = dead->locate->vx - human->locate->vx;
            delta.vz = dead->locate->vz - human->locate->vz;
            if (__builtin_abs(GetDirection(delta.vx, delta.vz,
                                           human->rotate->vy)) <= 900 &&
                SquareRoot0(delta.vx * delta.vx + delta.vz * delta.vz) <= 20000)
            {
                delta.vy = dead->locate->vy - human->locate->vy - human->height;

                while (__builtin_abs(delta.vx) > 500 ||
                       __builtin_abs(delta.vy) > 500 ||
                       __builtin_abs(delta.vz) > 500)
                {
                    scale <<= 1;
                    delta.vx >>= 1;
                    delta.vy >>= 1;
                    delta.vz >>= 1;
                }

                passage.vx = delta.vx;
                passage.vy = delta.vy;
                passage.vz = delta.vz;
                if (GetAreaMapPassage(GlobalAreaMap, human->locate,
                                      &passage, scale) == 0)
                {
                    RESET_ALERT_DURATION(alert_time);
                    Sound(human, 0xc);
                    SetNowMotion(human, 0x80e, 1);
                    dead->attribute |= ATTR_SEARCH;
                    human->attribute |= ATTR_SEARCH | PHASE_SUSPICIOUS;
                    human->chase[0] = dead->locate->vx;
                    chase_z = dead->locate->vz;
                    human->actcnt = 0;
                    human->actscnt = 0;
                    human->chase[1] = chase_z;
                }
            }
        }
    }
}
