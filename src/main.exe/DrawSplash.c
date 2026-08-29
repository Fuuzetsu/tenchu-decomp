#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawSplash(struct tag_EffectSlot *ef);
 *     EFFECT.C:978, 43 src lines, frame 88 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 * DrawSplash (0x800349e4) — the per-frame effect callback for a water
 * splash, drawn through the single shared sprSplash GsSPRITE rather
 * than a per-slot one. It projects its own world point: zeroes the
 * translation staged in the scratchpad, SetTransMatrix /
 * SetRotMatrix(&GsWSMATRIX), subtracts ViewInfo.vpx/vpy/vpz from
 * param->px/py/pz and RotTransPers into scr. Everything else — the
 * mode machine included — is gated on scr.vz > NEAR_DEPTH, so an
 * off-screen splash neither draws nor ages. scalex/scaley are
 * (param->sx * 300) / z + 1 and (param->sy * 300) / z + 1. Mode 0 is
 * the one-shot setup: count = 0, mode++, and a SetBleedsDir spray from
 * the same point along svec_y_n20_2[0] with grange 100, n 6, time 30
 * and colour 0x9098A0; it falls through into mode 1. Mode 1 is the
 * rise — scaley scaled by count/speed — and mode 2 the collapse —
 * scaley scaled by (speed - count)/speed with scalex halved; both
 * count up to speed, mode 1 then resetting count and advancing, mode 2
 * clearing ef->proc. GsSortSprite puts it in OTablePt at scr.vz >> 2,
 * clamped to DEPTH_LIMIT - 1, negatives to 0.
 */

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

extern void *memset(void *dst, int value, u32 size);

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
        (void *)TENCHU_SCRATCHPAD(0x28),
        (void *)TENCHU_SCRATCHPAD(0x2c));
    {
        s32 z;

        z = scr.vz;
        if (z > NEAR_DEPTH)
        {
            spr->x = scr.vx;
            spr->y = scr.vy;
            spr->scalex = (param->sx * 300) / z + 1;
            spr->scaley = (param->sy * 300) / z + 1;

            switch (param->mode)
            {
            case 0:
                param->count = 0;
                param->mode++;
                {
                    VECTOR pos = {param->px, param->py, param->pz};
                    SVECTOR direction = svec_y_n20_2[0];

                    SetBleedsDir(&pos, &direction, 100, 6, 30, 0x9098A0);
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
                    priority = 0x4E1;
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
