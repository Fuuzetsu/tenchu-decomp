#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1random(void);
 *     THINK_1.C:74, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a1       long xx
 *     reg   $v1       long zz
 *     reg   $s1       short pad
 *     reg   $a1       long vx
 *     reg   $v1       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short Attrib;
 * END PSX.SYM */

/*
 * Advance the random-walk counter. A new cycle picks a chase point near the
 * spawn point; later ticks either reset after reaching it or steer toward it
 * unless Attrib blocks the action.
 *
 * The direct vx/vz difference expressions are intentional. Splitting each
 * chase coordinate, locate coordinate, and pad value into extra scratch roles
 * changes global allocation and delay-slot scheduling. Keeping vx/vz as the
 * actual chase-minus-locate values yields the target load order, preserves
 * them in $a0/$a1 for GotoPosition, and leaves each absolute value
 * in a separate $v0 temporary.
 */

s16 Think1random(void)
{
    s32 pad;
    pad = 0;
    if (++Me_THINK_C->actcnt == 1)
    {
        Me_THINK_C->chase[HUMANOID_CHASE_X] = Me_THINK_C->point[HUMANOID_HOME_X] + rand() % 10000 - 5000;
        Me_THINK_C->chase[HUMANOID_CHASE_Z] = Me_THINK_C->point[HUMANOID_HOME_Z] + rand() % 10000 - 5000;
    }
    else
    {
        s32 vx, vz;
        VECTOR *locate;

        locate = Me_THINK_C->locate;
        vx = Me_THINK_C->chase[HUMANOID_CHASE_X] - locate->vx;
        vz = Me_THINK_C->chase[HUMANOID_CHASE_Z] - locate->vz;
        if ((((vx >= 0) ? vx : -vx) < 1000 &&
             ((vz >= 0) ? vz : -vz) < 1000) ||
            (Attrib & ATTR_WALL))
        {
            Me_THINK_C->actcnt = 0;
        }
        else
        {
            pad = GotoPosition(vx, vz);
        }
    }
    return pad;
}
