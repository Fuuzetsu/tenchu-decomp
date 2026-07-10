#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short UpdateMotion(struct MotionManager *mmp, short mid);
 *     ACTION.C:182, 34 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s0       struct MotionManager * mmp
 *     param $t0       short mid
 *     reg   $a0       struct MotionRegistType * mrp
 *     reg   $a2       short i
 *     reg   $a1       short j
 *     reg   $a3       short * xyz
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionRegistType MOTcommon[41];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/UpdateMotion", UpdateMotion);

// triage: MEDIUM — 158 insns, 4 loop, 1 callees, ~0.10 to GetMotionID
// likely-relevant cookbook sections:
//   - Loops: 4 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short UpdateMotion(MotionManager *mmp,short mid)
//
// {
//   byte bVar1;
//   short sVar2;
//   short sVar3;
//   MotionDataType *pMVar4;
//   int iVar5;
//   ushort uVar6;
//   int iVar7;
//   ushort uVar8;
//   MotionRegistType *pMVar9;
//   ModelArchiveType *pMVar10;
//   short *psVar11;
//   int iVar12;
//   int iVar13;
//   int iVar14;
//
//   sVar3 = -1;
//   if (mid != mmp->mid) {
//     pMVar9 = mmp->motreg;
//     sVar3 = pMVar9->mid;
//     iVar13 = 0;
//     while ((sVar3 != mid && (*(short *)((int)&pMVar9->mid + ((iVar13 << 0x10) >> 0xd)) != -1))) {
//       sVar3 = *(short *)((int)&pMVar9->mid + ((iVar13 + 1) * 0x10000 >> 0xd));
//       iVar13 = iVar13 + 1;
//     }
//     if (*(int *)((int)&pMVar9->motion + ((iVar13 << 0x10) >> 0xd)) == 0) {
//       pMVar9 = MOTcommon;
//       iVar13 = 0;
//       sVar3 = MOTcommon[0].mid;
//       while ((sVar3 != mid && (*(short *)((int)&MOTcommon[0].mid + ((iVar13 << 0x10) >> 0xd)) != -1)
//              )) {
//         sVar3 = *(short *)((int)&MOTcommon[0].mid + ((iVar13 + 1) * 0x10000 >> 0xd));
//         iVar13 = iVar13 + 1;
//       }
//       if (*(int *)((int)&MOTcommon[0].motion + ((iVar13 << 0x10) >> 0xd)) == 0) {
//         return 0;
//       }
//     }
//     pMVar4 = *(MotionDataType **)((int)&pMVar9->motion + ((iVar13 << 0x10) >> 0xd));
//     mmp->mid = mid;
//     mmp->motion = pMVar4;
//     bVar1 = pMVar4->sweep;
//     mmp->count = (ushort)bVar1;
//     if ((bVar1 & 0x80) != 0) {
//       mmp->count = bVar1 - 0x100;
//     }
//     mmp->loop = 0;
//     uVar8 = mmp->model->n;
//     uVar6 = (ushort)mmp->motion->n;
//     if ((short)uVar6 < (short)uVar8) {
//       uVar8 = uVar6;
//     }
//     mmp->n = uVar8;
//     SetupSpline(mmp);
//     pMVar10 = mmp->model;
//     iVar13 = 0;
//     if (0 < pMVar10->n) {
//       iVar5 = 0;
//       do {
//         iVar12 = 0;
//         iVar14 = *(int *)((iVar5 >> 0xe) + (int)pMVar10->object) + 0x50;
//         iVar5 = 0;
//         do {
//           psVar11 = (short *)((iVar5 >> 0xf) + iVar14);
//           sVar3 = *psVar11;
//           iVar7 = (int)sVar3;
//           iVar5 = iVar7;
//           if (iVar7 < 0) {
//             iVar5 = -iVar7;
//           }
//           if (0x800 < iVar5) {
//             if (iVar7 < 0) {
//               sVar2 = 0x1000;
//             }
//             else {
//               sVar2 = -0x1000;
//             }
//             *psVar11 = sVar3 + sVar2;
//           }
//           psVar11 = (short *)(((iVar12 << 0x10) >> 0xf) + iVar14);
//           sVar3 = *psVar11;
//           iVar5 = (int)sVar3;
//           if (iVar5 < 0) {
//             iVar5 = iVar5 + 0xfff;
//           }
//           *psVar11 = sVar3 + (short)(iVar5 >> 0xc) * -0x1000;
//           iVar12 = iVar12 + 1;
//           iVar5 = iVar12 * 0x10000;
//         } while (iVar12 * 0x10000 >> 0x10 < 3);
//         iVar13 = iVar13 + 1;
//         pMVar10 = mmp->model;
//         iVar5 = iVar13 * 0x10000;
//       } while (iVar13 * 0x10000 >> 0x10 < (int)pMVar10->n);
//     }
//     sVar3 = 1;
//   }
//   return sVar3;
// }
