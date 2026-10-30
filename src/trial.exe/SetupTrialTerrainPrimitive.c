#include "common.h"
#include "mission_editor.h"
#include "../main.exe/tmdfast.h"

/* A tile uses a 64-pixel square texture. Split faces select its upper (1)
 * or lower (2) half; the caller fills the four vertex indices separately. */
short SetupTrialTerrainPrimitive(TMD_P_TNG4 *primitive, short texture, short half)
{
    u8 height = 63;
    u8 u, v;
    TrialTileTexture *image;

    if (!primitive || texture == -1)
        return -1;
    image = &TrialTileTextures[texture];
    v = image->v;
    u = image->u;
    if (half)
    {
        height = 31;
        v += half == 1 ? 0 : 32;
    }

    /* 12 output words, 10 input words, unlit textured Gouraud quad. */
    *(u_long *)primitive = (TMD_PRIM_GT4 << 24) | (1 << 16) | (10 << 8) | 12;
    primitive->tpage = image->page;
    primitive->clut = image->clut;
    primitive->tu0 = u;
    primitive->tv0 = v;
    primitive->tu1 = u + 63;
    primitive->tv1 = v;
    primitive->tu2 = u;
    primitive->tv2 = v + height;
    primitive->tu3 = u + 63;
    primitive->tv3 = v + height;
    primitive->r0 = primitive->g0 = primitive->b0 =
        primitive->r1 = primitive->g1 = primitive->b1 =
        primitive->r2 = primitive->g2 = primitive->b2 =
        primitive->r3 = primitive->g3 = primitive->b3 = TrialTerrainBrightness;
    return texture;
}
