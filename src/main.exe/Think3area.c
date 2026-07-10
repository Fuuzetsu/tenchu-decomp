#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3area(void);
 *     THINK_3.C:128, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $s0       short pad
 *     reg   $s2       long xx
 *     reg   $s1       long zz
 *     reg   $s3       long dist
 *     reg   $s2       long vx
 *     reg   $s1       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/Think3area", Think3area);

// triage: MEDIUM — 211 insns, mul/div, indirect-call, 7 callees, ~0.06 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// int FUN_8002d638(void)
//
// {
//   ushort uVar1;
//   int iVar2;
//   long lVar3;
//   int iVar4;
//   uint uVar5;
//
//   uVar5 = 0;
//   if (Me_THINK_C->status == 7) {
//     uVar1 = SuccessionAttack(4000,500);
//     iVar2 = (uint)uVar1 << 0x10;
//   }
//   else {
//     if ((Distance < 10000) && (SR != -2)) {
//       SR = 0;
//     }
//     if ((int)((uint)(ushort)Me_THINK_C->wpatk << 0x10) >> 0x14 == 3) {
//       uVar1 = Think3attack();
//       iVar2 = (uint)uVar1 << 0x10;
//     }
//     else {
//       iVar4 = Me_THINK_C->point[0] - Me_THINK_C->locate->vx;
//       iVar2 = Me_THINK_C->point[1] - Me_THINK_C->locate->vz;
//       lVar3 = SquareRoot0(iVar4 * iVar4 + iVar2 * iVar2);
//       if (Me_THINK_C->actflg == '\0') {
//         if ((Attrib & 0x4000U) != 0) {
//           Me_THINK_C->actflg = '\x01';
//         }
//         if (lVar3 < 2000) {
//           if ((Me_THINK_C->motion->count & 7U) == 0) {
//             if (Degree < 0x1f5) {
//               if (Degree < -500) {
//                 uVar5 = 0xffff8000;
//               }
//               else {
//                 iVar2 = rand();
//                 if (iVar2 == (iVar2 / 10) * 10) {
//                   SetNowMotion(Me_THINK_C,0x713,1);
//                   Me_THINK_C->actflg = '\x01';
//                 }
//               }
//             }
//             else {
//               uVar5 = 0x2000;
//             }
//           }
//           else {
//             uVar5 = (uint)(Me_THINK_C->pad).data;
//           }
//           iVar2 = uVar5 << 0x10;
//           if (3999 < Distance) goto LAB_8002d964;
//           iVar2 = (int)Degree;
//           if (iVar2 < 0) {
//             iVar2 = -iVar2;
//           }
//           if (iVar2 < 100) {
//             uVar1 = SetCommand(&Me_THINK_C->pad,0x21);
//             uVar5 = (uint)uVar1;
//           }
//           else if (iVar2 < 1000) {
//             uVar5 = uVar5 | 0x80;
//           }
//           Me_THINK_C->actflg = '\x01';
//         }
//         else {
//           uVar5 = FUN_8002b990(iVar4,iVar2);
//           if ((Attrib & 0x400U) != 0) {
//             Me_THINK_C->actflg = '\x01';
//           }
//           iVar2 = uVar5 << 0x10;
//           if ((Me_THINK_C->motion->count != 0) || (iVar2 = uVar5 << 0x10, 2999 < Distance))
//           goto LAB_8002d964;
//           iVar4 = (int)Degree;
//           if (iVar4 < 0) {
//             iVar4 = -iVar4;
//           }
//           iVar2 = uVar5 << 0x10;
//           if (999 < iVar4) goto LAB_8002d964;
//           uVar5 = uVar5 | 0x80;
//         }
//       }
//       else {
//         uVar5 = (*(code *)AttackFunc[(int)((uint)(ushort)Me_THINK_C->wpatk << 0x10) >> 0x14])();
//         if (Distance < 4000) {
//           Me_THINK_C->actcnt = Me_THINK_C->actcnt + '\x01';
//           iVar2 = uVar5 << 0x10;
//           if ((Me_THINK_C->actcnt == '\0') && (iVar2 = uVar5 << 0x10, 4000 < lVar3)) {
//             Me_THINK_C->actflg = '\0';
//           }
//           goto LAB_8002d964;
//         }
//       }
//       iVar2 = uVar5 << 0x10;
//     }
//   }
// LAB_8002d964:
//   return iVar2 >> 0x10;
// }
