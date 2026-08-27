#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void MakeDifSub(struct VECTOR *src, struct VECTOR *target, struct VECTOR *dest, struct TMakeDifInfo *info);
 *     CAMERA.C:377, 40 src lines, frame 56 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
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

/*
 * MakeDifSub (0x800300b4, 0x2dc bytes) — CAMERA.C's smoothed-delta helper:
 * given a src/target pair of VECTORs, writes the eased step toward target
 * into *dest and updates the TMakeDifInfo scratch block (*info) it was
 * handed (`spd` holds last frame's speed and `bef` its raw delta).
 * Divides by runtime values throughout (the eased speed's fixed-point
 * division and the final per-axis dest = delta*speed/len), so this TU needs
 * `--expand-div` (Build.hs maspsxGpExterns' extra list + permute.py's
 * MASPSX_EXTRA) for ASPSX's guarded bnez/break-7/break-6 expansion.
 *
 * Matching notes:
 *  - PSX.SYM's own locals list two SVECTOR pointers, `a` and `b`. Their
 *    narrow inner scope is significant: `a = &nv` and `b = &info->bef` give
 *    cse1 the two bases for the adjacent theta/length reads without keeping
 *    either pointer live through the rest of the function. Later destination
 *    writes and the final `bef` copy therefore use fresh direct field loads.
 *  - `dx` is referenced by name twice more after being computed (theta's
 *    product's first term, lenA's first arg) and stays in its own register
 *    for both; `dy`/`dz` are never referenced again by name after being
 *    stored into `nv` — every later use goes through `nv.vy`/`nv.vz`
 *    instead, which is why only dx's reads skip a stack round-trip.
 *  - The two `[0,0xfff]`-then-`>>0xc` clamps need the GetVectorLength.c
 *    "default-then-override temp" shape (`t = v; if (v<0) t = v+0xfff; v =
 *    t>>0xc;`), not an in-place `if (v<0) v+=0xfff; v>>=0xc;` — same idiom,
 *    same reason (the branch's delay slot gets the unconditional default).
 *  - The eased-speed reduction is THREE statements, not one expression:
 *    `mspd = info->spd*(sla+0x1000); if (mspd<0) mspd+=0x1fff;
 *    spd = mspd>>0xd; spd = spd + info->ac;`. Two levers here, both
 *    length/register-critical:
 *      (a) the product needs its OWN temp `mspd` distinct from `spd` — the
 *          in-place `spd = info->spd*…; if (spd<0) spd+=0x1fff; spd = (spd>>0xd)
 *          + ac;` fused the pre-shift accumulator and the final speed
 *          into one pseudo (a0), which coloured the whole chain a0 and drifted
 *          6 bytes; a fresh `mspd` lets the accumulator take v1.
 *      (b) the final `spd = mspd>>0xd` and `spd = spd + info->ac` must be
 *          SEPARATE statements — the fused `spd = (mspd>>0xd) + info->ac;`
 *          keeps the shift result in mspd's register (v1) and only the add
 *          lands in spd (a0); splitting makes the shift itself target spd (a0)
 *          and the add happen in place (the last 2-byte tie).
 */
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
        dest->vx = 0;
        dest->vy = 0;
        dest->vz = 0;
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

    if (0x7FFFE < theta)
    {
        t = theta;
        if (theta < 0)
        {
            t = theta + 0xFFF;
        }
        theta = t >> 0xC;
        t = slab;
        if (slab < 0)
        {
            t = slab + 0xFFF;
        }
        slab = t >> 0xC;
    }

    if (slab == 0)
    {
        sla = 0;
    }
    else
    {
        sla = (theta << 0xC) / slab;
    }

    mspd = info->spd * (sla + 0x1000);
    if (mspd < 0)
    {
        mspd += 0x1FFF;
    }
    spd = mspd >> 0xD;
    spd = spd + info->ac;
    if (ip < spd)
    {
        spd = ip;
    }

    info->spd = (s16)spd;
    dest->vx = (nv.vx * spd) / len;
    dest->vy = (nv.vy * spd) / len;
    dest->vz = (nv.vz * spd) / len;
    info->bef.vx = nv.vx;
    info->bef.vy = nv.vy;
    info->bef.vz = nv.vz;
}
