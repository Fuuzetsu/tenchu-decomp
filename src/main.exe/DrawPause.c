#include "common.h"
#include "main.exe.h"
#include "images.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawPause(void);
 *     INFOVIEW.C:1224, 35 src lines, frame 304 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct DISPENV o_disp
 *     stack sp+40     struct DRAWENV o_draw
 *     stack sp+136    struct DRAWENV n_draw
 *     stack sp+232    struct POLY_GT4 ply
 *
 * Globals it touches, as the original declared them:
 *     extern enum TSystemFlag SystemFlag;
 * END PSX.SYM */

void DrawPause(int frame)
{
    DISPENV o_disp;
    DRAWENV o_draw;
    DRAWENV n_draw;
    POLY_GT4 ply;
    GsIMAGE *image;
    s32 t;
    s32 bias;
    u8 far_col;

    if ((SystemFlag & SYSFLAG_DEBUGPRINT) == 0)
    {
        GetDrawEnv(&o_draw);
        GetDispEnv(&o_disp);
        n_draw = o_draw;
        n_draw.clip = o_disp.disp;
        n_draw.ofs[0] = o_disp.disp.x;
        n_draw.ofs[1] = o_disp.disp.y;
        PutDrawEnv(&n_draw);
        image = GetImage(IMG_PAUSE);
        SetupImageToPolyGT4(image, &ply, (s16)(0xA0 - image->pw * 2), (s16)(0x78 - (image->ph >> 1)));
        t = (s16)frame * 0x44;
        /* Empty loop retained for code layout; its original source construct is unknown. */
        do
        {
        } while (0);
        bias = 0x80;
        far_col = rsin(t) * 125 / FIXED_ONE + bias;
        ply.r0 = rsin(t + ANGLE_HALF_QUADRANT) * 125 / FIXED_ONE + bias;
        ply.g0 = ply.r0;
        ply.b0 = ply.r0;
        ply.r1 = ply.r0;
        ply.g1 = ply.r0;
        ply.b1 = ply.r0;
        ply.r2 = far_col;
        ply.g2 = far_col;
        ply.b2 = far_col;
        ply.r3 = far_col;
        ply.g3 = far_col;
        ply.b3 = far_col;
        DrawPrim(&ply);
        PutDrawEnv(&o_draw);
    }
}
