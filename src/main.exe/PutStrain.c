#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PutStrain(void);
 *     INFOVIEW.C:218, 70 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       int newpow
 *     reg   $a2       int r
 *     reg   $s1       struct GsSPRITE * spr
 *
 * Globals it touches, as the original declared them:
 *     extern long StrainRatio;
 *     extern long GameClock;
 *     extern struct GsSPRITE NumberImage;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

extern s32 StrainRatio;
extern u16 StrainPhase;

void PutStrain(s32 x, s32 y)
{
    enum
    {
        speed = 30
    };
    enum
    {
        range = 255,
        powrange = 20000
    };
    s32 ratio;
    GsSPRITE *spr;
    s32 delta;
    s32 s;
    u16 phase;

    ratio = StrainRatio;
    if (ratio != 0x7fffffff)
    {
        if (ratio == 0)
        {
            spr = &KehaiRedImage;
        }
        else if (ratio < -powrange)
        {
            spr = &KehaiCriticalImage;
            ratio = 0;
        }
        else if (ratio < 0)
        {
            spr = &KehaiYellowImage;
            ratio = 0;
            if (GameClock % speed == 0)
            {
                SoundEx(0, SE_WARNING_BEEP);
            }
        }
        else
        {
            u8 base;
            GsSPRITE *img;
            s32 newpow;
            s32 r;

            if (ratio > powrange)
                return;
            spr = &KehaiGreenImage;
            NumberImage.w = 4;
            img = &NumberImage;
            img->x = (s16)(x + 0x22);
            base = img->u;
            img->y = (s16)(y + 8);
            newpow = (powrange - ratio) / 200;
        strainloop:
            r = newpow / 10;
            img->u = base + (newpow % 10) * 4;
            GsSortSprite(img, OTablePt, 0);
            img->x -= 6;
            newpow = r;
            if (newpow != 0)
                goto strainloop;
            do
            {
                img->u = base;
            } while (0);
        }

        delta = powrange - ratio;
        s = delta;
        if (delta < 0)
            s = delta + 0x1f;

        spr->x = (s16)x;
        spr->y = (s16)y;
        phase = StrainPhase + (s >> 5);
        StrainPhase = phase;
        spr->r = spr->g = spr->b =
            rsin(phase) * 0x60 / FIXED_ONE + range / 2;
        spr->scaley = spr->scalex =
            (s16)((delta << 0xb) / powrange) + FIXED_HALF;
        GsSortSprite(spr, OTablePt, 0);
    }
}
