#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1chase(void);
 *     THINK_1.C:119, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a0       struct Humanoid * enemy
 *     reg   $s1       long xx
 *     reg   $s2       long zz
 *     reg   $s3       short pad
 *     reg   $s1       long vx
 *     reg   $s2       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 * END PSX.SYM */

s16 Think1chase(void)
{
    s32 pad;
    pad = 0;
    if (++Me_THINK_C->actcnt == 1)
    {
        Humanoid *enemy;

        enemy = GetNearestHumanoid(Me_THINK_C, 5000);
        if (enemy != 0)
        {
            Me_THINK_C->chase[HUMANOID_CHASE_X] = enemy->locate->vx;
            Me_THINK_C->chase[HUMANOID_CHASE_Z] = enemy->locate->vz;
        }
        else
        {
            Me_THINK_C->chase[HUMANOID_CHASE_X] =
                Me_THINK_C->point[HUMANOID_HOME_X] + rand() % 10000 - 5000;
            Me_THINK_C->chase[HUMANOID_CHASE_Z] =
                Me_THINK_C->point[HUMANOID_HOME_Z] + rand() % 10000 - 5000;
        }
    }
    else
    {
        pad = GotoPosition(
            Me_THINK_C->chase[HUMANOID_CHASE_X] - Me_THINK_C->locate->vx,
            Me_THINK_C->chase[HUMANOID_CHASE_Z] - Me_THINK_C->locate->vz);
        if ((s16)pad == 0)
        {
            pad |= PADRleft;
            Me_THINK_C->actcnt = 0;
        }
    }
    return pad;
}
