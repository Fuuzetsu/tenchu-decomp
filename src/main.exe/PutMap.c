#include "common.h"
#include "sound.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PutMap(void);
 *     INFOVIEW.C:1322, 65 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s2       int rgb
 *     reg   $s1       struct POLY_XF4 * ply
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char PutMapMode;
 *     extern struct GsSPRITE MapImage;
 *     extern struct GsOT *OTablePt;
 *     extern int StageID;
 * END PSX.SYM */

extern s32 MapSlideX;
extern s32 MapSlideY;
extern MapPlacementType MapPlacement[N_STAGE_CONFIGS];

extern void SetPolyXF4(POLY_XF4 *ply, short attrib);
extern void draw_map_items_(s32 x, s32 z, MapPlacementType *placement);
extern void AddXF4(void *ot, POLY_XF4 *ply);

void PutMap(void)
{
    POLY_XF4 *ply;
    s32 rgb;

    ply = (POLY_XF4 *)GsGetWorkBase();
    GsSetWorkBase(ply + 1);
    SetPolyXF4(ply, GPU_BLEND_SUBTRACT);
    setXY4(&ply->ply, -SCREEN_W / 2, -SCREEN_H / 2, SCREEN_W / 2,
           -SCREEN_H / 2, -SCREEN_W / 2, SCREEN_H / 2, SCREEN_W / 2,
           SCREEN_H / 2);
    rgb = (0xA0 - MapSlideX) / 4;

    switch (PutMapMode)
    {
    case PUTMAP_OPEN:
        MapSlideX = 160;
        MapSlideY = 0;
        PutMapMode = PUTMAP_SLIDE_IN;
        ply->ply.r0 = 0;
        ply->ply.g0 = 0;
        ply->ply.b0 = 0;
        SoundEx((VECTOR *)0, SE_MAP_OPEN);
        break;
    case PUTMAP_SLIDE_IN:
        MapImage.r = 0x3C;
        MapImage.g = 0x3C;
        MapImage.b = 0x3C;
        MapImage.scalex = FIXED_ONE;
        MapImage.scaley = FIXED_ONE;
        MapImage.x = MapSlideX;
        MapImage.y = MapSlideY;
        GsSortSprite(&MapImage, OTablePt, 2);
        MapImage.r = 0x80;
        MapImage.g = 0x80;
        MapImage.b = 0x80;
        ply->ply.r0 = rgb;
        ply->ply.g0 = rgb;
        ply->ply.b0 = rgb;
        MapSlideX -= 0x28;
        if (MapSlideX <= 0)
        {
            PutMapMode++;
        }
        break;
    case PUTMAP_SHOWN:
        MapSlideX = 0;
        MapSlideY = 0;
        ply->ply.r0 = rgb;
        ply->ply.g0 = rgb;
        ply->ply.b0 = rgb;
        draw_map_items_(CamState.Owner->model->locate.coord.t[0],
                        CamState.Owner->model->locate.coord.t[2],
                        &MapPlacement[StageID]);
        break;
    }

    MapImage.scalex = FIXED_ONE;
    MapImage.scaley = FIXED_ONE;
    MapImage.x = MapSlideX;
    MapImage.y = MapSlideY;
    GsSortSprite(&MapImage, OTablePt, 1);
    AddXF4(OTablePt->org + 2, ply);
}
