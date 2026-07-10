#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawConstruction(void);
 *     WORLD.C:798, 234 src lines, frame 1920 bytes, saved-reg mask 0xc0ff0000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     reg   $s6       short j
 *     reg   $s4       short k
 *     reg   $s2       unsigned long plimit
 *     reg   $a2       int nx
 *     reg   $a1       int ny
 *     reg   $a0       int nz
 *     reg   $a0       long a
 *     reg   $a0       long a
 *     reg   $a0       long a
 *     stack sp+1848   int ndl
 *     stack sp+1852   int ndt
 *     stack sp+16     struct tag_ObjectSlotType *[153] DrawList
 *     stack sp+632    struct tag_ObjectSlotType [100] Slot
 *     stack sp+1832   struct ObjectSlotManager SlotMan
 *     reg   $s0       int sx
 *     stack sp+1856   int sy
 *     stack sp+1860   int sz
 *     stack sp+1864   int ex
 *     stack sp+1868   int ey
 *     stack sp+1872   int ez
 *     reg   $s3       int i
 *     reg   $s1       struct tag_ObjectSlotType * cur
 *     reg   $v0       long y
 *     reg   $v0       long z
 *     reg   $a1       int sz
 *     reg   $s3       long a
 *     reg   $v0       long x
 *     reg   $a3       long y
 *     reg   $a2       long z
 *     reg   $a1       int sz
 *     reg   $v0       int k
 *     reg   $s2       struct tag_ObjectSlotType ** slot
 *     reg   $s0       struct OrnamentType * model
 *     reg   $s0       struct tag_ObjectSlotType * cur
 *     reg   $s0       struct tag_ObjectSlotType * cur
 *
 * Globals it touches, as the original declared them:
 *     extern short SkipFrame;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct WorldType WorldMap[8][8][8];
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DrawConstruction", DrawConstruction);

// triage: HARD — 391 insns, mul/div, 2 loop, frame 0x7a0, 10 callees, ~0.06 to ReqItemDrop
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - Stack objects: 0x7a0 frame — buffer casts / spills

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void DrawConstruction(void)
//
// {
//   undefined2 uVar1;
//   int iVar2;
//   PACKET *pPVar3;
//   PACKET *pPVar4;
//   uint uVar5;
//   GsOT *ot;
//   char *pcVar6;
//   char *pcVar7;
//   int in_a3;
//   int iVar8;
//   uint uVar9;
//   int *piVar10;
//   undefined4 uVar11;
//   int iVar12;
//   short sVar13;
//   short sVar14;
//   int aiStack_20790 [32768];
//   int local_790 [154];
//   undefined1 auStack_528 [1200];
//   undefined1 *local_78;
//   int local_74;
//   int local_70;
//   GsOT local_68;
//   char *local_50;
//   char *local_4c;
//   int local_48;
//   int local_44;
//   char *local_40;
//   GsOT *local_3c;
//   int local_38;
//   int local_34;
//   int local_30;
//
//   if (SkipFrame != 1) {
//     if (ViewInfo.vpx < 0) {
//       iVar2 = ViewInfo.vpx / 16000 + -1;
//     }
//     else {
//       iVar2 = ViewInfo.vpx / 16000;
//     }
//     if (ViewInfo.vpy < 0) {
//       iVar12 = ViewInfo.vpy / 16000 + -1;
//     }
//     else {
//       iVar12 = ViewInfo.vpy / 16000;
//     }
//     if (ViewInfo.vpz < 0) {
//       local_38 = ViewInfo.vpz / 16000 + -1;
//     }
//     else {
//       local_38 = ViewInfo.vpz / 16000;
//     }
//     iVar8 = iVar2 + -2;
//     local_48 = iVar12 + -1;
//     local_44 = local_38 + -2;
//     pcVar6 = (char *)(iVar2 + 2);
//     ot = (GsOT *)(iVar12 + 1);
//     local_38 = local_38 + 2;
//     iVar12 = 0;
//     local_70 = 100;
//     local_78 = auStack_528;
//     local_50 = (char *)0x0;
//     local_4c = (char *)0x0;
//     local_74 = 0;
//     iVar2 = 0;
//     local_40 = pcVar6;
//     local_3c = ot;
//     do {
//       *(undefined4 *)((int)local_790 + (iVar2 >> 0xe)) = 0;
//       iVar12 = iVar12 + 1;
//       iVar2 = iVar12 * 0x10000;
//     } while (iVar12 * 0x10000 >> 0x10 < 0x99);
//     SetRotMatrix(&GsWSMATRIX);
//     Scratchpad[0x38] = (undefined1)ViewInfo.vpx;
//     Scratchpad[0x39] = ViewInfo.vpx._1_1_;
//     Scratchpad[0x3a] = ViewInfo.vpx._2_1_;
//     Scratchpad[0x3b] = ViewInfo.vpx._3_1_;
//     Scratchpad[0x3c] = (undefined1)ViewInfo.vpy;
//     Scratchpad[0x3d] = ViewInfo.vpy._1_1_;
//     Scratchpad[0x3e] = ViewInfo.vpy._2_1_;
//     Scratchpad[0x3f] = ViewInfo.vpy._3_1_;
//     Scratchpad[0x40] = (undefined1)ViewInfo.vpz;
//     Scratchpad[0x41] = ViewInfo.vpz._1_1_;
//     Scratchpad[0x42] = ViewInfo.vpz._2_1_;
//     Scratchpad[0x43] = ViewInfo.vpz._3_1_;
//     Scratchpad[0x44] = (undefined1)ViewInfo.vrx;
//     Scratchpad[0x45] = ViewInfo.vrx._1_1_;
//     Scratchpad[0x46] = ViewInfo.vrx._2_1_;
//     Scratchpad[0x47] = ViewInfo.vrx._3_1_;
//     Scratchpad[0x48] = (undefined1)ViewInfo.vry;
//     Scratchpad[0x49] = ViewInfo.vry._1_1_;
//     Scratchpad[0x4a] = ViewInfo.vry._2_1_;
//     Scratchpad[0x4b] = ViewInfo.vry._3_1_;
//     Scratchpad[0x4c] = (undefined1)ViewInfo.vrz;
//     Scratchpad[0x4d] = ViewInfo.vrz._1_1_;
//     Scratchpad[0x4e] = ViewInfo.vrz._2_1_;
//     Scratchpad[0x4f] = ViewInfo.vrz._3_1_;
//     Scratchpad[0x50] = (undefined1)ViewInfo.rz;
//     Scratchpad[0x51] = ViewInfo.rz._1_1_;
//     Scratchpad[0x52] = ViewInfo.rz._2_1_;
//     Scratchpad[0x53] = ViewInfo.rz._3_1_;
//     Scratchpad._84_4_ = ViewInfo.super;
//     for (; uVar5 = (uint)(short)iVar8, (int)uVar5 <= (int)local_40; iVar8 = iVar8 + 1) {
//       for (sVar14 = (short)local_48; ot = (GsOT *)(int)sVar14, (int)ot <= (int)local_3c;
//           sVar14 = sVar14 + 1) {
//         local_34 = ((uint)ot & 7) << 5;
//         local_30 = (uVar5 & 7) << 8;
//         for (sVar13 = (short)local_44; uVar9 = (uint)sVar13, (int)uVar9 <= local_38;
//             sVar13 = sVar13 + 1) {
//           pcVar6 = (char *)(uVar9 * 16000 + 8000);
//           in_a3 = 0x2bc1;
//           iVar2 = IsVisible(uVar5 * 16000 + 8000,(int)ot * 16000 + 8000,(long)pcVar6,0x2bc1);
//           if (iVar2 != 0) {
//             for (piVar10 = *(int **)((int)&WorldMap[0][0][uVar9 & 7].top + local_30 + local_34);
//                 piVar10 != (int *)0x0; piVar10 = (int *)*piVar10) {
//               iVar2 = piVar10[1];
//               in_a3 = (int)*(short *)(piVar10 + 2);
//               pcVar6 = *(char **)(iVar2 + 0x20);
//               iVar2 = IsVisible(*(long *)(iVar2 + 0x18),
//                                 *(int *)(iVar2 + 0x1c) + (int)*(short *)((int)piVar10 + 10),
//                                 (long)pcVar6,in_a3);
//               if (iVar2 != 0) {
//                 uVar1 = *(undefined2 *)(piVar10 + 2);
//                 iVar2 = (Scratchpad._8_4_ - (int)*(short *)(piVar10 + 2) >> 8) + -0xb;
//                 iVar12 = iVar2 * 4;
//                 if (iVar2 < 0) {
//                   iVar12 = 0;
//                 }
//                 uVar11 = piVar10[1];
//                 if (local_70 <= local_74) {
//                   AdtMessageBox("ModelSlot Overflow");
//                 }
//                 *(undefined4 *)(local_78 + local_74 * 0xc + 4) = uVar11;
//                 *(int *)(local_78 + local_74 * 0xc) = *(int *)((int)local_790 + iVar12);
//                 *(undefined2 *)(local_78 + local_74 * 0xc + 8) = uVar1;
//                 *(undefined2 *)(local_78 + local_74 * 0xc + 10) = 0;
//                 *(int *)((int)local_790 + iVar12) = (int)(local_78 + local_74 * 0xc);
//                 local_50 = local_50 + 1;
//                 local_74 = local_74 + 1;
//               }
//               local_4c = local_4c + 1;
//             }
//           }
//         }
//       }
//     }
//     pPVar3 = GsGetWorkBase();
//     _DrawTMDmode = 0x20;
//     local_68.length = OTablePt->length;
//     local_68.offset = OTablePt->offset;
//     local_68.point = OTablePt->point;
//     local_68.tag = OTablePt->tag;
//     local_68.org = OTablePt->org + 0x37;
//     piVar10 = (int *)local_790[0];
//     while( true ) {
//       sVar14 = 1;
//       if (piVar10 == (int *)0x0) break;
//       if ((GsCOORDINATE2 *)piVar10[1] != (GsCOORDINATE2 *)0x0) {
//         GsGetLs((GsCOORDINATE2 *)piVar10[1],(MATRIX *)Scratchpad);
//         GsSetLsMatrix((MATRIX *)Scratchpad);
//         ot = &local_68;
//         pcVar6 = &DAT_00000002;
//         in_a3 = 0x1f800000;
//         GsSortObject4((GsDOBJ2 *)(piVar10[1] + 0x50),ot,2,(u_long *)Scratchpad);
//       }
//       piVar10 = (int *)*piVar10;
//     }
//     for (; sVar14 < 0x99; sVar14 = sVar14 + 1) {
//       for (piVar10 = (int *)local_790[sVar14]; piVar10 != (int *)0x0; piVar10 = (int *)*piVar10) {
//         if ((GsCOORDINATE2 *)piVar10[1] != (GsCOORDINATE2 *)0x0) {
//           GsGetLs((GsCOORDINATE2 *)piVar10[1],(MATRIX *)Scratchpad);
//           GsSetLsMatrix((MATRIX *)Scratchpad);
//           pcVar6 = (char *)0x0;
//           ot = OTablePt;
//           DrawTMD(piVar10[1] + 0x50);
//         }
//         pPVar4 = GsGetWorkBase();
//         if (0x6400 < (uint)((int)pPVar4 - (int)pPVar3)) {
//           FntPrint("OVERLOAD!\n",(char *)ot,pcVar6,in_a3);
//           goto LAB_8003c3ec;
//         }
//       }
//     }
// LAB_8003c3ec:
//     uVar5 = GetPad(0);
//     if ((uVar5 & 0x100) != 0) {
//       FntPrint(&DAT_80097a98,(char *)ot,pcVar6,in_a3);
//       pcVar6 = local_50;
//       pcVar7 = local_4c;
//       FntPrint("objs D%d/%d;",local_50,local_4c,in_a3);
//       FntPrint(&DAT_80097aa0,pcVar6,pcVar7,in_a3);
//       pPVar4 = GsGetWorkBase();
//       FntPrint("pk size %d\n",(char *)(pPVar4 + -(int)pPVar3),pcVar7,in_a3);
//     }
//   }
//   return;
// }
