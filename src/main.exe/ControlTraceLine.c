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

/*
 * Steer a humanoid toward the current trace point. The current point controls
 * the turn-pad result; reaching it advances to the next point, with pad == -1
 * marking the end of the trace.
 *
 * The direct tail is important. PSX.SYM records no idx/sentinel temporaries,
 * and the human-shaped field increment, sentinel test, then normal-path OR
 * gives cc1 the target's early `li -1`, signed `lh`, unsigned `lhu`, and OR in
 * the branch delay slot. The former separate idx/sentinel/value scaffold was
 * the cause of the apparent 10-byte floor.
 */
extern s32 SquareRoot0(s32 val);
extern s32 ratan2(s32 y, s32 x);

short ControlTraceLine(Humanoid *human)
{
    TraceLine *trcl;
    TracePoint *point;

    s32 dx, dz;
    s32 dist;
    s16 cnt;
    s16 pad;
    u16 roty;
    s32 ang;
    short t;
    s16 a0;
    s16 degree;
    s32 absdeg;
    s32 d32;

    trcl = human->trace;
    pad = 0x1000;
    if (trcl == 0)
    {
        return 0;
    }
    point = trcl->point + trcl->index;
    dx = point->x - human->locate->vx;
    dz = point->z - human->locate->vz;
    dist = SquareRoot0(dx * dx + dz * dz);
    if ((human->attribute & 0x400) != 0)
    {
        trcl->count = -0x1e;
    }
    cnt = trcl->count;
    trcl->count = cnt + 1;
    if (cnt > 0)
    {
        roty = human->rotate->vy;
        ang = ratan2(-dx, -dz);
        t = ang - roty;
        a0 = (s16)t;
        if (0x801 <= a0)
        {
            t = 0x1000 - t;
        }
        else if (a0 < -0x7FF)
        {
            t = t + 0x1000;
        }
        d32 = t;
        degree = d32;
        if (human->turn <= d32)
        {
            pad |= 0x2000;
        }
        else if (d32 <= -human->turn)
        {
            pad |= 0x8000;
        }
        absdeg = degree;
        if (absdeg < 0)
        {
            absdeg = -absdeg;
        }
        if (absdeg > 500)
        {
            pad &= 0xA000;
        }
    }
    if (dist <= point->range)
    {
        trcl->index++;
        if (trcl->point[trcl->index].pad == -1)
        {
            trcl->index = 0;
            return -0x1000;
        }
        pad |= trcl->point[trcl->index].pad;
    }
    return pad;
}
