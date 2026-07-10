#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void Camera(void);
 *     CAMERA.C:784, 110 src lines, frame 72 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     stack sp+24     struct GsRVIEW2 vDif
 *     reg   $s1       short pad_dat
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TItem items[30];
 *     extern enum TSystemFlag SystemFlag;
 *     extern short SkipFrame;
 *     extern int Projection;
 * END PSX.SYM */

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/Camera", Camera);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/Camera", something_about_character_camera_and_fpv___override__prt_80031c34_f0048b67);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/Camera", something_about_character_camera_and_fpv___override__prt_80031c40_394600bf);

// triage: MEDIUM — 158 insns, 8 callees, ~0.09 to ReqItemDefault
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void Camera(void)
//
// {
//   ushort uVar1;
//   ModelArchiveType *pMVar2;
//   char *pcVar3;
//   char *pcVar4;
//   long lVar5;
//   GsRVIEW2 local_28;
//
//   uVar1 = GetPad(0);
//   if (((uint)CamState.Owner & 1) != 0) {
//     return;
//   }
//   if (CamState.Mode == CMODE_LOCK) {
//     local_28.vpx = 0;
//     local_28.vpy = 0;
//     local_28.vpz = 0;
//     local_28.vrx = 0;
//     local_28.vry = 0;
//     local_28.vrz = 0;
//   }
//   else {
//     if ((int)CamState.Mode < 0xe) {
//       if (CamState.Mode == CMODE_DIRECTION) {
//         CameraDirection(CamState.Owner,&local_28);
//         goto LAB_80031b2c;
//       }
//     }
//     else if (CamState.Mode == CMODE_FALL) {
//       local_28.vpx = 0;
//       local_28.vpy = 0;
//       local_28.vpz = 0;
//       local_28.vrx = ((CamState.Owner)->model->locate).coord.t[0] - ViewInfo.vrx;
//       local_28.vry = ((CamState.Owner)->model->locate).coord.t[1] - ViewInfo.vry;
//       local_28.vrz = ((CamState.Owner)->model->locate).coord.t[2] - ViewInfo.vrz;
//       goto LAB_80031b2c;
//     }
//     if ((((CamState.Owner)->pad).data & 4) != 0) {
//       SetCameraMode(CMODE_DIRECTION);
//       return;
//     }
//     CameraType1(CamState.Owner,&local_28);
//   }
// LAB_80031b2c:
//   ViewInfo.vrx = ViewInfo.vrx + local_28.vrx;
//   ViewInfo.vry = ViewInfo.vry + local_28.vry;
//   ViewInfo.vrz = ViewInfo.vrz + local_28.vrz;
//   ViewInfo.vpx = ViewInfo.vpx + local_28.vpx;
//   ViewInfo.vpy = ViewInfo.vpy + local_28.vpy;
//   ViewInfo.vpz = ViewInfo.vpz + local_28.vpz;
//   GsSetRefView2(&ViewInfo);
//   if ((((SystemFlag & 1) != 0) && (SkipFrame != 1)) && ((uVar1 & 0x100) != 0)) {
//     if ((uVar1 & 1) != 0) {
//       Projection = 300;
//     }
//     if ((uVar1 & 0x8000) == 0) {
//       if ((uVar1 & 0x2000) != 0) {
//         Projection = Projection + 1;
//       }
//     }
//     else {
//       Projection = Projection + -1;
//     }
//     pMVar2 = (CamState.Owner)->model;
//     pcVar3 = (char *)(pMVar2->locate).coord.t[0];
//     pcVar4 = (char *)(pMVar2->locate).coord.t[1];
//     lVar5 = (pMVar2->locate).coord.t[2];
//     FntPrint("OWNER: (%d, %d, %d) R:%d",pcVar3,pcVar4,lVar5);
//     FntPrint(&DAT_80097a38,pcVar3,pcVar4,lVar5);
//     GsSetProjection(Projection);
//     FUN_8003076c((int)(short)uVar1);
//   }
//   return;
// }
