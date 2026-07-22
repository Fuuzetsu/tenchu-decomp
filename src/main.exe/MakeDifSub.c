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
 *    cse1 the two bases for the adjacent dot/length reads without keeping
 *    either pointer live through the rest of the function. Later destination
 *    writes and the final `bef` copy therefore use fresh direct field loads.
 *  - `dx` is referenced by name twice more after being computed (the dot
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
    s32 dot;
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

        dot = (s16)dx * info->bef.vx + a->vy * b->vy + a->vz * b->vz;
        lenA = GetVectorLength((s16)dx, a->vy, a->vz);
        lenB = GetVectorLength(info->bef.vx, b->vy, b->vz);
    }
    slab = lenA * lenB;

    if (0x7FFFE < dot)
    {
        t = dot;
        if (dot < 0)
        {
            t = dot + 0xFFF;
        }
        dot = t >> 0xC;
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
        sla = (dot << 0xC) / slab;
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

// triage: MEDIUM — 183 insns, mul/div, 1 callees, ~0.08 to GetVectorRotation
// likely-relevant cookbook sections:
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
// This raw rendering used the obsolete demo-shaped TMakeDifInfo view; the
// active source above reflects retail's inserted `ac` field.
//
//
// void MakeDifSub(VECTOR *src,VECTOR *target,VECTOR *dest,TMakeDifInfo *info)
//
// {
//   long lVar1;
//   long lVar2;
//   long lVar3;
//   int dx;
//   int iVar4;
//   short sVar5;
//   int iVar6;
//   short sVar7;
//   int iVar8;
//   int iVar9;
//
//   dx = target->vx - src->vx;
//   iVar8 = target->vy - src->vy;
//   iVar6 = target->vz - src->vz;
//   lVar1 = GetVectorLength(dx,iVar8,iVar6);
//   if (lVar1 == 0) {
//     dest->vx = 0;
//     dest->vy = 0;
//     dest->vz = 0;
//   }
//   else {
//     sVar7 = (short)iVar8;
//     sVar5 = (short)iVar6;
//     iVar6 = dx * 0x10000 >> 0x10;
//     iVar9 = lVar1 >> ((int)info->div & 0x1fU);
//     iVar8 = iVar6 * (info->bef).vy + (int)sVar7 * (int)(info->bef).vz +
//             (int)sVar5 * (int)(info->bef).pad;
//     lVar2 = GetVectorLength(iVar6,(int)sVar7,(int)sVar5);
//     lVar3 = GetVectorLength((int)(info->bef).vy,(int)(info->bef).vz,(int)(info->bef).pad);
//     iVar6 = lVar2 * lVar3;
//     if (0x7fffe < iVar8) {
//       if (iVar8 < 0) {
//         iVar8 = iVar8 + 0xfff;
//       }
//       iVar8 = iVar8 >> 0xc;
//       if (iVar6 < 0) {
//         iVar6 = iVar6 + 0xfff;
//       }
//       iVar6 = iVar6 >> 0xc;
//     }
//     if (iVar6 == 0) {
//       iVar4 = 0;
//     }
//     else {
//       iVar4 = (iVar8 << 0xc) / iVar6;
//       if (iVar6 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar6 == -1) && (iVar8 << 0xc == -0x80000000)) {
//         trap(0x1800);
//       }
//     }
//     iVar6 = (int)(info->bef).vx * (iVar4 + 0x1000);
//     if (iVar6 < 0) {
//       iVar6 = iVar6 + 0x1fff;
//     }
//     iVar6 = (iVar6 >> 0xd) + (int)info->spd;
//     if (iVar9 < iVar6) {
//       iVar6 = iVar9;
//     }
//     (info->bef).vx = (short)iVar6;
//     iVar8 = (short)dx * iVar6;
//     if (lVar1 == 0) {
//       trap(0x1c00);
//     }
//     if ((lVar1 == -1) && (iVar8 == -0x80000000)) {
//       trap(0x1800);
//     }
//     dest->vx = iVar8 / lVar1;
//     if (lVar1 == 0) {
//       trap(0x1c00);
//     }
//     if ((lVar1 == -1) && (sVar7 * iVar6 == -0x80000000)) {
//       trap(0x1800);
//     }
//     dest->vy = (sVar7 * iVar6) / lVar1;
//     if (lVar1 == 0) {
//       trap(0x1c00);
//     }
//     if ((lVar1 == -1) && (sVar5 * iVar6 == -0x80000000)) {
//       trap(0x1800);
//     }
//     dest->vz = (sVar5 * iVar6) / lVar1;
//     (info->bef).vy = (short)dx;
//     (info->bef).vz = sVar7;
//     (info->bef).pad = sVar5;
//   }
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// s32 GetVectorLength(s16, s16, s16, s16);            /* extern */
//
// void MakeDifSub(void *arg0, void *arg1, void *arg2, void *arg3) {
//     s16 sp10;
//     s16 sp12;
//     s16 sp14;
//     s16 temp_a3;
//     s16 temp_s0;
//     s16 temp_s1;
//     s16 temp_s3;
//     s32 temp_s0_3;
//     s32 temp_s3_2;
//     s32 temp_v0;
//     s32 var_a0;
//     s32 var_s0;
//     s32 var_s0_2;
//     s32 var_s1;
//     s32 var_v0;
//     s32 var_v0_2;
//     s32 var_v1;
//     u16 temp_a1;
//     u16 temp_a2;
//     void *temp_s0_2;
//
//     temp_s0 = arg1->unk0 - arg0->unk0;
//     temp_s3 = arg1->unk4 - arg0->unk4;
//     temp_s1 = arg1->unk8 - arg0->unk8;
//     temp_v0 = GetVectorLength(temp_s0, temp_s3, temp_s1);
//     if (temp_v0 == 0) {
//         arg2->unk0 = 0;
//         arg2->unk4 = 0;
//         arg2->unk8 = 0;
//         return;
//     }
//     temp_a3 = arg3->unk0;
//     sp10 = temp_s0;
//     sp12 = temp_s3;
//     sp14 = temp_s1;
//     temp_a1 = sp10.unk2;
//     temp_s0_2 = arg3 + 6;
//     temp_a2 = sp10.unk4;
//     temp_s3_2 = temp_v0 >> temp_a3;
//     var_s1 = (temp_s0 * arg3->unk6) + ((s16) temp_a1 * temp_s0_2->unk2) + ((s16) temp_a2 * temp_s0_2->unk4);
//     temp_s0_3 = GetVectorLength(temp_s0, (s16) temp_a1, (s16) temp_a2, temp_a3);
//     var_s0 = temp_s0_3 * GetVectorLength(arg3->unk6, temp_s0_2->unk2, temp_s0_2->unk4);
//     if (var_s1 > 0x7FFFE) {
//         var_v0 = var_s1;
//         if (var_s1 < 0) {
//             var_v0 = var_s1 + 0xFFF;
//         }
//         var_s1 = var_v0 >> 0xC;
//         var_v0_2 = var_s0;
//         if (var_s0 < 0) {
//             var_v0_2 = var_s0 + 0xFFF;
//         }
//         var_s0 = var_v0_2 >> 0xC;
//     }
//     if (var_s0 == 0) {
//         var_s0_2 = 0;
//     } else {
//         var_s0_2 = (s32) (var_s1 << 0xC) / var_s0;
//     }
//     var_v1 = arg3->unk4 * (var_s0_2 + 0x1000);
//     if (var_v1 < 0) {
//         var_v1 += 0x1FFF;
//     }
//     var_a0 = (var_v1 >> 0xD) + arg3->unk2;
//     if (temp_s3_2 < var_a0) {
//         var_a0 = temp_s3_2;
//     }
//     arg3->unk4 = (s16) var_a0;
//     arg2->unk0 = (s32) ((s32) (sp10 * var_a0) / temp_v0);
//     arg2->unk4 = (s32) ((s32) (sp12 * var_a0) / temp_v0);
//     arg2->unk8 = (s32) ((s32) (sp14 * var_a0) / temp_v0);
//     arg3->unk6 = (s16) (u16) sp10;
//     arg3->unk8 = (u16) sp12;
//     arg3->unkA = (u16) sp14;
// }
