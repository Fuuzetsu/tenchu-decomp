#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetBleeds(struct VECTOR *pos, short grange, short srange, short n, int time, long col);
 *     EFFECT.C:963, 11 src lines, frame 88 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param stack+0   struct VECTOR * pos
 *     param $a1       short grange
 *     param $s5       short srange
 *     param $s6       short n
 *     param stack+16  int time
 *     param stack+20  long col
 *     reg   $fp       int time
 *     reg   $s7       long col
 *     stack sp+16     struct VECTOR npos
 *     stack sp+32     struct SVECTOR v
 *     reg   $a3       struct VECTOR * pos
 *     reg   $t0       int time
 *     reg   $s7       long col
 *     reg   $v1       struct BleedType * param
 *     reg   $a2       struct tag_EffectSlot * slot
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetBleeds", SetBleeds);

// triage: MEDIUM — 277 insns, mul/div, 1 loop, 2 callees, ~0.08 to InsertConflict
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void SetBleeds(short grange,short srange,short n)
//
// {
//   AreaNodeType *pAVar1;
//   int iVar2;
//   long lVar3;
//   int iVar4;
//   int iVar5;
//   undefined2 in_register_00000012;
//   int *piVar6;
//   tag_EffectSlot *ptVar7;
//   int in_a3;
//   int iVar8;
//   int iVar9;
//   int iVar10;
//   int iVar11;
//   int iVar12;
//   int in_stack_00000010;
//   undefined4 in_stack_00000014;
//   AreaNodeType *local_38;
//   int local_34;
//   undefined4 local_30;
//   long local_2c;
//
//   piVar6 = (int *)CONCAT22(in_register_00000012,grange);
//   iVar12 = (int)srange;
//   iVar11 = iVar12 << 1;
//   iVar10 = (int)((uint)(ushort)n << 0x10) >> 0xf;
//   do {
//     if (in_a3 << 0x10 < 1) {
//       return;
//     }
//     memset((uchar *)&local_38,'\0',0x10);
//     iVar8 = *piVar6;
//     iVar4 = -iVar12;
//     if (0 < iVar11) {
//       iVar4 = rand();
//       if (iVar11 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar11 == -1) && (iVar4 == -0x80000000)) {
//         trap(0x1800);
//       }
//       iVar4 = iVar4 % iVar11 - iVar12;
//     }
//     local_38 = (AreaNodeType *)(iVar8 + iVar4);
//     iVar8 = piVar6[1];
//     iVar4 = -iVar12;
//     if (0 < iVar11) {
//       iVar4 = rand();
//       if (iVar11 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar11 == -1) && (iVar4 == -0x80000000)) {
//         trap(0x1800);
//       }
//       iVar4 = iVar4 % iVar11 - iVar12;
//     }
//     local_34 = iVar8 + iVar4;
//     iVar8 = piVar6[2];
//     iVar4 = -iVar12;
//     if (0 < iVar11) {
//       iVar4 = rand();
//       if (iVar11 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar11 == -1) && (iVar4 == -0x80000000)) {
//         trap(0x1800);
//       }
//       iVar4 = iVar4 % iVar11 - iVar12;
//     }
//     lVar3 = local_2c;
//     iVar2 = local_34;
//     pAVar1 = local_38;
//     local_30 = (AreaNodeType *)(iVar8 + iVar4);
//     iVar4 = (int)local_30;
//     memset((uchar *)&local_30,'\0',8);
//     if (iVar10 < 1) {
//       local_30 = (AreaNodeType *)CONCAT22(local_30._2_2_,-n);
//     }
//     else {
//       iVar8 = rand();
//       if (iVar10 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar10 == -1) && (iVar8 == -0x80000000)) {
//         trap(0x1800);
//       }
//       local_30 = (AreaNodeType *)CONCAT22(local_30._2_2_,(short)(iVar8 % iVar10) - n);
//     }
//     if (iVar10 < 1) {
//       local_30 = (AreaNodeType *)CONCAT22(-n,(undefined2)local_30);
//     }
//     else {
//       iVar8 = rand();
//       if (iVar10 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar10 == -1) && (iVar8 == -0x80000000)) {
//         trap(0x1800);
//       }
//       local_30 = (AreaNodeType *)CONCAT22((short)(iVar8 % iVar10) - n,(undefined2)local_30);
//     }
//     if (iVar10 < 1) {
//       local_2c = CONCAT22(local_2c._2_2_,-n);
//     }
//     else {
//       iVar8 = rand();
//       if (iVar10 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar10 == -1) && (iVar8 == -0x80000000)) {
//         trap(0x1800);
//       }
//       local_2c = CONCAT22(local_2c._2_2_,(short)(iVar8 % iVar10) - n);
//     }
//     local_38 = local_30;
//     local_34 = local_2c;
//     iVar8 = in_stack_00000010 / 2;
//     iVar9 = in_stack_00000010 - iVar8;
//     if (0 < iVar9) {
//       iVar5 = rand();
//       if (iVar9 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar9 == -1) && (iVar5 == -0x80000000)) {
//         trap(0x1800);
//       }
//       iVar8 = iVar5 % iVar9 + iVar8;
//     }
//     iVar5 = 0;
//     ptVar7 = EffectSlot + DAT_80097a3c;
//     iVar9 = DAT_80097a3c;
//     do {
//       iVar9 = iVar9 + 1;
//       ptVar7 = ptVar7 + 1;
//       if (199 < iVar9) {
//         iVar9 = 0;
//         ptVar7 = EffectSlot;
//       }
//       if (ptVar7->proc == (undefined **)0x0) {
//         DAT_80097a3c = iVar9 + 1;
//         if (199 < iVar9 + 1) {
//           DAT_80097a3c = 0;
//         }
//         goto LAB_80034944;
//       }
//       iVar5 = iVar5 + 1;
//     } while (iVar5 < 200);
//     ptVar7 = &dmy;
// LAB_80034944:
//     in_a3 = in_a3 + -1;
//     (ptVar7->param).blood.hint = pAVar1;
//     (ptVar7->param).blood.px = iVar2;
//     (ptVar7->param).blood.py = iVar4;
//     (ptVar7->param).blood.pz = lVar3;
//     (ptVar7->param).blood.scale = (long)local_38;
//     (ptVar7->param).blood.rotate = local_34;
//     (ptVar7->param).bleed.r = (uchar)((uint)in_stack_00000014 >> 0x10);
//     (ptVar7->param).bleed.g = (uchar)((uint)in_stack_00000014 >> 8);
//     (ptVar7->param).bleed.time = (uchar)iVar8;
//     (ptVar7->param).bleed.b = (uchar)in_stack_00000014;
//     (ptVar7->param).bleed.mode = '\0';
//     ptVar7->proc = (undefined **)DrawBleed;
//   } while( true );
// }
