#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawBleed(struct tag_EffectSlot *ef);
 *     EFFECT.C:910, 34 src lines, frame 40 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct tag_EffectSlot * ef
 *     reg   $s1       struct BleedType * param
 *     stack sp+16     struct SVECTOR scr
 *     reg   $v0       long x
 *     reg   $a1       long y
 *     reg   $a2       long z
 *     reg   $s0       struct SVECTOR * scr
 *     reg   $v1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct POLY_F4 plyBleed;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

extern MATRIX GsWSMATRIX;

void DrawBleed(TEffectSlot *ef)
{
    BleedType *param = &ef->param.bleed;
    SVECTOR scr;
    SVECTOR *projected;
    long x, y, z;
    s32 otz;
    s16 pri;
    s16 sz;

    if (param->mode == 0)
    {
        if (param->time == 0)
        {
            ef->proc = 0;
        }
        else
        {
            param->pos.vx += param->vec.vx;
            param->pos.vy += param->vec.vy;
            param->pos.vz += param->vec.vz;
            param->vec.vy++;
        }
    }
    x = param->pos.vx;
    /* Preserve the scalar view of these VECTOR fields; it affects alias scheduling. */
    y = *(s32 *)&param->pos.vy;
    z = *(s32 *)&param->pos.vz;
    param->time--;

    *SCREEN_PROJECTION_TRANSLATION_X = 0;
    *SCREEN_PROJECTION_TRANSLATION_Y = 0;
    *SCREEN_PROJECTION_TRANSLATION_Z = 0;
    *SCREEN_PROJECTION_POINT_X = x - (short)ViewInfo.vpx;
    *SCREEN_PROJECTION_POINT_Y = y - (short)ViewInfo.vpy;
    *SCREEN_PROJECTION_POINT_Z = z - (short)ViewInfo.vpz;
    SetTransMatrix(SCREEN_PROJECTION_MATRIX);
    SetRotMatrix(&GsWSMATRIX);
    projected = &scr;
    projected->vz = (s16)RotTransPers(
        SCREEN_PROJECTION_POINT, (s32 *)projected,
        SCREEN_PROJECTION_PERSPECTIVE, SCREEN_PROJECTION_FLAG);

    otz = scr.vz;
    if (otz > NEAR_DEPTH)
    {
        plyBleed.x0 = scr.vx;
        plyBleed.y0 = scr.vy;
        plyBleed.y1 = scr.vy;
        plyBleed.x2 = scr.vx;
        sz = (s16)(900 / otz) + 1;
        plyBleed.x1 = scr.vx + sz;
        plyBleed.y2 = scr.vy + sz;
        plyBleed.x3 = scr.vx + sz;
        plyBleed.y3 = scr.vy + sz;
        plyBleed.r0 = param->r;
        plyBleed.g0 = param->g;
        plyBleed.b0 = param->b;
        pri = otz >> 2;
        if (pri >= 0)
        {
            pri = DEPTH_LIMIT - 1;
            if ((otz >> 2) < DEPTH_LIMIT)
            {
                pri = otz >> 2;
            }
        }
        else
        {
            pri = 0;
        }
        GsSortPoly(&plyBleed, OTablePt, (u16)pri);
    }
}
