#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawSplash(struct tag_EffectSlot *ef);
 *     EFFECT.C:978, 43 src lines, frame 88 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct tag_EffectSlot * ef
 *     reg   $s1       struct SplashType * param
 *     reg   $s2       struct GsSPRITE * spr
 *     stack sp+24     struct SVECTOR scr
 *     reg   $v0       long x
 *     reg   $a1       long y
 *     reg   $a2       long z
 *     reg   $s0       struct SVECTOR * scr
 *     stack sp+32     struct VECTOR pos
 *     reg   $v1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * MATCH notes:
 * - The scalar aliases on py/pz are intentional.  Plain structure-member
 *   reads carry cc1's in-structure memory marker, so CSE sinks both loads
 *   below the scratchpad stores and reuses v0/v1.  Reading the same 32-bit
 *   representation through scalar lvalues keeps the original long-lived
 *   y/z values in a1/a2, as recorded by PSX.SYM and emitted by retail.
 * - svec_y_n20_2 is an array in the original declaration.  Indexing element
 *   zero, rather than declaring one SVECTOR object, produces the target's
 *   separately scheduled address high/low around the VECTOR block copy.
 * - The second z is deliberately scoped after RotTransPers; PSX.SYM records
 *   it as a distinct int local from the outer long coordinate.
 */

extern MATRIX GsWSMATRIX;
extern SVECTOR svec_y_n20_2[];


void DrawSplash(TEffectSlot *ef)
{
    SplashType *param;
    GsSPRITE *spr;
    SVECTOR scr;
    SVECTOR *scrp;
    long x;
    long y;
    long z;
    s32 priority;

    param = &ef->param.splash;
    spr = &sprSplash;
    x = param->px;
    y = *(s32 *)&param->py;
    z = *(s32 *)&param->pz;

    *(s32 *)TENCHU_SCRATCHPAD(0x14) = 0;
    *(s32 *)TENCHU_SCRATCHPAD(0x18) = 0;
    *(s32 *)TENCHU_SCRATCHPAD(0x1c) = 0;
    *(s16 *)TENCHU_SCRATCHPAD(0x20) = x - (s16)ViewInfo.vpx;
    *(s16 *)TENCHU_SCRATCHPAD(0x22) = y - (s16)ViewInfo.vpy;
    *(s16 *)TENCHU_SCRATCHPAD(0x24) = z - (s16)ViewInfo.vpz;
    SetTransMatrix((MATRIX *)TENCHU_SCRATCHPAD_ADDRESS);
    SetRotMatrix(&GsWSMATRIX);
    scrp = &scr;
    scrp->vz = (s16)RotTransPers(
        (SVECTOR *)TENCHU_SCRATCHPAD(0x20), (s32 *)scrp,
        (s32 *)TENCHU_SCRATCHPAD(0x28),
        (s32 *)TENCHU_SCRATCHPAD(0x2c));
    {
        s32 z;

        z = scr.vz;
        if (z > NEAR_DEPTH)
        {
            spr->x = scr.vx;
            spr->y = scr.vy;
            spr->scalex = (param->sx * PROJECTION_DISTANCE) / z + 1;
            spr->scaley = (param->sy * PROJECTION_DISTANCE) / z + 1;

            switch (param->mode)
            {
            case 0:
                param->count = 0;
                param->mode++;
                {
                    VECTOR pos = {param->px, param->py, param->pz};
                    SVECTOR direction = svec_y_n20_2[0];

                    SetBleedsDir(&pos, &direction, 100, 6, 30, RGB24(144, 152, 160));
                }
                /* fall through */
            case 1:
                spr->scaley = (spr->scaley * param->count) / param->speed;
                param->count++;
                if (param->count >= param->speed)
                {
                    param->count = 0;
                    param->mode++;
                }
                break;
            case 2:
                spr->scaley = (spr->scaley * (param->speed - param->count)) /
                              param->speed;
                spr->scalex = spr->scalex / 2;
                param->count++;
                if (param->count >= param->speed)
                {
                    ef->proc = 0;
                }
                break;
            }

            {
                s32 t;

                t = (s16)(u16)scr.vz >> 2;
                if (t >= 0)
                {
                    priority = DEPTH_LIMIT - 1;
                    if (t < DEPTH_LIMIT)
                    {
                        priority = t;
                    }
                }
                else
                {
                    priority = 0;
                }
            }
            GsSortSprite(spr, OTablePt, (u16)priority);
        }
    }
}
