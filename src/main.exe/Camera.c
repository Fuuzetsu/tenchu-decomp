#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "padcmd.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void Camera(void);
 *     CAMERA.C:784, 110 src lines, frame 72 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
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
 *     extern enum TSystemFlag SystemFlag;
 *     extern short SkipFrame;
 *     extern int Projection;
 * END PSX.SYM */

extern TSystemFlag SystemFlag;
extern s32 Projection;

extern char D_80011BA4[]; /* "OWNER: (%d, %d, %d) R:%d" */
extern char D_80097A38[]; /* "\n" */

extern void CameraDirection(Humanoid *pl, GsRVIEW2 *vDif);
extern void CameraType1(Humanoid *pl, GsRVIEW2 *vDif);
extern void debug_output_edit_camera_settings(s16 param);

void Camera(void)
{
    GsRVIEW2 vDif;
    s16 pad_dat;

    pad_dat = GetPad(0);
    if ((s32)CamState.Owner & 1) {
        return;
    }

    switch ((s32)CamState.Mode) {
    case CMODE_DIRECTION:
        CameraDirection(CamState.Owner, &vDif);
        break;
    case CMODE_LOCK:
        vDif.vpx = 0;
        vDif.vpy = 0;
        vDif.vpz = 0;
        vDif.vrx = 0;
        vDif.vry = 0;
        vDif.vrz = 0;
        break;
    case CMODE_FALL:
        vDif.vpx = 0;
        vDif.vpy = 0;
        vDif.vpz = 0;
        vDif.vrx = CamState.Owner->model->locate.coord.t[0] - ViewInfo.vrx;
        vDif.vry = CamState.Owner->model->locate.coord.t[1] - ViewInfo.vry;
        vDif.vrz = CamState.Owner->model->locate.coord.t[2] - ViewInfo.vrz;
        break;
    default:
        if (CamState.Owner->pad.data & 4) {
            SetCameraMode(CMODE_DIRECTION);
            return;
        }
        CameraType1(CamState.Owner, &vDif);
        break;
    }
    ViewInfo.vrx = ViewInfo.vrx + vDif.vrx;
    ViewInfo.vry = ViewInfo.vry + vDif.vry;
    ViewInfo.vrz = ViewInfo.vrz + vDif.vrz;
    ViewInfo.vpx = ViewInfo.vpx + vDif.vpx;
    ViewInfo.vpy = ViewInfo.vpy + vDif.vpy;
    ViewInfo.vpz = ViewInfo.vpz + vDif.vpz;
    GsSetRefView2(&ViewInfo);

    if ((SystemFlag & SYSFLAG_DEBUGPRINT) != 0 && SkipFrame != 1 &&
        (pad_dat & 0x100) != 0) {
        ModelType *model;

        if (pad_dat & 1) {
            Projection = 300;
        }
        if (pad_dat & 0x8000) {
            Projection = Projection - 1;
        } else if (pad_dat & 0x2000) {
            Projection = Projection + 1;
        }
        model = CamState.Owner->model;
        FntPrint(D_80011BA4, model->locate.coord.t[0], model->locate.coord.t[1], model->locate.coord.t[2], model->rotate.vy);
        FntPrint(D_80097A38);
        GsSetProjection(Projection);
        debug_output_edit_camera_settings(pad_dat);
    }
}

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
