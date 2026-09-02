#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void FadeOutDirect(short time, short attrib, unsigned char r, unsigned char g, int b);
 *     EFFECT.C:1816, 32 src lines, frame 296 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short time
 *     param $a1       short attrib
 *     param $a2       unsigned char r
 *     param $a3       unsigned char g
 *     param stack+16  int b
 *     stack sp+16     struct DISPENV o_disp
 *     stack sp+40     struct DRAWENV o_draw
 *     stack sp+136    struct DRAWENV n_draw
 *     stack sp+232    struct POLY_XF4 ply
 * END PSX.SYM */

extern int VSync(int mode);

void FadeOutDirect(short time, short attrib, u8 r, u8 g, u8 b)
{
    DISPENV o_disp;
    DRAWENV o_draw;
    DRAWENV n_draw;
    POLY_XF4 ply;
    POLY_XF4 *packet;

    GetDrawEnv(&o_draw);
    GetDispEnv(&o_disp);
    n_draw = o_draw;
    n_draw.clip = o_disp.disp;
    n_draw.ofs[0] = o_disp.disp.x;
    n_draw.ofs[1] = o_disp.disp.y;
    PutDrawEnv(&n_draw);
    packet = &ply;
    setPolyF4(&packet->ply);
    setSemiTrans(&ply.ply, 1);
    setlen(&packet->tpage, GPU_PACKET_LENGTH(DR_TPAGE));
    ply.tpage.code[0] = GPU_DRAWMODE_BLEND(attrib) | GPU_DRAWMODE_DITHER;
    ply.ply.x0 = 0;
    ply.ply.y0 = 0;
    ply.ply.y1 = 0;
    ply.ply.x2 = 0;
    ply.ply.r0 = r;
    ply.ply.g0 = g;
    ply.ply.b0 = b;
    ply.ply.x1 = o_disp.disp.w;
    ply.ply.y2 = o_disp.disp.h;
    ply.ply.x3 = o_disp.disp.w;
    ply.ply.y3 = o_disp.disp.h;
loop:
    if (time == 0)
    {
        goto end;
    }
    DrawPrim((u8 *)&ply.ply);
    DrawPrim((u8 *)&ply.tpage);
    DrawSync(0);
    VSync(0);
    time--;
    goto loop;
end:
    PutDrawEnv(&o_draw);
}
