#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PutNumber(int x, int y, int cols, int n);
 *     INFOVIEW.C:197, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int x
 *     param $a1       int y
 *     param $a2       int cols
 *     param $a3       int n
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE NumberImage;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

void PutNumber(int x, int y, int cols, int n)
{
    enum
    {
        NW = 4
    };
    enum
    {
        GAP = 6
    };
    int base;
    GsSPRITE *img;
    int q;

    NumberImage.w = NW;
    img = &NumberImage;
    base = img->u;
    img->x = (s16)x;
    img->y = (s16)y;
loop:
    q = cols / 10;
    img->u = base + (cols % 10) * NW;
    GsSortSprite(img, OTablePt, 0);
    img->x -= GAP;
    cols = q;
    if (cols != 0)
        goto loop;
    img->u = base;
}
