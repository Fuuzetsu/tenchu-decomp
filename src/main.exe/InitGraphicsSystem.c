#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "item.h"
#include <psxsdk/libgpu.h>
#include "tmdfast.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitGraphicsSystem(void);
 *     3DCTRL.C:45, 29 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 *     extern struct GsFOGPARAM Fog;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct GsOT OTable[2];
 *     extern struct GsOT_TAG ZSortTable[2][2048];
 * END PSX.SYM */

extern GsOT_TAG ZSortTable[N_DRAW_PAGES][N_OT_TAGS];
extern s32 DepthPoint;
extern s32 SlightPoint;

/* Keep this literal until the persistent word's original declaration is recovered. */
#define STARTING_RNG_SEED (*(s32 *)TENCHU_PERSISTENT_RNG_ADDRESS)

extern void SetDepthQ(s32 dqa, s32 dqb);
extern void GsSetNearClip(s32 near);
extern void AdtFntLoad(int tx, int ty);
extern void AdtFntOpen(int x, int y, int w, int h, int isbg, int n);
extern s32 VSync(s32 mode);
extern void srand(u32 seed);

void InitGraphicsSystem(void)
{
    SetDispMask(0);
    GsInitGraph(SCREEN_W, SCREEN_H, 0x34, 1, 0);
    GsDefDispBuff(0, 0, 0, SCREEN_H);
    GsInit3D();
    GsInitCoordinate2(NULL, &World.locate);
    UpdateCoordinate(&World);
    DrawTMDmode = TMD_BANK_PLAIN;
    SetDepthQ(FOG_DQA, FOG_DQB);
    DepthPoint = DEPTH_LIMIT;
    SlightPoint = 150;
    Fog.dqa = FOG_DQA;
    Fog.dqb = FOG_DQB;
    Fog.bfc = 0;
    Fog.gfc = 0;
    Fog.rfc = 0;
    GsSetFogParam(&Fog);
    GsSetLightMode(0);
    GsSetAmbient(FIXED_HALF, FIXED_HALF, FIXED_HALF);
    GsSetProjection(PROJECTION_DISTANCE);
    ViewInfo.vpx = 0;
    ViewInfo.vpy = 0;
    ViewInfo.vpz = -1000;
    ViewInfo.vrx = 0;
    ViewInfo.vry = 0;
    ViewInfo.vrz = 0;
    ViewInfo.rz = 0;
    ViewInfo.super = &World.locate;
    GsSetRefView2(&ViewInfo);
    GsSetNearClip(0);
    AdtFntLoad(0x3c0, 0x100);
    AdtFntOpen(-(SCREEN_W / 2), -0x68, SCREEN_W, 0xd0, 0, 0x400);
    OTable[1].length = OT_LENGTH;
    OTable[0].length = OT_LENGTH;
    OTable[0].org = ZSortTable[0];
    OTable[1].org = ZSortTable[1];
    STARTING_RNG_SEED += VSync(-1);
    srand(STARTING_RNG_SEED);
}
