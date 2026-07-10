#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetBleedsDir(struct VECTOR *pos, struct SVECTOR *vec, short grange, short n, int time, long col);
 *     EFFECT.C:1115, 12 src lines, frame 88 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s7       struct VECTOR * pos
 *     param $fp       struct SVECTOR * vec
 *     param $a2       short grange
 *     param $s5       short n
 *     param stack+16  int time
 *     param stack+20  long col
 *     reg   $s4       int time
 *     reg   $s6       long col
 *     stack sp+16     struct VECTOR npos
 *     stack sp+32     struct SVECTOR v
 *     reg   $a3       struct VECTOR * pos
 *     reg   $t0       int time
 *     reg   $s6       long col
 *     reg   $v1       struct BleedType * param
 *     reg   $a2       struct tag_EffectSlot * slot
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetBleedsDir", SetBleedsDir);

// triage: MEDIUM — 219 insns, mul/div, 1 loop, 2 callees, ~0.08 to DebugMenuItemSet
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void SetBleedsDir(VECTOR *pos,SVECTOR *vec,short grange,short n)
//
// {
//   AreaNodeType *pAVar1;
//   int iVar2;
//   AreaNodeType *pAVar3;
//   long lVar4;
//   int iVar5;
//   int iVar6;
//   tag_EffectSlot *ptVar7;
//   int iVar8;
//   int iVar9;
//   int iVar10;
//   uint uVar11;
//   int in_stack_00000010;
//   undefined4 in_stack_00000014;
//   AreaNodeType *local_38;
//   int local_34;
//   AreaNodeType *local_30;
//   long local_2c;
//
//   uVar11 = (uint)(ushort)n;
//   iVar10 = (int)grange;
//   iVar9 = iVar10 << 1;
//   do {
//     if ((int)(uVar11 << 0x10) < 1) {
//       return;
//     }
//     memset((uchar *)&local_38,'\0',0x10);
//     iVar8 = pos->vx;
//     iVar5 = -iVar10;
//     if (0 < iVar9) {
//       iVar5 = rand();
//       if (iVar9 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar9 == -1) && (iVar5 == -0x80000000)) {
//         trap(0x1800);
//       }
//       iVar5 = iVar5 % iVar9 - iVar10;
//     }
//     local_38 = (AreaNodeType *)(iVar8 + iVar5);
//     iVar8 = pos->vy;
//     iVar5 = -iVar10;
//     if (0 < iVar9) {
//       iVar5 = rand();
//       if (iVar9 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar9 == -1) && (iVar5 == -0x80000000)) {
//         trap(0x1800);
//       }
//       iVar5 = iVar5 % iVar9 - iVar10;
//     }
//     local_34 = iVar8 + iVar5;
//     iVar8 = pos->vz;
//     iVar5 = -iVar10;
//     if (0 < iVar9) {
//       iVar5 = rand();
//       if (iVar9 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar9 == -1) && (iVar5 == -0x80000000)) {
//         trap(0x1800);
//       }
//       iVar5 = iVar5 % iVar9 - iVar10;
//     }
//     lVar4 = local_2c;
//     iVar2 = local_34;
//     pAVar1 = local_38;
//     local_30 = (AreaNodeType *)(iVar8 + iVar5);
//     pAVar3 = local_30;
//     memset((uchar *)&local_30,'\0',8);
//     local_38 = *(AreaNodeType **)vec;
//     local_2c = CONCAT22(local_2c._2_2_,vec->vz);
//     local_34 = local_2c;
//     iVar5 = in_stack_00000010;
//     if (in_stack_00000010 < 0) {
//       iVar5 = in_stack_00000010 + 7;
//     }
//     iVar5 = iVar5 >> 3;
//     iVar8 = in_stack_00000010 - iVar5;
//     local_30 = local_38;
//     if (0 < iVar8) {
//       iVar6 = rand();
//       if (iVar8 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar8 == -1) && (iVar6 == -0x80000000)) {
//         trap(0x1800);
//       }
//       iVar5 = iVar6 % iVar8 + iVar5;
//     }
//     iVar6 = 0;
//     ptVar7 = EffectSlot + DAT_80097a3c;
//     iVar8 = DAT_80097a3c;
//     do {
//       iVar8 = iVar8 + 1;
//       ptVar7 = ptVar7 + 1;
//       if (199 < iVar8) {
//         iVar8 = 0;
//         ptVar7 = EffectSlot;
//       }
//       if (ptVar7->proc == (undefined **)0x0) {
//         DAT_80097a3c = iVar8 + 1;
//         if (199 < iVar8 + 1) {
//           DAT_80097a3c = 0;
//         }
//         goto LAB_8003558c;
//       }
//       iVar6 = iVar6 + 1;
//     } while (iVar6 < 200);
//     ptVar7 = &dmy;
// LAB_8003558c:
//     uVar11 = uVar11 - 1;
//     (ptVar7->param).blood.hint = pAVar1;
//     (ptVar7->param).blood.px = iVar2;
//     (ptVar7->param).blood.py = (long)pAVar3;
//     (ptVar7->param).blood.pz = lVar4;
//     (ptVar7->param).blood.scale = (long)local_38;
//     (ptVar7->param).blood.rotate = local_34;
//     (ptVar7->param).bleed.r = (uchar)((uint)in_stack_00000014 >> 0x10);
//     (ptVar7->param).bleed.g = (uchar)((uint)in_stack_00000014 >> 8);
//     (ptVar7->param).bleed.time = (uchar)iVar5;
//     (ptVar7->param).bleed.b = (uchar)in_stack_00000014;
//     (ptVar7->param).bleed.mode = '\0';
//     ptVar7->proc = (undefined **)DrawBleed;
//   } while( true );
// }
