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

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80033bc0", FUN_80033bc0);

// triage: MEDIUM — 212 insns, mul/div, 1 loop, 1 callees, ~0.07 to AdtFntOpen
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_80033bc0(long *param_1,ushort param_2,short param_3,short param_4)
//
// {
//   short sVar1;
//   short sVar2;
//   short sVar3;
//   short sVar4;
//   short sVar5;
//   short sVar6;
//   int iVar7;
//   int iVar8;
//   _180fake_1 *p_Var9;
//   tag_EffectSlot *ptVar10;
//   int iVar11;
//
//   iVar11 = 0;
//   do {
//     iVar8 = 0;
//     if (param_4 <= iVar11) {
//       return;
//     }
//     ptVar10 = EffectSlot + DAT_80097a3c;
//     iVar7 = DAT_80097a3c;
//     do {
//       iVar7 = iVar7 + 1;
//       ptVar10 = ptVar10 + 1;
//       if (199 < iVar7) {
//         iVar7 = 0;
//         ptVar10 = EffectSlot;
//       }
//       if (ptVar10->proc == (undefined **)0x0) {
//         DAT_80097a3c = iVar7 + 1;
//         if (199 < iVar7 + 1) {
//           DAT_80097a3c = 0;
//         }
//         goto LAB_80033ca0;
//       }
//       iVar8 = iVar8 + 1;
//     } while (iVar8 < 200);
//     ptVar10 = &dmy;
// LAB_80033ca0:
//     iVar8 = (int)((uint)param_2 << 0x10) >> 0xf;
//     p_Var9 = &ptVar10->param;
//     if (iVar8 < 1) {
//       (ptVar10->param).smoke.vec.vx = -param_2;
//     }
//     else {
//       iVar7 = rand();
//       if (iVar8 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar8 == -1) && (iVar7 == -0x80000000)) {
//         trap(0x1800);
//       }
//       (ptVar10->param).smoke.vec.vx = (short)(iVar7 % iVar8) - param_2;
//     }
//     (ptVar10->param).smoke.vec.vy = -5;
//     iVar8 = (int)((uint)param_2 << 0x10) >> 0xf;
//     if (iVar8 < 1) {
//       (ptVar10->param).smoke.vec.vz = -param_2;
//     }
//     else {
//       iVar7 = rand();
//       if (iVar8 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar8 == -1) && (iVar7 == -0x80000000)) {
//         trap(0x1800);
//       }
//       (ptVar10->param).smoke.vec.vz = (short)(iVar7 % iVar8) - param_2;
//     }
//     sVar1 = (p_Var9->smoke).vec.vx;
//     if (param_3 == 0) {
//       trap(0x1c00);
//     }
//     if ((param_3 == -1) && (sVar1 == -0x80000000)) {
//       trap(0x1800);
//     }
//     sVar2 = (ptVar10->param).smoke.vec.vy;
//     if (param_3 == 0) {
//       trap(0x1c00);
//     }
//     if ((param_3 == -1) && (sVar2 == -0x80000000)) {
//       trap(0x1800);
//     }
//     sVar3 = (ptVar10->param).smoke.vec.vz;
//     if (param_3 == 0) {
//       trap(0x1c00);
//     }
//     if ((param_3 == -1) && (sVar3 == -0x80000000)) {
//       trap(0x1800);
//     }
//     sVar4 = (ptVar10->param).smoke.vec.vz;
//     sVar5 = (p_Var9->smoke).vec.vx;
//     (ptVar10->param).blood.py = *param_1;
//     (ptVar10->param).blood.pz = param_1[1];
//     (ptVar10->param).blood.scale = param_1[2];
//     sVar6 = (ptVar10->param).smoke.vec.vy;
//     (ptVar10->param).blood.py = (ptVar10->param).blood.py + (int)sVar5;
//     (ptVar10->param).blood.pz = (ptVar10->param).blood.pz + (int)sVar6;
//     (ptVar10->param).blood.scale = (ptVar10->param).blood.scale + (int)sVar4;
//     (p_Var9->smoke).vec.vx = sVar1 / param_3;
//     (ptVar10->param).smoke.vec.vy = sVar2 / param_3;
//     (ptVar10->param).smoke.vec.vz = sVar3 / param_3;
//     iVar7 = rand();
//     iVar8 = iVar7;
//     if (iVar7 < 0) {
//       iVar8 = iVar7 + 0x1fff;
//     }
//     (ptVar10->param).smoke.scale = iVar7 + (iVar8 >> 0xd) * -0x2000 + 0x1000;
//     (ptVar10->param).smoke.rotate = 0;
//     (ptVar10->param).blood.mode = '\x0f';
//     iVar8 = rand();
//     iVar11 = iVar11 + 1;
//     *(undefined1 *)((int)&ptVar10->param + 0x22) = 1;
//     (ptVar10->param).blood.bright =
//          ((ptVar10->param).blood.mode + 0xf8) - ((char)iVar8 + (char)(iVar8 / 0xf) * -0xf);
//     ptVar10->proc = (undefined **)DrawSmoke;
//   } while( true );
// }
