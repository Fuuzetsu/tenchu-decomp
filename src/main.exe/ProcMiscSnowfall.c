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

extern void *memset(void *s, int c, u32 n);

void ProcMiscSnowfall(TMisc *m, TMiscMessage msg)
{
    TSnowfall *param = &m->param.snowfall;

    if (msg == MM_CREATE)
    {
        goto do_create;
    }
    if (MM_DO <= msg)
    {
        goto do_tick;
    }
    return;

do_create:
{
    s32 w;
    s32 h;

    w = m->param.snowfall.w;
    h = m->param.snowfall.h;
    m->mode = 0;
    param->w = w;
    param->h = h;
}
    return;

do_tick:
    if ((GameClock & 3) == 0)
    {
        SVECTOR vel;
        SVECTOR jitter;
        VECTOR pos;
        VECTOR posRaw;

        memset(&jitter, 0, sizeof(jitter));
        jitter.vx = rand() % 20 - 10;
        jitter.vy = rand() % 50 + 50;
        jitter.vz = rand() % 20 - 10;
        vel = jitter;

        memset(&posRaw, 0, sizeof(posRaw));
        posRaw.vx = ViewInfo.vrx + (rand() % SNOW_SPAN - SNOW_RANGE);
        posRaw.vy = ViewInfo.vry + (rand() % SNOW_RANGE - SNOW_SPAN);
        posRaw.vz = ViewInfo.vrz + (rand() % SNOW_SPAN - SNOW_RANGE);
        pos = posRaw;
        SetSnow(&pos, &vel, FIXED_ONE, SNOW_SPRITE_DEFAULT);
    }
}
