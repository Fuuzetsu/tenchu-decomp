#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawImpact(struct tag_EffectSlot *ef);
 *     EFFECT.C:876, 15 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 * ordinary reusable `work` local for the latter two roles gives those loads
 * the pointer call's $a0 preference and reproduces all four register fields.
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
    s32 z;
    s16 scale;
    s32 priority;
    s32 work;

    param = &ef->param.impact;
    ratio = (param->count << 12) / param->time;
    spr = &sprImpact[param->type];
    spr->rotate = param->rotate << 12;
    inverse = 0x1000 - ratio;

    start = param->start_size * inverse;
    param->rotate = param->rotate + param->rotate_speed;
    if (start < 0)
    {
        start = start + 0xfff;
    }

    size = (start >> 12) + (param->end_size * ratio) / 0x1000;

    start = param->start_color.channel.r;
    start = start * inverse;
    end_raw = param->end_color.channel.r;
    if (start < 0)
    {
        start = start + 0xfff;
    }
    spr->r = (start >> 12) + (end_raw * ratio) / 0x1000;

    work = param->start_color.channel.g;
    start2 = work * inverse;
    end_raw = param->end_color.channel.g;
    if (start2 < 0)
    {
        start2 = start2 + 0xfff;
    }
    start2 = start2 >> 12;
    spr->g = start2 + (end_raw * ratio) / 0x1000;

    work = param->start_color.channel.b;
    start2 = work * inverse;
    end_raw = param->end_color.channel.b;
    if (start2 < 0)
    {
        start2 = start2 + 0xfff;
    }
    start2 = start2 >> 12;
    spr->b = start2 + (end_raw * ratio) / 0x1000;

    end = param->px;
    /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
    do
    {
    } while (0);
    start2 = param->py;
    work = (s32)param->super;
    inverse = param->pz;
    if (work != 0)
    {
        *(s16 *)TENCHU_SCRATCHPAD(0x20) = end;
        *(s16 *)TENCHU_SCRATCHPAD(0x22) = start2;
        *(s16 *)TENCHU_SCRATCHPAD(0x24) = inverse;
        GsGetLs((GsCOORDINATE2 *)work,
                (MATRIX *)TENCHU_SCRATCHPAD_ADDRESS);
        GsSetLsMatrix((MATRIX *)TENCHU_SCRATCHPAD_ADDRESS);
        scr.vz = (s16)RotTransPers(
            (SVECTOR *)TENCHU_SCRATCHPAD(0x20), (s32 *)&scr,
            (s32 *)TENCHU_SCRATCHPAD(0x28),
            (s32 *)TENCHU_SCRATCHPAD(0x2c));
    }
    else
    {
        GetScreenPosition(end, start2, inverse, &scr);
    }

    z = scr.vz;
    if (z > NEAR_DEPTH)
    {
        scale = (s16)((size * PROJECTION_DISTANCE) / z) + 1;
        spr->scaley = scale;
        spr->scalex = scale;
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
