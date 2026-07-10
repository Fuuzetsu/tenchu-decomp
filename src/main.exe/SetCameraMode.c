#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetCameraMode(enum TCameraMode mode);
 *     CAMERA.C:85, 9 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $a0       enum TCameraMode mode
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

/*
 * SetCameraMode (0x800316b8) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=SetCameraMode
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetCameraMode", SetCameraMode);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetCameraMode", switchD_80031704__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetCameraMode", switchD_80031704__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetCameraMode", switchD_80031704__caseD_f);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetCameraMode", switchD_80031704__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetCameraMode", switchD_80031704__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetCameraMode", switchD_80031704__caseD_2);

/* jump-table pool @ 0x80011c50 (16 words; tables at 0x80011c50) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 SetCameraMode_jtbl[16] = {
    0x80031978, 0x800318A0, 0x800319C4, 0x800318A0,
    0x8003170C, 0x800319C4, 0x800319C4, 0x800319C4,
    0x800319C4, 0x800319C4, 0x800319C4, 0x800319C4,
    0x800319C4, 0x800319C4, 0x800319C4, 0x80031888,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// void SetCameraMode(TCameraMode mode)
// 
// {
//   bool bVar1;
//   TCameraMode TVar2;
//   short sVar3;
//   int iVar4;
//   int iVar5;
//   SVECTOR *pSVar6;
//   VECTOR *pVVar7;
//   VECTOR VStack_78;
//   VECTOR VStack_68;
//   VECTOR VStack_58;
//   VECTOR VStack_48;
//   long lStack_38;
//   int local_34;
//   int local_30 [2];
//   
//   switch(mode) {
//   case CMODE_NORMAL:
//     TVar2 = mode;
//     if ((((CamState.Owner)->pad).data & 4) != 0) {
//       SetCameraMode(CMODE_DIRECTION);
//       TVar2 = CamState.Mode;
//     }
//     break;
//   case CMODE_DIRECTION:
//   case CMODE_SIGHT:
//     if ((CamState.Mode != CMODE_DIRECTION) && (CamState.Mode != CMODE_LOCK)) {
//       GetVectorRotation((VECTOR *)&ViewInfo,(VECTOR *)&ViewInfo.vrx,&local_34,local_30);
//       local_30[0] = local_30[0] - ((CamState.Owner)->model->rotate).vy;
//       iVar4 = local_30[0] + 0x2800;
//       iVar5 = iVar4;
//       if (iVar4 < 0) {
//         iVar5 = local_30[0] + 0x37ff;
//       }
//       local_30[0]._0_2_ = (short)iVar4 + (short)(iVar5 >> 0xc) * -0x1000 + -0x800;
//       CamState.DirectionRX = 0;
//       CamState.DirectionRY = (short)local_30[0];
//     }
//     goto LAB_8003196c;
//   default:
//     TVar2 = mode;
//     break;
//   case CMODE_CRITICAL_HIT:
//     iVar4 = rand();
//     iVar5 = iVar4;
//     if (iVar4 < 0) {
//       iVar5 = iVar4 + 3;
//     }
//     CamState.OldMode._0_1_ = (char)iVar4 + (char)(iVar5 >> 2) * -4;
//     iVar5 = 0;
//     bVar1 = true;
//     while (bVar1) {
//       CamState.OldMode._0_1_ = (undefined1)CamState.OldMode + CMODE_DIRECTION;
//       if (CMODE_SIGHT < (byte)(undefined1)CamState.OldMode) {
//         CamState.OldMode._0_1_ = CMODE_NORMAL;
//       }
//       iVar4 = (uint)(byte)(undefined1)CamState.OldMode * 0x20;
//       pVVar7 = (CamState.Owner)->locate;
//       pSVar6 = (CamState.Owner)->rotate;
//       sVar3 = FUN_8002fd9c();
//       Scratchpad._64_2_ = pSVar6->vx + sVar3;
//       Scratchpad._66_2_ = pSVar6->vy;
//       Scratchpad._68_2_ = pSVar6->vz;
//       RotMatrixYXZ((SVECTOR *)(Scratchpad + 0x40),(MATRIX *)(Scratchpad + 0x80));
//       Scratchpad._148_4_ = pVVar7->vx;
//       Scratchpad._152_4_ = pVVar7->vy;
//       Scratchpad._156_4_ = pVVar7->vz;
//       SetRotMatrix((MATRIX *)(Scratchpad + 0x80));
//       SetTransMatrix((MATRIX *)(Scratchpad + 0x80));
//       RotTrans((SVECTOR *)(&DAT_80089f50 + iVar4),&VStack_78,&lStack_38);
//       RotTrans((SVECTOR *)(&DAT_80089f58 + iVar4),&VStack_68,&lStack_38);
//       RotTrans((SVECTOR *)(&DAT_80089f60 + iVar4),&VStack_58,&lStack_38);
//       RotTrans((SVECTOR *)(&DAT_80089f68 + iVar4),&VStack_48,&lStack_38);
//       iVar4 = FUN_80039ddc(&VStack_58,&VStack_48,0,0);
//       iVar5 = iVar5 + 1;
//       if (0x7ff < iVar4) {
//         CamState.Mode = mode;
//         CamState.OldMode._1_1_ = 1;
//         return;
//       }
//       bVar1 = iVar5 < 4;
//     }
//     CamState.OldMode._0_1_ = CMODE_NORMAL;
//     TVar2 = CamState.Mode;
//     break;
//   case CMODE_FALL|CMODE_DIRECTION:
//     CamState.DirectionRX = 0;
//     CamState.DirectionRY = 0;
// LAB_8003196c:
//     CamState.Mode = CMODE_DIRECTION;
//     CamState.OldMode._0_1_ = (undefined1)mode;
//     TVar2 = CamState.Mode;
//   }
//   CamState.Mode = TVar2;
//   return;
// }

#endif /* NON_MATCHING */
