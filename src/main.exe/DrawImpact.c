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

/*
 * MATCH.
 *
 * The final 4-byte residual was not a conflict-free local-alloc floor.  The
 * target itself uses $a0 for three disjoint roles: red's interpolation work,
 * the green/blue start-colour inputs, and the later coordinate pointer.  One
 * reusable `work` union for the colour and coordinate-parent roles gives
 * those loads the pointer call's $a0 preference and reproduces all four
 * register fields without converting the pointer through an integer.
 * The colour-lerp locals end/start2/inverse are reused the same way for
 * the px/py/pz captures (and start2 a third time for the OT depth) —
 * same shared-role lever.
 * No priority fence, dead carrier, or no-op is needed.  The one-shot wrapper
 * around the px capture remains load-bearing: unwrapping it changes 7 bytes
 * in the following grouped-load block.
 *
 * The superseded round-by-round investigation log for this function lives
 * in docs/matching-archive.md.
 */

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
    union
    {
        s32 color;
        GsCOORDINATE2 *super;
    } work;

    param = &ef->param.impact;
    ratio = (param->count << 12) / param->time;
    spr = &sprImpact[param->type];
    spr->rotate = param->rotate << 12;
    inverse = FIXED_ONE - ratio;

    start = param->start_size * inverse;
    param->rotate += param->rotate_speed;
    if (start < 0)
    {
        start += FIXED_ONE - 1;
    }

    size = (start >> 12) + (param->end_size * ratio) / FIXED_ONE;

    start = param->start_color.channel.r;
    start = start * inverse;
    end_raw = param->end_color.channel.r;
    if (start < 0)
    {
        start += FIXED_ONE - 1;
    }
    spr->r = (start >> 12) + (end_raw * ratio) / FIXED_ONE;

    work.color = param->start_color.channel.g;
    start2 = work.color * inverse;
    end_raw = param->end_color.channel.g;
    if (start2 < 0)
    {
        start2 += FIXED_ONE - 1;
    }
    start2 = start2 >> 12;
    spr->g = start2 + (end_raw * ratio) / FIXED_ONE;

    work.color = param->start_color.channel.b;
    start2 = work.color * inverse;
    end_raw = param->end_color.channel.b;
    if (start2 < 0)
    {
        start2 += FIXED_ONE - 1;
    }
    start2 = start2 >> 12;
    spr->b = start2 + (end_raw * ratio) / FIXED_ONE;

    end = param->px;
    /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
    do
    {
    } while (0);
    start2 = param->py;
    work.super = param->super;
    inverse = param->pz;
    if (work.super != 0)
    {
        *(s16 *)TENCHU_SCRATCHPAD(SCRATCH_POINT_X) = end;
        *(s16 *)TENCHU_SCRATCHPAD(SCRATCH_POINT_Y) = start2;
        *(s16 *)TENCHU_SCRATCHPAD(SCRATCH_POINT_Z) = inverse;
        GsGetLs(work.super,
                (MATRIX *)TENCHU_SCRATCHPAD_ADDRESS);
        GsSetLsMatrix((MATRIX *)TENCHU_SCRATCHPAD_ADDRESS);
        scr.vz = (s16)RotTransPers(
            (SVECTOR *)TENCHU_SCRATCHPAD(SCRATCH_POINT), (s32 *)&scr,
            (s32 *)TENCHU_SCRATCHPAD(SCRATCH_RTP_P),
            (s32 *)TENCHU_SCRATCHPAD(SCRATCH_RTP_FLAG));
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
