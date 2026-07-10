#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void MakeDifSub(struct VECTOR *src, struct VECTOR *target, struct VECTOR *dest, struct TMakeDifInfo *info);
 *     CAMERA.C:377, 40 src lines, frame 56 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
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

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/MakeDifSub", MakeDifSub);

// triage: MEDIUM — 183 insns, mul/div, 1 callees, ~0.08 to GetVectorRotation
// likely-relevant cookbook sections:
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
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
