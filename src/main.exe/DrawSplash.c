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

extern MATRIX GsWSMATRIX;
extern SVECTOR svec_y_n20_2[];

void DrawSplash(TEffectSlot *ef)
{
    SplashType *param;
    GsSPRITE *spr;
    SVECTOR scr;
    SVECTOR *projected;
    long x;
    long y;
    long z;
    s32 priority;

    param = &ef->param.splash;
    spr = &sprSplash;
    x = param->px;
    y = *(s32 *)&param->py;
    z = *(s32 *)&param->pz;

    *SCREEN_PROJECTION_TRANSLATION_X = 0;
    *SCREEN_PROJECTION_TRANSLATION_Y = 0;
    *SCREEN_PROJECTION_TRANSLATION_Z = 0;
    *SCREEN_PROJECTION_POINT_X = x - (s16)ViewInfo.vpx;
    *SCREEN_PROJECTION_POINT_Y = y - (s16)ViewInfo.vpy;
    *SCREEN_PROJECTION_POINT_Z = z - (s16)ViewInfo.vpz;
    SetTransMatrix(SCREEN_PROJECTION_MATRIX);
    SetRotMatrix(&GsWSMATRIX);
    projected = &scr;
    projected->vz = (s16)RotTransPers(
        SCREEN_PROJECTION_POINT, (s32 *)projected,
        SCREEN_PROJECTION_PERSPECTIVE, SCREEN_PROJECTION_FLAG);
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
            case SPLASH_MODE_SPAWN:
                param->count = 0;
                param->mode++;
                {
                    VECTOR pos = {param->px, param->py, param->pz};
                    SVECTOR direction = svec_y_n20_2[0];

                    SetBleedsDir(&pos, &direction, 100, 6, 30, RGB24(144, 152, 160));
                }
                /* fall through */
            case SPLASH_MODE_RISE:
                spr->scaley = (spr->scaley * param->count) / param->speed;
                param->count++;
                if (param->count >= param->speed)
                {
                    param->count = 0;
                    param->mode++;
                }
                break;
            case SPLASH_MODE_FALL:
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
