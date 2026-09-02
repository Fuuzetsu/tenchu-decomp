#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AntiWall(struct GsRVIEW2 *vinfo, struct GsRVIEW2 *target);
 *     CAMERA.C:432, 91 src lines, frame 88 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct GsRVIEW2 * vinfo
 *     param $s2       struct GsRVIEW2 * target
 *     stack sp+24     struct SVECTOR vsL
 *     stack sp+32     struct SVECTOR vsR
 *     reg   $s3       int lvR
 *     reg   $s0       int rmap
 *     stack sp+40     struct SVECTOR av
 *     stack sp+48     int rx
 *     stack sp+52     int ry
 *     reg   $s5       int sx
 *     reg   $s3       int sy
 *     reg   $s0       int sz
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

extern SVECTOR WallProbeL;
extern SVECTOR WallProbeR;
extern char str_mark_l[];     /* (L) */
extern char str_mark_r[];     /* (R) */
extern char str_mark_alert[]; /* (!) */

void AntiWall(GsRVIEW2 *vinfo, GsRVIEW2 *target)
{
    VECTOR vsL;
    VECTOR vsR;
    int lvR;
    enum camera_probe_mask rmap;
    VECTOR av;
    int rx;
    int ry;
    int sx;
    int sy;
    int sz;

    GetVectorRotation(CAMERA_VIEWPOINT(target), CAMERA_REFERENCE(target),
                      &rx, &ry);
    ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS)->vz = 0;
    ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS)->vx = rx;
    ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS)->vy = ry;
    RotMatrixYXZ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS,
                 (MATRIX *)TENCHU_SCRATCHPAD(0x40));
    SetRotMatrix((MATRIX *)TENCHU_SCRATCHPAD(0x40));

    ApplyRotMatrix(&WallProbeL, &vsL);
    ApplyRotMatrix(&WallProbeR, &vsR);

    lvR = GetAreaMapLevel(GlobalAreaMap,
                          target->vpx + vsR.vx,
                          target->vpy + vsR.vy,
                          target->vpz + vsR.vz, AREA_LEVEL_DEFAULT);
    rmap = CAMERA_PROBE_CLEAR;
    if (GetAreaMapLevel(GlobalAreaMap,
                        target->vpx + vsL.vx,
                        target->vpy + vsL.vy,
                        target->vpz + vsL.vz, AREA_LEVEL_DEFAULT) <= target->vpy)
    {
        rmap = CAMERA_PROBE_FRONT_LEFT;
        FntPrint(str_mark_l);
    }
    if (lvR <= target->vpy)
    {
        rmap |= CAMERA_PROBE_FRONT_RIGHT;
        FntPrint(str_mark_r);
    }

    rmap &= CAMERA_PROBE_FRONT_MASK;
    if (rmap != CAMERA_PROBE_CLEAR)
    {
        av.vx = 0;
        av.vy = 0;
        av.vz = 0;
        if (rmap == CAMERA_PROBE_FRONT_LEFT)
        {
            ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS)->vx = WALL_AVOID_PUSH;
            ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS)->vy = 0;
            ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS)->vz = 0;
            ApplyRotMatrix((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS, &av);
        }
        if (rmap == CAMERA_PROBE_FRONT_RIGHT)
        {
            ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS)->vx = -WALL_AVOID_PUSH;
            ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS)->vy = 0;
            ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS)->vz = 0;
            ApplyRotMatrix((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS, &av);
        }

        sx = av.vx / 2;
        sy = av.vy / 2;
        sz = av.vz / 2;
        if (GetAreaMapLevel(GlobalAreaMap,
                            target->vpx + av.vx + sx,
                            target->vpy + av.vy + sy,
                            target->vpz + av.vz + sz,
                            AREA_LEVEL_DEFAULT) <= target->vpy)
        {
            av.vx = sx / 2;
            av.vy = sy / 2;
            av.vz = sz / 2;
            FntPrint(str_mark_alert);
        }
        target->vpx += av.vx;
        target->vpy += av.vy;
        target->vpz += av.vz;
    }
}
