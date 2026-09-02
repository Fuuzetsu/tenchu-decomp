#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void MakeDif(struct GsRVIEW2 *vinfo, struct GsRVIEW2 *target, struct GsRVIEW2 *vdif);
 *     CAMERA.C:418, 8 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct GsRVIEW2 * vinfo
 *     param $a1       struct GsRVIEW2 * target
 *     param $a2       struct GsRVIEW2 * vdif
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

extern TMakeDifInfo ref;
extern TMakeDifInfo pnt;
extern void MakeDifSub(VECTOR *src, VECTOR *target, VECTOR *dest, TMakeDifInfo *info);

void MakeDif(GsRVIEW2 *vinfo, GsRVIEW2 *target, GsRVIEW2 *vdif)
{
    if (CamState.snap_pending == 1)
    {
        vdif->vpx = target->vpx - vinfo->vpx;
        vdif->vpy = target->vpy - vinfo->vpy;
        vdif->vpz = target->vpz - vinfo->vpz;
        vdif->vrx = target->vrx - vinfo->vrx;
        vdif->vry = target->vry - vinfo->vry;
        vdif->vrz = target->vrz - vinfo->vrz;
        CamState.snap_pending = 0;
    }
    else
    {
        MakeDifSub(CAMERA_REFERENCE(vinfo), CAMERA_REFERENCE(target),
                   CAMERA_REFERENCE(vdif), &ref);
        MakeDifSub(CAMERA_VIEWPOINT(vinfo), CAMERA_VIEWPOINT(target),
                   CAMERA_VIEWPOINT(vdif), &pnt);
    }
}
