#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawAfterimage(struct AfterimageType *afi, short disp);
 *     EFFECT.C:1717, 49 src lines, frame 72 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s2       struct AfterimageType * afi
 *     param $a1       short disp
 *     reg   $s0       struct POLY_GT4 * poly
 *     reg   $s3       short i
 *     reg   $s1       short tplv
 *     stack sp+16     struct MATRIX mat
 *     reg   $v1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR UnitVector;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DrawAfterimage", DrawAfterimage);

// triage: MEDIUM — 158 insns, mul/div, 2 loop, 4 callees, ~0.05 to bow_shoot_logic
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short DrawAfterimage(AfterimageType *afi,short disp)
//
// {
//   uint uVar1;
//   long lVar2;
//   undefined4 *puVar3;
//   int iVar4;
//   int iVar5;
//   u_char uVar6;
//   int iVar7;
//   int iVar8;
//   MATRIX MStack_38;
//
//   iVar7 = 0x7f;
//   if (disp == 0) {
//     if (0 < afi->n) {
//       afi->n = afi->n + -1;
//       goto LAB_80038914;
//     }
//   }
//   else {
//     if ((int)afi->n < afi->maxn + -1) {
//       afi->n = afi->n + 1;
//     }
//     uVar1 = (uint)(ushort)afi->n;
//     while (uVar1 = uVar1 - 1, 0 < (int)(uVar1 * 0x10000)) {
//       iVar5 = (int)(uVar1 * 0x10000) >> 0xe;
//       puVar3 = (undefined4 *)(iVar5 + (int)afi->p1);
//       *puVar3 = puVar3[-1];
//       puVar3 = (undefined4 *)(iVar5 + (int)afi->p2);
//       *puVar3 = puVar3[-1];
//     }
//     GsGetLs(&afi->model->locate,&MStack_38);
//     GsSetLsMatrix(&MStack_38);
//     RotTransPers(&afi->vector1,afi->p1,(long *)0x0,(long *)0x0);
//     RotTransPers(&afi->vector2,afi->p2,(long *)0x0,(long *)0x0);
//     lVar2 = RotTransPers(&UnitVector,(long *)0x0,(long *)0x0,(long *)0x0);
//     afi->sz = lVar2;
//     if (lVar2 != 0) {
// LAB_80038914:
//       lVar2 = *afi->p1;
//       (afi->poly).x0 = (short)lVar2;
//       (afi->poly).y0 = (short)((uint)lVar2 >> 0x10);
//       lVar2 = *afi->p2;
//       iVar8 = 1;
//       (afi->poly).x2 = (short)lVar2;
//       (afi->poly).y2 = (short)((uint)lVar2 >> 0x10);
//       iVar5 = 0x10000;
//       while( true ) {
//         iVar5 = iVar5 >> 0x10;
//         if (afi->n <= iVar5) break;
//         (afi->poly).x1 = (afi->poly).x0;
//         (afi->poly).y1 = (afi->poly).y0;
//         lVar2 = afi->p1[iVar5];
//         (afi->poly).x3 = (afi->poly).x2;
//         (afi->poly).y3 = (afi->poly).y2;
//         (afi->poly).x0 = (short)lVar2;
//         (afi->poly).y0 = (short)((uint)lVar2 >> 0x10);
//         lVar2 = afi->p2[iVar5];
//         uVar6 = (u_char)iVar7;
//         (afi->poly).b1 = uVar6;
//         (afi->poly).g1 = uVar6;
//         (afi->poly).r1 = uVar6;
//         (afi->poly).b3 = uVar6;
//         (afi->poly).g3 = uVar6;
//         (afi->poly).r3 = uVar6;
//         (afi->poly).x2 = (short)lVar2;
//         (afi->poly).y2 = (short)((uint)lVar2 >> 0x10);
//         iVar4 = (int)afi->n;
//         iVar5 = (iVar4 - iVar5) * 0x7f;
//         iVar7 = iVar5 / iVar4;
//         if (iVar4 == 0) {
//           trap(0x1c00);
//         }
//         if ((iVar4 == -1) && (iVar5 == -0x80000000)) {
//           trap(0x1800);
//         }
//         uVar6 = (u_char)iVar7;
//         (afi->poly).b0 = uVar6;
//         (afi->poly).g0 = uVar6;
//         (afi->poly).r0 = uVar6;
//         (afi->poly).b2 = uVar6;
//         (afi->poly).g2 = uVar6;
//         (afi->poly).r2 = uVar6;
//         iVar5 = afi->sz >> 2;
//         if (iVar5 < 0) {
//           iVar4 = 0;
//         }
//         else {
//           iVar4 = 0x4e1;
//           if (iVar5 < 0x4e2) {
//             iVar4 = iVar5;
//           }
//         }
//         iVar8 = iVar8 + 1;
//         GsSortPoly(&afi->poly,OTablePt,(ushort)iVar4);
//         iVar5 = iVar8 * 0x10000;
//       }
//       return afi->n;
//     }
//   }
//   return 0;
// }
