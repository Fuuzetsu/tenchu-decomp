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

/*
 * MATCH.
 *
 * PutMap (0x8004b848, 0x214 bytes) — draws the pause-menu map: a diamond
 * POLY_XF4 wipe outline (fresh each call from the GPU work buffer) plus the
 * MapImage sprite, animated through a 3-state sequence in `PutMapMode`
 * (gp-relative, this TU's own): 0 = just opened (init the wipe position/
 * sound), 1 = wipe animating closed (draws a highlighted-outline "seam"
 * frame each tick, then slides `x` toward 0 by 0x28/tick), 2 =
 * fully closed (draws the player's on-map marker via `draw_map_items_`).
 *
 * `rgb` (PSX.SYM's declared local) is a brightness ramp derived from the
 * wipe position: `(0xA0 - x) / 4` — plain integer division by the
 * constant 4 (not a hand-written bias+shift): cc1 emits the
 * `if (t<0) t+=3; t>>=2;` guard automatically for INT division by a
 * power-of-2 constant (same lever as PutLifeBar's `mx/4`).
 *
 * `ply` is this call's POLY_XF4, borrowed from the GPU's rolling work
 * buffer (GsGetWorkBase/GsSetWorkBase, bumped by exactly `sizeof(POLY_XF4)`
 * via plain `ply + 1` pointer arithmetic) — same local `POLY_XF4`/`POLY_F4`/
 * `DR_TPAGE` layout shared in game_types.h.
 *
 * The mode-2 draw call's first two args come from
 * `CamState.Owner->model->locate.coord.t[0/2]`. `->model` is item.h's
 * `ModelArchiveType *model` @0x58, and
 * `.locate.coord.t[0]/.t[2]` are the MATRIX translation X/Z (offsets
 * 0x18/0x20 from `model`, matching the raw `lw 0x18(v1)`/`lw 0x20(v1)`).
 * The 3rd arg is `&MapPlacement[StageID]`, the per-stage scale/rotation/screen
 * placement row. NOTE: MapPlacement/MapSlideX/MapSlideY (splat's x/y) were only
 * auto-labeled by splat while PutMap's OWN asm carve referenced them; once
 * this file compiles as plain C, nothing else references them, so they
 * needed explicit `config/symbols.main.exe.txt` entries (same lever as
 * LifeBarStyle in PutLifeBar.c) — first hit here, worth remembering for
 * any OTHER TU where you're the last matcher touching a shared small.
 *
 * The dispatch on `PutMapMode` (values 0/1/2, no default) MUST be written
 * as a real `switch`, not an if/else-if ladder: only `switch` reproduces
 * the target's `lbu`-once + SIGNED `slti` compare (an if/else-if ladder
 * over this exact same u8 read compiles the "smart" UNSIGNED `sltiu`
 * instead — verified by isolated cc1 experiment; neither `signed char` nor
 * an explicit cast reproduces `lbu`+`slti` together, only genuine switch
 * lowering does). Case bodies are laid out in SOURCE order 0,1,2 — the
 * TEST order (1, then <2, then 2) differs from body order (cookbook
 * Dispatch: "case-body memory order reveals the source case order").
 * The x0/y0/x1/y1/x2/y2/x3/y3 POLY_XF4 field assignments must be
 * INTERLEAVED (x0,y0,x1,y1,x2,y2,x3,y3), not grouped (y0..y3 then x0..x3,
 * matching Ghidra's rendering) — grouped order ties two of the four
 * repeated-constant registers to the wrong class (a0/v1 swapped),
 * cascading through ~15 register-only diffs; interleaved order matches
 * the target exactly with no register diffs at all.
 */
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
