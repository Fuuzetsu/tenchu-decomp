#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int SetFlyWire(struct VECTOR *start, struct VECTOR *end);
 *     EFFECT.C:1384, 40 src lines, frame 48 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 *     param $a2       struct VECTOR * start
 *     param $a3       struct VECTOR * end
 *     reg   $s5       struct tag_EffectSlot * slot
 *     reg   $s3       struct FlyWireType * param
 *     reg   $s2       int dist
 *     reg   $a1       int i
 *     reg   $s3       struct VECTOR * v1
 *     reg   $a0       long dz
 *     reg   $a2       long dy
 *     reg   $t0       long dx
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetFlyWire", SetFlyWire);

// triage: MEDIUM — 246 insns, mul/div, 1 loop, 3 callees, ~0.06 to DebugMenuItemSet
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// int SetFlyWire(VECTOR *start,VECTOR *end)
//
// {
//   short sVar1;
//   bool bVar2;
//   int iVar3;
//   int iVar4;
//   int iVar5;
//   long lVar6;
//   long lVar7;
//   long lVar8;
//   int iVar9;
//   int iVar10;
//   tag_EffectSlot *ptVar11;
//
//   iVar5 = 0;
//   ptVar11 = EffectSlot + DAT_80097a3c;
//   iVar3 = DAT_80097a3c;
//   while( true ) {
//     iVar3 = iVar3 + 1;
//     ptVar11 = ptVar11 + 1;
//     if (199 < iVar3) {
//       iVar3 = 0;
//       ptVar11 = EffectSlot;
//     }
//     iVar5 = iVar5 + 1;
//     if (ptVar11->proc == (undefined **)0x0) break;
//     if (199 < iVar5) {
//       ptVar11 = &dmy;
// LAB_80036fa4:
//       bVar2 = false;
//       lVar6 = start->vy;
//       lVar7 = start->vz;
//       lVar8 = start->pad;
//       (ptVar11->param).blood.hint = (AreaNodeType *)start->vx;
//       (ptVar11->param).blood.px = lVar6;
//       (ptVar11->param).blood.py = lVar7;
//       (ptVar11->param).blood.pz = lVar8;
//       lVar6 = end->vy;
//       lVar7 = end->vz;
//       lVar8 = end->pad;
//       (ptVar11->param).blood.scale = end->vx;
//       (ptVar11->param).blood.rotate = lVar6;
//       (ptVar11->param).smoke.rotate = lVar7;
//       (ptVar11->param).smoke.scale = lVar8;
//       (ptVar11->param).flywire.count = 0;
//       (ptVar11->param).flywire.mode = '\0';
//       iVar5 = (int)(ptVar11->param).blood.hint - (ptVar11->param).blood.scale;
//       iVar9 = (ptVar11->param).blood.px - (ptVar11->param).blood.rotate;
//       iVar10 = (ptVar11->param).blood.py - (ptVar11->param).smoke.rotate;
//       iVar3 = abs(iVar5);
//       if (((0x1000 < iVar3) || (iVar3 = abs(iVar9), 0x1000 < iVar3)) ||
//          (iVar3 = abs(iVar10), 0x1000 < iVar3)) {
//         bVar2 = true;
//       }
//       if (bVar2) {
//         if (iVar5 < 0) {
//           iVar5 = iVar5 + 0xff;
//         }
//         if (iVar9 < 0) {
//           iVar9 = iVar9 + 0xff;
//         }
//         if (iVar10 < 0) {
//           iVar10 = iVar10 + 0xff;
//         }
//         lVar6 = SquareRoot0((iVar5 >> 8) * (iVar5 >> 8) + (iVar9 >> 8) * (iVar9 >> 8) +
//                             (iVar10 >> 8) * (iVar10 >> 8));
//         iVar3 = lVar6 << 8;
//       }
//       else {
//         iVar3 = SquareRoot0(iVar5 * iVar5 + iVar9 * iVar9 + iVar10 * iVar10);
//       }
//       iVar10 = (ptVar11->param).blood.rotate;
//       iVar9 = (ptVar11->param).blood.px;
//       (ptVar11->param).flywire.NCenter.vx =
//            ((int)&((ptVar11->param).blood.hint)->y + (ptVar11->param).blood.scale) / 2;
//       iVar5 = (ptVar11->param).blood.py;
//       iVar4 = (ptVar11->param).smoke.rotate;
//       (ptVar11->param).flywire.NCenter.vy = (iVar9 + iVar10) / 2;
//       (ptVar11->param).flywire.NCenter.vz = (iVar5 + iVar4) / 2;
//       (ptVar11->param).flywire.time = (short)(iVar3 / 1000);
//       if (iVar3 < 0) {
//         iVar3 = iVar3 + 0xf;
//       }
//       iVar3 = iVar3 >> 4;
//       iVar9 = (ptVar11->param).flywire.NCenter.vx;
//       iVar5 = iVar3 << 1;
//       if (iVar5 < 1) {
//         iVar5 = -iVar3;
//       }
//       else {
//         iVar10 = rand();
//         if (iVar5 == 0) {
//           trap(0x1c00);
//         }
//         if ((iVar5 == -1) && (iVar10 == -0x80000000)) {
//           trap(0x1800);
//         }
//         iVar5 = iVar10 % iVar5 - iVar3;
//       }
//       iVar10 = (ptVar11->param).flywire.NCenter.vy;
//       (ptVar11->param).flywire.center.vx = iVar9 + iVar5;
//       if (iVar3 < 1) {
//         iVar5 = -iVar3;
//       }
//       else {
//         iVar5 = rand();
//         if (iVar3 == 0) {
//           trap(0x1c00);
//         }
//         if ((iVar3 == -1) && (iVar5 == -0x80000000)) {
//           trap(0x1800);
//         }
//         iVar5 = iVar5 % iVar3 - iVar3;
//       }
//       iVar4 = (ptVar11->param).flywire.NCenter.vz;
//       iVar9 = iVar3 << 1;
//       (ptVar11->param).flywire.center.vy = iVar10 + iVar5;
//       if (iVar9 < 1) {
//         iVar3 = -iVar3;
//       }
//       else {
//         iVar5 = rand();
//         if (iVar9 == 0) {
//           trap(0x1c00);
//         }
//         if ((iVar9 == -1) && (iVar5 == -0x80000000)) {
//           trap(0x1800);
//         }
//         iVar3 = iVar5 % iVar9 - iVar3;
//       }
//       sVar1 = (ptVar11->param).flywire.time;
//       (ptVar11->param).flywire.center.vz = iVar4 + iVar3;
//       if (sVar1 < 1) {
//         iVar3 = 0;
//       }
//       else {
//         ptVar11->proc = (undefined **)DrawFlyWire;
//         iVar3 = (ptVar11->param).flywire.time + 5;
//       }
//       return iVar3;
//     }
//   }
//   DAT_80097a3c = iVar3 + 1;
//   if (199 < DAT_80097a3c) {
//     DAT_80097a3c = 0;
//   }
//   goto LAB_80036fa4;
// }
