#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SweepMotion(struct MotionManager *mmp);
 *     ACTION.C:272, 31 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $s3       struct MotionManager * mmp
 *     reg   $s2       struct MotionDataType * mot
 *     reg   $a2       struct ModelType * object
 *     reg   $s1       short i
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SweepMotion", SweepMotion);

// triage: MEDIUM — 246 insns, mul/div, 1 loop, 1 callees, ~0.07 to update_something_for_each_visible_enemy_
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short SweepMotion(MotionManager *mmp)
//
// {
//   int iVar1;
//   int iVar2;
//   short sVar3;
//   int iVar4;
//   uint uVar5;
//   ModelType *pMVar6;
//   MotionDataType *pMVar7;
//
//   sVar3 = mmp->count;
//   pMVar7 = mmp->motion;
//   mmp->count = sVar3 + 1;
//   sVar3 = -sVar3;
//   if ((mmp->mask & 1U) != 0) {
//     pMVar6 = *mmp->model->object;
//     iVar2 = (pMVar6->locate).coord.t[0];
//     iVar4 = (int)sVar3;
//     iVar1 = pMVar7->locate->x - iVar2;
//     if (iVar4 == 0) {
//       trap(0x1c00);
//     }
//     if ((iVar4 == -1) && (iVar1 == -0x80000000)) {
//       trap(0x1800);
//     }
//     (pMVar6->locate).coord.t[0] = iVar2 + iVar1 / iVar4;
//     iVar2 = (pMVar6->locate).coord.t[2];
//     iVar1 = pMVar7->locate->z - iVar2;
//     if (iVar4 == 0) {
//       trap(0x1c00);
//     }
//     if ((iVar4 == -1) && (iVar1 == -0x80000000)) {
//       trap(0x1800);
//     }
//     (pMVar6->locate).coord.t[2] = iVar2 + iVar1 / iVar4;
//     iVar2 = (pMVar6->locate).coord.t[1];
//     iVar1 = ((int)(mmp->model->rotate).pad * (int)pMVar7->locate->y >> 0xc) - iVar2;
//     if (iVar4 == 0) {
//       trap(0x1c00);
//     }
//     if ((iVar4 == -1) && (iVar1 == -0x80000000)) {
//       trap(0x1800);
//     }
//     (pMVar6->locate).coord.t[1] = iVar2 + iVar1 / iVar4;
//     iVar1 = (int)pMVar7->rotate[0]->x - (int)(pMVar6->rotate).vx;
//     if (iVar4 == 0) {
//       trap(0x1c00);
//     }
//     if ((iVar4 == -1) && (iVar1 == -0x80000000)) {
//       trap(0x1800);
//     }
//     (pMVar6->rotate).vx = (pMVar6->rotate).vx + (short)(iVar1 / iVar4);
//     iVar1 = (int)pMVar7->rotate[0]->y - (int)(pMVar6->rotate).vy;
//     if (iVar4 == 0) {
//       trap(0x1c00);
//     }
//     if ((iVar4 == -1) && (iVar1 == -0x80000000)) {
//       trap(0x1800);
//     }
//     (pMVar6->rotate).vy = (pMVar6->rotate).vy + (short)(iVar1 / iVar4);
//     iVar1 = (int)pMVar7->rotate[0]->z - (int)(pMVar6->rotate).vz;
//     if (iVar4 == 0) {
//       trap(0x1c00);
//     }
//     if ((iVar4 == -1) && (iVar1 == -0x80000000)) {
//       trap(0x1800);
//     }
//     (pMVar6->rotate).vz = (pMVar6->rotate).vz + (short)(iVar1 / iVar4);
//     UpdateCoordinate(pMVar6);
//   }
//   iVar1 = 1;
//   if (1 < mmp->n) {
//     iVar2 = (int)sVar3;
//     do {
//       uVar5 = (uint)(short)iVar1;
//       if (((int)mmp->mask >> (uVar5 & 0x1f) & 1U) != 0) {
//         pMVar6 = mmp->model->object[uVar5];
//         iVar4 = (int)pMVar7->rotate[uVar5]->x - (int)(pMVar6->rotate).vx;
//         if (iVar2 == 0) {
//           trap(0x1c00);
//         }
//         if ((iVar2 == -1) && (iVar4 == -0x80000000)) {
//           trap(0x1800);
//         }
//         (pMVar6->rotate).vx = (pMVar6->rotate).vx + (short)(iVar4 / iVar2);
//         iVar4 = (int)pMVar7->rotate[uVar5]->y - (int)(pMVar6->rotate).vy;
//         if (iVar2 == 0) {
//           trap(0x1c00);
//         }
//         if ((iVar2 == -1) && (iVar4 == -0x80000000)) {
//           trap(0x1800);
//         }
//         (pMVar6->rotate).vy = (pMVar6->rotate).vy + (short)(iVar4 / iVar2);
//         iVar4 = (int)pMVar7->rotate[uVar5]->z - (int)(pMVar6->rotate).vz;
//         if (iVar2 == 0) {
//           trap(0x1c00);
//         }
//         if ((iVar2 == -1) && (iVar4 == -0x80000000)) {
//           trap(0x1800);
//         }
//         (pMVar6->rotate).vz = (pMVar6->rotate).vz + (short)(iVar4 / iVar2);
//         UpdateCoordinate(pMVar6);
//       }
//       iVar1 = iVar1 + 1;
//     } while (iVar1 * 0x10000 >> 0x10 < (int)mmp->n);
//   }
//   return mmp->count;
// }
