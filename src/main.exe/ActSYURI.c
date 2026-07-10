#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSYURI(void);
 *     MOTION.C:1908, 37 src lines, frame 64 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $v0       struct VECTOR * p
 *     stack sp+16     struct PARAM_ITEM_LAUNCH item
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern struct Humanoid *StagePlayer;
 *     extern short motID;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ActSYURI", ActSYURI);

// triage: MEDIUM — 169 insns, 6 callees, ~0.10 to ReturnNormal
// likely-relevant cookbook sections:
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ActSYURI(void)
//
// {
//   VECTOR *pVVar1;
//   int iVar2;
//   PARAM_ITEM_USE local_30;
//
//   if (dtM->mid == 0xe00) {
//     if (Me_MOTION_C != StagePlayer) {
//       if (dtM->count != 0) {
//         return;
//       }
//       if (dtM->loop == 0) {
//         return;
//       }
//       motID = 0xe01;
//       DAT_80097f0e = 1;
//     }
//     if ((dtM->count == 0) && (dtM->loop != 0)) {
//       dtM->loop = -1;
//     }
//     if (dtM->count == 1) {
//       local_30.type = ITEM_SHURIKEN;
//       local_30.user = Me_MOTION_C;
//       pVVar1 = GetAbsolutePosition(Me_MOTION_C->model->object[2],0,0,0);
//       local_30.start.vx = pVVar1->vx;
//       local_30.start.vy = pVVar1->vy;
//       local_30.start.vz = pVVar1->vz;
//       local_30.end.pad = local_30.start.pad;
//       local_30.end.vx = local_30.start.vx;
//       local_30.end.vy = local_30.start.vy;
//       local_30.end.vz = local_30.start.vz;
//       ReqItemUse(&local_30);
//       Sound(Me_MOTION_C,0x1e);
//     }
//     else {
//       iVar2 = FUN_8004a368(1,Me_MOTION_C);
//       if (iVar2 == 0) {
//         motID = 0xe01;
//         DAT_80097f0e = 1;
//         Sound(Me_MOTION_C,0x1f);
//       }
//       else if (((Me_MOTION_C->pad).trig & 0xe0) != 0) {
//         FUN_8004a368(0,0);
//         if (Me_MOTION_C == StagePlayer) {
//           SetCameraMode(CMODE_NORMAL);
//         }
//         if ((Me_MOTION_C->attribute & 0x40U) == 0) {
//           motID = 0;
//           DAT_80097f0e = 1;
//         }
//         else {
//           motID = 0x501;
//           DAT_80097f0e = 1;
//         }
//       }
//     }
//   }
//   else if (dtM->mid == 0xe01) {
//     if ((dtM->count == 1) && (Me_MOTION_C != StagePlayer)) {
//       ReqItemDefault(Me_MOTION_C,ITEM_SHURIKEN);
//     }
//     if ((dtM->count == 0) && (dtM->loop != 0)) {
//       if (Me_MOTION_C == StagePlayer) {
//         SetCameraMode(CMODE_NORMAL);
//       }
//       if ((Me_MOTION_C->attribute & 0x40U) == 0) {
//         motID = 0;
//       }
//       else {
//         motID = 0x501;
//       }
//       DAT_80097f0e = 1;
//     }
//   }
//   return;
// }
