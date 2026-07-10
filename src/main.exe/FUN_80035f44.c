#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 *     extern long GameClock;
 *
 * PSX.SYM suggests this may be `DrawGore` (LOW confidence, EFFECT.C) — NOT
 * adopted. Corroborate with `tools/callmatch.py --verify` before renaming.
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80035f44", FUN_80035f44);

// triage: MEDIUM — 208 insns, mul/div, 2 loop, 5 callees, ~0.06 to ReqItemNingyo
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_80035f44(GsCOORDINATE2 *param_1,SVECTOR *param_2,undefined4 *param_3)
//
// {
//   short sVar1;
//   short sVar2;
//   short sVar3;
//   int iVar4;
//   uint uVar5;
//   int iVar6;
//   tag_EffectSlot *ptVar7;
//   VECTOR local_78;
//   MATRIX MStack_68;
//   SVECTOR local_48;
//   VECTOR local_40 [2];
//   long alStack_20 [2];
//
//   GsGetLw(param_1,&MStack_68);
//   GsSetLsMatrix(&MStack_68);
//   RotTrans(param_2,&local_78,alStack_20);
//   iVar6 = 0;
//   ptVar7 = EffectSlot + DAT_80097a3c;
//   iVar4 = DAT_80097a3c;
//   do {
//     iVar4 = iVar4 + 1;
//     ptVar7 = ptVar7 + 1;
//     if (199 < iVar4) {
//       iVar4 = 0;
//       ptVar7 = EffectSlot;
//     }
//     if (ptVar7->proc == (undefined **)0x0) {
//       DAT_80097a3c = iVar4 + 1;
//       if (199 < iVar4 + 1) {
//         DAT_80097a3c = 0;
//       }
//       goto LAB_80036014;
//     }
//     iVar6 = iVar6 + 1;
//   } while (iVar6 < 200);
//   ptVar7 = &dmy;
// LAB_80036014:
//   iVar4 = rand();
//   *(char *)((int)&ptVar7->param + 0x22) = (char)iVar4 + (char)(iVar4 / 2) * -2;
//   (ptVar7->param).blood.scale = 0x2000;
//   iVar4 = rand();
//   (ptVar7->param).blood.rotate = (iVar4 % 0x168) * 0x1000;
//   (ptVar7->param).blood.px = local_78.vx;
//   (ptVar7->param).blood.py = local_78.vy;
//   (ptVar7->param).blood.pz = local_78.vz;
//   local_48._0_4_ = *param_3;
//   local_48._4_4_ = param_3[1];
//   ApplyRotMatrix(&local_48,local_40);
//   (ptVar7->param).blood.vx = (short)local_40[0].vx;
//   (ptVar7->param).blood.vy = (short)local_40[0].vy;
//   (ptVar7->param).blood.vz = (short)local_40[0].vz;
//   iVar4 = rand();
//   (ptVar7->param).blood.time = (short)iVar4 + (short)(iVar4 / 0xf) * -0xf + 10;
//   (ptVar7->param).blood.hint = (AreaNodeType *)0x0;
//   *(undefined2 *)((int)&ptVar7->param + 0x20) = 0x80;
//   *(undefined1 *)((int)&ptVar7->param + 0x23) = 0;
//   uVar5 = GameClock & 3;
//   ptVar7->proc = (undefined **)FUN_8003562c;
//   if (uVar5 == 0) {
//     sVar1 = param_2->vx;
//     sVar2 = param_2->vy;
//     iVar6 = 0;
//     sVar3 = param_2->vz;
//     ptVar7 = EffectSlot + DAT_80097a3c;
//     iVar4 = DAT_80097a3c;
//     do {
//       iVar4 = iVar4 + 1;
//       ptVar7 = ptVar7 + 1;
//       if (199 < iVar4) {
//         iVar4 = 0;
//         ptVar7 = EffectSlot;
//       }
//       if (ptVar7->proc == (undefined **)0x0) {
//         DAT_80097a3c = iVar4 + 1;
//         if (199 < iVar4 + 1) {
//           DAT_80097a3c = 0;
//         }
//         goto LAB_80036208;
//       }
//       iVar6 = iVar6 + 1;
//     } while (iVar6 < 200);
//     ptVar7 = &dmy;
// LAB_80036208:
//     ptVar7->proc = (undefined **)FUN_80033f10;
//     *(int *)&ptVar7->param = (int)sVar1;
//     *(int *)((int)&ptVar7->param + 4) = (int)sVar2;
//     (ptVar7->param).blood.vz = 0x50;
//     (ptVar7->param).blood.time = 0x2000;
//     (ptVar7->param).blood.vx = 0x2000;
//     *(undefined1 *)((int)&ptVar7->param + 0x22) = 3;
//     (ptVar7->param).blood.pz = (long)param_1;
//     (ptVar7->param).blood.vy = 0;
//     (ptVar7->param).blood.scale = 0x808080;
//     (ptVar7->param).blood.rotate = 0x808080;
//     (ptVar7->param).blood.bright = '\0';
//     (ptVar7->param).blood.mode = '\x02';
//     *(int *)((int)&ptVar7->param + 8) = (int)sVar3;
//   }
//   return;
// }
