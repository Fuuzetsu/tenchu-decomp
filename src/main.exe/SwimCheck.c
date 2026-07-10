#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SwimCheck(void);
 *     MOTION.C:219, 33 src lines, frame 40 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     stack sp+16     struct VECTOR vect
 *     reg   $a2       struct ModelArchiveType * mdl
 *     reg   $a1       short i
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short motID;
 *     extern struct MotionManager *dtM;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct VECTOR *dtL;
 *     extern struct Humanoid *StagePlayer;
 *     extern short ActionHalt;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SwimCheck", SwimCheck);

// triage: HARD — 216 insns, mul/div, 2 loop, 11 callees, ~0.10 to FUN_80027304
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// short SwimCheck(void)
//
// {
//   Humanoid *h;
//   VECTOR *pVVar1;
//   short sVar2;
//   ushort uVar3;
//   int iVar4;
//   uint uVar5;
//   uint uVar6;
//   int iVar7;
//   VECTOR local_20;
//
//   pVVar1 = dtL;
//   sVar2 = 0;
//   if ((Me_MOTION_C->map).height < 1) {
//     if ((((Me_MOTION_C->map).attrib & 4U) == 0) || (sVar2 = Me_MOTION_C->status, sVar2 == 4)) {
//       sVar2 = 0;
//     }
//     else {
//       if (sVar2 != 3) {
//         if (sVar2 == 0x11) {
//           if (motID == 0x1108) {
//             return 1;
//           }
//           if (dtM->loop == -1) {
//             return 0;
//           }
//         }
//         if ((Me_MOTION_C->status != 0xb) &&
//            (iVar7 = (int)(*Me_MOTION_C->model->object)->id, -1 < iVar7)) {
//           dtL->vx = ConflictObject[iVar7].position.vx;
//           pVVar1->vz = ConflictObject[iVar7].position.vz;
//         }
//         local_20.vy = (Me_MOTION_C->map).level;
//         iVar7 = 0;
//         do {
//           iVar4 = rand();
//           local_20.vx = (long)Me_MOTION_C->width;
//           if (local_20.vx == 0) {
//             trap(0x1c00);
//           }
//           if ((local_20.vx == -1) && (iVar4 == -0x80000000)) {
//             trap(0x1800);
//           }
//           local_20.vx = (dtL->vx + (iVar4 % local_20.vx) * 2) - local_20.vx;
//           iVar4 = rand();
//           local_20.vz = (long)Me_MOTION_C->width;
//           if (local_20.vz == 0) {
//             trap(0x1c00);
//           }
//           if ((local_20.vz == -1) && (iVar4 == -0x80000000)) {
//             trap(0x1800);
//           }
//           local_20.vz = (dtL->vz + (iVar4 % local_20.vz) * 2) - local_20.vz;
//           uVar5 = rand();
//           uVar6 = rand();
//           SetSplash(&local_20,(short)((uVar5 & 7) << 0xc),(short)((uVar6 & 7) << 0xc),6);
//           iVar7 = iVar7 + 1;
//         } while (iVar7 * 0x10000 >> 0x10 < 0x14);
//         AttackCancelControl(3);
//         if (Me_MOTION_C == StagePlayer) {
//           SetCameraMode(CMODE_SWIM);
//           PadShockAR(0,0xff,10,0);
//         }
//         ActionHalt = 0;
//         FUN_800270f8(Me_MOTION_C,1);
//         uVar3 = GetMotionID(dtM,0x300);
//         if (((int)((uint)uVar3 << 0x10) < 0) || (Me_MOTION_C->life == 0)) {
//           motID = 0x1108;
//           DAT_80097f0e = 1;
//           Sound(Me_MOTION_C,8);
//           h = Me_MOTION_C;
//           Me_MOTION_C->life = 0;
//           ReqLifeBar(h);
//         }
//         else {
//           motID = 0x300;
//           DAT_80097f0e = 1;
//         }
//         iVar7 = 0;
//         if (MotionUpdateMode != 0) {
//           iVar4 = 0;
//           do {
//             iVar7 = iVar7 + 1;
//             if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar4 >> 0xd)) == Me_MOTION_C)
//             goto LAB_8001cc64;
//             iVar4 = iVar7 * 0x10000;
//           } while (iVar7 * 0x10000 >> 0x10 < 5);
//         }
//         SetNowMotion(Me_MOTION_C,motID,DAT_80097f0e);
//         DAT_80097f0e = -1;
// LAB_8001cc64:
//         Sound(Me_MOTION_C,0x16);
//         FUN_8002f7f4();
//       }
//       sVar2 = 1;
//     }
//   }
//   return sVar2;
// }
