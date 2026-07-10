#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TItem items[30];
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80034dbc", FUN_80034dbc);

// triage: MEDIUM — 182 insns, mul/div, 4 callees, ~0.06 to ReqItemShinsoku
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_80034dbc(undefined4 *param_1)
//
// {
//   long lVar1;
//   short sVar2;
//   int iVar3;
//   int iVar4;
//   long lVar5;
//   uint uVar6;
//   uint uVar7;
//   int iVar8;
//   int iVar9;
//   long x;
//   undefined2 local_30;
//   undefined2 local_2e;
//   ushort local_2c;
//
//   lVar1 = ViewInfo.vrz;
//   lVar5 = ViewInfo.vrx;
//   x = param_1[1] + (int)*(short *)(param_1 + 7);
//   iVar9 = param_1[3] + (int)*(short *)(param_1 + 8);
//   iVar8 = param_1[2] + (int)*(short *)((int)param_1 + 0x1e);
//   if ((int)param_1[4] < iVar8) {
// LAB_80034f60:
//     *param_1 = 0;
//   }
//   else {
//     uVar6 = iVar8 - ViewInfo.vry;
//     uVar7 = x - ViewInfo.vrx;
//     if (3000 < (int)uVar6) {
//       iVar8 = ViewInfo.vry + (uVar6 % 6000 - 3000);
//     }
//     iVar3 = abs(uVar7);
//     if (3000 < iVar3) {
//       x = lVar5 + (uVar7 % 6000 - 3000);
//     }
//     uVar7 = iVar9 - lVar1;
//     iVar4 = abs(uVar7);
//     if (3000 < iVar4) {
//       iVar9 = lVar1 + (uVar7 % 6000 - 3000);
//     }
//     if (3000 < iVar4 || (3000 < iVar3 || 3000 < (int)uVar6)) {
//       lVar5 = GetAreaMapLevel(GlobalAreaMap,x,param_1[5]);
//       if (lVar5 < iVar8) goto LAB_80034f60;
//       param_1[4] = lVar5;
//     }
//     param_1[1] = x;
//     param_1[2] = iVar8;
//     param_1[3] = iVar9;
//     iVar8 = (&DAT_80097f2c)[*(byte *)((int)param_1 + 0x22)];
//     iVar3 = param_1[6];
//     FUN_800396c0();
//     iVar9 = (int)(short)local_2c;
//     if (0x24 < iVar9) {
//       if (iVar9 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar9 == -1) && (iVar3 * 300 == -0x80000000)) {
//         trap(0x1800);
//       }
//       sVar2 = (short)((iVar3 * 300) / iVar9) + 1;
//       *(short *)(iVar8 + 0x86) = sVar2;
//       *(short *)(iVar8 + 0x84) = sVar2;
//       *(undefined2 *)(iVar8 + 0x6c) = local_30;
//       *(undefined2 *)(iVar8 + 0x6e) = local_2e;
//       iVar9 = (int)((uint)local_2c << 0x10) >> 0x12;
//       if (iVar9 < 0) {
//         iVar3 = 0;
//       }
//       else {
//         iVar3 = 0x4e1;
//         if (iVar9 < 0x4e2) {
//           iVar3 = iVar9;
//         }
//       }
//       GsSortSprite((GsSPRITE *)(iVar8 + 0x68),OTablePt,(ushort)iVar3);
//     }
//   }
//   return;
// }
