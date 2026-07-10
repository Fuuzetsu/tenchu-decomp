#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActITEM(void);
 *     MOTION.C:1949, 36 src lines, frame 64 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     stack sp+16     struct PARAM_ITEM_LAUNCH item
 *     reg   $a1       short flag
 *     reg   $a0       short mode
 *     reg   $v0       struct VECTOR * p
 * END PSX.SYM */

/*
 * ActITEM (0x80026074) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=ActITEM
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", ActITEM);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", switchD_800260b8__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", switchD_800260b8__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", switchD_800260b8__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", switchD_800260b8__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", switchD_800260b8__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", switchD_800260b8__caseD_1);

/* jump-table pool @ 0x800115d8 (6 words; tables at 0x800115d8) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 handle_char_state_using_item__jtbl[6] = {
    0x800260C0, 0x800261A4, 0x800260E8, 0x8002614C,
    0x80026174, 0x80026174,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// void FUN_80026074(void)
// 
// {
//   bool bVar1;
//   VECTOR *pVVar2;
//   TItemType TVar3;
//   short sVar4;
//   PARAM_ITEM_USE local_30;
//   
//   bVar1 = false;
//   switch((int)(((ushort)dtM->mid - 0xf00) * 0x10000) >> 0x10) {
//   case 0:
//     bVar1 = false;
//     if (dtM->count != 10) goto LAB_800261a8;
//     bVar1 = true;
//     local_30.type = ITEM_MAKIBISHI;
//     break;
//   case 2:
//     bVar1 = false;
//     if (dtM->count != 5) goto LAB_800261a8;
//     sVar4 = 4;
//     if (Me_MOTION_C == StagePlayer) {
//       sVar4 = DAT_80097b1e;
//     }
//     TVar3 = (TItemType)sVar4;
//     if ((TVar3 != ITEM_FIRE) && (bVar1 = false, TVar3 != ITEM_SMOKE)) goto LAB_800261a8;
//     bVar1 = true;
//     local_30.type = TVar3;
//     break;
//   case 3:
//     bVar1 = false;
//     if (dtM->count != 5) goto LAB_800261a8;
//     bVar1 = true;
//     local_30.type = ITEM_JIRAI;
//     break;
//   case 4:
//   case 5:
//     if (dtM->count != 0) {
//       return;
//     }
//     if (dtM->loop == 0) {
//       return;
//     }
//     dtM->loop = -1;
//     return;
//   }
// LAB_800261a8:
//   if (bVar1) {
//     local_30.user = Me_MOTION_C;
//     pVVar2 = GetAbsolutePosition(Me_MOTION_C->model->object[2],0,0,0);
//     local_30.start.vx = pVVar2->vx;
//     local_30.start.vy = pVVar2->vy;
//     local_30.start.vz = pVVar2->vz;
//     local_30.end.pad = local_30.start.pad;
//     local_30.end.vx = local_30.start.vx;
//     local_30.end.vy = local_30.start.vy;
//     local_30.end.vz = local_30.start.vz;
//     ReqItemUse(&local_30);
//   }
//   if ((dtM->count == 0) && (dtM->loop != 0)) {
//     if (Me_MOTION_C == StagePlayer) {
//       SetCameraMode(CMODE_NORMAL);
//     }
//     if ((Me_MOTION_C->attribute & 0x40U) == 0) {
//       motID = 0;
//     }
//     else {
//       motID = 0x501;
//     }
//     DAT_80097f0e = 1;
//   }
//   return;
// }

#endif /* NON_MATCHING */
