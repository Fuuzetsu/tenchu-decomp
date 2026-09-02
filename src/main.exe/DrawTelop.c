#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawTelop(void);
 *     CHRANIM.C:420, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct POLY_F4 TelopbgP;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

extern u8 TelopText[];

extern s32 telop_text_width_(u8 *str);
extern void draw_telop_line_(GsOT_TAG *org, s32 x, s32 y, u8 *str);

void DrawTelop(void)
{
    enum
    {
        TELOP_INNER_Y = 90
    };
    s32 w;

    TelopbgP.y1 = TELOP_INNER_Y;
    TelopbgP.y0 = TELOP_INNER_Y;
    TelopbgP.y3 = SCREEN_H / 2;
    TelopbgP.y2 = SCREEN_H / 2;
    GsSortPoly(&TelopbgP, OTablePt, 1);
    TelopbgP.y1 = -(SCREEN_H / 2);
    TelopbgP.y0 = -(SCREEN_H / 2);
    TelopbgP.y3 = -TELOP_INNER_Y;
    TelopbgP.y2 = -TELOP_INNER_Y;
    GsSortPoly(&TelopbgP, OTablePt, 1);
    w = telop_text_width_(TelopText);
    draw_telop_line_(OTablePt->org, -(w / 2), 92, TelopText);
}
