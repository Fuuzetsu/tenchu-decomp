#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void MakeDifSub(struct VECTOR *src, struct VECTOR *target, struct VECTOR *dest, struct TMakeDifInfo *info);
 *     CAMERA.C:377, 40 src lines, frame 56 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $t0       struct VECTOR * src
 *     param $a1       struct VECTOR * target
 *     param $s5       struct VECTOR * dest
 *     param $s4       struct TMakeDifInfo * info
 *     reg   $s2       int len
 *     reg   $s6       int mspd
 *     reg   $s0       long theta
 *     reg   $s0       long dx
 *     reg   $s3       long dy
 *     reg   $s1       long dz
 *     stack sp+16     struct SVECTOR nv
 *     reg   $v1       struct SVECTOR * a
 *     reg   $s0       struct SVECTOR * b
 *     reg   $s0       long slab
 *     reg   $s0       long sla
 *     reg   $s1       long ip
 * END PSX.SYM */

extern long GetVectorLength(long dx, long dy, long dz);

void MakeDifSub(VECTOR *src, VECTOR *target, VECTOR *dest, TMakeDifInfo *info)
{
    s32 dx, dy, dz;
    s32 len;
    SVECTOR nv;
    s32 theta;
    s32 ip;
    s32 lenA, lenB;
    s32 slab;
    s32 sla;
    s32 spd;
    s32 t;
    s32 mspd;

    dx = target->vx - src->vx;
    dy = target->vy - src->vy;
    dz = target->vz - src->vz;
    len = GetVectorLength(dx, dy, dz);
    if (len == 0)
    {
        setVector(dest, 0, 0, 0);
        return;
    }

    ip = len >> info->div;

    nv.vx = dx;
    nv.vy = dy;
    nv.vz = dz;

    {
        SVECTOR *a = &nv;
        SVECTOR *b = &info->bef;

        theta = (s16)dx * info->bef.vx + a->vy * b->vy + a->vz * b->vz;
        lenA = GetVectorLength((s16)dx, a->vy, a->vz);
        lenB = GetVectorLength(info->bef.vx, b->vy, b->vz);
    }
    slab = lenA * lenB;

    if (theta >= 0x7FFFF)
    {
        t = theta;
        if (theta < 0)
        {
            t = theta + FIXED_TRUNC_BIAS;
        }
        theta = t >> FIXED_SHIFT;
        t = slab;
        if (slab < 0)
        {
            t = slab + FIXED_TRUNC_BIAS;
        }
        slab = t >> FIXED_SHIFT;
    }

    if (slab == 0)
    {
        sla = 0;
    }
    else
    {
        sla = (theta << FIXED_SHIFT) / slab;
    }

    mspd = info->spd * (sla + FIXED_ONE);
    if (mspd < 0)
    {
        mspd += 2 * FIXED_ONE - 1;
    }
    spd = mspd >> (FIXED_SHIFT + 1);
    spd += info->ac;
    if (ip < spd)
    {
        spd = ip;
    }

    info->spd = (s16)spd;
    setVector(dest, (nv.vx * spd) / len, (nv.vy * spd) / len, (nv.vz * spd) / len);
    info->bef.vx = nv.vx;
    info->bef.vy = nv.vy;
    info->bef.vz = nv.vz;
}
