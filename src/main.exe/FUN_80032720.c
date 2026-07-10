#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80032720", FUN_80032720);

// triage: MEDIUM — 140 insns, mul/div, 3 loop, 1 callees, ~0.05 to ReqItemLaunch
// likely-relevant cookbook sections:
//   - Loops: 3 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_80032720(int param_1,short param_2,short param_3)
//
// {
//   short sVar1;
//   short sVar2;
//   short sVar3;
//   int iVar4;
//   int iVar5;
//   int iVar6;
//   tag_EffectSlot *ptVar7;
//
//   sVar2 = DAT_80097f32;
//   sVar3 = DAT_80097f30;
//   iVar6 = 0;
//   ptVar7 = EffectSlot + DAT_80097a3c;
//   iVar5 = DAT_80097a3c;
//   while( true ) {
//     iVar5 = iVar5 + 1;
//     ptVar7 = ptVar7 + 1;
//     if (199 < iVar5) {
//       iVar5 = 0;
//       ptVar7 = EffectSlot;
//     }
//     if (ptVar7->proc == (undefined **)0x0) break;
//     iVar6 = iVar6 + 1;
//     if (199 < iVar6) {
//       ptVar7 = &dmy;
// LAB_800327dc:
//       (ptVar7->param).smoke.vec.vy = 0;
//       (ptVar7->param).smoke.vec.vx = 0;
//       (ptVar7->param).splash.sx = sVar3;
//       (ptVar7->param).splash.sy = sVar2;
//       sVar1 = *(short *)(param_1 + 4);
//       iVar5 = 0;
//       *(short *)((int)&ptVar7->param + 8) = sVar1;
//       (ptVar7->param).bleed.vec.vx = sVar1;
//       sVar1 = *(short *)(param_1 + 6);
//       *(short *)((int)&ptVar7->param + 10) = sVar1;
//       (ptVar7->param).bleed.vec.vy = sVar1;
//       (ptVar7->param).bleed.vec.vz = *(short *)(param_1 + 8);
//       (ptVar7->param).bleed.vec.pad = *(short *)(param_1 + 10);
//       do {
//         iVar4 = 0;
//         iVar6 = 0;
//         do {
//           if ((0xf >> ((short)iVar5 * 2 + (iVar6 >> 0x10) & 0x1fU) & 1U) != 0) {
//             MoveImage((RECT *)&(ptVar7->param).bleed.vec,
//                       (int)sVar3 + (uint)*(ushort *)(param_1 + 8) * (iVar6 >> 0x10),
//                       (int)sVar2 + (uint)*(ushort *)(param_1 + 10) * (int)(short)iVar5);
//           }
//           iVar4 = iVar4 + 1;
//           iVar6 = iVar4 * 0x10000;
//         } while (iVar4 * 0x10000 >> 0x10 < 2);
//         iVar5 = iVar5 + 1;
//       } while (iVar5 * 0x10000 >> 0x10 < 2);
//       sVar3 = DAT_80097f32 + 0x40;
//       DAT_80097f32 = sVar3;
//       (ptVar7->param).smoke.vec.vz = param_2;
//       (ptVar7->param).smoke.vec.pad = param_3;
//       ptVar7->proc = (undefined **)UpdateTexScroll;
//       if (0x200 < sVar3) {
//         DAT_80097f32 = 0x100;
//         DAT_80097f30 = DAT_80097f30 + 0x40;
//       }
//       return;
//     }
//   }
//   DAT_80097a3c = iVar5 + 1;
//   if (199 < iVar5 + 1) {
//     DAT_80097a3c = 0;
//   }
//   goto LAB_800327dc;
// }
