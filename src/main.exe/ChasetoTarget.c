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
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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

extern short Attrib;
extern int rand(void);

short ChasetoTarget(long length)
{
    Humanoid *me;
    long target_x, target_z;
    long xx, zz;
    long *chase;
    long vx, vz;
    short deg;

    me = Me_THINK_C;
    chase = &me->chase[0];
    if (me->target == 0) {
        return 0;
    }

    target_x = me->target->locate.coord.t[0];
    xx = target_x + me->chase[0]
         - me->locate->vx;
    target_z = me->target->locate.coord.t[2];
    zz = target_z + chase[1] - me->locate->vz;

    if (((xx >= 0 ? xx : -xx) < 500 &&
         (zz >= 0 ? zz : -zz) < 500) ||
        (Attrib & 0x400) != 0 || Distance < 1000) {
        return 0;
    }

    if ((Attrib & 0xc000) != 0 ||
        (me->chase[0] | chase[1]) == 0) {
        deg = rand();
        vx = rcos(deg) * length >> 12;
        me->chase[0] = vx;
        vz = rsin(deg) * length >> 12;
        chase[1] = vz;
    }
    return turn_towards_player_(xx, zz);
}
