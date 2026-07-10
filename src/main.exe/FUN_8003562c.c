#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE sprBlood;
 *     extern struct GsOT *OTablePt;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct AreaNodeType *FieldArea;
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8003562c", FUN_8003562c);

// triage: HARD — 582 insns, mul/div, 1 loop, 7 callees, ~0.07 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_8003562c(undefined4 *param_1)
//
// {
//   byte bVar1;
//   undefined1 uVar2;
//   short sVar3;
//   uint uVar4;
//   int iVar5;
//   _180fake_1 *p_Var6;
//   AreaNodeType *node;
//   tag_EffectSlot *ptVar7;
//   GsSPRITE *_29;
//   ushort uVar8;
//   int iVar9;
//   int iVar10;
//   uchar uVar11;
//   int iVar12;
//   long lVar13;
//   GsSPRITE *_29_00;
//   undefined4 local_48;
//   int local_44;
//   AreaNodeType *local_40;
//   long local_3c;
//   long local_38;
//   long local_34;
//   AreaNodeType *local_30;
//   long local_2c;
//   int local_28;
//   long local_24;
//
//   uVar4 = (uint)*(byte *)((int)param_1 + 0x26);
//   iVar5 = uVar4 * 0x24;
//   _29_00 = &sprBlood + uVar4;
//   _29 = (GsSPRITE *)(&DAT_800be9e8 + uVar4 * 9);
//   bVar1 = *(byte *)((int)param_1 + 0x27);
//   if (bVar1 == 2) {
//     uVar8 = *(ushort *)(param_1 + 7);
//     *(ushort *)(param_1 + 7) = uVar8 - 1;
//     if ((int)((uint)uVar8 << 0x10) < 1) {
//       *(undefined2 *)(param_1 + 7) = 0x80;
//       *(char *)((int)param_1 + 0x27) = *(char *)((int)param_1 + 0x27) + '\x01';
//     }
//   }
//   else {
//     if (bVar1 < 3) {
//       if (bVar1 == 1) {
//         iVar9 = rand();
//         iVar5 = iVar9;
//         if (iVar9 < 0) {
//           iVar5 = iVar9 + 0xfff;
//         }
//         uVar8 = *(ushort *)(param_1 + 7);
//         param_1[5] = param_1[5] + iVar9 + (iVar5 >> 0xc) * -0x1000;
//         *(ushort *)(param_1 + 7) = uVar8 - 1;
//         if ((int)((uint)uVar8 << 0x10) < 1) {
//           *(char *)((int)param_1 + 0x27) = *(char *)((int)param_1 + 0x27) + '\x01';
//           iVar5 = rand();
//           *(short *)(param_1 + 7) = (short)iVar5 + (short)(iVar5 / 0x5a) * -0x5a;
//         }
//         goto LAB_80035df0;
//       }
//     }
//     else if (bVar1 == 3) {
//       sVar3 = *(short *)(param_1 + 9);
//       *(ushort *)(param_1 + 9) = sVar3 - 5U;
//       if ((int)((uint)(ushort)(sVar3 - 5U) << 0x10) < 1) {
//         *(undefined2 *)(param_1 + 9) = 0;
//         *param_1 = 0;
//       }
//       _29_00->attribute = 0x50000000;
//       iVar9 = param_1[3];
//       iVar12 = param_1[5];
//       uVar8 = *(ushort *)(param_1 + 9);
//       param_1[3] = iVar9 + *(short *)(param_1 + 8);
//       lVar13 = param_1[6];
//       iVar10 = (uint)uVar8 << 0x10;
//       FUN_800396c0(param_1[2],iVar9 + *(short *)(param_1 + 8),param_1[4],&local_48);
//       iVar9 = (int)(short)local_44;
//       if (iVar9 < 0x25) {
//         return;
//       }
//       if (iVar9 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar9 == -1) && (iVar12 * 300 == -0x80000000)) {
//         trap(0x1800);
//       }
//       sVar3 = (short)((iVar12 * 300) / iVar9) + 1;
//       (&sprBlood)[uVar4].scaley = sVar3;
//       (&sprBlood)[uVar4].scalex = sVar3;
//       *(short *)(&DAT_800bea06 + iVar5) = sVar3;
//       *(short *)(&DAT_800bea04 + iVar5) = sVar3;
//       (&sprBlood)[uVar4].rotate = lVar13;
//       *(long *)(&DAT_800bea08 + iVar5) = lVar13;
//       (&sprBlood)[uVar4].x = (short)local_48;
//       *(short *)(&DAT_800be9ec + iVar5) = (short)local_48;
//       (&sprBlood)[uVar4].y = local_48._2_2_;
//       *(short *)(&DAT_800be9ee + iVar5) = local_48._2_2_;
//       uVar11 = (uchar)uVar8;
//       (&sprBlood)[uVar4].r = uVar11;
//       (&sprBlood)[uVar4].g = uVar11;
//       (&sprBlood)[uVar4].b = uVar11;
//       uVar2 = (undefined1)((iVar10 >> 0x10) - (iVar10 >> 0x1f) >> 1);
//       (&DAT_800be9fc)[iVar5] = uVar2;
//       (&DAT_800be9fd)[iVar5] = uVar2;
//       (&DAT_800be9fe)[iVar5] = uVar2;
//       iVar5 = (local_44 << 0x10) >> 0x12;
//       if (iVar5 < 0) {
//         iVar9 = 0;
//       }
//       else {
//         iVar9 = 0x4e1;
//         if (iVar5 < 0x4e2) {
//           iVar9 = iVar5;
//         }
//       }
//       GsSortSprite(_29_00,OTablePt,(ushort)iVar9);
//       iVar5 = (local_44 << 0x10) >> 0x12;
//       if (iVar5 < 0) {
//         iVar9 = 0;
//       }
//       else {
//         iVar9 = 0x4e1;
//         if (iVar5 < 0x4e2) {
//           iVar9 = iVar5;
//         }
//       }
//       uVar8 = (ushort)iVar9;
//       goto LAB_80035f10;
//     }
//     iVar12 = param_1[3];
//     iVar5 = (int)param_1[2] / 10;
//     *(short *)(param_1 + 8) = *(short *)(param_1 + 8) + 10;
//     node = (AreaNodeType *)param_1[1];
//     iVar9 = (int)param_1[4] / 10;
//     if (node == (AreaNodeType *)0x0) {
// LAB_800359d4:
//       iVar10 = GetAreaMapLevel(GlobalAreaMap,param_1[2],iVar12 + -300);
//       if ((iVar12 <= iVar10) && (FieldArea->division == -1)) {
//         param_1[1] = FieldArea;
//       }
//     }
//     else {
//       iVar10 = (int)node->y;
//       if (((((iVar12 / 10 < iVar10 + -200) || (iVar10 < iVar12 / 10)) || (iVar5 < node->x1)) ||
//           ((iVar9 < node->z1 || (node->x2 < iVar5)))) || (node->z2 < iVar9)) goto LAB_800359d4;
//       if ((node->dy == 0) || (iVar10 = ComputeAreaLevel(node,iVar5,iVar9), iVar10 != -0x80000000)) {
//         iVar10 = iVar10 * 10;
//       }
//     }
//     if ((int)param_1[3] < iVar10) {
//       uVar8 = *(ushort *)(param_1 + 7);
//       *(ushort *)(param_1 + 7) = uVar8 - 1;
//       if ((int)((uint)uVar8 << 0x10) < 1) {
//         *param_1 = 0;
//       }
//     }
//     else {
//       *(undefined2 *)((int)param_1 + 0x22) = 0;
//       *(undefined2 *)(param_1 + 8) = 0;
//       *(undefined2 *)((int)param_1 + 0x1e) = 0;
//       if (iVar10 == -0x80000000) {
//         iVar9 = rand();
//         iVar5 = iVar9;
//         if (iVar9 < 0) {
//           iVar5 = iVar9 + 7;
//         }
//         *(short *)(param_1 + 8) = (short)iVar9 + (short)(iVar5 >> 3) * -8 + 8;
//         param_1[6] = 0;
//         iVar5 = rand();
//         *(char *)((int)param_1 + 0x26) = *(char *)((int)param_1 + 0x26) + '\x02';
//         param_1[5] = iVar5 % 0x2ab + 0x555;
//       }
//       else {
//         param_1[3] = iVar10;
//       }
//       *(undefined1 *)((int)param_1 + 0x27) = 1;
//       iVar5 = rand();
//       *(short *)(param_1 + 7) = (short)iVar5 + (short)(iVar5 / 10) * -10;
//       SoundEx((VECTOR *)(param_1 + 2),0x37);
//     }
//     memset((uchar *)&local_30,'\0',0x10);
//     iVar5 = rand();
//     local_30 = (AreaNodeType *)(param_1[2] + -0x3c + iVar5 % 0x78);
//     iVar5 = rand();
//     local_2c = param_1[3] + -0x3c + iVar5 % 0x78;
//     iVar5 = rand();
//     local_38 = param_1[4] + -0x3c + iVar5 % 0x78;
//     local_40 = local_30;
//     local_3c = local_2c;
//     local_34 = local_24;
//     local_28 = local_38;
//     memset((uchar *)&local_30,'\0',8);
//     iVar9 = 0;
//     iVar5 = (uint)*(ushort *)((int)param_1 + 0x1e) << 0x10;
//     local_48 = (AreaNodeType *)
//                CONCAT22((short)(((int)((uint)*(ushort *)(param_1 + 8) << 0x10) >> 0x10) -
//                                 ((int)((uint)*(ushort *)(param_1 + 8) << 0x10) >> 0x1f) >> 1),
//                         (short)((iVar5 >> 0x10) - (iVar5 >> 0x1f) >> 1));
//     ptVar7 = EffectSlot + DAT_80097a3c;
//     iVar5 = (uint)*(ushort *)((int)param_1 + 0x22) << 0x10;
//     local_2c = CONCAT22(local_2c._2_2_,(short)((iVar5 >> 0x10) - (iVar5 >> 0x1f) >> 1));
//     local_44 = local_2c;
//     iVar5 = DAT_80097a3c;
//     do {
//       iVar5 = iVar5 + 1;
//       ptVar7 = ptVar7 + 1;
//       if (199 < iVar5) {
//         iVar5 = 0;
//         ptVar7 = EffectSlot;
//       }
//       iVar9 = iVar9 + 1;
//       if (ptVar7->proc == (undefined **)0x0) {
//         DAT_80097a3c = iVar5 + 1;
//         p_Var6 = &ptVar7->param;
//         if (199 < DAT_80097a3c) {
//           DAT_80097a3c = 0;
//         }
//         goto LAB_80035d88;
//       }
//     } while (iVar9 < 200);
//     ptVar7 = &dmy;
//     p_Var6 = &dmy.param;
// LAB_80035d88:
//     (ptVar7->param).blood.hint = local_40;
//     (ptVar7->param).blood.px = local_3c;
//     (ptVar7->param).blood.py = local_38;
//     (ptVar7->param).blood.pz = local_34;
//     (ptVar7->param).blood.scale = (long)local_48;
//     (ptVar7->param).blood.rotate = local_2c;
//     (p_Var6->bleed).time = '\a';
//     *(undefined1 *)((int)p_Var6 + 0x18) = 0x7f;
//     (p_Var6->bleed).g = '\x10';
//     (p_Var6->bleed).b = '\x17';
//     *(undefined1 *)((int)p_Var6 + 0x1c) = 0;
//     ptVar7->proc = (undefined **)DrawBleed;
//     local_30 = local_48;
//   }
// LAB_80035df0:
//   param_1[3] = param_1[3] + (int)*(short *)(param_1 + 8);
//   param_1[2] = param_1[2] + (int)*(short *)((int)param_1 + 0x1e);
//   param_1[4] = param_1[4] + (int)*(short *)((int)param_1 + 0x22);
//   (&sprBlood)[uVar4].rotate = param_1[6];
//   _29_00->attribute = 0;
//   (&sprBlood)[uVar4].r = *(uchar *)(param_1 + 9);
//   (&sprBlood)[uVar4].g = *(uchar *)(param_1 + 9);
//   (&sprBlood)[uVar4].b = *(uchar *)(param_1 + 9);
//   iVar9 = param_1[5];
//   FUN_800396c0(param_1[2],param_1[3],param_1[4],&local_48);
//   iVar5 = (int)(short)local_44;
//   if (iVar5 < 0x25) {
//     return;
//   }
//   if (iVar5 == 0) {
//     trap(0x1c00);
//   }
//   if ((iVar5 == -1) && (iVar9 * 300 == -0x80000000)) {
//     trap(0x1800);
//   }
//   sVar3 = (short)((iVar9 * 300) / iVar5) + 1;
//   (&sprBlood)[uVar4].scaley = sVar3;
//   (&sprBlood)[uVar4].scalex = sVar3;
//   (&sprBlood)[uVar4].x = (short)local_48;
//   (&sprBlood)[uVar4].y = local_48._2_2_;
//   iVar5 = (local_44 << 0x10) >> 0x12;
//   if (iVar5 < 0) {
//     iVar9 = 0;
//   }
//   else {
//     iVar9 = 0x4e1;
//     if (iVar5 < 0x4e2) {
//       iVar9 = iVar5;
//     }
//   }
//   uVar8 = (ushort)iVar9;
//   _29 = _29_00;
// LAB_80035f10:
//   GsSortSprite(_29,OTablePt,uVar8);
//   return;
// }
