#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawImpact(struct tag_EffectSlot *ef);
 *     EFFECT.C:876, 15 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct tag_EffectSlot * ef
 *     reg   $s1       struct ImpactType * param
 *     reg   $s0       struct Sprite3D * spr
 * END PSX.SYM */

void DrawImpact(TEffectSlot *ef)
{
    ImpactType *param;
    GsSPRITE *spr;
    SVECTOR scr;
    s32 ratio;
    s32 inverse;
    s32 start;
    s32 start2;
    s32 end;
    s32 end_raw;
    s32 size;
    s32 priority;
    s32 work;

    param = &ef->param.impact;
    ratio = (param->count << FIXED_SHIFT) / param->time;
    spr = &sprImpact[param->type];
    spr->rotate = param->rotate << FIXED_SHIFT;
    inverse = FIXED_ONE - ratio;

    start = param->start_size * inverse;
    param->rotate += param->rotate_speed;
    if (start < 0)
    {
        start += FIXED_TRUNC_BIAS;
    }

    size = (start >> FIXED_SHIFT) +
           (param->end_size * ratio) / FIXED_ONE;

    start = param->start_color.channel.r;
    start = start * inverse;
    end_raw = param->end_color.channel.r;
    if (start < 0)
    {
        start += FIXED_TRUNC_BIAS;
    }
    spr->r = (start >> FIXED_SHIFT) + (end_raw * ratio) / FIXED_ONE;

    work = param->start_color.channel.g;
    start2 = work * inverse;
    end_raw = param->end_color.channel.g;
    if (start2 < 0)
    {
        start2 += FIXED_TRUNC_BIAS;
    }
    start2 = start2 >> FIXED_SHIFT;
    spr->g = start2 + (end_raw * ratio) / FIXED_ONE;

    work = param->start_color.channel.b;
    start2 = work * inverse;
    end_raw = param->end_color.channel.b;
    if (start2 < 0)
    {
        start2 += FIXED_TRUNC_BIAS;
    }
    start2 = start2 >> FIXED_SHIFT;
    spr->b = start2 + (end_raw * ratio) / FIXED_ONE;

    end = param->px;
    /* Empty loop retained for code layout; its original source construct is unknown. */
    do
    {
    } while (0);
    start2 = param->py;
    work = (s32)param->super;
    inverse = param->pz;
    if (work != 0)
    {
        *SCREEN_PROJECTION_POINT_X = end;
        *SCREEN_PROJECTION_POINT_Y = start2;
        *SCREEN_PROJECTION_POINT_Z = inverse;
        GsGetLs((GsCOORDINATE2 *)work, SCREEN_PROJECTION_MATRIX);
        GsSetLsMatrix(SCREEN_PROJECTION_MATRIX);
        scr.vz = (s16)RotTransPers(
            SCREEN_PROJECTION_POINT, (s32 *)&scr,
            SCREEN_PROJECTION_PERSPECTIVE, SCREEN_PROJECTION_FLAG);
    }
    else
    {
        GetScreenPosition(end, start2, inverse, &scr);
    }

    if (scr.vz > NEAR_DEPTH)
    {
        spr->scalex = spr->scaley =
            (s16)((size * PROJECTION_DISTANCE) / scr.vz) + 1;
        spr->x = scr.vx;
        spr->y = scr.vy;

        start2 = (s16)(u16)scr.vz >> 2;
        CLAMP_SORT_DEPTH(priority, start2);
        GsSortSprite(spr, OTablePt, (u16)priority);
    }

    if (param->count >= param->time)
    {
        ef->proc = 0;
    }
    param->count++;
}
