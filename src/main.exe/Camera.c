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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
    /* The debug owner menu stores AdtSelect's return here — an odd value
     * (its -1 cancel sentinel) is not a real Humanoid. */
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
        if (CamState.Owner->pad.data & PADL1)
        {
            SetCameraMode(CMODE_DIRECTION);
            return;
        }
        CameraType1(CamState.Owner, &vDif);
        break;
    }
    ViewInfo.vrx += vDif.vrx;
    ViewInfo.vry += vDif.vry;
    ViewInfo.vrz += vDif.vrz;
    ViewInfo.vpx += vDif.vpx;
    ViewInfo.vpy += vDif.vpy;
    ViewInfo.vpz += vDif.vpz;
    GsSetRefView2(&ViewInfo);

    if ((SystemFlag & SYSFLAG_DEBUGPRINT) != 0 && SkipFrame != SKIPFRAME_SKIPPED &&
        (pad_dat & PADselect) != 0)
    {
        ModelType *model;

        if (pad_dat & PADL2)
        {
            Projection = PROJECTION_DISTANCE;
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
