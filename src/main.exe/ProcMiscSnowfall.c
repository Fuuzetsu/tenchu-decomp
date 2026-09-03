#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "misc.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcMiscSnowfall(struct tag_TMisc *m, enum TMiscMessage msg);
 *     MISC.C:525, 53 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct tag_TMisc * m
 *     param $a1       enum TMiscMessage msg
 *     reg   $s4       struct TSnowfall * param
 *     reg   $s1       int i
 *     reg   $v0       int w
 *     reg   $v1       int h
 *     reg   $s0       struct SVECTOR * pos
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

void ProcMiscSnowfall(TMisc *m, TMiscMessage msg)
{
    TSnowfall *param = &m->param.snowfall;

    switch (msg)
    {
    case MM_CREATE:
    {
        s32 w = m->param.snowfall.w;
        s32 h = m->param.snowfall.h;

        m->mode = 0;
        param->w = w;
        param->h = h;
        break;
    }

    case MM_DESTROY:
    case MM_PAUSE:
    case MM_RESUME:
        break;

    default:
        if ((GameClock & 3) == 0)
        {
            SVECTOR velocity = {
                rand() % 20 - 10,
                rand() % 50 + 50,
                rand() % 20 - 10
            };
            VECTOR position = {
                ViewInfo.vrx + (rand() % SNOW_SPAN - SNOW_RANGE),
                ViewInfo.vry + (rand() % SNOW_RANGE - SNOW_SPAN),
                ViewInfo.vrz + (rand() % SNOW_SPAN - SNOW_RANGE)
            };

            SetSnow(&position, &velocity, FIXED_ONE, SNOW_SPRITE_DEFAULT);
        }
    }
}
