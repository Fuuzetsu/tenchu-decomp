#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int PutLifeBarS(void);
 *     INFOVIEW.C:328, 16 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s2       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct INFOVIEW__198fake LifeBar[4];
 * END PSX.SYM */

extern void PutLifeBar(s32 x, s32 y, s32 life, s32 lifemax,
                       life_bar_style style);

s32 PutLifeBarS(void)
{
    s32 i;

    i = 0;
    do
    {
        if (LifeBar[i].count > 0)
        {
            PutLifeBar(i * 60 - 140, -90, LifeBar[i].life,
                       LifeBar[i].max, LifeBar[i].style);
            LifeBar[i].count--;
        }
        i++;
    } while (i < nLifeBar);
    return 0;
}
