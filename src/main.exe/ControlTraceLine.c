#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ControlTraceLine(struct Humanoid *human);
 *     HUMAN.C:526, 33 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s5       struct Humanoid * human
 *     reg   $s4       struct TracePoint * point
 *     reg   $s1       struct TraceLine * trcl
 *     reg   $s2       long dx
 *     reg   $s0       long dz
 *     reg   $s6       long dist
 *     reg   $s3       short pad
 *     reg   $s2       long dx
 *     reg   $s0       long dz
 *     reg   $s0       short roty
 *     reg   $a1       short degree
 * END PSX.SYM */

short ControlTraceLine(Humanoid *human)
{
    TraceLine *trcl;
    TracePoint *point;

    s32 dx, dz;
    s32 dist;
    s16 cnt;
    trace_pad pad;
    u16 roty;
    s32 ang;
    short t;
    s16 diff;
    s16 degree;
    s32 absdeg;
    s32 d32;

    trcl = human->trace;
    pad = PADLup;
    if (trcl == 0)
    {
        return 0;
    }
    point = trcl->point + trcl->index;
    dx = point->x - human->locate->vx;
    dz = point->z - human->locate->vz;
    dist = SquareRoot0(dx * dx + dz * dz);
    if ((human->attribute & ATTR_WALL) != 0)
    {
        trcl->count = -30;
    }
    cnt = trcl->count;
    trcl->count = cnt + 1;
    if (cnt > 0)
    {
        roty = human->rotate->vy;
        ang = ratan2(-dx, -dz);
        t = ang - roty;
        diff = t;
        if (diff > ANGLE_HALF)
        {
            t = ANGLE_FULL - t;
        }
        else if (diff <= -ANGLE_HALF)
        {
            t += ANGLE_FULL;
        }
        d32 = t;
        degree = d32;
        if (human->turn <= d32)
        {
            pad |= PADLright;
        }
        else if (d32 <= -human->turn)
        {
            pad |= PADLleft;
        }
        absdeg = degree;
        if (absdeg < 0)
        {
            absdeg = -absdeg;
        }
        if (absdeg > 500)
        {
            pad &= (PADLleft | PADLright);
        }
    }
    if (dist <= point->range)
    {
        trcl->index++;
        if (trcl->point[trcl->index].pad == TRACE_POINT_END)
        {
            trcl->index = 0;
            return (s16)PAD_DIRECTION_BUTTONS;
        }
        pad |= trcl->point[trcl->index].pad;
    }
    return pad;
}
