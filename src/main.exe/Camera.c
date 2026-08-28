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

extern s32 Projection;

extern char fmt_owner_r[]; /* OWNER: (%d, %d, %d) R:%d */
extern char str_newline[]; /* "\n" */

extern void CameraDirection(Humanoid *pl, GsRVIEW2 *vDif);
extern void CameraType1(Humanoid *pl, GsRVIEW2 *vDif);
extern void debug_output_edit_camera_settings(s16 param);

void Camera(void)
{
    GsRVIEW2 vDif;
    s16 pad_dat;

    pad_dat = GetPad(0);
    if ((s32)CamState.Owner & 1)
    {
        return;
    }

    switch ((s32)CamState.Mode)
    {
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
        if (CamState.Owner->pad.data & 4)
        {
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
        (pad_dat & PADselect) != 0)
    {
        ModelType *model;

        if (pad_dat & 1)
        {
            Projection = 300;
        }
        if (pad_dat & PADLleft)
        {
            Projection--;
        }
        else if (pad_dat & PADLright)
        {
            Projection++;
        }
        model = CamState.Owner->model;
        FntPrint(fmt_owner_r, model->locate.coord.t[0], model->locate.coord.t[1], model->locate.coord.t[2], model->rotate.vy);
        FntPrint(str_newline);
        GsSetProjection(Projection);
        debug_output_edit_camera_settings(pad_dat);
    }
}
