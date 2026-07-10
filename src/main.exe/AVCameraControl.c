#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AVCameraControl(void);
 *     CHRANIM.C:322, 44 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     stack sp+16     struct SVECTOR vect
 *     reg   $s1       long xx
 *     reg   $s0       long zz
 *     reg   $s2       long len
 *     reg   $a1       short ry
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

/*
 * AVCameraControl (0x80051228) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=AVCameraControl
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/AVCameraControl", AVCameraControl);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/AVCameraControl", switchD_800512b8__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/AVCameraControl", switchD_800512b8__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/AVCameraControl", switchD_800512b8__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/AVCameraControl", switchD_800512b8__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/AVCameraControl", switchD_800512b8__caseD_6);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/AVCameraControl", switchD_800512b8__caseD_8);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/AVCameraControl", switchD_800512b8__caseD_9);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/AVCameraControl", switchD_800512b8__caseD_0);

/* jump-table pool @ 0x8001368c (9 words; tables at 0x8001368c) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 AVCameraControl_jtbl[9] = {
    0x80051404, 0x800512C0, 0x800512D0, 0x800512D0,
    0x80051300, 0x80051300, 0x8005133C, 0x8005133C,
    0x800513AC,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// void AVCameraControl(void)
// 
// {
//   short ry;
//   long lVar1;
//   int iVar2;
//   int dx;
//   short ordr;
//   SVECTOR local_18;
//   
//   dx = ViewInfo.vpx - ViewInfo.vrx;
//   iVar2 = ViewInfo.vpz - ViewInfo.vrz;
//   lVar1 = SquareRoot0(dx * dx + iVar2 * iVar2);
//   ordr = (short)lVar1;
//   ry = GetDirection(dx,iVar2,0);
//   switch(DAT_80097cca) {
//   case 0:
//     goto switchD_800512b8_caseD_0;
//   case 1:
//     Camera();
//     return;
//   case 2:
//   case 3:
//     if (DAT_80097cca == 2) {
//       ry = ry + DAT_80097cc8;
//     }
//     else {
//       ry = ry - DAT_80097cc8;
//     }
//     goto LAB_80051364;
//   case 4:
//   case 5:
//     if (DAT_80097cca == 4) {
//       iVar2 = -(int)DAT_80097cc8;
//     }
//     else {
//       iVar2 = (int)DAT_80097cc8;
//     }
//     ViewInfo.vpy = ViewInfo.vpy + iVar2;
//     break;
//   case 6:
//   case 7:
//     if (DAT_80097cca == 6) {
//       ordr = ordr - DAT_80097cc8;
//     }
//     else {
//       ordr = ordr + DAT_80097cc8;
//     }
// LAB_80051364:
//     GetMoveSpeed(&local_18,ry,ordr,0);
//     ViewInfo.vpx = ViewInfo.vrx + local_18.vx;
//     ViewInfo.vpz = ViewInfo.vrz + local_18.vz;
//     break;
//   case 8:
//     ViewInfo.vrx = **(long **)(DAT_80097cc4 + 0x38);
//     ViewInfo.vry = (*(int *)(*(int *)(DAT_80097cc4 + 0x38) + 4) -
//                    (int)*(short *)(DAT_80097cc4 + 0xe)) + 300;
//     ViewInfo.vrz = *(long *)(*(int *)(DAT_80097cc4 + 0x38) + 8);
//   }
//   GsSetRefView2(&ViewInfo);
// switchD_800512b8_caseD_0:
//   return;
// }

#endif /* NON_MATCHING */
