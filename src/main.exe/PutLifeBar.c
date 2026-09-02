#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PutLifeBar(int x, int y, int n, int mx, int style);
 *     INFOVIEW.C:292, 32 src lines, frame 72 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       int x
 *     param $s4       int y
 *     param $s1       int n
 *     param $a3       int mx
 *     param stack+16  int style
 *     reg   $s5       int style
 *     stack sp+16     struct POLY_F4 poly
 *     reg   $a3       int w
 *     reg   $v0       int x
 *     reg   $a0       int y
 *     reg   $a3       int n
 *     reg   $s2       int ou
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE NumberImage;
 *     extern struct INFOVIEW__196fake LifeBarStyle[2];
 *     extern struct GsOT *OTablePt;
 *     extern long GameClock;
 * END PSX.SYM */

void PutLifeBar(s32 x, s32 y, s32 n, s32 mx, life_bar_style style)
{
    GsSPRITE *img;
    GsSPRITE *ou;
    s32 q;
    s16 oldh;
    s32 color;
    s32 dx;
    s32 dy;
    s32 u;

    {
        s32 px;
        s32 py;
        s32 count;

        count = n;
        NumberImage.w = (dx = LifeBarStyle[style].dx,
                         dy = LifeBarStyle[style].dy, 4);
        img = &NumberImage;
        u = img->u;
        px = x + dx;
        py = y + dy;
        img->x = px;
        img->y = py;

        {
            s32 q;

        loop:
            q = count / 10;
            img->u = u + (count % 10) * 4;
            GsSortSprite(img, OTablePt, 0);
            img->x -= 6;
            count = q;
        }
        if (count != 0)
            goto loop;
    }
    img->u = u;

    ou = &LifeBarStyle[style].frame;
    ou->x = x;
    ou->y = y;
    GsSortSprite(ou, OTablePt, 1);

    q = LifeBarStyle[style].scale * n / mx;
    ou = &LifeBarStyle[style].fill;
    oldh = ou->h;
    ou->h = LifeBarStyle[style].base + q;

    if (mx / 4 < n)
        color = 0x80;
    else
    {
        color = GameClock & 1;
        if (color != 0)
            color = 0xE6;
        else
            color = 0x80;
    }
    ou->b = color;
    ou->g = color;
    if (u != 0)
    {
        ou->r = color;
    }
    else
    {
        ou->r = color;
    }

    ou->x = x;
    ou->y = y;
    GsSortSprite(ou, OTablePt, 0);
    ou->h = oldh;
}
