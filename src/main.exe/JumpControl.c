#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void JumpControl(void);
 *     MOTION.C:367, 37 src lines, frame 24 bytes, saved-reg mask 0x80000000
 *
 * Globals it touches, as the original declared them:
 *     extern struct VECTOR *dtL;
 *     extern struct MotionManager *dtM;
 *     extern short motID;
 *     extern struct Humanoid *StagePlayer;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct SVECTOR *dtV;
 *     extern short dtPAD;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/JumpControl", JumpControl);

// triage: MEDIUM — 146 insns, 4 callees, ~0.17 to FUN_80027304
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void JumpControl(void)
//
// {
//   ushort uVar1;
//   VECTOR *pVVar2;
//   SVECTOR *pSVar3;
//   ushort uVar4;
//   int iVar5;
//   short ordr;
//
//   FUN_80033bc0(dtL,0x96,0xc,8);
//   uVar4 = GetMotionID(dtM,0x900);
//   pVVar2 = dtL;
//   if (-1 < (int)((uint)uVar4 << 0x10)) {
//     if (motID == 0x607) {
//       if ((dtM->count < 0xb) && (uVar4 = GetMotionID(dtM,0x906), -1 < (int)((uint)uVar4 << 0x10))) {
//         motID = 0x906;
//         DAT_80097f0e = 0;
//         MoveHumanoid(Me_MOTION_C,0x7f,0);
//         if (Me_MOTION_C == StagePlayer) {
//           Sound(Me_MOTION_C,0x48);
//         }
//         Sound(Me_MOTION_C,0x17);
//       }
//     }
//     else {
//       iVar5 = (int)(*Me_MOTION_C->model->object)->id;
//       if (-1 < iVar5) {
//         dtL->vx = ConflictObject[iVar5].position.vx;
//         pVVar2->vz = ConflictObject[iVar5].position.vz;
//       }
//       pSVar3 = dtV;
//       uVar1 = dtPAD;
//       motID = 0x900;
//       DAT_80097f0e = 0;
//       uVar4 = dtPAD & 0x1000;
//       dtV->vy = 0;
//       if (uVar4 == 0) {
//         if ((uVar1 & 0x4000) == 0) {
//           if ((uVar1 & 0x2000) != 0) {
//             uVar4 = GetMotionID(dtM,0x904);
//             if (-1 < (int)((uint)uVar4 << 0x10)) {
//               motID = 0x904;
//               DAT_80097f0e = 0;
//             }
//             MoveHumanoid(Me_MOTION_C,0,-100);
//             return;
//           }
//           if ((uVar1 & 0x8000) != 0) {
//             uVar4 = GetMotionID(dtM,0x905);
//             if (-1 < (int)((uint)uVar4 << 0x10)) {
//               motID = 0x905;
//               DAT_80097f0e = 0;
//             }
//             MoveHumanoid(Me_MOTION_C,0,100);
//             return;
//           }
//           pSVar3->vz = 0;
//           pSVar3->vx = 0;
//           return;
//         }
//         uVar4 = GetMotionID(dtM,0x903);
//         if (-1 < (int)((uint)uVar4 << 0x10)) {
//           motID = 0x903;
//           DAT_80097f0e = 0;
//         }
//         ordr = -100;
//       }
//       else {
//         uVar4 = GetMotionID(dtM,0x902);
//         if (-1 < (int)((uint)uVar4 << 0x10)) {
//           motID = 0x902;
//           DAT_80097f0e = 0;
//         }
//         ordr = 100;
//       }
//       MoveHumanoid(Me_MOTION_C,ordr,0);
//     }
//   }
//   return;
// }
