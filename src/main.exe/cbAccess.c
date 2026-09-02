#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void cbAccess(void);
 *     FILEIO.C:115, 27 src lines, frame 240 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct DISPENV o_disp
 *     stack sp+40     struct DRAWENV o_draw
 *     stack sp+136    struct DRAWENV n_draw
 *
 * Globals it touches, as the original declared them:
 *     extern struct POLY_GT4 AccessImage;
 *     extern int AccessPower;
 * END PSX.SYM */

void cbAccess(void)
{
    DISPENV o_disp;
    DRAWENV o_draw;
    DRAWENV n_draw;
    u32 intensity;

    intensity = (AccessPower + 8) & 0xff;
    AccessPower = intensity;
    AccessImage.r0 = intensity;
    AccessImage.g0 = AccessImage.r0;
    AccessImage.b0 = 0xff - AccessImage.r0;
    AccessImage.r1 = AccessImage.r0;
    AccessImage.g1 = AccessImage.b0;
    AccessImage.b1 = AccessImage.r0;
    AccessImage.r2 = AccessImage.b0;
    AccessImage.g2 = AccessImage.r0;
    AccessImage.b2 = AccessImage.r0;
    AccessImage.r3 = AccessImage.r0;
    AccessImage.g3 = AccessImage.r0;
    AccessImage.b3 = AccessImage.r0;
    GetDrawEnv(&o_draw);
    if (AccessPower != 0)
        GetDispEnv(&o_disp);
    else
        GetDispEnv(&o_disp);
    n_draw = o_draw;
    n_draw.clip = o_disp.disp;
    n_draw.ofs[0] = o_disp.disp.x;
    n_draw.ofs[1] = o_disp.disp.y;
    PutDrawEnv(&n_draw);
    DrawPrim((u8 *)&AccessImage);
    PutDrawEnv(&o_draw);
}
