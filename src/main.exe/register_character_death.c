#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

extern u16 D_800979DE;
extern s32 FRAMES_UNTIL_END_OF_ALERT;

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
 *    target stores to D_800979DE; writing the two stores adjacently lets dead
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
    if ((dead->attribute & 0x10) == 0 && gNannido != DIFFICULTY_EASY)
    {
        next = D_800979DE + 1;
        D_800979DE = next;
        index = next % Humans;
        human = HumanGroup[index];
        D_800979DE = index;

        if ((human->attribute & 0x83) == 0 &&
            human->status != 0x11 && human->status != 0x10 &&
            human != StagePlayer)
        {
            delta.vx = dead->locate->vx - human->locate->vx;
            delta.vz = dead->locate->vz - human->locate->vz;
            if (__builtin_abs(GetDirection(delta.vx, delta.vz,
                                           human->rotate->vy)) < 0x385 &&
                SquareRoot0(delta.vx * delta.vx + delta.vz * delta.vz) < 0x4e21)
            {
                delta.vy = dead->locate->vy - human->locate->vy - human->height;

                while (__builtin_abs(delta.vx) >= 0x1f5 ||
                       __builtin_abs(delta.vy) >= 0x1f5 ||
                       __builtin_abs(delta.vz) >= 0x1f5)
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
                    alert_time = 300;
                    if (gNannido == DIFFICULTY_HARD)
                    {
                        alert_time = 600;
                    }
                    FRAMES_UNTIL_END_OF_ALERT = alert_time;
                    Sound(human, 0xc);
                    SetNowMotion(human, 0x80e, 1);
                    dead->attribute |= 0x10;
                    human->attribute |= 0x11;
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
