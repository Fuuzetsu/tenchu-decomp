#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActACTION(void);
 *     MOTION.C:963, 27 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     reg   $v1       short i
 * END PSX.SYM */

/*
 * ActACTION (0x8001fb98) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=ActACTION
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActACTION", ActACTION);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActACTION", switchD_8001fbdc__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActACTION", switchD_8001fbdc__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActACTION", switchD_8001fbdc__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActACTION", switchD_8001fbdc__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActACTION", switchD_8001fbdc__caseD_6);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActACTION", switchD_8001fbdc__caseD_2);

/* jump-table pool @ 0x80011308 (7 words; tables at 0x80011308) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 handle_char_state_posing__jtbl[7] = {
    0x8001FD74, 0x8001FBE4, 0x8002007C, 0x8002007C,
    0x8001FC54, 0x8001FC54, 0x80020028,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// void FUN_8001fb98(void)
// 
// {
//   short sVar1;
//   Humanoid *human;
//   SVECTOR *pSVar2;
//   MotionManager *mmp;
//   int iVar3;
//   int iVar4;
//   ModelType *model;
//   
//   switch((int)(((ushort)dtM->mid - 0x100) * 0x10000) >> 0x10) {
//   case 0:
//     if (dtM->count == 1) {
//       sVar1 = Me_MOTION_C->wpatk;
//       if (sVar1 == 2) {
//         DeleteConflict(Me_MOTION_C->model->object[8]);
//         model = Me_MOTION_C->model->object[0xb];
// LAB_8001fe68:
//         DeleteConflict(model);
//       }
//       else {
//         if (2 < sVar1) {
//           if (sVar1 != 3) goto LAB_8001fe28;
//           model = Me_MOTION_C->model->object[2];
//           goto LAB_8001fe68;
//         }
//         if (sVar1 != 0) {
// LAB_8001fe28:
//           DeleteConflict(Me_MOTION_C->model->object[0xd]);
//           model = Me_MOTION_C->model->object[0xe];
//           goto LAB_8001fe68;
//         }
//       }
//       if ((AfterimageType *)Me_MOTION_C->illusion[0] != (AfterimageType *)0x0) {
//         DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[0]);
//         Me_MOTION_C->illusion[0] = (undefined *)0x0;
//       }
//       if ((AfterimageType *)Me_MOTION_C->illusion[1] != (AfterimageType *)0x0) {
//         DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[1]);
//         Me_MOTION_C->illusion[1] = (undefined *)0x0;
//       }
//       human = Me_MOTION_C;
//       dtM->mask = 0x7fff;
//       if ((human->wpatk == 0x2a) && (human->weapon[3] != (OrnamentType *)0x0)) {
//         human->weapon[2] = human->weapon[0];
//         human->weapon[0] = human->weapon[3];
//         human->weapon[3] = (OrnamentType *)0x0;
//         Sound(human,1);
//       }
//     }
//     mmp = dtM;
//     if ((dtM->count == 0) && (0 < dtM->loop)) {
//       dtM->count = dtM->motion->time + -1;
//       PlayMotion(mmp,1);
//       pSVar2 = dtV;
//       dtM->loop = -1;
//       pSVar2->vz = 0;
//       pSVar2->vx = 0;
//     }
//     if ((dtM->loop == -1) && (dtPAD != 0)) {
//       motID = 0x100c;
//       DAT_80097f0e = 1;
//       iVar4 = 0;
//       if (MotionUpdateMode != 0) {
//         iVar3 = 0;
//         do {
//           iVar4 = iVar4 + 1;
//           if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar3 >> 0xd)) == Me_MOTION_C)
//           goto LAB_80020018;
//           iVar3 = iVar4 * 0x10000;
//         } while (iVar4 * 0x10000 >> 0x10 < 5);
//       }
//       SetNowMotion(Me_MOTION_C,0x100c,1);
//       DAT_80097f0e = 0xffff;
// LAB_80020018:
//       dtM->count = -0xf;
//     }
//     break;
//   case 1:
//     if (dtM->loop == 0) {
//       return;
//     }
//     if (dtPAD == 0) {
//       return;
//     }
//     if (Me_MOTION_C == StagePlayer) {
//       SetCameraMode(CMODE_NORMAL);
//     }
//     if ((Me_MOTION_C->attribute & 0x40U) == 0) {
// LAB_800200ec:
//       motID = 0;
//     }
//     else {
//       motID = 0x501;
//     }
//     goto LAB_800200f4;
//   default:
//     if (dtM->count != 0) {
//       return;
//     }
//     if (dtM->loop == 0) {
//       return;
//     }
//     if (Me_MOTION_C == StagePlayer) {
//       SetCameraMode(CMODE_NORMAL);
//     }
//     if ((Me_MOTION_C->attribute & 0x40U) == 0) goto LAB_800200ec;
//     motID = 0x501;
// LAB_800200f4:
//     DAT_80097f0e = 1;
//     break;
//   case 4:
//   case 5:
//     if ((Me_MOTION_C->life != Me_MOTION_C->lifemax) || ((Me_MOTION_C->attribute & 1U) != 0)) {
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
//     if (dtM->count == 1) {
//       Sound(Me_MOTION_C,0xf);
//     }
//     else if ((dtM->count == 0) && (dtM->loop != 0)) {
//       if (Me_MOTION_C == StagePlayer) {
//         SetCameraMode(CMODE_NORMAL);
//       }
//       if ((Me_MOTION_C->attribute & 0x40U) == 0) {
//         motID = 0;
//         DAT_80097f0e = 1;
//       }
//       else {
//         motID = 0x501;
//         DAT_80097f0e = 1;
//       }
//     }
//     break;
//   case 6:
//     if (dtM->count == 1) {
//       Sound(Me_MOTION_C,6);
//     }
//     else if ((dtM->count == 0) && (dtM->loop != 0)) {
//       motID = 0x80e;
//       DAT_80097f0e = 1;
//     }
//   }
//   return;
// }

#endif /* NON_MATCHING */
