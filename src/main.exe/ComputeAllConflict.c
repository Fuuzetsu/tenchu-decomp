#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ComputeAllConflict(void);
 *     CONFLICT.C:329, 50 src lines, frame 72 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $s1       struct ConflictObjectType * confop
 *     reg   $s0       struct ModelType * model
 *     stack sp+16     struct MATRIX mat
 *     reg   $s2       short i
 *     reg   $t0       short j
 *
 * Globals it touches, as the original declared them:
 *     extern short ConflictObjects;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct ModelType World;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ComputeAllConflict", ComputeAllConflict);

// triage: MEDIUM — 204 insns, 3 loop, 4 callees, ~0.12 to update_something_for_each_visible_enemy_
// likely-relevant cookbook sections:
//   - Loops: 3 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Type propagation algorithm not settling */
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void ComputeAllConflict(void)
//
// {
//   short sVar1;
//   short sVar2;
//   int iVar3;
//   int iVar4;
//   ModelType *pMVar5;
//   int iVar6;
//   ModelType *pMVar7;
//   int iVar8;
//   MATRIX MStack_38;
//
//   iVar8 = 0;
//   if (0 < (short)ConflictObjects) {
//     iVar3 = 0;
//     do {
//       iVar3 = iVar3 >> 0x10;
//       pMVar7 = ConflictObject[iVar3].model;
//       if ((pMVar7->attribute & 0x4000U) != 0) {
//         memset(ConflictObject[iVar3].result,'\0',0x50);
//         ConflictObject[iVar3].offset.pad = 0;
//         pMVar5 = (ModelType *)(pMVar7->locate).super;
//         pMVar7->attribute = pMVar7->attribute & 0x7fff;
//         if (pMVar5 == &World) {
//           sVar2 = ConflictObject[iVar3].offset.vy;
//           ConflictObject[iVar3].position.vx =
//                (pMVar7->locate).coord.t[0] + (int)ConflictObject[iVar3].offset.vx;
//           sVar1 = ConflictObject[iVar3].offset.vz;
//           ConflictObject[iVar3].position.vy = (pMVar7->locate).coord.t[1] + (int)sVar2;
//           ConflictObject[iVar3].position.vz = (pMVar7->locate).coord.t[2] + (int)sVar1;
//         }
//         else {
//           GsGetLw(&pMVar7->locate,&MStack_38);
//           GsSetLsMatrix(&MStack_38);
//           RotTrans(&ConflictObject[iVar3].offset,&ConflictObject[iVar3].position,(long *)0x0);
//         }
//       }
//       iVar8 = iVar8 + 1;
//       iVar3 = iVar8 * 0x10000;
//     } while (iVar8 * 0x10000 >> 0x10 < (int)(short)ConflictObjects);
//   }
//   iVar8 = 0;
//   if (0 < (short)ConflictObjects) {
//     iVar3 = 0;
//     do {
//       iVar3 = iVar3 >> 0x10;
//       iVar4 = iVar8 + 1;
//       if ((((ConflictObject[iVar3].model)->attribute & 0x4000U) != 0) &&
//          (iVar4 * 0x10000 < (int)((uint)ConflictObjects << 0x10))) {
//         do {
//           sVar2 = (short)iVar4;
//           if (((ConflictObject[sVar2].model)->attribute & 0x4000U) != 0) {
//             iVar6 = ConflictObject[sVar2].position.vy - ConflictObject[iVar3].position.vy;
//             if (iVar6 < 0) {
//               iVar6 = -iVar6;
//             }
//             if (iVar6 <= (int)ConflictObject[iVar3].size.vy + (int)ConflictObject[sVar2].size.vy) {
//               iVar6 = ConflictObject[sVar2].position.vz - ConflictObject[iVar3].position.vz;
//               if (iVar6 < 0) {
//                 iVar6 = -iVar6;
//               }
//               if (iVar6 <= (int)ConflictObject[iVar3].size.vz + (int)ConflictObject[sVar2].size.vz)
//               {
//                 iVar6 = ConflictObject[sVar2].position.vx - ConflictObject[iVar3].position.vx;
//                 if (iVar6 < 0) {
//                   iVar6 = -iVar6;
//                 }
//                 if (iVar6 <= (int)ConflictObject[iVar3].size.vx + (int)ConflictObject[sVar2].size.vx
//                    ) {
//                   ConflictObject[iVar3].result[sVar2] = (byte)ConflictObject[sVar2].size.pad | 0x80;
//                   ConflictObject[sVar2].result[iVar3] = (byte)ConflictObject[iVar3].size.pad | 0x80;
//                   pMVar7 = ConflictObject[iVar3].model;
//                   pMVar7->attribute = pMVar7->attribute | 0x8000;
//                   pMVar7 = ConflictObject[sVar2].model;
//                   pMVar7->attribute = pMVar7->attribute | 0x8000;
//                   ConflictObject[iVar3].offset.pad = ConflictObject[iVar3].offset.pad + 1;
//                   ConflictObject[sVar2].offset.pad = ConflictObject[sVar2].offset.pad + 1;
//                 }
//               }
//             }
//           }
//           iVar4 = iVar4 + 1;
//         } while (iVar4 * 0x10000 >> 0x10 < (int)(short)ConflictObjects);
//       }
//       iVar8 = iVar8 + 1;
//       iVar3 = iVar8 * 0x10000;
//     } while (iVar8 * 0x10000 >> 0x10 < (int)(short)ConflictObjects);
//   }
//   return;
// }
