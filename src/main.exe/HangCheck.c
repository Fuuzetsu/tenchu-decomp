#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short HangCheck(void);
 *     MOTION.C:291, 51 src lines, frame 56 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     stack sp+24     struct SVECTOR vect
 *     reg   $s0       long yy
 *     reg   $s0       long y
 *     reg   $s1       long ry
 *     reg   $s4       long oy
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short motID;
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern unsigned long *GlobalAreaMap;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/HangCheck", HangCheck);

// triage: MEDIUM — 305 insns, 1 loop, 5 callees, ~0.05 to MotionAndMove
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// short HangCheck(void)
//
// {
//   long *plVar1;
//   Humanoid *pHVar2;
//   VECTOR *pVVar3;
//   ushort uVar4;
//   long lVar5;
//   long lVar6;
//   int iVar7;
//   ushort ry;
//   int iVar8;
//   SVECTOR local_28;
//
//   if ((Me_MOTION_C->type & 0xf0U) == 0xa0) {
//     return 0;
//   }
//   if (((0 < (Me_MOTION_C->map).height) && (motID != 0x901)) && (Me_MOTION_C->itmctl != 0xb)) {
//     iVar8 = dtL->vy - (int)Me_MOTION_C->height;
//     GetMoveSpeed(&local_28,dtR->vy,Me_MOTION_C->width >> 1,0);
//     lVar5 = GetAreaMapLevel(GlobalAreaMap,dtL->vx + (int)local_28.vx,iVar8 + -0x122);
//     if (lVar5 < dtL->vy) {
//       lVar5 = GetAreaMapLevel(GlobalAreaMap,dtL->vx - (int)local_28.vx,iVar8 + -0x122);
//       pVVar3 = dtL;
//       if (lVar5 == -0x80000000) {
//         return 0;
//       }
//       if (lVar5 <= dtL->vy) {
//         return 0;
//       }
//       dtL->vx = dtL->vx - ((int)((uint)(ushort)local_28.vx << 0x10) >> 0x11);
//       pVVar3->vz = pVVar3->vz - ((int)((uint)(ushort)local_28.vz << 0x10) >> 0x11);
//     }
//     else {
//       lVar5 = GetAreaMapLevel(GlobalAreaMap,dtL->vx,iVar8 + -300);
//       if (lVar5 < dtL->vy - (int)Me_MOTION_C->height) {
//         return 0;
//       }
//       GetMoveSpeed(&local_28,dtR->vy,(Me_MOTION_C->width >> 1) + 300,0);
//       lVar5 = GetAreaMapLevel(GlobalAreaMap,dtL->vx + (int)local_28.vx,iVar8 + -400);
//       pHVar2 = Me_MOTION_C;
//       if ((lVar5 != -0x80000000) && (lVar5 < 0x191)) {
//         dtL->vy = dtL->vy + -0x69 + lVar5;
//         if (pHVar2->status == 10) {
//           return 1;
//         }
//         ry = dtR->vy;
//         if (((ry & 0xff) != 0) && (uVar4 = ry & 0x200, ry = ry & 0xc00, uVar4 != 0)) {
//           ry = ry + 0x400;
//         }
//         GetMoveSpeed(&local_28,ry,(Me_MOTION_C->width >> 1) + 300,0);
//         iVar8 = (dtL->vy - (int)Me_MOTION_C->height) + -400;
//         lVar6 = GetAreaMapLevel(GlobalAreaMap,dtL->vx + (int)local_28.vx,iVar8);
//         pHVar2 = Me_MOTION_C;
//         if ((lVar6 == -0x80000000) || (400 < lVar6)) {
//           dtL->vy = (dtL->vy + 5) - lVar5;
//           return 0;
//         }
//         dtR->vy = ry;
//         GetMoveSpeed(&local_28,ry,(pHVar2->width >> 1) + 100,0);
//         lVar5 = GetAreaMapLevel(GlobalAreaMap,dtL->vx + (int)local_28.vx,iVar8);
//         if ((lVar5 != -0x80000000) && (lVar5 < 0x191)) {
//           GetMoveSpeed(&local_28,dtR->vy,-200,0);
//           pVVar3 = dtL;
//           plVar1 = &dtL->vy;
//           dtL->vx = dtL->vx + (int)local_28.vx;
//           pVVar3->vy = *plVar1 + -0x69 + lVar5;
//           pVVar3->vz = pVVar3->vz + (int)local_28.vz;
//         }
//         motID = 0xa01;
//         DAT_80097f0e = 1;
//         iVar8 = 0;
//         if (MotionUpdateMode != 0) {
//           iVar7 = 0;
//           do {
//             iVar8 = iVar8 + 1;
//             if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar7 >> 0xd)) == Me_MOTION_C)
//             goto LAB_8001d310;
//             iVar7 = iVar8 * 0x10000;
//           } while (iVar8 * 0x10000 >> 0x10 < 5);
//         }
//         SetNowMotion(Me_MOTION_C,0xa01,1);
//         DAT_80097f0e = 0xffff;
// LAB_8001d310:
//         Sound(Me_MOTION_C,0x1b);
//         if (StagePlayer != Me_MOTION_C) {
//           return -1;
//         }
//         PadShockAR(0,0x7f,0,0x1e);
//         return -1;
//       }
//     }
//   }
//   return 0;
// }
