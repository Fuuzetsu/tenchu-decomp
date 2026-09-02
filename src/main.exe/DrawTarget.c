#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawTarget(long x, long y, long z, long color);
 *     EFFECT.C:602, 6 src lines, frame 40 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long x
 *     param $a1       long y
 *     param $a2       long z
 *     param $a3       long color
 *     stack sp+16     struct SVECTOR scr
 * END PSX.SYM */

extern MATRIX GsWSMATRIX;
extern void DrawTargetS(s32 x, s32 y, s32 z, s32 color);

void DrawTarget(s32 x, s32 y, s32 z, s32 color)
{
    SVECTOR scr;
    SVECTOR *projected;

    *SCREEN_PROJECTION_TRANSLATION_X = 0;
    *SCREEN_PROJECTION_TRANSLATION_Y = 0;
    *SCREEN_PROJECTION_TRANSLATION_Z = 0;
    *SCREEN_PROJECTION_POINT_X = x - (s16)ViewInfo.vpx;
    *SCREEN_PROJECTION_POINT_Y = y - (s16)ViewInfo.vpy;
    *SCREEN_PROJECTION_POINT_Z = z - (s16)ViewInfo.vpz;
    SetTransMatrix(SCREEN_PROJECTION_MATRIX);
    SetRotMatrix(&GsWSMATRIX);
    projected = &scr;
    projected->vz = RotTransPers(
        SCREEN_PROJECTION_POINT, (s32 *)projected,
        SCREEN_PROJECTION_PERSPECTIVE, SCREEN_PROJECTION_FLAG);
    DrawTargetS(scr.vx, scr.vy, scr.vz - 5, color);
}
