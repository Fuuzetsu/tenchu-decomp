#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void UpdateItemState(void);
 *     ITEM.C:3176, 25 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $s4       int i
 *     reg   $s1       int mode
 *     reg   $v0       int t
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/UpdateItemState", UpdateItemState);

// triage: EASY — 110 insns, 3 callees, ~0.15 to ClearItemLayout
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void UpdateItemState(void)
//
// {
//   short sVar1;
//   short sVar2;
//   bool bVar3;
//   short sVar4;
//   int iVar5;
//   ModelType *model;
//   tag_TItem *ptVar6;
//   int iVar7;
//
//   ptVar6 = items;
//   for (iVar7 = 0; iVar7 < 0x1e; iVar7 = iVar7 + 1) {
//     if (ptVar6->proc != (undefined **)0x0) {
//       bVar3 = false;
//       iVar5 = abs(ViewInfo.vpx - (ptVar6->locate->locate).coord.t[0]);
//       if ((iVar5 < 15000) &&
//          (iVar5 = abs(ViewInfo.vpy - (ptVar6->locate->locate).coord.t[1]), iVar5 < 15000)) {
//         iVar5 = abs(ViewInfo.vpz - (ptVar6->locate->locate).coord.t[2]);
//         bVar3 = iVar5 < 15000;
//       }
//       if (bVar3) {
//         if (((ptVar6->collision).pause != 0) && (sVar1 = (ptVar6->collision).size, sVar1 != 0)) {
//           sVar2 = (ptVar6->collision).ofsY;
//           iVar5 = (ptVar6->collision).mode;
//           DeleteConflict(ptVar6->locate);
//           sVar4 = InsertConflict(ptVar6->locate);
//           ConflictObject[sVar4].offset.vx = 0;
//           ConflictObject[sVar4].offset.vz = 0;
//           ConflictObject[sVar4].offset.vy = sVar2;
//           ConflictObject[sVar4].size.vz = sVar1;
//           ConflictObject[sVar4].size.vy = sVar1;
//           ConflictObject[sVar4].size.vx = sVar1;
//           ConflictObject[sVar4].common = (undefined *)0x1;
//           ConflictObject[sVar4].size.pad = (short)iVar5;
//           (ptVar6->collision).size = sVar1;
//           (ptVar6->collision).ofsY = sVar2;
//           (ptVar6->collision).mode = iVar5;
//           (ptVar6->collision).pause = 0;
//         }
//       }
//       else if (((ptVar6->collision).pause == 0) &&
//               ((ptVar6->locate->locate).super == (_GsCOORDINATE2 *)0x0)) {
//         model = ptVar6->locate;
//         (ptVar6->collision).pause = 1;
//         DeleteConflict(model);
//       }
//     }
//     ptVar6 = ptVar6 + 1;
//   }
//   return;
// }
