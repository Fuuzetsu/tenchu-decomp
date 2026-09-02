#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetScreenPosition(long x, long y, long z, struct SVECTOR *scr);
 *     EFFECT.C:543, 32 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long x
 *     param $a1       long y
 *     param $a2       long z
 *     param $a3       struct SVECTOR * scr
 * END PSX.SYM */

extern MATRIX GsWSMATRIX;

void GetScreenPosition(long x, long y, long z, SVECTOR *scr)
{
    MATRIX *m = SCREEN_PROJECTION_MATRIX;
    SVECTOR *sv = SCREEN_PROJECTION_POINT;

    m->t[0] = 0;
    m->t[1] = 0;
    m->t[2] = 0;
    sv->vx = x - (short)ViewInfo.vpx;
    sv->vy = y - (short)ViewInfo.vpy;
    sv->vz = z - (short)ViewInfo.vpz;
    SetTransMatrix(m);
    SetRotMatrix(&GsWSMATRIX);
    scr->vz = RotTransPers(sv, (s32 *)scr, SCREEN_PROJECTION_PERSPECTIVE,
                           SCREEN_PROJECTION_FLAG);
}
