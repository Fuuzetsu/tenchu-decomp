#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ChasetoTarget(long length);
 *     THINK.C:269, 21 src lines, frame 48 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s5       long length
 *     reg   $s3       long xx
 *     reg   $s2       long zz
 *     reg   $s4       long * chase
 *     reg   $s3       long vx
 *     reg   $s2       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short Attrib;
 *     extern long Distance;
 * END PSX.SYM */

/*
 * Think helper: steer toward the target through a persistent random flank
 * offset (chase.point[0]/chase.point[1], re-rolled at `length` radius when
 * cleared or when the hit/push contact bits are up), returning
 * GotoPosition's command, or 0 when there is no target, the offset point is
 * nearly reached, the wall-contact bit is set, or the target is already close.
 */
extern int rand(void);

short ChasetoTarget(long length)
{
    Humanoid *me;
    long xx, zz;
    long *chase;
    long vx, vz;
    short deg;

    me = Me_THINK_C;
    /* chase is formed before the target guard: byte-required (the addiu
     * fills the branch's delay slot; measured). */
    chase = &me->chase.point[HUMANOID_CHASE_X];
    if (me->target.model == 0)
    {
        return 0;
    }

    xx = me->target.model->locate.coord.t[0] + me->chase.point[HUMANOID_CHASE_X] - me->locate->vx;
    zz = me->target.model->locate.coord.t[2] + chase[HUMANOID_CHASE_Z] - me->locate->vz;

    if (((xx >= 0 ? xx : -xx) < 500 &&
         (zz >= 0 ? zz : -zz) < 500) ||
        (Attrib & ATTR_WALL) != 0 || Distance < 1000)
    {
        return 0;
    }

    if ((Attrib & (ATTR_HIT | ATTR_PUSH)) != 0 ||
        (me->chase.point[HUMANOID_CHASE_X] | chase[HUMANOID_CHASE_Z]) == 0)
    {
        deg = rand();
        vx = rcos(deg) * length >> FIXED_SHIFT;
        me->chase.point[HUMANOID_CHASE_X] = vx;
        vz = rsin(deg) * length >> FIXED_SHIFT;
        chase[HUMANOID_CHASE_Z] = vz;
    }
    return GotoPosition(xx, zz);
}
