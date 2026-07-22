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
 * MATCH notes:
 * - The scalar aliases on py/pz are intentional.  Plain structure-member
 *   reads carry cc1's in-structure memory marker, so CSE sinks both loads
 *   below the scratchpad stores and reuses v0/v1.  Reading the same 32-bit
 *   representation through scalar lvalues keeps the original long-lived
 *   y/z values in a1/a2, as recorded by PSX.SYM and emitted by retail.
 * - D_80097A50 is an array in the original declaration.  Indexing element
 *   zero, rather than declaring one SVECTOR object, produces the target's
 *   separately scheduled address high/low around the VECTOR block copy.
 * - The second z is deliberately scoped after RotTransPers; PSX.SYM records
 *   it as a distinct int local from the outer long coordinate.
 */

extern MATRIX GsWSMATRIX;
extern GsSPRITE sprSplash;
extern SVECTOR D_80097A50[];

extern void *memset(void *dst, int value, u32 size);

void DrawSplash(TEffectSlot *ef)
{
    SplashType *param;
    GsSPRITE *spr;
    SVECTOR screen;
    SVECTOR *screenp;
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
    screenp = &screen;
    screenp->vz = (s16)RotTransPers(
        (SVECTOR *)TENCHU_SCRATCHPAD(0x20), (s32 *)screenp,
        (void *)TENCHU_SCRATCHPAD(0x28),
        (void *)TENCHU_SCRATCHPAD(0x2c));
    {
        s32 z;

        z = screen.vz;
        if (z > 0x24)
        {
            spr->x = screen.vx;
            spr->y = screen.vy;
            spr->scalex = (param->sx * 300) / z + 1;
            spr->scaley = (param->sy * 300) / z + 1;

            switch (param->mode)
            {
            case 0:
                param->count = 0;
                param->mode = param->mode + 1;
                {
                    VECTOR pos = { param->px, param->py, param->pz };
                    SVECTOR direction = D_80097A50[0];

                    SetBleedsDir(&pos, &direction, 100, 6, 30, 0x9098A0);
                }
                /* fall through */
            case 1:
                spr->scaley = (spr->scaley * param->count) / param->speed;
                param->count = param->count + 1;
                if (param->count >= param->speed)
                {
                    param->count = 0;
                    param->mode = param->mode + 1;
                }
                break;
            case 2:
                spr->scaley = (spr->scaley * (param->speed - param->count)) /
                              param->speed;
                spr->scalex = spr->scalex / 2;
                param->count = param->count + 1;
                if (param->count >= param->speed)
                {
                    ef->proc = 0;
                }
                break;
            }

            {
                s32 t;

                t = (s32)((u16)screen.vz << 16) >> 18;
                if (t < 0)
                {
                    goto zero;
                }
                priority = 0x4E1;
                if (t < 0x4E2)
                {
                    priority = t;
                }
            }
            goto done;
        zero:
            priority = 0;
        done:
            GsSortSprite(spr, OTablePt, (u16)priority);
        }
    }
}
