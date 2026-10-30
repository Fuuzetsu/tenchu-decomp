#include "common.h"
#include "mission_editor.h"

/* The 4-bit atlas packs four 32x64 portraits vertically in each column.
 * Each portrait has its own palette; (x,y) is its bottom-center anchor. */
void DrawTrialCharacterThumbnail(short kind, short x, short y,
                                 int depth, short scale)
{
    POLY_FT4 quad;
    GsIMAGE image;
    short i = 0;
    short u, v;

    while (TrialCharacterLabels[i].kind != CHARACTER_KIND_END)
    {
        if (TrialCharacterLabels[i].kind == kind)
            break;
        i++;
    }
    u = i / 4;
    v = i % 4;
    image.px = u * 8 + 592;
    image.py = v * 64;
    image.cx = 576;
    image.cy = i;
    setPolyFT4(&quad);
    setRGB0(&quad, 128, 128, 128);
    image.pw = scale * 32 / 100;
    image.ph = scale * 64 / 100;
    quad.tpage = getTPage(0, 0, image.px, image.py);
    quad.clut = getClut(image.cx, image.cy);
    quad.u0 = (short)(image.px % 64) * 4;
    quad.v0 = image.py % 256;
    quad.u1 = quad.u0 + 31;
    quad.v1 = quad.v0;
    quad.u2 = quad.u0;
    quad.v2 = quad.v0 + 63;
    quad.u3 = quad.u1;
    quad.v3 = quad.v2;
    x -= image.pw / 2;
    y -= image.ph;
    setXY4(&quad, x, y, image.pw + x, y,
                 x, image.ph + y, image.pw + x, image.ph + y);
    GsSortPoly(&quad, OTablePt, depth);
}
