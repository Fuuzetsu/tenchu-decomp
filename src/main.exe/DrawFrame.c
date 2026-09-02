#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawFrame(struct tag_EffectSlot *ef);
 *     EFFECT.C:1044, 49 src lines, frame 104 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct tag_EffectSlot * ef
 *     reg   $s1       struct FrameType * param
 *     reg   $s2       struct GsSPRITE * spr
 *     stack sp+16     struct SVECTOR scr
 *     stack sp+24     struct SVECTOR sv
 *     stack sp+40     struct MATRIX mat
 *     stack sp+72     long p
 *     stack sp+76     long flag
 *     reg   $v0       long x
 *     reg   $a1       long y
 *     reg   $a2       long z
 *     reg   $s0       struct SVECTOR * scr
 *     reg   $v1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE sprFrame[4];
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

void DrawFrame(TEffectSlot *ef)
{
    enum
    {
        FRAME_FLASH_LEVEL = 0x80
    };
    FrameType *param = &ef->param.frame;
    GsSPRITE *spr;
    SVECTOR scr;
    s16 idx;
    s32 otz;
    s32 t;
    s32 pri;
    s32 px, py, pz;
    GsCOORDINATE2 *hint;
    s32 size;

    idx = param->count % MaxFrames;
    spr = &sprFrame[idx];

    switch (param->mode)
    {
    case FRAME_MODE_FLASH:
        spr->b = FRAME_FLASH_LEVEL;
        spr->g = FRAME_FLASH_LEVEL;
        spr->r = FRAME_FLASH_LEVEL;
        param->count--;
        if (param->count <= 0)
        {
            param->count = FRAME_FLASH_LEVEL;
            param->mode++;
        }
        break;
    case FRAME_MODE_FADE:
        spr->r = spr->g = spr->b = (u8)param->count;
        param->count -= 29;
        if (param->count <= 0)
        {
            ef->proc = 0;
        }
        break;
    }
    px = param->px;
    py = param->py;
    pz = param->pz;
    hint = param->super;
    size = param->size;

    if (hint != 0)
    {
        *SCREEN_PROJECTION_POINT_X = px;
        *SCREEN_PROJECTION_POINT_Y = py;
        *SCREEN_PROJECTION_POINT_Z = pz;
        GsGetLs(hint, SCREEN_PROJECTION_MATRIX);
        GsSetLsMatrix(SCREEN_PROJECTION_MATRIX);
        scr.vz = (s16)RotTransPers(
            SCREEN_PROJECTION_POINT, (s32 *)&scr,
            SCREEN_PROJECTION_PERSPECTIVE, SCREEN_PROJECTION_FLAG);
    }
    else
    {
        GetScreenPosition(px, py, pz, &scr);
    }

    otz = scr.vz;
    if (otz > NEAR_DEPTH)
    {
        spr->scalex = spr->scaley =
            (s16)((size * PROJECTION_DISTANCE) / otz) + 1;
        spr->x = scr.vx;
        spr->y = scr.vy;
        t = scr.vz - 0x32;
        t = t >> 2;
        CLAMP_SORT_DEPTH(pri, t);
        GsSortSprite(spr, OTablePt, (u16)pri);
    }
}
